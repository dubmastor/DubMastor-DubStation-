// GuitarSkank.h — a guitarra do skank tocando junto com o piano (botão GTR SKANK do Reggae Keys MRK-80)
//
// Mesmo timbre da versão de teste do Mastor Reggae Guitar MRG-68 (Tele 68, captador da ponte, Twin Reverb):
//  - corda por Karplus-Strong com duas polarizações (±0,7 cent), posição da palheta e do captador (filtros pente)
//  - palhetada para baixo: as cordas entram da mais grave para a mais aguda, com o "spread" da mão
//  - enquanto a tecla do piano está presa a corda soa; ao soltar, a mão abafa (o chop curto do skank)
//  - captador (passa-baixas ressonante), pré de válvula e caixa do Twin (graves, presença e corte em 5 kHz)
// A guitarra pega as notas que o piano toca e leva para a região do skank (G3 a E5), no máximo 4 cordas.
// Header-only, sem JUCE.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rk
{
struct GtrBiquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (int type, float f, float q, float gainDb, float sr)   // 0 passa-baixas, 1 passa-altas, 2 peaking
    {
        const float w = 6.2831853f * std::min (f, 0.45f * sr) / sr, cw = std::cos (w), sw = std::sin (w), al = sw / (2.0f * q);
        float B0, B1, B2, A0, A1, A2;
        if (type == 0)      { B0 = (1 - cw) * 0.5f; B1 = 1 - cw; B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }
        else if (type == 1) { B0 = (1 + cw) * 0.5f; B1 = -(1 + cw); B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }
        else
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            B0 = 1 + al * A; B1 = -2 * cw; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cw; A2 = 1 - al / A;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
};

struct GtrString
{
    static constexpr int MaxN = 2048;
    bool active = false, held = false;
    int note = -1, delay = 0, age = 0;
    float vel = 0.8f;
    // duas polarizações
    std::vector<float> dl[2], exc[2];
    int N[2] {}, wi[2] {}, excLen[2] {};
    float apC[2] {}, wPrev[2] {}, lPrev[2] {}, aPrev[2] {}, bright = 0.5f;
    // captador (pente)
    std::vector<float> pick; int pq = 1, pw = 0;
    float energy = 0;

    GtrString()
    {
        for (auto& d : dl) d.assign (MaxN + 4, 0.0f);
        for (auto& e : exc) e.assign (MaxN + 4, 0.0f);
        pick.assign (MaxN + 4, 0.0f);
    }

    static float rnd (uint32_t& s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) s / 4294967296.0f * 2.0f - 1.0f; }

    void start (int n, float v, int startDelay, float sr, uint32_t seed)
    {
        active = true; held = true; note = n; vel = v; delay = startDelay; age = 0; energy = 1;
        const float f0 = 440.0f * std::pow (2.0f, ((float) n - 69.0f) / 12.0f);
        bright = std::min (0.62f, n < 60 ? 0.5f : 0.5f + 0.004f * (float) (n - 60));
        for (int pol = 0; pol < 2; ++pol)
        {
            const float dc = pol == 0 ? -0.7f : 0.7f;
            const float f = 440.0f * std::pow (2.0f, ((float) n - 69.0f + dc / 100.0f) / 12.0f);
            const float P = sr / f, s0 = 0.5f;
            int Nn = (int) std::floor (P - s0 - 0.1f);
            Nn = std::max (4, std::min (MaxN, Nn));
            const float d = P - s0 - (float) Nn;
            apC[pol] = (1.0f - d) / (1.0f + d);
            N[pol] = Nn; wi[pol] = 0; wPrev[pol] = lPrev[pol] = aPrev[pol] = 0;
            std::fill (dl[pol].begin(), dl[pol].end(), 0.0f);
            // excitação: triângulo da palhetada + ruído filtrado, pente na posição da palheta (~1/8 da corda)
            uint32_t r = seed * 31u + (uint32_t) pol * 7u + (uint32_t) n + 1u;
            const float pickPos = 0.12f + 0.03f * (rnd (r) * 0.5f + 0.5f);
            const int k = std::max (2, (int) (pickPos * (float) Nn));
            const float a = 0.55f - 0.35f * v;
            float lp = 0, mean = 0;
            auto& x = exc[pol];
            for (int i = 0; i < Nn; ++i)
            {
                const float tri = i < k ? (float) i / (float) k : 1.0f - (float) (i - k) / (float) (Nn - k);
                lp = (1 - a) * rnd (r) + a * lp;
                x[(size_t) i] = 0.55f * tri + 0.45f * lp;
                mean += x[(size_t) i];
            }
            mean /= (float) Nn;
            for (int i = 0; i < Nn; ++i) x[(size_t) i] -= mean;
            for (int i = Nn - 1; i >= k; --i) x[(size_t) i] -= 0.9f * x[(size_t) (i - k)];
            const float fc = 1800.0f + 2600.0f * v, pa = std::exp (-6.2831853f * fc / sr);
            for (int pass = 0; pass < 2; ++pass)
            {
                float y = 0;
                for (int i = 0; i < Nn; ++i) { y = (1 - pa) * x[(size_t) i] + pa * y; x[(size_t) i] = y; }
            }
            excLen[pol] = Nn;
        }
        pq = std::max (1, (int) (0.1f * sr / f0));   // captador da ponte
        std::fill (pick.begin(), pick.end(), 0.0f); pw = 0;
    }

    void release() { held = false; }

    inline float tick()
    {
        if (delay > 0) { --delay; return 0.0f; }
        float out = 0;
        const float g = held ? 0.9965f : 0.8f;          // corda soando / mão abafando (o chop do skank)
        const float s = held ? bright : 0.5f;
        for (int pol = 0; pol < 2; ++pol)
        {
            auto& d = dl[pol];
            const int L = N[pol] + 2;
            const float w = d[(size_t) ((wi[pol] + L - N[pol]) % L)];
            const float l = (1 - s) * w + s * wPrev[pol]; wPrev[pol] = w;
            const float ap = apC[pol] * l + lPrev[pol] - apC[pol] * aPrev[pol]; lPrev[pol] = l; aPrev[pol] = ap;
            const float y = (age < excLen[pol] ? exc[pol][(size_t) age] * vel : 0.0f) + g * ap;
            d[(size_t) wi[pol]] = y;
            wi[pol] = (wi[pol] + 1) % L;
            out += (pol == 0 ? 0.62f : 0.38f) * y;
        }
        ++age;
        // captador: pente na posição do captador da ponte
        const int PL = (int) pick.size();
        const float back = pick[(size_t) ((pw + PL - pq) % PL)];
        pick[(size_t) pw] = out; pw = (pw + 1) % PL;
        const float y = out - 0.7f * back;
        energy = 0.999f * energy + 0.001f * y * y;
        if (! held && age > 2000 && energy < 1.0e-9f) active = false;
        return y;
    }
};

