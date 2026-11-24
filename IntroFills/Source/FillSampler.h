// FillSampler.h — Mastor Intro Fills MIF: o kit (samples DM do Mastor Reggae Drums MRD-12 + tons e surdo) com a
// dinâmica por nota do estudo, igual à versão para celular (função hit):
//  1) volume em dB (pianíssimo e forte bem separados)   2) brilho: batida fraca perde agudos
//  3) ataque mais seco na batida forte                    4) afinação: pele forte estica e volta em ~35 ms
//  5) sustentação: batida fraca soa mais curta            6) variação a cada batida (no lugar do round robin)
// O chimbal aberto é cortado pelo fechado ou pelo pedal. Depois: saturador e Tape Echo (de fábrica desligados) e um
// compressor leve. Header-only, sem JUCE.
#pragma once

#include "FillEngine.h"
#include <cstring>

namespace mif
{
struct Sample { std::vector<float> data; double rate = 44100; };

// lê um WAV PCM de 16 bits (mono ou estéreo -> mono)
inline bool readWav (const void* bytes, size_t size, Sample& out)
{
    const uint8_t* b = (const uint8_t*) bytes;
    if (size < 44 || std::memcmp (b, "RIFF", 4) != 0 || std::memcmp (b + 8, "WAVE", 4) != 0) return false;
    auto u16 = [&] (size_t o) { return (uint32_t) b[o] | ((uint32_t) b[o + 1] << 8); };
    auto u32 = [&] (size_t o) { return u16 (o) | (u16 (o + 2) << 16); };
    size_t pos = 12; int ch = 1, bits = 16; uint32_t rate = 44100;
    while (pos + 8 <= size)
    {
        const uint32_t len = u32 (pos + 4);
        if (std::memcmp (b + pos, "fmt ", 4) == 0) { ch = (int) u16 (pos + 10); rate = u32 (pos + 12); bits = (int) u16 (pos + 22); }
        else if (std::memcmp (b + pos, "data", 4) == 0)
        {
            if (bits != 16 || ch < 1) return false;
            const size_t n = std::min<size_t> (len, size - pos - 8) / (size_t) (2 * ch);
            out.data.resize (n); out.rate = rate;
            for (size_t i = 0; i < n; ++i)
            {
                float s = 0;
                for (int c = 0; c < ch; ++c) s += (float) (int16_t) u16 (pos + 8 + (i * (size_t) ch + (size_t) c) * 2) / 32768.0f;
                out.data[i] = s / (float) ch;
            }
            return true;
        }
        pos += 8 + len + (len & 1);
    }
    return false;
}

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void lowpass (float f, float q, float sr)
    {
        const float w = 6.2831853f * std::min (f, 0.45f * sr) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        b0 = (1 - cw) * 0.5f / a0; b1 = (1 - cw) / a0; b2 = b0; a1 = -2 * cw / a0; a2 = (1 - al) / a0;
    }
    void highpass (float f, float q, float sr)
    {
        const float w = 6.2831853f * std::min (f, 0.45f * sr) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        b0 = (1 + cw) * 0.5f / a0; b1 = -(1 + cw) / a0; b2 = b0; a1 = -2 * cw / a0; a2 = (1 - al) / a0;
    }
    void peak (float f, float q, float db, float sr)
    {
        const float A = std::pow (10.0f, db / 40.0f), w = 6.2831853f * std::min (f, 0.45f * sr) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al / A;
        b0 = (1 + al * A) / a0; b1 = -2 * cw / a0; b2 = (1 - al * A) / a0; a1 = -2 * cw / a0; a2 = (1 - al / A) / a0;
    }
    inline float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};

struct DynInfo { float db; float bend; float tauLo, tauHi; bool hasTau; };
inline const DynInfo& dyn (int p)
{
    static const DynInfo d[NumPieces] = {
        { 22, 0.025f, 0.10f, 0.40f, true },   // bumbo
        { 26, 0.008f, 0.05f, 0.25f, true },   // aro
        { 32, 0.018f, 0.07f, 0.35f, true },   // caixa
        { 28, 0.020f, 0.06f, 0.30f, true },   // timbal
        { 34, 0, 0, 0, false },               // hh fechado
        { 28, 0.040f, 0.14f, 0.60f, true },   // surdo
        { 26, 0, 0, 0, false },               // hh pedal
        { 30, 0, 0, 0, false },               // hh aberto
        { 28, 0.035f, 0.12f, 0.50f, true },   // tom 2
        { 26, 0, 0, 0, false },               // prato
        { 28, 0.035f, 0.12f, 0.45f, true },   // tom 1
    };
    return d[std::clamp (p, 0, NumPieces - 1)];
}

