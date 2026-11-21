// Dub Drums — bateria interna para o PREVIEW
// Timbre de reggae limpo, todo sintetizado (sem samples): bumbo redondo e curto,
// aro seco, caixa afinada alta, hi-hats metálicos, tons cantando e prato suave.
// Sem alocação na thread de áudio: vozes fixas.
#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "PatternGenerator.h"

namespace dubdrums
{

class DrumSynth
{
public:
    enum class Kit { Acoustic = 0, Digital };
    void setKit (Kit k) noexcept { kit = k; }

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0 ? sampleRate : 44100.0;
        for (auto& v : voices) v.active = false;
    }

    // nota GM do gerador (não a nota mapeada pelo usuário) + força 1..127
    void trigger (int gmNote, int velocity)
    {
        const float vel = std::clamp (velocity / 127.0f, 0.0f, 1.0f);

        // hi-hat fechado corta o aberto, como no pedal de verdade
        if (gmNote == gm::ClosedHat || gmNote == gm::PedalHat)
            for (auto& v : voices)
                if (v.active && v.note == gm::OpenHat) v.choke = true;

        Voice* slot = nullptr;
        for (auto& v : voices) if (! v.active) { slot = &v; break; }
        if (slot == nullptr)   // rouba a voz mais antiga
        {
            slot = &voices[0];
            for (auto& v : voices) if (v.age > slot->age) slot = &v;
        }

        *slot = Voice {};
        slot->active = true;
        slot->note = gmNote;
        slot->vel = vel * vel * 0.9f + 0.1f * vel;  // curva de força mais natural
        slot->noiseState = 0x9E3779B9u ^ ((uint32_t) gmNote * 7919u + counter++);

        switch (gmNote)
        {
            case gm::Kick:      slot->pan = 0.0f;   break;
            case gm::SideStick: slot->pan = -0.12f; break;
            case gm::Snare:     slot->pan = -0.10f; break;
            case gm::ClosedHat: case gm::PedalHat: case gm::OpenHat: slot->pan = 0.30f; break;
            case gm::TomHigh:   slot->pan = 0.22f;  break;
            case gm::TomMid:    slot->pan = -0.05f; break;
            case gm::TomLow:    slot->pan = -0.30f; break;
            case gm::Crash:     slot->pan = -0.35f; break;
            default:            slot->pan = 0.0f;   break;
        }
    }

    void stopAll() { for (auto& v : voices) v.active = false; }

    // soma no buffer estéreo (left/right) de start até start+num
    void render (float* left, float* right, int start, int num, float gain)
    {
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const float gl = gain * std::sqrt (0.5f * (1.0f - v.pan));
            const float gr = gain * std::sqrt (0.5f * (1.0f + v.pan));
            for (int i = start; i < start + num; ++i)
            {
                const float s = next (v);
                left[i]  += s * gl;
                if (right != nullptr) right[i] += s * gr;
                if (! v.active) break;
            }
        }
    }

