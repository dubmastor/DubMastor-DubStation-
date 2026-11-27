#pragma once
// Botão EQ da BASS MACHINE: corta tudo acima de 2 kHz no baixo.
// Passa-baixa Butterworth de 16ª ordem (8 biquads, 96 dB por oitava) com a quina em 1,8 kHz, para 2 kHz já sair cortado:
// 1,5 kHz passa inteiro, -3 dB em 1,8 kHz, -15 dB em 2 kHz, -46 dB em 2,5 kHz, -71 dB em 3 kHz, -111 dB em 4 kHz.
// Ligar e desligar cruza o som seco e o filtrado em 20 ms (sem estalo). O filtro roda sempre, para não estalar ao ligar.
// Usado pela BASS MACHINE (MRBM-69 e MRM-0420, em renderSynth) e pelo Mastor Reggae Bass MRB-69 (no baixo, antes do maximizer).
// Versão para celular: site/bassmachine/ (8 BiquadFilter lowpass com a mesma quina e os mesmos Q).
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace mbm
{
class HighCut2k
{
public:
    static constexpr float kCutoffHz = 1800.0f;
    static constexpr int   kStages   = 8;
    // Q de cada estágio de um Butterworth de 16ª ordem: 1 / (2 sen ((2k-1) pi / 32))
    static constexpr std::array<float, kStages> kQ { 5.10114862f, 1.72244710f, 1.06067769f, 0.78815462f,
                                                     0.64682178f, 0.56694403f, 0.52249861f, 0.50241929f };

    void prepare (double sampleRate)
    {
        for (int s = 0; s < kStages; ++s)
        {
            auto c = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, kCutoffHz, kQ[(size_t) s]);
            for (auto& ch : filters) { ch[(size_t) s].coefficients = c; ch[(size_t) s].reset(); }
        }
        mix.reset (sampleRate, 0.02);
        mix.setCurrentAndTargetValue (on ? 1.0f : 0.0f);
    }

    void setOn (bool shouldBeOn) { on = shouldBeOn; mix.setTargetValue (on ? 1.0f : 0.0f); }
    bool isOn() const noexcept   { return on; }

    // processa até 2 canais no lugar
    void process (juce::AudioBuffer<float>& buf, int numSamples)
    {
        const int nch = juce::jmin (2, buf.getNumChannels());
        if (! mix.isSmoothing() && mix.getTargetValue() == 0.0f)
        {
            for (int ch = 0; ch < nch; ++ch)            // desligado: só mantém o filtro aquecido
                for (int i = 0; i < numSamples; ++i) runFilter (ch, buf.getSample (ch, i));
            return;
        }
        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mix.getNextValue();
            for (int ch = 0; ch < nch; ++ch)
            {
                float* d = buf.getWritePointer (ch);
                const float wet = runFilter (ch, d[i]);
                d[i] = d[i] + m * (wet - d[i]);
            }
        }
    }

private:
    float runFilter (int ch, float x)
    {
        for (auto& f : filters[(size_t) ch]) x = f.processSample (x);
        return x;
    }

    std::array<std::array<juce::dsp::IIR::Filter<float>, kStages>, 2> filters;
    juce::SmoothedValue<float> mix;
    bool on = false;
};
}