class FillSampler
{
public:
    static constexpr int NumVoices = 48;

    void setSample (int p, Sample s) { if (p >= 0 && p < NumPieces) samples[(size_t) p] = std::move (s); }
    bool hasSamples() const { for (auto& s : samples) if (s.data.empty()) return false; return true; }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& v : voices) v.active = false;
        echoBuf.assign ((size_t) (2.2 * sr) + 8, 0.0f); echoW = 0; echoLp.reset(); echoHp.reset();
        echoLp.lowpass (2600, 0.707f, sr); echoHp.highpass (300, 0.707f, sr);
        env = 0; lfo = 0;
    }

    // human = 0..1 (a variação de afinação/pan/começo segue o HUMANIZAR)
    void hit (int p, int velocity, double human)
    {
        if (p < 0 || p >= NumPieces || samples[(size_t) p].data.empty()) return;
        const auto& info = piece (p);
        const auto& d = dyn (p);
        const float n = std::clamp ((float) velocity / 127.0f, 0.01f, 1.0f);
        if (p == HHF || p == HHP)                         // o chimbal fechado/pedal corta o aberto
            for (auto& v : voices) if (v.active && v.piece == HHA) v.choke = true;
        Voice& v = freeVoice();
        v = Voice();
        v.active = true; v.piece = p;
        const auto& smp = samples[(size_t) p];
        const float base = 1.0f + (rnd() - 0.5f) * 0.012f * (float) human;
        v.rateBase = (float) (smp.rate / sr) * base;
        v.bend = d.bend * n * n;
        v.pos = (double) (rnd() * 0.0015f * (float) human * (float) smp.rate);
        const bool bright = info.hat || p == Prato;
        v.lp.lowpass (bright ? 2600.0f * std::pow (2.0f, n * 2.8f) : 900.0f * std::pow (2.0f, n * 4.4f), 0.4f, sr);
        v.pk.peak (info.hat ? 9000.0f : 3500.0f, 0.8f, (n - 0.6f) * 8.0f, sr);
        v.amp = info.gain * std::pow (10.0f, -d.db * (1.0f - n) / 20.0f);
        v.attInc = 1.0f / std::max (1.0f, ((1.0f - n) * 0.0035f + 0.0003f) * sr);
        v.tauStart = (int) (0.03f * sr);
        if (d.hasTau)
        {
            const float tau = d.tauLo + (d.tauHi - d.tauLo) * n;
            v.sus = 0.25f + 0.75f * n;
            v.susCoef = std::exp (-1.0f / (tau * sr));
        }
        if (info.hat)
        {
            const float pan = 0.18f + (rnd() - 0.5f) * 0.12f * (float) human;           // StereoPanner da versão web
            const float x = (pan + 1.0f) * 0.5f;
            v.gl = std::cos (x * 1.5707963f); v.gr = std::sin (x * 1.5707963f);
        }
        v.bendCoef = std::exp (-1.0f / (0.035f * sr));
    }

    void allOff() { for (auto& v : voices) v.choke = true; }

    // soma no buffer estéreo (não limpa); echo e sat seguem os botões; bpm para o tempo do eco (3/4 de tempo)
    void render (float* L, float* R, int n, bool echoOn, bool satOn, double bpm, float volume)
    {
        for (int i = 0; i < n; ++i)
        {
            float l = 0, r = 0;
            for (auto& v : voices) if (v.active) { float a, b; v.tick (samples[(size_t) v.piece], a, b); l += a; r += b; }
            if (satOn)
            {   // saturador: tanh(2,6x)/tanh(2,6), 35% seco + 75% saturado
                const float k = 2.6f, nk = std::tanh (k);
                l = 0.35f * l + 0.75f * std::tanh (k * std::clamp (l, -1.0f, 1.0f)) / nk;
                r = 0.35f * r + 0.75f * std::tanh (k * std::clamp (r, -1.0f, 1.0f)) / nk;
            }
            // Tape Echo: 3/4 de tempo, realimentação 45%, banda 300 Hz..2,6 kHz, wow leve
            float wet = 0;
            if (! echoBuf.empty())
            {
                const float send = echoOn ? 0.32f : 0.0f;
                lfo += 0.6f / sr; if (lfo >= 1) lfo -= 1;
                const float dSec = (float) (60.0 / std::max (20.0, bpm) * 0.75) + 0.0018f * std::sin (6.2831853f * lfo);
                const float dS = std::clamp (dSec * sr, 1.0f, (float) echoBuf.size() - 3);
                const int N = (int) echoBuf.size();
                float rp = (float) echoW - dS; while (rp < 0) rp += (float) N;
                const int i0 = (int) rp; const float fr = rp - (float) i0;
                const float y = echoBuf[(size_t) i0] * (1 - fr) + echoBuf[(size_t) ((i0 + 1) % N)] * fr;
                wet = echoHp.process (echoLp.process (y));
                echoBuf[(size_t) echoW] = 0.5f * (l + r) * send + wet * 0.45f;
                echoW = (echoW + 1) % N;
            }
            l = (l + wet) * 0.9f * volume;
            r = (r + wet) * 0.9f * volume;
            // compressor leve (limiar -8 dB, 3:1), como o da versão web
            const float pk = std::max (std::abs (l), std::abs (r));
            env = pk > env ? env + (pk - env) * 0.3f : env * 0.9995f;
            const float th = 0.398f;
            if (env > th) { const float g = std::pow (th / env, 1.0f - 1.0f / 3.0f); l *= g; r *= g; }
            L[i] += std::clamp (l, -1.5f, 1.5f);
            R[i] += std::clamp (r, -1.5f, 1.5f);
        }
    }