private:
    struct Voice
    {
        bool active = false, choke = false;
        int note = 0;
        float vel = 1.0f, pan = 0.0f;
        double t = 0.0;            // segundos desde o toque
        double phase[6] {};        // osciladores
        float held = 0.0f; double holdPhase = 1.0;   // kit digital: amostragem baixa
        float hp = 0.0f, hpIn = 0.0f, lp = 0.0f, tape = 0.0f, bp1 = 0.0f, bp2 = 0.0f, chokeGain = 1.0f;
        uint32_t noiseState = 1;
        int64_t age = 0;
    };

    static constexpr double twoPi = 6.283185307179586;

    float noise (Voice& v)
    {
        v.noiseState ^= v.noiseState << 13; v.noiseState ^= v.noiseState >> 17; v.noiseState ^= v.noiseState << 5;
        return (float) ((int32_t) v.noiseState) * (1.0f / 2147483648.0f);
    }

    // passa-altas de 1 polo
    float highpass (Voice& v, float x, float cutoff)
    {
        const float a = (float) std::exp (-twoPi * cutoff / sr);
        v.hp = a * (v.hp + x - v.hpIn);
        v.hpIn = x;
        return v.hp;
    }

    // passa-baixas de 1 polo
    float lowpass (Voice& v, float x, float cutoff)
    {
        const float a = (float) std::exp (-twoPi * cutoff / sr);
        v.lp = a * v.lp + (1.0f - a) * x;
        return v.lp;
    }

    // passa-faixa simples (state variable)
    float bandpass (Voice& v, float x, float freq, float q)
    {
        const float f = (float) (2.0 * std::sin (3.141592653589793 * std::min (freq, (float) sr * 0.24f) / sr));
        const float hpOut = x - v.bp2 - v.bp1 / q;
        v.bp1 += f * hpOut;
        v.bp2 += f * v.bp1;
        return v.bp1;
    }

    float osc (Voice& v, int i, double freq)
    {
        v.phase[i] += freq / sr;
        v.phase[i] -= std::floor (v.phase[i]);
        return (float) std::sin (twoPi * v.phase[i]);
    }

    float square (Voice& v, int i, double freq)
    {
        v.phase[i] += freq / sr;
        v.phase[i] -= std::floor (v.phase[i]);
        return v.phase[i] < 0.5 ? 1.0f : -1.0f;
    }

    static float env (double t, double decay) { return (float) std::exp (-t / decay); }

    float metal (Voice& v)
    {
        // seis quadradas desafinadas, como o 808: base do hi-hat e do prato
        static const double f[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
        float s = 0.0f;
        for (int i = 0; i < 6; ++i) s += square (v, i, f[i] * 1.55);
        return s / 6.0f;
    }

    // Kit DIGITAL (King Jammy / Sleng Teng, 1985): sons de teclado Casio dos anos 80,
    // curtos e "de brinquedo", gerados em baixa resolução e baixa taxa de amostragem.
    float nextDigital (Voice& v)
    {
        const double t = v.t;
        float out = 0.0f;
        double end = 0.5;
        switch (v.note)
        {
            case gm::Kick:      // "bip" grave e curto: seno com queda rápida
            {
                const double f = 62.0 + 150.0 * std::exp (-t / 0.012);
                out = osc (v, 0, f) * env (t, 0.075) * 1.1f;
                end = 0.35;
                break;
            }
            case gm::Snare:     // chiado de ruído com um tiquinho de tom
            {
                out = noise (v) * env (t, 0.075) * 0.75f + square (v, 0, 235.0) * env (t, 0.018) * 0.25f;
                end = 0.35;
                break;
            }
            case gm::SideStick: // clique quadrado
            {
                out = square (v, 0, 1250.0) * env (t, 0.010) * 0.6f;
                end = 0.08;
                break;
            }
            case gm::ClosedHat: case gm::PedalHat:   // tique metálico
            {
                out = highpass (v, noise (v), 5000.0f) * env (t, 0.018) * 1.4f;
                end = 0.12;
                break;
            }
            case gm::OpenHat:
            {
                if (v.choke) { v.chokeGain *= 0.99f; if (v.chokeGain < 0.001f) v.active = false; }
                out = highpass (v, noise (v), 4500.0f) * env (t, 0.14) * 1.1f * v.chokeGain;
                end = 0.8;
                break;
            }
            case gm::TomHigh: case gm::TomMid: case gm::TomLow:   // "blips" quadrados afinados
            {
                const double base = v.note == gm::TomHigh ? 330.0 : (v.note == gm::TomMid ? 247.0 : 185.0);
                out = square (v, 0, base * (1.0 + 0.5 * std::exp (-t / 0.02))) * env (t, 0.09) * 0.45f;
                end = 0.45;
                break;
            }
            case gm::Crash:
            {
                out = highpass (v, noise (v), 3000.0f) * env (t, 0.45) * 0.6f;
                end = 2.0;
                break;
            }
            default:
                v.active = false;
                return 0.0f;
        }
        if (t > end) v.active = false;

        // baixa resolução (6 bits) e baixa taxa de amostragem (~11 kHz)
        v.holdPhase += 11025.0 / sr;
        if (v.holdPhase >= 1.0)
        {
            v.holdPhase -= std::floor (v.holdPhase);
            v.held = std::round (std::clamp (out, -1.0f, 1.0f) * 31.0f) / 31.0f;
        }
        return v.held * v.vel * 0.72f;
    }

    float next (Voice& v)
    {
        if (kit == Kit::Digital)
        {
            const float o = nextDigital (v);
            v.t += 1.0 / sr;
            ++v.age;
            return o;
        }

        const double t = v.t;
        v.t += 1.0 / sr;
        ++v.age;
        float out = 0.0f;
        double end = 1.0;

        switch (v.note)
        {
            case gm::Kick:
            {
                // bumbo de estúdio jamaicano dos anos 70 / começo dos 80 (Channel One, Volcano):
                // casco grande e abafado, batedor de feltro, sem a queda longa de afinação do 808,
                // e um pouco de saturação de fita
                const double f = 57.0 * (1.0 + 0.10 * std::exp (-t / 0.030));
                const float attack = 1.0f - env (t, 0.0015);
                const float body   = osc (v, 0, f) * env (t, 0.115);
                const float shell  = osc (v, 1, f * 1.59) * env (t, 0.065) * 0.38f
                                   + osc (v, 2, f * 2.37) * env (t, 0.032) * 0.16f;
                const float felt   = lowpass (v, noise (v), 1600.0f) * env (t, 0.007) * 0.9f
                                   + bandpass (v, noise (v), 210.0f, 1.4f) * env (t, 0.022) * 0.9f;
                const float x = (body + shell) * attack + felt;
                out = std::tanh (1.8f * x) / std::tanh (1.8f) * 0.95f;   // fita
                end = 0.9;
                break;
            }
            case gm::SideStick:   // aro: "toc" de madeira, seco
            {
                out = (osc (v, 0, 1720.0) * 0.5f + osc (v, 1, 830.0) * 0.6f) * env (t, 0.016)
                    + bandpass (v, noise (v), 2600.0f, 3.0f) * env (t, 0.010) * 0.9f;
                out *= 0.85f;
                end = 0.15;
                break;
            }
            case gm::Snare:
            {
                // caixa jamaicana: afinada alta, cantando, com estalo de aro (rimshot) e esteira curta
                const double f = 305.0 * (1.0 + 0.07 * std::exp (-t / 0.008));
                const float tone  = osc (v, 0, f) * env (t, 0.085) * 0.55f
                                  + osc (v, 1, f * 1.47) * env (t, 0.055) * 0.30f
                                  + osc (v, 2, f * 2.14) * env (t, 0.030) * 0.15f;
                const float wires = bandpass (v, noise (v), 3300.0f, 1.1f) * env (t, 0.11) * 0.60f;
                const float crack = lowpass (v, noise (v), 5000.0f) * env (t, 0.004) * 1.4f;   // estalo do aro (filtro próprio)
                out = std::tanh (1.4f * (tone + wires + crack)) * 0.9f;
                // fita dos anos 70: tira o chiado de cima
                const float a = (float) std::exp (-twoPi * 6500.0 / sr);
                v.tape = a * v.tape + (1.0f - a) * out;
                out = v.tape * 1.25f;
                end = 0.7;
                break;
            }
            case gm::ClosedHat:
            case gm::PedalHat:
            {
                out = highpass (v, metal (v) * 0.7f + noise (v) * 0.3f, 5500.0f) * env (t, 0.032) * 4.5f;
                end = 0.25;
                break;
            }
            case gm::OpenHat:
            {
                if (v.choke) { v.chokeGain *= 0.995f; if (v.chokeGain < 0.001f) v.active = false; }
                out = highpass (v, metal (v) * 0.7f + noise (v) * 0.3f, 5500.0f) * env (t, 0.22) * 3.0f * v.chokeGain;
                end = 1.5;
                break;
            }
            case gm::TomHigh: case gm::TomMid: case gm::TomLow:
            {
                const double base = v.note == gm::TomHigh ? 205.0 : (v.note == gm::TomMid ? 155.0 : 112.0);
                const double f = base * (1.0 + 0.35 * std::exp (-t / 0.03));
                out = osc (v, 0, f) * env (t, 0.24) * 0.8f
                    + bandpass (v, noise (v), (float) base * 4.0f, 2.0f) * env (t, 0.02) * 0.25f;
                end = 1.5;
                break;
            }
            case gm::Crash:
            {
                out = highpass (v, metal (v) * 0.5f + noise (v) * 0.5f, 4500.0f) * env (t, 0.9) * 0.30f
                    * (1.0f - env (t, 0.003));
                end = 4.0;
                break;
            }
            default:
                v.active = false;
                return 0.0f;
        }

        if (t > end) v.active = false;
        return out * v.vel;
    }

    std::array<Voice, 32> voices {};
    double sr = 44100.0;
    uint32_t counter = 1;
    Kit kit = Kit::Acoustic;
};

} // namespace dubdrums
