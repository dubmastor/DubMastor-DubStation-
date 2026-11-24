// Teste do Mastor Reggae Keys MRK-80 com a aba CHORD MACHINE (MRCM-1240 embutido).
// cmake -B build -DJUCE_DIR=... -DROOTSKEYS_TESTS=ON && cmake --build build --target RootsKeysTest
// ./RootsKeysTest [pasta-das-imagens]   -> falha (exit 1) se algum item não passar
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../ReggaeChordMachine/Source/PluginProcessor.h"
#include "../../RiddimBank/Source/RiddimBankMenu.h"

namespace
{
int failures = 0;
void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "ok   " : "FALHA", what.toRawUTF8());
    if (! ok) ++failures;
}

struct FakePlayHead : juce::AudioPlayHead
{
    double bpm = 90.0, ppq = 0.0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setPpqPosition (ppq);
        p.setIsPlaying (playing);
        p.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        return p;
    }
};

void setP (juce::AudioProcessorValueTreeState& s, const juce::String& id, float v)
{
    auto* p = s.getParameter (id);
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}
float getP (juce::AudioProcessorValueTreeState& s, const juce::String& id) { return s.getRawParameterValue (id)->load(); }

// toca `seconds` de DAW e devolve o RMS; ruim = NaN/Inf
double render (RootsKeysProcessor& p, double seconds, bool& finite)
{
    const double sr = 48000.0; const int bs = 512;
    FakePlayHead ph;
    p.setPlayHead (&ph);
    p.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> buf (2, bs);
    double sum = 0.0; long n = 0; finite = true;
    const int blocks = (int) (seconds * sr / bs);
    for (int b = 0; b < blocks; ++b)
    {
        // deixa a thread de mensagens montar o loop da machine (timer de 30 ms)
        if (b % 8 == 0) juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
        juce::MidiBuffer midi;
        buf.clear();
        p.processBlock (buf, midi);
        ph.ppq += bs / sr * ph.bpm / 60.0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < bs; ++i)
            {
                const float v = buf.getSample (ch, i);
                if (! std::isfinite (v)) finite = false;
                sum += (double) v * v; ++n;
            }
    }
    p.setPlayHead (nullptr);
    return std::sqrt (sum / (double) juce::jmax (1L, n));
}
// toca com a DAW parada (só o PREVIEW da machine) e devolve o RMS de cada lado
void renderLR (RootsKeysProcessor& p, double seconds, double& l, double& r)
{
    const double sr = 48000.0; const int bs = 512;
    FakePlayHead ph; ph.playing = false;
    p.setPlayHead (&ph);
    p.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> buf (2, bs);
    double sl = 0, sr2 = 0; long n = 0;
    for (int b = 0; b < (int) (seconds * sr / bs); ++b)
    {
        if (b % 8 == 0) juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
        juce::MidiBuffer midi; buf.clear(); p.processBlock (buf, midi);
        for (int i = 0; i < bs; ++i) { sl += (double) buf.getSample (0, i) * buf.getSample (0, i); sr2 += (double) buf.getSample (1, i) * buf.getSample (1, i); ++n; }
    }
    p.setPlayHead (nullptr);
    l = std::sqrt (sl / (double) juce::jmax (1L, n)); r = std::sqrt (sr2 / (double) juce::jmax (1L, n));
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                       : juce::File::getCurrentWorkingDirectory();

    {   // 1) machine desligada: a DAW tocando não gera acorde nenhum
        RootsKeysProcessor p;
        bool fin = true;
        const double rms = render (p, 4.0, fin);
        check (fin && rms < 1.0e-5, "CHORD MACHINE desligada fica em silencio (rms " + juce::String (rms, 7) + ")");
    }
    {   // 2) ligada, SOM INTERNO desligado: os acordes tocam no piano do instrumento
        RootsKeysProcessor p;
        check (getP (p.getMachine().apvts, "sound") < 0.5f, "instancia nova: SOM INTERNO da machine desligado");
        setP (p.apvts, "machineOn", 1.0f);
        bool fin = true;
        const double rms = render (p, 6.0, fin);
        check (fin && rms > 1.0e-3, "CHORD MACHINE toca no piano do MRK-80 (rms " + juce::String (rms, 5) + ")");
        setP (p.apvts, "mode", 1.0f);   // projeto antigo no modo orgao: 3.1 toca no piano
        const double rmsOld = render (p, 6.0, fin);
        check (fin && std::abs (rmsOld - rms) < rms * 0.25, "modo orgao antigo toca no piano (rms " + juce::String (rmsOld, 5) + ")");
        // BANG BANG: o bang dobrado, no & e no a de cada tempo (caixa no 2 e no 4)
        setP (p.getMachine().apvts, "pattern", 1.0f);
        double beats = 0;
        const auto ev = dub::buildLoop (p.getMachine().getSlots(), p.getMachine().getSettings(), beats);
        int onAnd = 0, onA = 0, other = 0;
        for (const auto& e : ev)
        {
            if (e.channel != dub::ChSkank) continue;
            const int st = (int) std::lround (e.start * 4.0) % 4;
            if (st == 2) ++onAnd; else if (st == 3) ++onA; else ++other;
        }
        check (onAnd > 0 && onA == onAnd && other == 0, "BANG BANG: skank no & e no a (" + juce::String (onAnd) + " / " + juce::String (onA) + ")");
        const double rmsBang = render (p, 6.0, fin);
        check (fin && rmsBang > rms, "BANG BANG toca mais (rms " + juce::String (rmsBang, 5) + ")");
    }
    {   // 3) SOM INTERNO ligado (projeto antigo): 3.1 toca skank e bubble no piano do instrumento, sem o Hammond da machine
        RootsKeysProcessor p;
        setP (p.apvts, "machineOn", 1.0f);
        setP (p.getMachine().apvts, "sound", 1.0f);
        setP (p.getMachine().apvts, "bubble", 1.0f);
        bool fin = true;
        const double rms = render (p, 6.0, fin);
        check (fin && rms > 1.0e-3, "SOM INTERNO antigo: skank + bubble no piano (rms " + juce::String (rms, 5) + ")");
    }
    {   // 4) projeto salvo: a machine volta igual
        RootsKeysProcessor a, b;
        setP (a.getMachine().apvts, "root0", 4.0f);
        setP (a.getMachine().apvts, "qual0", 1.0f);
        setP (a.getMachine().apvts, "sound", 1.0f);
        setP (a.apvts, "machineOn", 1.0f);
        juce::MemoryBlock m;
        a.getStateInformation (m);
        b.setStateInformation (m.getData(), (int) m.getSize());
        auto& mb = b.getMachine().apvts;
        check (juce::roundToInt (getP (mb, "root0")) == 4 && juce::roundToInt (getP (mb, "qual0")) == 1
                 && getP (mb, "sound") > 0.5f && getP (b.apvts, "machineOn") > 0.5f,
               "projeto salvo reabre com a CHORD MACHINE igual");
    }
    {   // 5) projeto do Roots Keys 2.4 (aba DUB CHORDS): vira a CHORD MACHINE
        juce::ValueTree old ("PARAMS");
        auto add = [&old] (const char* id, double v)
        {
            juce::ValueTree c ("PARAM"); c.setProperty ("id", id, nullptr); c.setProperty ("value", v, nullptr);
            old.appendChild (c, nullptr);
        };
        add ("mode", 1); add ("dcOn", 1); add ("dcRoot0", 4); add ("dcQual0", 3 /* m7 */); add ("dcDur0", 1);
        add ("dcRoot1", 9); add ("dcQual1", 4 /* maj7 */); add ("dcAct1", 1); add ("dcAct2", 0); add ("dcAct3", 0);
        add ("dcPattern", 0 /* Auto: orgao = bubble */); add ("dcRegister", 55); add ("dcBpm", 82);
        old.setProperty ("program", 0, nullptr);
        juce::MemoryBlock m;
        juce::AudioProcessor::copyXmlToBinary (*old.createXml(), m);
        RootsKeysProcessor p;
        p.setStateInformation (m.getData(), (int) m.getSize());
        auto& mb = p.getMachine().apvts;
        check (juce::roundToInt (getP (mb, "root0")) == 4 && juce::roundToInt (getP (mb, "qual0")) == 1
                 && juce::roundToInt (getP (mb, "dur0")) == 1 && juce::roundToInt (getP (mb, "root1")) == 9
                 && juce::roundToInt (getP (mb, "qual1")) == 0 && getP (mb, "on1") > 0.5f && getP (mb, "on2") < 0.5f,
               "projeto antigo: acordes da DUB CHORDS (m7 -> menor, maj7 -> maior)");
        check (getP (mb, "bubble") > 0.5f && juce::roundToInt (getP (mb, "center")) == 55
                 && juce::roundToInt (getP (mb, "bpm")) == 82 && getP (mb, "sound") < 0.5f,
               "projeto antigo: padrao Auto no orgao vira bubble, registro e BPM mantidos");
        check (getP (p.apvts, "machineOn") > 0.5f && juce::roundToInt (getP (p.apvts, "mode")) == 1,
               "projeto antigo: 'tocar com a DAW' vira CHORD MACHINE ligada (o valor do modo fica guardado)");
    }
    {   // 6) 3.3: PREVIEW na aba do instrumento, PAN do piano e da guitarra, presets do mega banco
        RootsKeysProcessor p;
        check (std::abs (getP (p.apvts, "pianoPan")) < 1e-6f && std::abs (getP (p.apvts, "gtrPan")) < 1e-6f, "PAN do piano e da guitarra começam no centro");
        auto* pp = p.machinePreviewParam();
        check (pp != nullptr, "PREVIEW da aba INSTRUMENTO ligado ao PREVIEW da CHORD MACHINE");
        if (pp != nullptr) pp->setValueNotifyingHost (1.0f);
        double l = 0, r = 0;
        renderLR (p, 3.0, l, r);
        check (l > 0.01 && r > 0.01 && std::abs (l - r) / (l + r) < 0.2, "PREVIEW toca com a DAW parada, piano no centro (L " + juce::String (l, 4) + " R " + juce::String (r, 4) + ")");
        setP (p.apvts, "pianoPan", -1.0f);
        renderLR (p, 3.0, l, r);
        check (l > 0.01 && r < l * 0.2, "PAN do piano todo para a esquerda (sobra só o reverb) (L " + juce::String (l, 4) + " R " + juce::String (r, 5) + ")");
        setP (p.apvts, "pianoPan", 0.0f);
        setP (p.apvts, "gtrOn", 1.0f); setP (p.apvts, "pianoLevel", 0.0f); setP (p.apvts, "gtrPan", 1.0f);
        renderLR (p, 3.0, l, r);
        check (r > 0.003 && l < r * 0.2, "PAN da guitarra do skank toda para a direita (L " + juce::String (l, 5) + " R " + juce::String (r, 4) + ")");
        int withH = 0, pend = 0;
        for (int i = 0; i < riddimbank::numEntries; ++i) { withH += riddimbank::hasHarmony (i); pend += riddimbank::entries[i].harmony >= 2; }
        check (withH > 300 && withH + pend == riddimbank::numEntries, "mega banco: " + juce::String (withH) + juce::String::fromUTF8 (" músicas com harmonia, ") + juce::String (pend) + " pendentes fora");
        {
            juce::PopupMenu m; riddimbank::addMegaMenus (m, 0, "MRK-80-teste");
            check (m.getNumItems() >= 6, "mega banco: menu com estilos, riddims A-Z, musicas A-Z e favoritos");
            const bool before = riddimbank::isFavorite ("MRK-80-teste", 0);
            riddimbank::toggleFavorite ("MRK-80-teste", 0);
            check (riddimbank::isFavorite ("MRK-80-teste", 0) != before, "favoritar e desfavoritar uma musica");
            riddimbank::toggleFavorite ("MRK-80-teste", 0);
            riddimbank::favFile ("MRK-80-teste").getFile().deleteFile();
        }
    }
    {   // 7) capturas da interface
        RootsKeysProcessor p;
        setP (p.getMachine().apvts, "bubble", 1.0f);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        auto* rk = dynamic_cast<RootsKeysEditor*> (ed.get());
        for (int f = 0; f < 6; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
        auto save = [&] (const juce::String& name)
        {
            for (int f = 0; f < 6; ++f) juce::MessageManager::getInstance()->runDispatchLoopUntil (34);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.4f);
            juce::File f = outDir.getChildFile (name);
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat fmt;
            check (os.openedOk() && fmt.writeImageToStream (img, os), "imagem " + name);
        };
        save ("interface-piano.png");
        {   // o gabinete muda com o timbre: Rhodes, Suitcase, Wurlitzer, Yamaha (Grand/CP/C7) e Casio
            const int types[5] = { 10, 11, 12, 4, 9 };
            const char* names[5] = { "interface-piano-rhodes.png", "interface-piano-suitcase.png", "interface-piano-wurlitzer.png",
                                     "interface-piano-yamaha.png", "interface-piano-casio.png" };
            for (int i = 0; i < 5; ++i) { setP (p.apvts, "pianoType", (float) types[i]); save (names[i]); }
            setP (p.apvts, "pianoType", 7.0f);
            check (getP (p.apvts, "gtrOn") < 0.5f, "GTR SKANK começa desligado");
            setP (p.apvts, "gtrOn", 1.0f);
            save ("interface-piano-gtr.png");
            setP (p.apvts, "gtrOn", 0.0f);
        }
        rk->setPage (1);
        save ("interface-chord-machine.png");
        setP (p.getMachine().apvts, "pattern", 1.0f);
        save ("interface-chord-machine-bang-bang.png");
    }

    std::printf ("\n%s\n", failures == 0 ? "TUDO OK" : "HA FALHAS");
    return failures == 0 ? 0 : 1;
}