class GuitarSkank
{
public:
    static constexpr int NumVoices = 12;

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& v : voices) v.active = false;
        pendingCount = 0;
        pickupLp.set (0, 4200.0f, 2.4f, 0, sr);
        hp.set (1, 85.0f, 0.7f, 0, sr);
        lowPeak.set (2, 140.0f, 1.2f, 6.0f, sr);
        midScoop.set (2, 650.0f, 0.8f, -4.0f, sr);   // o "meio cavado" do tone stack do Twin
        presence.set (2, 2600.0f, 1.3f, 2.0f, sr);
        cab1.set (0, 5000.0f, 0.54f, 0, sr);
        cab2.set (0, 5000.0f, 1.3f, 0, sr);
        for (auto* b : { &pickupLp, &hp, &lowPeak, &midScoop, &presence, &cab1, &cab2 }) b->reset();
    }

    // notas do piano: guardadas até o próximo bloco, para a palhetada sair em ordem (grave -> agudo)
    void noteOn (int note, float vel)
    {
        int n = note;
        while (n < 55) n += 12;      // região do skank: G3..E5
        while (n > 76) n -= 12;
        for (int i = 0; i < pendingCount; ++i) if (pending[i].note == n) return;
        if (pendingCount < MaxPending) pending[pendingCount++] = { n, vel, note };
    }

    void noteOff (int note)
    {
        for (int i = 0; i < pendingCount; ++i)
            if (pending[i].src == note) { pending[i].releaseNow = true; }
        for (auto& v : voices) if (v.active && v.held && srcOf (v) == note) v.release();
    }

    void allNotesOff() { for (auto& v : voices) v.release(); pendingCount = 0; }

    // chamado no começo de cada bloco: dispara a palhetada com as notas que chegaram juntas
    void flush()
    {
        if (pendingCount == 0) return;
        std::sort (pending, pending + pendingCount, [] (const Pending& a, const Pending& b) { return a.note < b.note; });
        // no máximo 4 cordas: as mais agudas (o skank fica nas cordas de cima)
        const int first = std::max (0, pendingCount - 4);
        float vel = 0;
        for (int i = first; i < pendingCount; ++i) vel = std::max (vel, pending[i].vel);
        const float spread = (0.002f + 0.35f * 0.008f) * (1.2f - 0.4f * vel);
        for (int i = first, k = 0; i < pendingCount; ++i, ++k)
        {
            auto& v = pickVoice (pending[i].note);
            const float vv = clamp01 (pending[i].vel) * (1.0f - 0.09f * (float) k);
            v.start (pending[i].note, vv, (int) ((float) k * spread * sr), sr, ++seed);
            voiceSrc[&v - voices] = pending[i].src;
            if (pending[i].releaseNow) v.release();
        }
        pendingCount = 0;
    }

    // um sample da guitarra já com o amplificador
    inline float tick()
    {
        float s = 0;
        for (auto& v : voices) if (v.active) s += v.tick();
        if (s == 0.0f && quiet > 4096) return 0.0f;
        quiet = s == 0.0f ? quiet + 1 : 0;
        float x = pickupLp.process (s * 0.6f);
        x = hp.process (x);
        x = std::tanh (1.3f * (x + 0.08f)) - std::tanh (1.3f * 0.08f);   // pré de válvula
        x = lowPeak.process (x);
        x = midScoop.process (x);
        x = presence.process (x);
        x = cab2.process (cab1.process (x));
        return x * 1.6f;
    }

private:
    struct Pending { int note; float vel; int src; bool releaseNow = false; };
    static constexpr int MaxPending = 16;
    static float clamp01 (float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

    GtrString& pickVoice (int note)
    {
        for (auto& v : voices) if (v.active && v.note == note) return v;   // repalheta a mesma corda
        for (auto& v : voices) if (! v.active) return v;
        GtrString* best = &voices[0];
        for (auto& v : voices) if (! v.held && v.age > best->age) best = &v;
        return *best;
    }
    int srcOf (const GtrString& v) const { return voiceSrc[&v - voices]; }

    float sr = 48000.0f;
    GtrString voices[NumVoices];
    int voiceSrc[NumVoices] {};
    Pending pending[MaxPending];
    int pendingCount = 0, quiet = 0;
    uint32_t seed = 1;
    GtrBiquad pickupLp, hp, lowPeak, midScoop, presence, cab1, cab2;
};
} // namespace rk
