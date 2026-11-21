// Mastor Bass Machine — acompanhamento do PREVIEW (C++ puro, sem JUCE)
//
// BATERIA: quatro levadas simples, com um compasso de virada no fim do loop.
//   ROOTS      rockers da Channel One: bumbo no 1 e no 3, aro no 3, hi-hat em colcheias com
//              fantasmas e o chimbal aberto no "e" do 4. Kit acústico do Dub Drums.
//   ONE DROP   bumbo e aro só no 3, hi-hat em colcheias, virada em tercina no 4. Kit acústico.
//   DIGITAL    caixa de ritmos de 1985 (era Jammys / Sleng Teng): bumbo no 1 e no 3, palmas no 2 e
//              no 4, hi-hat reto. Kit eletrônico.
//   DANCEHALL  dancehall digital do fim dos anos 80: bumbo nos quatro tempos, caixa no "dem bow"
//              (3-3-2), timbales. Kit eletrônico.
// TECLADO: o som do Roots Keys (piano M1 e Hammond) seguindo a HARMONIA: skank do piano no
// contratempo e/ou bubble do órgão.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "BassEngine.h"

namespace mbm
{
//==============================================================================
enum DrumStyle { DrumRoots, DrumOneDrop, DrumDigital, DrumDancehall, NumDrumStyles };

inline const char* drumStyleName (int d)
{
    static const char* n[NumDrumStyles] = { "ROOTS", "ONE DROP", "DIGITAL", "DANCEHALL" };
    return n[std::clamp (d, 0, NumDrumStyles - 1)];
}

enum KeysMode { KeysPiano, KeysOrgan, KeysBoth, NumKeysModes };

inline const char* keysModeName (int k)
{
    static const char* n[NumKeysModes] = { "PIANO", "ORGAO", "AMBOS" };
    return n[std::clamp (k, 0, NumKeysModes - 1)];
}

// destino de cada evento de acompanhamento
enum AccTarget { AccAcousticDrum, AccDigitalDrum, AccPiano, AccOrgan };

struct AccEvent
{
    double start;    // tempos dentro do loop
    double length;   // só para o teclado (bateria é só disparo)
    int note;
    int velocity;    // 1..127
    int target;      // AccTarget
};

// notas General MIDI usadas pelas duas baterias
namespace drum
{
    constexpr int Kick = 36, Rim = 37, Snare = 38, Clap = 39, ClosedHat = 42, OpenHat = 46;
    constexpr int TomLow = 43, TomMid = 47, TomHigh = 50, TimbaleHi = 65, TimbaleLo = 66;
}

inline bool isDigitalKit (int style) { return style == DrumDigital || style == DrumDancehall; }

//==============================================================================
// Um compasso de bateria (16 semicolcheias); fill = compasso de virada
struct DrumHit { int step; int note; float vel; double offset = 0.0; };

inline std::vector<DrumHit> drumBar (int style, bool fill)
{
    using namespace drum;
    std::vector<DrumHit> h;
    auto hats8 = [&] (float on, float off, int until)
    {
        for (int s = 0; s < until; s += 2) h.push_back ({ s, ClosedHat, (s % 4 == 0) ? on : off });
    };

    switch (style)
    {
        case DrumRoots:
            h.push_back ({ 0, Kick, 0.95f }); h.push_back ({ 8, Kick, 0.95f });
            h.push_back ({ 8, Rim, 0.90f });
            hats8 (0.70f, 0.55f, fill ? 12 : 16);
            h.push_back ({ 3, ClosedHat, 0.30f }); h.push_back ({ 11, ClosedHat, 0.30f });
            if (! fill) h.push_back ({ 14, OpenHat, 0.60f });
            else
            {
                h.push_back ({ 12, Snare, 0.80f }); h.push_back ({ 13, TomHigh, 0.70f });
                h.push_back ({ 14, TomMid, 0.75f }); h.push_back ({ 15, TomLow, 0.85f });
            }
            break;

        case DrumOneDrop:
            h.push_back ({ 8, Kick, 1.0f }); h.push_back ({ 8, Rim, 0.95f });
            for (int s = 0; s < (fill ? 12 : 16); s += 2)
                h.push_back ({ s, ClosedHat, (s % 4 == 0) ? 0.65f : 0.50f, (s % 4 == 2) ? 0.06 : 0.0 });
            if (fill)   // tercinas puxadas no 4, como o Carlton Barrett
            {
                h.push_back ({ 12, Snare, 0.70f, 0.0 });
                h.push_back ({ 12, TomHigh, 0.75f, 1.0 / 3.0 });
                h.push_back ({ 12, TomMid, 0.80f, 2.0 / 3.0 });
            }
            break;

        case DrumDigital:
            h.push_back ({ 0, Kick, 1.0f }); h.push_back ({ 8, Kick, 0.95f });
            h.push_back ({ 4, Clap, 0.95f }); h.push_back ({ 12, Clap, 0.95f });
            hats8 (0.65f, 0.55f, 16);
            h.push_back ({ 14, Rim, 0.45f });
            if (fill) { h.push_back ({ 13, Clap, 0.60f }); h.push_back ({ 14, Clap, 0.75f }); h.push_back ({ 15, Clap, 0.90f }); }
            else        h.push_back ({ 15, ClosedHat, 0.40f });
            break;

        case DrumDancehall:
        default:
            for (int s : { 0, 4, 8, 12 }) h.push_back ({ s, Kick, s == 0 ? 1.0f : 0.90f });
            for (int s : { 3, 6, 11, 14 }) h.push_back ({ s, Snare, (s == 6 || s == 14) ? 0.95f : 0.75f });
            hats8 (0.55f, 0.60f, 16);
            if (fill)
            {
                h.push_back ({ 12, TimbaleHi, 0.80f }); h.push_back ({ 13, TimbaleHi, 0.70f });
                h.push_back ({ 14, TimbaleLo, 0.80f }); h.push_back ({ 15, TimbaleLo, 0.90f });
            }
            else
            {
                h.push_back ({ 7, TimbaleHi, 0.55f }); h.push_back ({ 15, TimbaleLo, 0.60f });
            }
            break;
    }
    return h;
}

inline std::vector<AccEvent> buildDrums (double loopBeats, int style, float swing)
{
    std::vector<AccEvent> ev;
    if (loopBeats <= 0) return ev;
    const int target = isDigitalKit (style) ? AccDigitalDrum : AccAcousticDrum;
    const int bars = (int) std::ceil (loopBeats / 4.0 - 1e-9);
    const int fillBar = std::max (0, (int) std::floor ((loopBeats - 1e-6) / 4.0) - (std::fmod (loopBeats, 4.0) > 1e-6 ? 1 : 0));

    for (int b = 0; b < bars; ++b)
        for (const auto& hit : drumBar (style, b == fillBar && bars > 1))
        {
            double st = b * 4.0 + hit.step * 0.25 + hit.offset;
            const int stepInBeat = hit.step % 4;
            if (stepInBeat == 2 && hit.offset < 1e-9)      st += swing * (2.0 / 3.0 - 0.5) * 0.6;
            else if (stepInBeat % 2 == 1)                  st += swing * (1.0 / 12.0) * 0.6;
            if (st >= loopBeats - 1e-6) continue;
            ev.push_back ({ st, 0.1, hit.note, std::clamp ((int) std::lround (hit.vel * 127.0f), 1, 127), target });
        }
    std::stable_sort (ev.begin(), ev.end(), [] (const AccEvent& a, const AccEvent& b) { return a.start < b.start; });
    return ev;
}

//==============================================================================
// Teclado: voicing com condução de vozes (inversão mais próxima do acorde anterior)
inline std::vector<int> chordIntervals (int q)
{
    switch (q)
    {
        case Maj:  return { 0, 4, 7 };
        case Dom7: return { 0, 4, 7, 10 };
        case Min7: return { 0, 3, 7, 10 };
        case Min:
        default:   return { 0, 3, 7 };
    }
}

inline std::vector<int> voiceKeys (int root, int quality, int center, const std::vector<int>& prev)
{
    const auto iv = chordIntervals (quality);
    const int n = (int) iv.size();
    std::vector<int> best;
    double bestScore = 1e9;
    for (int inv = 0; inv < n; ++inv)
        for (int bottom = center - 12; bottom <= center + 6; ++bottom)
        {
            if (pc (bottom) != pc (root + iv[(size_t) inv])) continue;
            std::vector<int> cand { bottom };
            for (int k = 1; k < n; ++k)
            {
                const int want = pc (root + iv[(size_t) ((inv + k) % n)]);
                int note = cand.back() + 1;
                while (pc (note) != want) ++note;
                cand.push_back (note);
            }
            double mean = 0; for (int x : cand) mean += x; mean /= n;
            double score = 0.35 * std::abs (mean - center);
            if (! prev.empty())
                for (size_t k = 0; k < std::min (prev.size(), cand.size()); ++k) score += std::abs (cand[k] - prev[k]);
            if (cand.back() - cand.front() > 14) score += 4;
            if (score < bestScore) { bestScore = score; best = cand; }
        }
    return best;
}

inline std::vector<AccEvent> buildKeys (const std::vector<ChordSlot>& slots, int mode, float swing)
{
    std::vector<AccEvent> ev;
    std::vector<int> prev;
    double pos = 0;
    for (const auto& c : slots)
    {
        if (c.beats <= 0) continue;
        const auto v = voiceKeys (c.root, c.quality, 62, prev);
        prev = v;
        const int first = (int) std::lround (pos * 4.0), steps = (int) std::lround (c.beats * 4.0);
        for (int st = first; st < first + steps; ++st)
        {
            const int sb = st % 16;
            const double swingOff = (sb % 4 == 2) ? swing * (2.0 / 3.0 - 0.5) * 0.6 : (sb % 2 == 1 ? swing * (1.0 / 12.0) * 0.6 : 0.0);
            const double start = st * 0.25 + swingOff;
            if (mode != KeysOrgan && sb % 4 == 2)                       // skank do piano no contratempo
                for (int n : v) ev.push_back ({ start, 0.2, n, 92, AccPiano });
            if (mode != KeysPiano && sb % 4 != 0)                       // bubble do órgão (uma oitava abaixo)
                for (int n : v) ev.push_back ({ start, 0.17, n - 12, (sb % 4 == 2) ? 100 : 62, AccOrgan });
        }
        pos += c.beats;
    }
    std::stable_sort (ev.begin(), ev.end(), [] (const AccEvent& a, const AccEvent& b) { return a.start < b.start; });
    return ev;
}

//==============================================================================
// Kit de bateria eletrônica dos anos 80 (DIGITAL e DANCEHALL), todo sintetizado, sem alocação
class DigitalKit
{
public:
    void prepare (double sampleRate) { sr = sampleRate > 0 ? sampleRate : 44100.0; for (auto& v : voices) v.active = false; }
    void stopAll() { for (auto& v : voices) v.active = false; }

