// GuitarSkank.h — a guitarra do skank tocando junto com o piano (botão GTR SKANK do Reggae Keys MRK-80)
//
// Usa o MOTOR DO PRÓPRIO Mastor Reggae Guitar MRG-68 (ReggaeGuitar/Source/GuitarEngine.h), sem cópia:
// as mesmas cordas (mrg::StringVoice) e o mesmo amplificador (mrg::Chain: Tele 68, captador da ponte, Fender Twin,
// mola; Tape Echo e saturador em bypass). Mudou o som no MRG-68? A guitarra do MRK-80 muda junto.
//
// A palhetada é a mesma do strum() do MRG-68: para baixo (grave -> agudo), duas guitarras abertas em estéreo
// (a segunda 12 ms depois). A guitarra pega as notas que o piano toca, leva para a região do skank (G3 a E5),
// no máximo 4 cordas; enquanto a tecla está presa a corda soa e, ao soltar, a mão abafa (o chop do skank).
//
// Memória: as cordas do MRG-68 guardam linhas de atraso grandes, então só o MRK-80 liga a guitarra
// (enable() antes do prepare). Os outros plugins que usam o RootsKeysEngine não gastam nada com ela.
#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <vector>
#include "../../ReggaeGuitar/Source/GuitarEngine.h"

namespace rk
{
class GuitarSkank
{
public:
    static constexpr int NumVoices = 16;
    static constexpr int MaxBlock = 256;

    void enable() { enabled = true; }
    bool isEnabled() const { return enabled; }

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        pendingCount = 0;
        idle = true;
        if (! enabled) return;
        if (voices.empty()) voices.resize (NumVoices);
        for (auto& v : voices) v.active = false;
        if (! chain) chain = std::make_unique<mrg::Chain>();
        mrg::SoundParams sp;              // o timbre de fábrica do MRG-68: Tele 68, ponte, Twin, mola
        sp.echoOn = false; sp.satOn = false;
        chain->p = sp;
        chain->prepare (sr);
    }

    // notas do piano: guardadas até o próximo bloco, para a palhetada sair em ordem (grave -> agudo)
    void noteOn (int note, float vel)
    {
        if (! enabled) return;
        int n = note;
        while (n < 55) n += 12;      // região do skank: G3..E5
        while (n > 76) n -= 12;
        for (int i = 0; i < pendingCount; ++i) if (pending[i].note == n) return;
        if (pendingCount < MaxPending) pending[pendingCount++] = { n, vel, note, false };
    }

    void noteOff (int note)
    {
        if (! enabled) return;
        for (int i = 0; i < pendingCount; ++i) if (pending[i].src == note) pending[i].releaseNow = true;
        for (size_t i = 0; i < voices.size(); ++i)
            if (voices[i].active && voices[i].held && src[i] == note) voices[i].release();
    }

    void allNotesOff()
    {
        pendingCount = 0;
        for (auto& v : voices) v.release();
    }

    // um bloco (até MaxBlock) da guitarra já com o amplificador do MRG-68, somado em L/R
    void render (float* L, float* R, int n)
    {
        if (! enabled) return;
        flush();
        bool any = false;
        for (auto& v : voices) if (v.active) { any = true; break; }
        if (! any && idle) return;
        std::fill (tL, tL + n, 0.0f); std::fill (tR, tR + n, 0.0f);
        for (int s = 0; s < n; ++s)
            for (auto& v : voices) if (v.active) v.tick (tL[s], tR[s]);
        chain->process (tL, tR, n);
        float pk = 0;
        for (int s = 0; s < n; ++s) { L[s] += tL[s]; R[s] += tR[s]; pk = std::max (pk, std::fabs (tL[s]) + std::fabs (tR[s])); }
        idle = ! any && pk < 1.0e-5f;     // espera a cauda da mola acabar antes de parar de processar
    }

private:
    struct Pending { int note; float vel; int src; bool releaseNow; };
    static constexpr int MaxPending = 16;

    // a palhetada do strum() do MRG-68 (skank seco: para baixo, duas guitarras em estéreo)
    void flush()
    {
        if (pendingCount == 0) return;
        std::sort (pending, pending + pendingCount, [] (const Pending& a, const Pending& b) { return a.note < b.note; });
        const int first = std::max (0, pendingCount - 4);       // no máximo 4 cordas: as mais agudas
        float vel = 0;
        for (int i = first; i < pendingCount; ++i) vel = std::max (vel, std::min (1.0f, pending[i].vel));
        const double sp = (0.002 + 0.35 * 0.008) * (1.2 - 0.4 * vel);   // "strum" de fábrica do MRG-68
        const double pickupPos = mrg::pickupModel (mrg::PkPonte).pos;
        for (int take = 0; take < 2; ++take)
        {
            const int takeOff = take ? (int) (0.012 * sr) : 0;
            const float pan = take ? 0.35f : -0.35f, tv = take ? vel * 0.96f : vel;
            for (int i = first, k = 0; i < pendingCount; ++i, ++k)
            {
                const float v = tv * (1.0f - 0.09f * (float) k) * (0.92f + 0.08f * rng.uni());
                const int idx = freeVoice();
                voices[(size_t) idx].start (sr, pending[i].note, v, 0, takeOff + (int) ((double) k * sp * sr),
                                            0.12 + 0.03 * rng.uni(), pickupPos, 0.0f, 0.9965, 0.8, pan, ++seed, -1, true);
                src[(size_t) idx] = pending[i].src;
                if (pending[i].releaseNow) voices[(size_t) idx].release();
            }
        }
        pendingCount = 0;
        idle = false;
    }

    int freeVoice()
    {
        int best = 0;
        for (int i = 0; i < (int) voices.size(); ++i)
        {
            if (! voices[(size_t) i].active) return i;
            if (! voices[(size_t) i].held && voices[(size_t) i].age > voices[(size_t) best].age) best = i;
        }
        return best;
    }

    bool enabled = false, idle = true;
    double sr = 48000.0;
    std::vector<mrg::StringVoice> voices;         // no heap: cada corda guarda as linhas de atraso (~50 kB)
    std::array<int, NumVoices> src {};
    std::unique_ptr<mrg::Chain> chain;
    Pending pending[MaxPending] {};
    int pendingCount = 0;
    uint32_t seed = 1;
    mrg::Rng rng { 7 };
    float tL[MaxBlock] {}, tR[MaxBlock] {};
};
} // namespace rk
