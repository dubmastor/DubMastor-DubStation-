// Mastor Bass Machine MBM-710 — som do preview com o sistema de timbres do Dub Bass
//
// Usa as mesmas peças do Dub Bass (pasta ../../DubBass/Source): a voz BassVoice (Classic Sine com
// harmônicos ou 808), o filtro passa-baixa, as saturações TUBE / TAPE / FUZZ e o Maximizer.
// Cada TIMBRE é um conjunto de ajustes do Dub Bass; TONE e DRIVE mexem por cima do timbre.
// Se o Dub Bass mudar o som, o preview do Bass Machine acompanha.
#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../../DubBass/Source/BassVoice.h"
#include "../../DubBass/Source/Maximizer.h"

namespace mbm
{
struct DubBassPreset
{
    const char* name;
    const char* tagline;
    BassVoice::Mode mode;
    float harm2, harm3, pitchAmt, pitchTimeMs;
    float attackMs, decayMs, sustain, releaseMs;
    float cutoff, reso;
    int   drvType;     // 0 TUBE, 1 TAPE, 2 FUZZ
    float driveDb, drvToneHz, drvMix;
};

enum TimbreId { ClassicSine, SineQuente, SubPuro, FitaRoots, Eight08, Fuzz808, SlengTeng, NumTimbres };

inline const DubBassPreset& timbre (int i)
{
    using M = BassVoice::Mode;
    static const DubBassPreset t[NumTimbres] = {
        //  nome            descrição                        modo            h2     h3     808p   808t   A     D      S      R      cutoff  reso  tipo drive tone   mix
        { "CLASSIC SINE",  "PADRAO DO DUB BASS",  M::ClassicSine, 0.15f, 0.08f, 12.0f, 70.0f, 3.0f, 600.0f, 0.85f, 180.0f, 900.0f, 0.8f, 0, 6.0f,  5000.0f, 0.50f },
        { "SINE QUENTE",   "HARMONICOS + VALVULA",   M::ClassicSine, 0.45f, 0.25f, 12.0f, 70.0f, 3.0f, 600.0f, 0.80f, 160.0f, 1400.0f, 0.9f, 0, 12.0f, 4000.0f, 0.60f },
        { "SUB PURO",      "SENOIDE LIMPA, SEM DRIVE", M::ClassicSine, 0.0f,  0.0f,  12.0f, 70.0f, 5.0f, 800.0f, 0.95f, 200.0f, 320.0f, 0.7f, 0, 0.0f,  5000.0f, 0.00f },
        { "FITA ROOTS",    "SATURACAO DE FITA", M::ClassicSine, 0.30f, 0.15f, 12.0f, 70.0f, 2.0f, 420.0f, 0.60f, 140.0f, 700.0f, 1.2f, 1, 10.0f, 3000.0f, 0.70f },
        { "808",           "DROP DE PITCH",     M::Eight08,     0.10f, 0.05f, 12.0f, 70.0f, 1.0f, 600.0f, 0.0f,  180.0f, 2000.0f, 0.8f, 0, 6.0f,  5000.0f, 0.50f },
        { "808 FUZZ",      "808 CURTO COM FUZZ",            M::Eight08,     0.10f, 0.05f, 7.0f,  40.0f, 1.0f, 350.0f, 0.0f,  120.0f, 3000.0f, 0.8f, 2, 9.0f,  3500.0f, 0.45f },
        // teclado Casio de 1985: senoide + 3º harmônico quadrada no FUZZ, ataque seco e decaimento curto
        { "SLENG TENG",    "TECLADO DIGITAL 85 (JAMMYS)",   M::ClassicSine, 0.0f,  0.60f, 12.0f, 70.0f, 1.0f, 300.0f, 0.55f, 60.0f, 1800.0f, 0.9f, 2, 18.0f, 6000.0f, 1.00f },
    };
    return t[std::clamp (i, 0, NumTimbres - 1)];
}

//==============================================================================
class DubBassPreview
{
public:
    void prepare (double sampleRate, int blockSize)
    {
        sr = sampleRate;
        voice.prepare (sampleRate);
        filter.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, blockSize), 1 });
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        maximizer.prepare (sampleRate, 1);
        toneState = dcX = dcY = 0.0f;
        apply();
    }

    int getLatencySamples() const { return maximizer.getLatencySamples(); }

    void set (int timbreId, float tone, float drive)
    {
        if (timbreId != tim || ! juce::exactlyEqual (tone, toneKnob) || ! juce::exactlyEqual (drive, driveKnob))
        {
            tim = juce::jlimit (0, NumTimbres - 1, timbreId);
            toneKnob = tone;
            driveKnob = drive;
            apply();
        }
    }

    // ganho de entrada (dB): o sinal da voz antes da saturação e do maximizer
    void setInputGainDb (float db) { inputGain = juce::Decibels::decibelsToGain (juce::jlimit (-24.0f, 24.0f, db)); }

    void noteOn (int note, float velocity) { voice.noteOn (note, velocity); }
    void noteOff (int note)                { voice.noteOff (note); }
    void allNotesOff()                     { voice.allNotesOff(); }
    bool isActive() const                  { return voice.isActive(); }

    // gera a voz filtrada (antes da saturação) em out[start..end)
    void renderVoice (float* out, int start, int end)
    {
        for (int i = start; i < end; ++i)
            out[i] = filter.processSample (0, voice.process()) * inputGain;
    }

    // TUBE / TAPE / FUZZ + maximizer do Dub Bass, no buffer mono já gerado
    void finish (juce::AudioBuffer<float>& mono, int numSamples)
    {
        float* d = mono.getWritePointer (0);
        if (mix > 0.0f)
        {
            const auto twoPi = juce::MathConstants<double>::twoPi;
            const float toneCoeff = (float) (1.0 - std::exp (-twoPi * drvTone / sr));
            const float dcR = (float) (1.0 - twoPi * 8.0 / sr);
            for (int i = 0; i < numSamples; ++i)
            {
                const float x = d[i];
                float y = shape (x * driveGain, drvType);
                toneState += toneCoeff * (y - toneState);
                y = toneState;
                const float dc = y - dcX + dcR * dcY;
                dcX = y; dcY = dc;
                d[i] = x + mix * (dc - x);
            }
        }
        maximizer.setParameters (3.0f, -0.3f, 120.0f, false);
        maximizer.process (mono, numSamples);
        mono.applyGain (0, 0, numSamples, outputTrim);   // nível geral 5 dB abaixo do Dub Bass
    }

    static constexpr float outputTrimDb = -5.0f;

