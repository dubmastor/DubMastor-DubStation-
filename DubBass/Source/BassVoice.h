#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

//==============================================================================
// Voz monofônica de baixo: senoide + harmônicos 2 e 3 (modo Classic Sine)
// ou senoide com envelope de pitch + saturação interna (modo 808).
// Mono/legato com glide, retrigger sempre no modo 808.
//==============================================================================
class BassVoice
{
public:
    enum class Mode { ClassicSine = 0, Eight08 = 1, SuperTone = 2 };

    struct Params
    {
        Mode  mode        = Mode::ClassicSine;
        float harm2       = 0.15f;
        float harm3       = 0.08f;
        float glideMs     = 20.0f;
        float pitchAmt    = 12.0f;   // semitons (808)
        float pitchTimeMs = 70.0f;   // constante de tempo do drop (808)
        float attackMs    = 3.0f;
        float decayMs     = 600.0f;
        float sustain     = 0.85f;
        float releaseMs   = 180.0f;
    };

    BassVoice() { held.reserve (128); }

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        setParams (params);
    }

    void reset()
    {
        held.clear();
        env.reset();
        phase    = 0.0;
        pitchEnv = 0.0f;
    }

    void setParams (const Params& p)
    {
        params = p;
        const auto sr = (float) sampleRate;
        env.setParameters (p.attackMs, p.decayMs,
                           p.mode == Mode::Eight08 ? 0.0f : p.sustain,
                           p.releaseMs, sr);

        glideCoeff = p.glideMs <= 0.5f
                       ? 1.0f
                       : 1.0f - std::exp (-1.0f / (p.glideMs * 0.001f * 0.33f * sr));

        clickLen = std::max (1, (int) (0.004f * sr));
        dcR = 1.0f - 6.2832f * 8.0f / sr;
        pitchEnvCoeff = std::exp (-1.0f / (std::max (1.0f, p.pitchTimeMs) * 0.001f * sr));
    }

    void setPitchBend (float semitones) noexcept { pitchBend = semitones; }

    bool  isActive() const noexcept       { return env.isActive(); }
    float getCurrentNote() const noexcept { return currentNote + pitchBend; }

    void noteOn (int note, float velocity)
    {
        const bool wasIdle = ! env.isActive();
        const bool legato  = ! held.empty();

        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        held.push_back (note);
        targetNote = (float) note;

        if (wasIdle)
        {
            currentNote = targetNote;
            phase = 0.0;
        }

        if (params.glideMs <= 0.5f)
            currentNote = targetNote;

        if (wasIdle || ! legato || params.mode == Mode::Eight08)
        {
            velTarget = 0.35f + 0.65f * velocity;
            if (wasIdle) velGain = velTarget;
            env.noteOn();
            pitchEnv = 1.0f;
            clickPos = 0;
        }
    }

    void noteOff (int note)
    {
        held.erase (std::remove (held.begin(), held.end(), note), held.end());

        if (held.empty())
        {
            env.noteOff();
        }
        else
        {
            targetNote = (float) held.back();
            if (params.glideMs <= 0.5f)
                currentNote = targetNote;
        }
    }

    void allNotesOff()
    {
        held.clear();
        env.noteOff();
    }

    float process() noexcept
    {
        if (! env.isActive())
            return 0.0f;

        currentNote += (targetNote - currentNote) * glideCoeff;
        velGain     += (velTarget - velGain) * 0.002f;

        float semis = currentNote + pitchBend;

        if (params.mode == Mode::Eight08)
        {
            semis    += params.pitchAmt * pitchEnv;
            pitchEnv *= pitchEnvCoeff;
        }

        const double freq = 440.0 * std::pow (2.0, (semis - 69.0) / 12.0);
        constexpr double twoPi = juce::MathConstants<double>::twoPi;

        phase += twoPi * freq / sampleRate;
        while (phase >= twoPi)
            phase -= twoPi;

        const auto ph  = (float) phase;
        const double nyq = sampleRate * 0.45;

        float s    = std::sin (ph);
        float norm = 1.0f;

        if (params.harm2 > 0.0f && freq * 2.0 < nyq)
        {
            s    += params.harm2 * 0.5f * std::sin (2.0f * ph);
            norm += params.harm2 * 0.5f;
        }

        if (params.harm3 > 0.0f && freq * 3.0 < nyq)
        {
            s    += params.harm3 * 0.33f * std::sin (3.0f * ph);
            norm += params.harm3 * 0.33f;
        }

        s /= norm;

        if (params.mode == Mode::SuperTone)
            s = superTone (ph, freq);

        if (params.mode == Mode::Eight08)
        {
            // corpo: segundo harmônico que some junto com o drop de pitch (o "knock" do 808)
            s += 0.30f * pitchEnv * std::sin (2.0f * ph);

            // clique de ataque: rajada curtinha na 4ª harmônica, como o bumbo do 808
            if (clickPos < clickLen)
            {
                const float w = 1.0f - (float) clickPos / (float) clickLen;
                s += 0.45f * w * w * std::sin (4.0f * ph);
                ++clickPos;
            }

            // saturação assimétrica (harmônicos pares = grave mais gordo e audível em caixa pequena)
            constexpr float drive = 1.9f, bias = 0.18f;
            static const float tb = std::tanh (bias), k = 1.0f / (std::tanh (drive + bias) - tb);
            s = (std::tanh (drive * s + bias) - tb) * k;
        }

        // tira o DC que a saturação assimétrica deixa
        const float y = s - dcX + dcR * dcY;
        dcX = s; dcY = y;
        s = y;

        return s * env.next() * velGain;
    }