private:
    struct Voice
    {
        bool active = false, choke = false;
        int piece = 0, age = 0, tauStart = 0;
        double pos = 0;
        float rateBase = 1, bend = 0, bendCoef = 0.99f, amp = 1, g = 0, attInc = 1, sus = 1, susCoef = 1, level = 1, chokeG = 1;
        float gl = 1, gr = 1;
        Biquad lp, pk;
        void tick (const Sample& s, float& outL, float& outR)
        {
            const size_t N = s.data.size();
            const size_t i0 = (size_t) pos;
            if (i0 + 1 >= N) { active = false; outL = outR = 0; return; }
            const float fr = (float) (pos - (double) i0);
            float x = s.data[i0] * (1 - fr) + s.data[i0 + 1] * fr;
            if (age > (int) (0.004f * 44100)) bend *= bendCoef;     // a pele estica no ataque e volta
            pos += (double) (rateBase * (1.0f + bend));
            x = pk.process (lp.process (x));
            g = std::min (1.0f, g + attInc);
            if (age > tauStart) level = sus + (level - sus) * susCoef;
            if (choke) { chokeG *= 0.9981f; if (chokeG < 1e-4f) active = false; }
            const float y = x * amp * g * level * chokeG;
            outL = y * gl; outR = y * gr;
            ++age;
        }
    };

    Voice& freeVoice()
    {
        for (auto& v : voices) if (! v.active) return v;
        Voice* oldest = &voices[0];
        for (auto& v : voices) if (v.age > oldest->age) oldest = &v;
        return *oldest;
    }
    float rnd() { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 16777216.0f; }

    std::array<Sample, NumPieces> samples;
    std::array<Voice, NumVoices> voices;
    float sr = 48000;
    uint32_t seed = 12345;
    std::vector<float> echoBuf; int echoW = 0;
    Biquad echoLp, echoHp;
    float env = 0, lfo = 0;
};

// toca a sequência numa linha do tempo em semínimas: chama noteFn (piece, vel) para as notas que caem em [b0, b1)
struct SeqPlayer
{
    std::vector<Note> notes;
    double lengthBeats = 0, firstGrooveBeat = 0;
    template <typename Fn> void span (double b0, double b1, Fn&& fn) const
    {
        for (auto& n : notes) if (n.beat >= b0 && n.beat < b1) fn (n);
    }
};
} // namespace mif
