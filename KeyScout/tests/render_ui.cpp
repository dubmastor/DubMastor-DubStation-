// Teste com JUCE do Mastor Key Scout MKS-0420: o áudio passa igual, o link chega ao Mastor Tune e a
// interface vira PNG (imagem do card do site).
//   cmake -B build -DMKS_RENDER_TEST=ON -DMKS_FORMATS=VST3 && cmake --build build --target mks_render
//   ./build/mks_render_artefacts/Release/mks_render <pasta de saída>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static void pump (int times = 6) { for (int i = 0; i < times; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (60); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory());
    out.createDirectory();
    int fails = 0;

    KeyScoutProcessor proc;
    const double fs = 48000.0;
    proc.setPlayConfigDetails (2, 2, fs, 512);
    proc.prepareToPlay (fs, 512);

    // riddim sintético em A menor: Am - Dm - Em - Am, baixo + skank
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    double ph[4] = {}; long long n = 0; double maxDiff = 0;
    const int prog[4] = { 9, 2, 4, 9 };
    for (int b = 0; b < (int) (fs * 24 / 512); ++b)
    {
        for (int i = 0; i < 512; ++i, ++n)
        {
            const int r = prog[(n / (long long) (fs * 1.6)) % 4];
            const int notes[4] = { 36 + r, 60 + r, 63 + r, 67 + r };
            const double beat = std::fmod ((double) n / fs, 0.4);
            float s = 0;
            for (int k = 0; k < 4; ++k)
            {
                ph[k] += 440.0 * std::pow (2.0, (notes[k] - 69) / 12.0) / fs; if (ph[k] > 1) ph[k] -= 1;
                const float v = (float) (std::sin (2 * M_PI * ph[k]) + 0.4 * std::sin (4 * M_PI * ph[k]));
                s += k == 0 ? 0.25f * v : 0.08f * v * ((beat > 0.2 && beat < 0.32) ? 1.0f : 0.15f);
            }
            buf.setSample (0, i, s); buf.setSample (1, i, s * 0.9f);
        }
        juce::AudioBuffer<float> copy (buf);
        proc.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i) maxDiff = std::max (maxDiff, (double) std::abs (buf.getSample (c, i) - copy.getSample (c, i)));
        if (b % 8 == 0) pump (1);
    }
    pump (4);
    std::printf ("audio: diferenca maxima %.9f (tem que ser 0)\n", maxDiff);
    if (maxDiff != 0.0) ++fails;

    auto st = proc.getStatus();
    std::printf ("tom: %d %s, acorde %d%s, sinal %d\n", st.keyRoot, st.keyMinor ? "menor" : "maior", st.chordRoot, st.chordMinor ? "m" : "", (int) st.signal);
    if (! (st.keyRoot == 9 && st.keyMinor)) ++fails;

    scoutlink::Reader reader;
    scoutlink::Data d {};
    bool got = false;
    for (int k = 0; k < 3 && ! got; ++k) { got = reader.read (d); pump (2); }
    std::printf ("link: lido %d, tom %d %s, sinal %d\n", (int) got, d.keyRoot, d.keyMinor ? "menor" : "maior", d.signal);
    if (! got || d.keyRoot != 9 || ! d.keyMinor) ++fails;

    auto* ed = dynamic_cast<KeyScoutEditor*> (proc.createEditor());
    ed->setSize (mksui::Panel::kW, mksui::Panel::kH);
    pump (8);
    auto img = ed->getPanel().createComponentSnapshot (ed->getPanel().getLocalBounds(), true, 1.0f);
    auto f = out.getChildFile ("mks-interface.png");
    f.deleteFile();
    { juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os); }
    std::printf ("  %s (%d x %d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    delete ed;

    std::printf (fails ? "ERRO (%d)\n" : "OK\n", fails);
    return fails ? 1 : 0;
}
