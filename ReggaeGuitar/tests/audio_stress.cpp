// Teste no estilo do pluginval ("Audio processing"): várias taxas e blocos, notas MIDI aleatórias 0..127,
// pitch bend e CC; falha se aparecer NaN/Inf ou pico absurdo.
//   cmake -B build-snap -DRG_SNAPSHOT=ON -DRG_FORMATS=VST3 && cmake --build build-snap --target ReggaeGuitarStress
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include <cmath>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int falhas = 0;
    const int modo = argc > 2 ? std::atoi (argv[2]) : 0;   // depuração: 1 sem ruído, 2 sem bend/CC, 3 só notas < 60, 4 só notas >= 60
    for (int seed = 0; seed < (argc > 1 ? std::atoi (argv[1]) : 40); ++seed)
    {
        juce::Random rnd (seed);
        ReggaeGuitarProcessor proc;
        proc.applyPlayer (seed % 12);
        if (seed % 2) proc.setParam ("play", 1.0f);
        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (int bs : { 64, 128, 256, 512, 1024 })
            {
                proc.prepareToPlay (sr, bs);
                juce::AudioBuffer<float> b (2, bs);
                for (int k = 0; k < 10; ++k)
                {
                    juce::MidiBuffer m;
                    for (int n = 0; n < 4; ++n)
                    {
                        int note = rnd.nextInt (128); if (modo == 3) note %= 60; if (modo == 4) note = 60 + note % 68; const int pos = rnd.nextInt (bs);
                        if (rnd.nextBool()) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) (1 + rnd.nextInt (127))), pos);
                        else m.addEvent (juce::MidiMessage::noteOff (1, note), pos);
                    }
                    if (modo != 2 && rnd.nextInt (4) == 0) m.addEvent (juce::MidiMessage::pitchWheel (1, rnd.nextInt (16384)), 0);
                    if (modo != 2 && rnd.nextInt (4) == 0) m.addEvent (juce::MidiMessage::controllerEvent (1, 1, rnd.nextInt (128)), 0);
                    for (int c = 0; c < 2; ++c) for (int i = 0; i < bs; ++i) b.setSample (c, i, modo == 1 ? 0.0f : rnd.nextFloat() * 2 - 1);
                    proc.processBlock (b, m);
                    for (int c = 0; c < 2; ++c) for (int i = 0; i < bs; ++i)
                    {
                        const float x = b.getSample (c, i);
                        if (! std::isfinite (x) || std::abs (x) > 50.0f)
                        {
                            if (falhas < 10) std::printf ("seed %d sr %.0f bs %d bloco %d: %g\n", seed, sr, bs, k, x);
                            ++falhas; c = 2; break;
                        }
                    }
                }
            }
    }
    std::printf (falhas ? "FALHOU: %d blocos ruins\n" : "OK: sem NaN\n", falhas);
    return falhas ? 1 : 0;
}