private:
    // as mesmas curvas do Dub Bass
    static float shape (float x, int type) noexcept
    {
        switch (type)
        {
            case 0: { constexpr float bias = 0.22f; static const float tb = std::tanh (bias); return std::tanh (x + bias) - tb; }
            case 1:  return (2.0f / juce::MathConstants<float>::pi) * std::atan (x * 1.3f);
            default: return juce::jlimit (-1.0f, 1.0f, x * 1.5f) * 0.9f;
        }
    }

    void apply()
    {
        const auto& p = timbre (tim);
        BassVoice::Params vp;
        vp.mode = p.mode; vp.harm2 = p.harm2; vp.harm3 = p.harm3;
        vp.glideMs = 20.0f; vp.pitchAmt = p.pitchAmt; vp.pitchTimeMs = p.pitchTimeMs;
        vp.attackMs = p.attackMs; vp.decayMs = p.decayMs; vp.sustain = p.sustain; vp.releaseMs = p.releaseMs;
        voice.setParams (vp);

        // TONE: ±1,5 oitava no cutoff do timbre; DRIVE: 0..2x o drive do timbre (0,5 = como no Dub Bass)
        const float cut = juce::jlimit (40.0f, (float) sr * 0.45f, p.cutoff * std::pow (2.0f, (toneKnob - 0.5f) * 3.0f));
        filter.setCutoffFrequency (cut);
        filter.setResonance (p.reso);
        const float extra = juce::jmax (0.0f, (driveKnob - 0.5f) * 24.0f);
        const float db = p.driveDb * juce::jmin (1.0f, driveKnob * 2.0f) + extra;
        driveGain = juce::Decibels::decibelsToGain (juce::jmin (36.0f, db));
        mix = (driveKnob <= 0.001f) ? 0.0f : juce::jmax (p.drvMix, driveKnob > 0.5f ? 0.5f : 0.0f);
        drvType = p.drvType;
        drvTone = p.drvToneHz;
    }

    BassVoice voice;
    juce::dsp::StateVariableTPTFilter<float> filter;
    Maximizer maximizer;
    double sr = 44100.0;
    int tim = -1, drvType = 0;
    float toneKnob = 0.5f, driveKnob = 0.5f, driveGain = 1.0f, mix = 0.5f, drvTone = 5000.0f;
    float toneState = 0, dcX = 0, dcY = 0;
    float inputGain = 1.0f;
    const float outputTrim = juce::Decibels::decibelsToGain (outputTrimDb);
};

} // namespace mbm
