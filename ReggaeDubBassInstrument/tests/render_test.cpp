// Teste offline do Dub Bass (sem DAW):
//   cmake -B build-test -DDUBBASS_TESTS=ON && cmake --build build-test --target DubBassTest
//   ./build-test/DubBassTest_artefacts/Release/DubBassTest [pasta-de-saida]
// Toca uma linha de baixo nos dois modos, confere que o maximizer segura o teto,
// que não sai NaN, que o estado salva/carrega, e salva interface.png com a válvula quente.
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

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

    std::printf ("\n%s\n", failures == 0 ? "TODOS OS TESTES PASSARAM" : "HOUVE FALHAS");
    return failures == 0 ? 0 : 1;
}