    void trigger (int note, int velocity)
    {
        if (note == drum::ClosedHat)
            for (auto& v : voices) if (v.active && v.note == drum::OpenHat) v.choke = true;
        Voice* slot = nullptr;
        for (auto& v : voices) if (! v.active) { slot = &v; break; }
        if (slot == nullptr) { slot = &voices[0]; for (auto& v : voices) if (v.t > slot->t) slot = &v; }
        *slot = Voice {};
        slot->active = true;
        slot->note = note;
        const float vel = std::clamp (velocity / 127.0f, 0.0f, 1.0f);
        slot->vel = vel * vel * 0.85f + 0.15f * vel;
        slot->noise = 0x2545F491u ^ ((uint32_t) note * 2654435761u + ++counter);
        slot->pan = (note == drum::ClosedHat || note == drum::OpenHat) ? 0.28f
                  : (note == drum::TimbaleHi ? 0.35f : (note == drum::TimbaleLo ? -0.3f : (note == drum::Clap ? -0.06f : 0.0f)));
    }

    void render (float* left, float* right, int start, int num, float gain)
    {
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const float gl = gain * std::sqrt (0.5f * (1.0f - v.pan)), gr = gain * std::sqrt (0.5f * (1.0f + v.pan));
            for (int i = start; i < start + num; ++i)
            {
                const float s = next (v);
                left[i] += s * gl;
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
        float vel = 1, pan = 0, chokeGain = 1;
        double t = 0, ph[3] {};
        float hp = 0, hpIn = 0, b1 = 0, b2 = 0, lp = 0;
        uint32_t noise = 1;
    };

    static constexpr double twoPi = 6.283185307179586;

    float rnd (Voice& v) { v.noise ^= v.noise << 13; v.noise ^= v.noise >> 17; v.noise ^= v.noise << 5; return (float) (int32_t) v.noise * (1.0f / 2147483648.0f); }
    float sine (Voice& v, int i, double f) { v.ph[i] += f / sr; v.ph[i] -= std::floor (v.ph[i]); return (float) std::sin (twoPi * v.ph[i]); }
    float pulse (Voice& v, int i, double f) { v.ph[i] += f / sr; v.ph[i] -= std::floor (v.ph[i]); return v.ph[i] < 0.5 ? 1.0f : -1.0f; }
    float highpass (Voice& v, float x, float fc) { const float a = (float) std::exp (-twoPi * fc / sr); v.hp = a * (v.hp + x - v.hpIn); v.hpIn = x; return v.hp; }
    float bandpass (Voice& v, float x, float f, float q)
    {
        const float k = (float) (2.0 * std::sin (3.141592653589793 * std::min (f, (float) sr * 0.24f) / sr));
        const float h = x - v.b2 - v.b1 / q; v.b1 += k * h; v.b2 += k * v.b1; return v.b1;
    }
    static float env (double t, double d) { return (float) std::exp (-t / d); }

    float next (Voice& v)
    {
        const double t = v.t;
        v.t += 1.0 / sr;
        float out = 0; double end = 1.0;
        switch (v.note)
        {
            case drum::Kick:   // bumbo de caixa de ritmos: seno com queda longa, clique curto e um pouco de saturação
            {
                const double f = 50.0 + 110.0 * std::exp (-t / 0.035);
                out = std::tanh (1.6f * sine (v, 0, f) * env (t, 0.32)) * 0.95f
                    + highpass (v, rnd (v), 2500.0f) * env (t, 0.003) * 0.35f;
                end = 1.4; break;
            }
            case drum::Clap:   // palmas: três estalos de ruído e a cauda, filtrados em 1,1 kHz
            {
                float e = 0;
                for (double o : { 0.0, 0.011, 0.022 }) if (t >= o) e += env (t - o, 0.0035);
                if (t >= 0.030) e += env (t - 0.030, 0.12) * 0.8f;
                out = bandpass (v, rnd (v), 1150.0f, 1.6f) * e * 2.2f;
                end = 0.7; break;
            }
            case drum::Rim:    // aro de teclado Casio: dois pulsos curtos
                out = (pulse (v, 0, 1650.0) * 0.4f + pulse (v, 1, 2460.0) * 0.25f) * env (t, 0.011)
                    + highpass (v, rnd (v), 3000.0f) * env (t, 0.004) * 0.4f;
                out *= 0.6f; end = 0.12; break;
            case drum::Snare:  // caixa digital estalada
            {
                const double f = 195.0 + 50.0 * std::exp (-t / 0.012);
                out = sine (v, 0, f) * env (t, 0.06) * 0.6f + highpass (v, rnd (v), 2000.0f) * env (t, 0.15) * 0.75f;
                end = 0.8; break;
            }
            case drum::ClosedHat:
                out = highpass (v, rnd (v) * 0.8f + pulse (v, 0, 7300.0) * 0.2f, 7000.0f) * env (t, 0.028) * 1.6f;
                end = 0.25; break;
            case drum::OpenHat:
                if (v.choke) { v.chokeGain *= 0.995f; if (v.chokeGain < 0.001f) v.active = false; }
                out = highpass (v, rnd (v) * 0.8f + pulse (v, 0, 7300.0) * 0.2f, 6500.0f) * env (t, 0.22) * 1.2f * v.chokeGain;
                end = 1.2; break;
            case drum::TimbaleHi: case drum::TimbaleLo:   // timbales de dancehall: metálicos, cantando
            {
                const double base = v.note == drum::TimbaleHi ? 520.0 : 360.0;
                const double f = base * (1.0 + 0.08 * std::exp (-t / 0.02));
                out = (sine (v, 0, f) * 0.7f + sine (v, 1, f * 1.58) * 0.25f) * env (t, 0.17)
                    + bandpass (v, rnd (v), (float) base * 3.0f, 2.5f) * env (t, 0.006) * 0.5f;
                out *= 0.7f; end = 1.0; break;
            }
            default:   // toms caem no timbale grave
            {
                const double f = 140.0 * (1.0 + 0.3 * std::exp (-t / 0.03));
                out = sine (v, 0, f) * env (t, 0.2) * 0.7f; end = 1.0; break;
            }
        }
        if (t > end) v.active = false;
        return out * v.vel;
    }

    std::array<Voice, 24> voices {};
    double sr = 44100.0;
    uint32_t counter = 1;
};

} // namespace mbm