private:
    //==========================================================================
    // SUPER TONE: o baixo digital da era Super Power / Sleng Teng (teclado Casio dos anos 80).
    // Quadrada com polyBLEP (sem aliasing) + um pouco de senoide para o peso, oitava acima
    // bem baixa, e o grão digital de poucos bits (quantização em 7 bits).
    static float polyBlep (double t, double dt) noexcept
    {
        if (t < dt)        { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt)  { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    float superTone (float ph, double freq) noexcept
    {
        constexpr double twoPi = juce::MathConstants<double>::twoPi;
        const double t  = (double) ph / twoPi;
        const double dt = freq / sampleRate;

        float sq = t < 0.5 ? 1.0f : -1.0f;                   // quadrada 50 %
        sq += polyBlep (t, dt);
        sq -= polyBlep (std::fmod (t + 0.5, 1.0), dt);

        // passa-baixa de 1 polo acompanhando a nota: corpo "oco" do timbre de teclado
        const float a = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * std::min (freq * 9.0, sampleRate * 0.4) / sampleRate));
        stLp += a * (sq - stLp);

        float s = 0.62f * stLp + 0.48f * std::sin (ph) + 0.10f * std::sin (2.0f * ph);

        constexpr float steps = 63.0f;                      // 7 bits
        s = std::round (s * steps) / steps;
        return s * 0.92f;
    }

    float stLp = 0.0f;
    int   clickPos = 1 << 30, clickLen = 192;
    float dcX = 0.0f, dcY = 0.0f, dcR = 0.999f;

    //==========================================================================
    // Envelope ADSR com decay/release exponenciais (soa mais natural no 808)
    struct Envelope
    {
        enum class Stage { Idle, Attack, Decay, Sustain, Release };

        void setParameters (float aMs, float dMs, float s, float rMs, float sr)
        {
            attackInc    = 1.0f / std::max (1.0f, aMs * 0.001f * sr);
            decayCoeff   = std::exp (-6.9f / std::max (1.0f, dMs * 0.001f * sr));
            releaseCoeff = std::exp (-6.9f / std::max (1.0f, rMs * 0.001f * sr));
            sustain      = s;
        }

        void reset()   { stage = Stage::Idle; value = 0.0f; }
        void noteOn()  { stage = Stage::Attack; }
        void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }
        bool isActive() const noexcept { return stage != Stage::Idle; }

        float next() noexcept
        {
            switch (stage)
            {
                case Stage::Attack:
                    value += attackInc;
                    if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
                    break;

                case Stage::Decay:
                    value = sustain + (value - sustain) * decayCoeff;
                    if (value - sustain < 1.0e-4f)
                    {
                        value = sustain;
                        stage = Stage::Sustain;
                    }
                    break;

                case Stage::Sustain:
                    value += (sustain - value) * 0.001f;
                    break;

                case Stage::Release:
                    value *= releaseCoeff;
                    if (value < 1.0e-4f) reset();
                    break;

                case Stage::Idle:
                default:
                    value = 0.0f;
                    break;
            }
            return value;
        }

        Stage stage = Stage::Idle;
        float value = 0.0f, attackInc = 0.01f, decayCoeff = 0.999f, releaseCoeff = 0.999f, sustain = 1.0f;
    };

    Params   params;
    Envelope env;
    std::vector<int> held;

    double sampleRate    = 44100.0;
    double phase         = 0.0;
    float  currentNote   = 36.0f, targetNote = 36.0f;
    float  glideCoeff    = 1.0f;
    float  pitchEnv      = 0.0f, pitchEnvCoeff = 0.999f;
    float  pitchBend     = 0.0f;
    float  velGain       = 1.0f, velTarget = 1.0f;
};
