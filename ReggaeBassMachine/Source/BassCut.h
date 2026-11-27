#pragma once
// EQ da BASS MACHINE: SEM CORTE, -2k ou -1k. Os dois cortes deixam só os graves.
// Passa-baixa Butterworth de 16ª ordem (8 biquads, 96 dB por oitava), com a quina um pouco abaixo da marca
// para a própria marca já sair cortada:
//   -2k: quina em 1,8 kHz → 1,5 kHz passa inteiro, -15 dB em 2 kHz, -46 dB em 2,5 kHz, -71 dB em 3 kHz
//   -1k: quina em 900 Hz  → 750 Hz passa inteiro,  -15 dB em 1 kHz, -46 dB em 1,25 kHz, -71 dB em 1,5 kHz
// Trocar de corte cruza os sons em 20 ms (sem estalo). Os filtros rodam sempre, para não estalar ao ligar.
// Usado pela BASS MACHINE (MRBM-69 e MRM-0420, em renderSynth) e pelo Mastor Reggae Bass MRB-69 (no baixo, antes do maximizer),
// e desenhado no EQ GRÁFICO do MRB-69. Versão para celular: site/bassmachine/ (os mesmos 8 biquads e Q).
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace mbm
{
class BassCut
{
public:
    enum Mode { Off = 0, Cut2k = 1, Cut1k = 2, NumModes };
    static constexpr int kStages = 8;
    // Q de cada estágio de um Butterworth de 16ª ordem: 1 / (2 sen ((2k-1) pi / 32))
    static constexpr std::array<float, kStages> kQ { 5.10114862f, 1.72244710f, 1.06067769f, 0.78815462f,
                                                     0.64682178f, 0.56694403f, 0.52249861f, 0.50241929f };
    static float cornerHz (int mode) { return mode == Cut1k ? 900.0f : 1800.0f; }   // quina do filtro
    static float markHz   (int mode) { return mode == Cut1k ? 1000.0f : 2000.0f; }  // o que o botão promete cortar
    static const char* name (int mode) { return mode == Cut1k ? "-1k" : mode == Cut2k ? "-2k" : "SEM CORTE"; }

    void prepare (double sampleRate)
    {
        for (int b = 0; b < 2; ++b)
            for (int s = 0; s < kStages; ++s)
            {
                auto c = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, cornerHz (b + 1), kQ[(size_t) s]);
                for (auto& ch : banks[(size_t) b]) { ch[(size_t) s].coefficients = c; ch[(size_t) s].reset(); }
            }
        for (int b = 0; b < 2; ++b)
        {
            mix[(size_t) b].reset (sampleRate, 0.02);
            mix[(size_t) b].setCurrentAndTargetValue (mode == b + 1 ? 1.0f : 0.0f);
        }
    }

    void setMode (int m)
    {
        mode = juce::jlimit (0, NumModes - 1, m);
        for (int b = 0; b < 2; ++b) mix[(size_t) b].setTargetValue (mode == b + 1 ? 1.0f : 0.0f);
    }
    int getMode() const noexcept { return mode; }

    // processa até 2 canais no lugar: saída = seco·(1 - m2 - m1) + corte2k·m2 + corte1k·m1
    void process (juce::AudioBuffer<float>& buf, int numSamples)
    {
        const int nch = juce::jmin (2, buf.getNumChannels());
        for (int i = 0; i < numSamples; ++i)
        {
            const float m2 = mix[0].getNextValue(), m1 = mix[1].getNextValue();
            for (int ch = 0; ch < nch; ++ch)
            {
                float* d = buf.getWritePointer (ch);
                const float x = d[i];
                const float y2 = run (0, ch, x), y1 = run (1, ch, x);
                d[i] = x * (1.0f - m2 - m1) + y2 * m2 + y1 * m1;
            }
        }
    }

private:
    float run (int bank, int ch, float x)
    {
        for (auto& f : banks[(size_t) bank][(size_t) ch]) x = f.processSample (x);
        return x;
    }

    std::array<std::array<std::array<juce::dsp::IIR::Filter<float>, kStages>, 2>, 2> banks;   // [corte][canal][estágio]
    std::array<juce::SmoothedValue<float>, 2> mix;
    int mode = Off;
};
}
