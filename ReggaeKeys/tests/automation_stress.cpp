// Teste no estilo do pluginval "Automation": a cada sub-bloco de 32 amostras todos os parâmetros mudam para valores
// aleatórios, com notas MIDI aleatórias; várias taxas e tamanhos de bloco. Falha com NaN/Inf ou pico absurdo
// (e, compilado com -fsanitize=address, acusa qualquer acesso fora da memória).
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include <cmath>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int falhas = 0;
    const int seeds = argc > 1 ? std::atoi (argv[1]) : 6;
    for (int seed = 0; seed < seeds; ++seed)
    {
        juce::Random rnd (seed + 1);
        RootsKeysProcessor proc;
        auto params = proc.getParameters();
        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (int bs : { 64, 128, 256, 512, 1024 })
            {
                proc.prepareToPlay (sr, bs);
                juce::AudioBuffer<float> b (2, bs);
                for (int k = 0; k < 12; ++k)
                {
                    for (int sub = 0; sub < bs; sub += 32)
                    {
                        for (auto* prm : params) if (rnd.nextInt (3) == 0) prm->setValueNotifyingHost (rnd.nextFloat());
                        const int n = std::min (32, bs - sub);
                        juce::AudioBuffer<float> sb (b.getArrayOfWritePointers(), 2, sub, n);
                        juce::MidiBuffer m;
                        for (int e = 0; e < 3; ++e)
                        {
                            const int note = rnd.nextInt (128), pos = rnd.nextInt (n);
                            if (rnd.nextBool()) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) (1 + rnd.nextInt (127))), pos);
                            else m.addEvent (juce::MidiMessage::noteOff (1, note), pos);
                        }
                        if (rnd.nextInt (6) == 0) m.addEvent (juce::MidiMessage::pitchWheel (1, rnd.nextInt (16384)), 0);
                        if (rnd.nextInt (6) == 0) m.addEvent (juce::MidiMessage::controllerEvent (1, rnd.nextBool() ? 1 : 64, rnd.nextInt (128)), 0);
                        sb.clear();
                        proc.processBlock (sb, m);
                        for (int c = 0; c < 2; ++c)
                            for (int i = 0; i < n; ++i)
                            {
                                const float x = sb.getSample (c, i);
                                if (! std::isfinite (x) || std::abs (x) > 50.0f)
                                {
                                    if (falhas < 10) std::printf ("seed %d sr %.0f bs %d bloco %d: %g\n", seed, sr, bs, k, x);
                                    ++falhas; c = 2; break;
                                }
                            }
                    }
                }
            }
        std::printf ("seed %d ok\n", seed);
    }
    std::printf (falhas ? "FALHOU: %d blocos ruins\n" : "OK: automacao sem NaN nem crash\n", falhas);
    return falhas ? 1 : 0;
}
