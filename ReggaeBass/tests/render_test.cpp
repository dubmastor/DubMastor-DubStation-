// Teste offline do Dub Bass (sem DAW):
//   cmake -B build-test -DDUBBASS_TESTS=ON && cmake --build build-test --target DubBassTest
//   ./build-test/DubBassTest_artefacts/Release/DubBassTest [pasta-de-saida]
// Toca uma linha de baixo nos dois modos, confere que o maximizer segura o teto,
// que não sai NaN, que o estado salva/carrega, e salva interface.png com a válvula quente.
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../../ReggaeBassMachine/Source/PluginProcessor.h"   // aba BASS MACHINE (MRBM-69)
#include "../../ReggaeBassMachine/Source/RiddimPresets.h"
#include "../../ReggaeBassMachine/Source/AccompEngine.h"
#include "../../ReggaeChordMachine/Source/CadenceHints.h"
#include "../../RiddimBank/Source/RiddimBankMenu.h"

static int failures = 0;
static void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "OK    " : "FALHOU", what.toRawUTF8());
    if (! ok) ++failures;
}

static void setP (DubBassAudioProcessor& p, const juce::String& id, float v)
{
    auto* par = p.apvts.getParameter (id);
    par->setValueNotifyingHost (par->convertTo0to1 (v));
}
static void setP (BassMachineProcessor& m, const juce::String& id, float v)
{
    if (auto* prm = m.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// toca a linha e devolve o pico em dBFS; grava WAV se pedido
static float play (DubBassAudioProcessor& p, double sr, const juce::File& wav, bool& nan)
{
    const int bs = 512, total = (int) (sr * 4.0);
    p.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> out (2, total);
    const int notes[] = { 36, 36, 43, 41, 36, 31, 34, 36 };
    float peak = 0.0f;
    for (int pos = 0; pos < total; pos += bs)
    {
        const int n = std::min (bs, total - pos);
        juce::AudioBuffer<float> b (2, n);
        juce::MidiBuffer m;
        for (int k = 0; k < 8; ++k)
        {
            const int on = (int) (k * 0.45 * sr), off = on + (int) (0.35 * sr);
            if (on >= pos && on < pos + n)   m.addEvent (juce::MidiMessage::noteOn (1, notes[k], (juce::uint8) 100), on - pos);
            if (off >= pos && off < pos + n) m.addEvent (juce::MidiMessage::noteOff (1, notes[k]), off - pos);
        }
        p.processBlock (b, m);
        for (int c = 0; c < 2; ++c)
        {
            out.copyFrom (c, pos, b, c, 0, n);
            for (int i = 0; i < n; ++i)
            {
                const float v = b.getSample (c, i);
                if (! std::isfinite (v)) nan = true;
                peak = std::max (peak, std::abs (v));
            }
        }
    }
    if (wav != juce::File())
    {
        wav.deleteFile();
        juce::WavAudioFormat fmt;
        if (std::unique_ptr<juce::AudioFormatWriter> w { fmt.createWriterFor (new juce::FileOutputStream (wav), sr, 2, 24, {}, 0) })
            w->writeFromAudioSampleBuffer (out, 0, total);
    }
    return juce::Decibels::gainToDecibels (peak, -100.0f);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "test-out");
    dir.createDirectory();

    DubBassAudioProcessor p;
    bool nan = false;

    float pk = play (p, 48000.0, dir.getChildFile ("classic.wav"), nan);
    check (pk <= -0.29f && pk > -12.0f, "Classic Sine: pico " + juce::String (pk, 2) + " dBFS respeita o teto de -0.3");

    setP (p, ParamID::mode, 1); setP (p, ParamID::boost, 1); setP (p, ParamID::drive, 20); setP (p, ParamID::maxGain, 12);
    pk = play (p, 44100.0, dir.getChildFile ("808.wav"), nan);
    check (pk <= -0.29f, "808 + boost + drive alto: pico " + juce::String (pk, 2) + " dBFS respeita o teto");
    check (p.gainReductionDb.load() < -0.5f || p.driveLevel.load() >= 0.0f, "maximizer reduzindo ganho / medidores ativos");

    setP (p, ParamID::mode, 2); setP (p, ParamID::boost, 0); setP (p, ParamID::drive, 6); setP (p, ParamID::maxGain, 3);
    pk = play (p, 48000.0, dir.getChildFile ("supertone.wav"), nan);
    check (pk <= -0.29f && pk > -12.0f, "Super Tone: pico " + juce::String (pk, 2) + " dBFS respeita o teto");

    DubBassAudioProcessor fresh;
    check (fresh.apvts.getRawParameterValue (ParamID::drvOn)->load() < 0.5f
           && fresh.apvts.getRawParameterValue (ParamID::revOn)->load() < 0.5f, "valvula e reverb comecam desligados");

    setP (p, ParamID::mode, 1); setP (p, ParamID::boost, 1); setP (p, ParamID::drive, 20); setP (p, ParamID::maxGain, 12);
    setP (p, ParamID::maxBypass, 1);
    pk = play (p, 48000.0, {}, nan);
    check (pk > -0.3f, "BYPASS do maximizer deixa passar acima do teto (" + juce::String (pk, 2) + " dBFS)");
    setP (p, ParamID::maxBypass, 0);

    setP (p, ParamID::mode, 1); setP (p, ParamID::drvOn, 1); setP (p, ParamID::drvType, 3); setP (p, ParamID::drive, 14);
    pk = play (p, 48000.0, dir.getChildFile ("808_fita.wav"), nan);
    check (pk <= -0.29f && pk > -12.0f, "808 + saturacao FITA: pico " + juce::String (pk, 2) + " dBFS respeita o teto");

    setP (p, ParamID::drvOn, 1); setP (p, ParamID::revOn, 1);
    setP (p, ParamID::drvType, 2); setP (p, "eq2", 6.0f); setP (p, "eq6", -8.0f);
    pk = play (p, 96000.0, {}, nan);
    check (pk <= -0.29f, "Fuzz + EQ a 96 kHz respeita o teto");
    check (! nan, "nenhuma amostra NaN/inf");

    juce::MemoryBlock mb;
    p.getStateInformation (mb);
    DubBassAudioProcessor q;
    q.setStateInformation (mb.getData(), (int) mb.getSize());
    check (std::abs (q.apvts.getRawParameterValue ("eq6")->load() + 8.0f) < 0.01f && q.apvts.getRawParameterValue (ParamID::mode)->load() > 0.5f,
           "estado salva e carrega");

    {   // BASS MACHINE (MRBM-69 dentro do Mastor Reggae Bass MRB-69)
        DubBassAudioProcessor b;
        b.prepareToPlay (48000.0, 512);
        auto& mach = b.getMachine();
        auto runBlocks = [&] (int n) {
            float peak = 0.0f; bool noteSeen = false;
            for (int i = 0; i < n; ++i)
            {
                juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer mm;
                b.processBlock (buf, mm);
                peak = juce::jmax (peak, buf.getMagnitude (0, 0, 512));
                noteSeen |= b.currentNote.load() >= 0.0f;
            }
            return std::make_pair (peak, noteSeen);
        };
        // um riddim digital do MRBM-69 com SUPER TONE: o timbre vai para o oscilador do Dub Bass
        int digital = -1;
        for (int i = 0; i < mbmriddims::numRiddims && digital < 0; ++i) if (mbmriddims::riddims[i].timbre == mbm::SuperTone) digital = i;
        mach.loadRiddim (digital);
        setP (mach, "drumsOn", 0.0f); setP (mach, "keysOn", 0.0f);
        auto off = runBlocks (60);
        check (off.first < 1.0e-4f, "BASS MACHINE desligada e sem PLAY: silencio");
        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        check (juce::roundToInt (b.apvts.getRawParameterValue (ParamID::mode)->load()) == (int) BassVoice::Mode::SuperTone,
               "riddim com SUPER TONE leva o oscilador do Dub Bass para Super Tone");
        setP (mach, "preview", 1.0f);
        auto on = runBlocks (200);
        check (on.first > 0.01f && on.second, "PLAY da BASS MACHINE toca a linha pela voz do Dub Bass (pico "
               + juce::String (juce::Decibels::gainToDecibels (on.first), 1) + " dB)");
        setP (mach, "preview", 0.0f);
        juce::MemoryBlock st;
        b.getStateInformation (st);
        DubBassAudioProcessor c;
        c.setStateInformation (st.getData(), (int) st.getSize());
        check (c.getMachine().currentRiddim() == digital && juce::roundToInt (c.apvts.getRawParameterValue (ParamID::mode)->load()) == 2,
               "estado guarda o riddim da BASS MACHINE e o som do baixo");
        DubBassAudioProcessor old;   // projeto antigo (sem BASS MACHINE no estado) continua abrindo
        juce::MemoryBlock legacy;
        if (auto xml = old.apvts.copyState().createXml()) juce::AudioProcessor::copyXmlToBinary (*xml, legacy);
        DubBassAudioProcessor d;
        d.setStateInformation (legacy.getData(), (int) legacy.getSize());
        check (d.getMachine().currentRiddim() < 0, "projeto antigo do Dub Bass abre sem BASS MACHINE");
    }

    // interface: válvula quente, nota tocando e medidores cheios
    setP (p, ParamID::mode, 0); setP (p, ParamID::drvType, 3); setP (p, ParamID::drive, 18);
    setP (p, ParamID::drvOn, 1);
    play (p, 48000.0, {}, nan);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    {
        // mantém uma nota soando e deixa a interface rodar ~1,5 s (válvula esquenta, VU enche)
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 33, (juce::uint8) 110), 0);
        for (int f = 0; f < 45; ++f)
        {
            for (int k = 0; k < 3; ++k)
            {
                juce::AudioBuffer<float> b (2, 512);
                p.processBlock (b, m);
                m.clear();
            }
            juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
        }
    }
    const float scale = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale * (float) ed->getChildComponent (0)->getWidth() / (float) ed->getWidth());
    auto png = dir.getChildFile ("interface.png");
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat fmt;
    check (os.openedOk() && fmt.writeImageToStream (img, os), "interface salva em " + png.getFullPathName());
    {   // foto do EQ com o corte do botão EQ -2k da BASS MACHINE desenhado na tela verde
        setP (p.getMachine(), "eq2k", 1.0f);
        for (int f = 0; f < 6; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
        auto imgEq = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale * (float) ed->getChildComponent (0)->getWidth() / (float) ed->getWidth());
        auto pngEq = dir.getChildFile ("interface_eq2k.png");
        pngEq.deleteFile();
        juce::FileOutputStream osEq (pngEq);
        check (osEq.openedOk() && juce::PNGImageFormat().writeImageToStream (imgEq, osEq), "interface com EQ -2k salva");
        setP (p.getMachine(), "eq2k", 0.0f);
    }
    // segunda foto: modo 808 com o preset MÉDIO (as teclas de preset aparecem no envelope)
    setP (p, ParamID::mode, 1); setP (p, ParamID::attack, 1.0f); setP (p, ParamID::decay, 750.0f);
    setP (p, ParamID::release, 220.0f); setP (p, ParamID::pitchTime, 70.0f);
    for (int f = 0; f < 6; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
    auto img808 = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale * (float) ed->getChildComponent (0)->getWidth() / (float) ed->getWidth());
    auto png808 = dir.getChildFile ("interface_808.png");
    png808.deleteFile();
    juce::FileOutputStream os808 (png808);
    check (os808.openedOk() && fmt.writeImageToStream (img808, os808), "interface 808 salva em " + png808.getFullPathName());
    ed.reset();
    {   // terceira foto: aba BASS MACHINE (o MRBM-69 inteiro dentro do Mastor Reggae Bass MRB-69)
        p.apvts.state.setProperty ("tab", 1, nullptr);
        p.getMachine().loadRiddim (0);
        std::unique_ptr<juce::AudioProcessorEditor> edm (p.createEditor());
        for (int f = 0; f < 10; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
        auto imgM = edm->createComponentSnapshot (edm->getLocalBounds(), true, scale * 1300.0f / (float) edm->getWidth());
        auto pngM = dir.getChildFile ("interface_machine.png");
        pngM.deleteFile();
        juce::FileOutputStream osM (pngM);
        check (osM.openedOk() && fmt.writeImageToStream (imgM, osM), "interface BASS MACHINE salva em " + pngM.getFullPathName());
        p.apvts.state.setProperty ("tab", 0, nullptr);
    }

    {   // 2.1: chimbal 1/8 ou 1/16 e banco só com baixo original
        BassMachineProcessor m;
        check (m.apvts.getRawParameterValue ("hat16")->load() < 0.5f, "BASS MACHINE: chimbal começa em 1/8, como antes");
        int h8 = 0, h16 = 0;
        for (const auto& e : mbm::buildDrums (8.0, mbm::DrumOneDrop, 0.2f, false)) h8 += e.note == mbm::drum::ClosedHat;
        for (const auto& e : mbm::buildDrums (8.0, mbm::DrumOneDrop, 0.2f, true))  h16 += e.note == mbm::drum::ClosedHat;
        check (h16 > h8 && h16 == 28, "HH 1/16: semicolcheias no chimbal (" + juce::String (h8) + " -> " + juce::String (h16) + ")");
        int on = 0;
        for (int i = 0; i < riddimbank::numEntries; ++i) on += riddimbank::active (i, true);
        check (on == 160, "banco: só os " + juce::String (on) + " riddims com baixo original ficam ativos");
    }
    {   // 2.2: próximas harmonias (cadências do reggae) na BASS MACHINE — mesmo cálculo do MRK-80
        auto h = cadence::suggest (9, 1, 9, 1);   // Am → G (bVII) primeiro
        check (! h.empty() && h[0].root == 7 && h[0].minor == 0 && h.size() == 3, "cadencia: depois de Am a mais comum e G (bVII)");
        h = cadence::suggest (0, 0, 0, 0);        // C → F (IV)
        check (! h.empty() && h[0].root == 5 && h[0].minor == 0, "cadencia: depois de C a mais comum e F (IV)");
    }
    {   // 2.2: teclado da tônica da BASS MACHINE igual ao da CHORD MACHINE (ouvir ao apertar, notas pintadas no play)
        DubBassAudioProcessor b;
        b.prepareToPlay (48000.0, 512);
        auto& mach = b.getMachine();
        setP (mach, "drumsOn", 0.0f); setP (mach, "keysOn", 0.0f);
        auto run = [&] (int n) { float pk = 0.0f; for (int i = 0; i < n; ++i) { juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer mm; b.processBlock (buf, mm); pk = juce::jmax (pk, buf.getMagnitude (0, 0, 512)); } return pk; };
        check (run (20) < 1.0e-4f, "ouvir tonica: silencio antes do clique");
        setP (mach, "root1", 7.0f);                 // acorde 2 = G
        mach.auditionStart (1);
        const float held = run (30);
        const int note = juce::roundToInt (b.currentNote.load());
        const int base = juce::roundToInt (mach.apvts.getRawParameterValue ("base")->load());
        check (held > 0.01f && note % 12 == 7 && note >= base && note < base + 12,
               "ouvir tonica: G soa no baixo, no REGISTRO da linha, com a BASS MACHINE desligada (nota " + juce::String (note) + ")");
        mach.auditionStop();
        run (4);
        const float tail = run (200);
        check (tail < held * 0.2f, "ouvir tonica: soltar o mouse solta a nota");

        setP (mach, "preview", 1.0f);               // play: o acorde 1 acende a tônica dele (e as notas de passagem que ele tocar)
        juce::uint32 seen[4] {};
        for (int i = 0; i < 300; ++i) { run (1); for (int c = 0; c < 4; ++c) seen[c] |= mach.takePlayingMask (c); }
        const int r0 = juce::roundToInt (mach.apvts.getRawParameterValue ("root0")->load());
        check ((seen[0] >> r0) & 1u, "tonica no play: o acorde 1 acende a tonica dele (mask " + juce::String::toHexString ((int) seen[0]) + ")");
        setP (mach, "preview", 0.0f);
        run (4); for (int c = 0; c < 4; ++c) mach.takePlayingMask (c);
        juce::uint32 after = 0; for (int c = 0; c < 4; ++c) after |= mach.takePlayingMask (c);
        check (after == 0, "tonica no play: parar apaga as teclas");
    }
    {   // 2.2: botão EQ da BASS MACHINE corta tudo acima de 2 kHz (no MRB-69 e no som da própria machine)
        auto measure = [] (auto& proc, double sr, int blocks)   // energia acima de 3 kHz e total (dB), depois de estabilizar
        {
            juce::dsp::IIR::Filter<float> hp[4];   // passa-alta Butterworth de 8ª ordem em 3 kHz (o que está acima de 2k)
            const float q[4] = { 0.50979558f, 0.60134489f, 0.89997622f, 2.56291545f };
            for (int k = 0; k < 4; ++k) hp[k].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 3000.0f, q[k]);
            double hi = 0, all = 0;
            for (int b = 0; b < blocks; ++b)
            {
                juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer mm;
                proc.processBlock (buf, mm);
                if (b < 20) continue;
                for (int i = 0; i < 512; ++i) { const float x = buf.getSample (0, i); float h = x; for (auto& f : hp) h = f.processSample (h); hi += h * h; all += x * x; }
            }
            return std::make_pair (10.0 * std::log10 (hi + 1e-20), 10.0 * std::log10 (all + 1e-20));
        };
        for (int round = 0; round < 2; ++round)
        {
            DubBassAudioProcessor b;
            setP (b, ParamID::mode, 2);                         // SUPER TONE: cheio de harmônicos
            setP (b, ParamID::drvOn, 1); setP (b, ParamID::drive, 12);
            b.prepareToPlay (48000.0, 512);
            auto& mach = b.getMachine();
            setP (mach, "drumsOn", 0.0f); setP (mach, "keysOn", 0.0f);
            check (mach.apvts.getRawParameterValue ("eq2k")->load() < 0.5f || round == 1, "EQ -2k comeca desligado");
            if (round == 1) setP (mach, "eq2k", 1.0f);
            setP (mach, "preview", 1.0f);
            static std::pair<double, double> offRes;
            const auto r = measure (b, 48000.0, 300);
            if (round == 0) offRes = r;
            else
                check (r.first - r.second < -60.0 && offRes.first - r.first > 20.0 && offRes.second - r.second < 3.0,
                       "MRB-69: com EQ -2k o que sobra acima de 3 kHz fica " + juce::String (r.first - r.second, 1)
                       + " dB abaixo do total (desligado: " + juce::String (offRes.first - offRes.second, 1) + " dB) e o grave fica");
        }
        {   // a machine sozinha (MRBM-69 / MRM-0420), com o som interno
            double cut[2] {};
            for (int round = 0; round < 2; ++round)
            {
                BassMachineProcessor m;
                setP (m, "timbre", (float) mbm::SuperTone); setP (m, "drumsOn", 0.0f); setP (m, "keysOn", 0.0f);
                setP (m, "eq2k", round == 1 ? 1.0f : 0.0f); setP (m, "preview", 1.0f);
                m.prepareToPlay (48000.0, 512);
                const auto rr = measure (m, 48000.0, 300); cut[round] = rr.first - rr.second;
            }
            check (cut[1] < -70.0 && cut[0] - cut[1] > 20.0, "BASS MACHINE sozinha: com EQ -2k o que sobra acima de 3 kHz fica "
                   + juce::String (cut[1], 1) + " dB abaixo do total (desligado: " + juce::String (cut[0], 1) + " dB)");
        }
    }
    std::printf ("\n%s\n", failures == 0 ? "TODOS OS TESTES PASSARAM" : "HOUVE FALHAS");
    return failures == 0 ? 0 : 1;
}
