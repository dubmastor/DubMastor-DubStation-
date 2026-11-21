#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

//==============================================================================
// Maximizer: limiter brickwall com lookahead de 1.5 ms, link estéreo,
// ganho de entrada, ceiling e release. Com BYPASS o sinal passa limpo,
// mas a latência continua a mesma (sem saltos de fase no DAW).
//==============================================================================
class Maximizer
{
public:
    void prepare (double sampleRate, int numChannels)
    {
        sr        = sampleRate;
        channels  = juce::jlimit (1, 2, numChannels);
        lookahead = std::max (1, (int) std::round (sr * 0.0015));

        delay.assign ((size_t) channels, std::vector<float> ((size_t) lookahead, 0.0f));
        targets.assign ((size_t) lookahead, 1.0f);

        writePos    = 0;
        gain        = 1.0f;
        attackCoeff = 1.0f - std::exp (-4.0f / (float) lookahead);

        inGain.reset (sr, 0.05);
        inGain.setCurrentAndTargetValue (1.0f);
        setParameters (0.0f, -0.3f, 120.0f, false);
    }

    int getLatencySamples() const noexcept { return lookahead; }

    void setParameters (float gainDb, float ceilingDb, float releaseMs, bool shouldBypass)
    {
        bypassed = shouldBypass;
        inGain.setTargetValue (juce::Decibels::decibelsToGain (gainDb));
        ceiling      = juce::Decibels::decibelsToGain (ceilingDb);
        releaseCoeff = 1.0f - std::exp (-1.0f / (std::max (1.0f, releaseMs) * 0.001f * (float) sr));
    }

    // Retorna o menor ganho aplicado no bloco (para o medidor de GR)
    float process (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const int nCh = std::min (channels, buffer.getNumChannels());
        auto* const* data = buffer.getArrayOfWritePointers();

        float minGain = 1.0f;
        float in[2] = { 0.0f, 0.0f };

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = inGain.getNextValue();
            float peak = 0.0f;

            for (int ch = 0; ch < nCh; ++ch)
            {
                in[ch] = bypassed ? data[ch][i] : data[ch][i] * g;
                peak = std::max (peak, std::abs (in[ch]));
            }

            targets[(size_t) writePos] = (! bypassed && peak > ceiling) ? ceiling / peak : 1.0f;

            const float minTarget = *std::min_element (targets.begin(), targets.end());
            gain += (minTarget - gain) * (minTarget < gain ? attackCoeff : releaseCoeff);

            for (int ch = 0; ch < nCh; ++ch)
            {
                auto& line = delay[(size_t) ch];
                const float delayed = line[(size_t) writePos];
                line[(size_t) writePos] = in[ch];

                data[ch][i] = bypassed ? delayed
                                       : juce::jlimit (-ceiling, ceiling, delayed * gain);
            }

            if (++writePos >= lookahead)
                writePos = 0;

            minGain = std::min (minGain, gain);
        }

        if (bypassed)
        {
            gain = 1.0f;
            return 1.0f;
        }

        return minGain;
    }

private:
    double sr = 44100.0;
    int channels = 2, lookahead = 64, writePos = 0;
    bool bypassed = false;
    float gain = 1.0f, ceiling = 1.0f, attackCoeff = 0.1f, releaseCoeff = 0.001f;
    juce::SmoothedValue<float> inGain;
    std::vector<std::vector<float>> delay;
    std::vector<float> targets;
};
