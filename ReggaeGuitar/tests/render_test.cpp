// Teste offline do Mastor Reggae Guitar MRG-68 (sem JUCE):
//   g++ -std=c++17 -O2 -Wall -Wextra -I Source tests/render_test.cpp -o render_test && ./render_test
// Confere: contagem com a caixa no 2 e no 4 (skank nos "e"), bang-bang 1/16 depois e em upstroke nas agudas,
// variação no último compasso do ciclo, afinação da corda, e renderiza os 12 guitarristas sem NaN nem clipping.
#include "GuitarEngine.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace mrg;
static int falhas = 0;
#define CHECK(cond, msg) do { if (! (cond)) { std::printf ("FALHOU: %s\n", msg); ++falhas; } } while (0)

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), bytes = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    for (uint32_t i = 0; i < n; ++i)
    { int16_t a = (int16_t) std::lround (std::clamp (L[i], -1.0f, 1.0f) * 32767), b = (int16_t) std::lround (std::clamp (R[i], -1.0f, 1.0f) * 32767);
      std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f); }
    std::fclose (f);
}

int main()
{
    const double sr = 48000, bpm = 76;

    // 1) contagem: skank só nos "e" (passos 3, 7, 11, 15), aro e bumbo no 2 e no 4
    {
        GrooveSettings s; s.vary = false; s.band = true;
        for (int st = 0; st < 16; ++st)
        {
            const auto acts = grooveStep (s, st, bpm);
            bool strum = false, rim = false;
            for (auto& a : acts) { strum |= a.kind == Action::Strum; rim |= a.kind == Action::Rim; }
            CHECK (strum == (st % 4 == 2), "skank fora do 'e'");
            CHECK (rim == (st == 4 || st == 12), "aro fora do 2 e do 4");
        }
    }
    // 2) bang-bang: downstroke no "e" + upstroke nas 3 agudas uma semicolcheia depois, mais fraco
    {
        GrooveSettings s; s.vary = false; s.stroke = StBangBang;
        const auto acts = grooveStep (s, 2, bpm);
        CHECK (acts.size() == 2, "bang-bang precisa de dois golpes");
        if (acts.size() == 2)
        {
            CHECK (! acts[0].up && acts[1].up, "bang-bang: 1o down, 2o up");
            CHECK (acts[1].top3 && ! acts[0].top3, "bang-bang: 2o golpe so nas agudas");
            CHECK (std::fabs ((acts[1].offsetBeats - acts[0].offsetBeats) - 0.25) < 1e-9, "bang-bang: 2o golpe 1/16 depois");
            CHECK (acts[1].vel < acts[0].vel, "bang-bang: 2o golpe mais fraco");
            CHECK (acts[0].holdSec < 0.25 * 60.0 / bpm, "bang-bang: 1o golpe abafado antes do 2o");
        }
    }
    // 3) variação: só no último compasso do ciclo, a partir do tempo escolhido
    {
        GrooveSettings s; s.every = 4; s.start = 3;
        for (int i = 0; i < 16; ++i) s.fill[i] = varPreset (VarLick).s[i];
        bool leadFora = false, leadDentro = false;
        for (long long st = 0; st < 16 * 8; ++st)
            for (auto& a : grooveStep (s, st, bpm))
                if (a.kind == Action::Lead)
                {
                    const long long bar = st / 16; const int i = (int) (st % 16);
                    if (bar % 4 == 3 && i >= 8) leadDentro = true; else leadFora = true;
                }
        CHECK (leadDentro && ! leadFora, "variacao fora do 4o compasso / do tempo 3");
    }
    // 4) afinação da corda (A3 = 220 Hz)
    {
        static StringVoice v;
        v.start (sr, 57, 0.9f, (int) sr, 0, 0.13, 0.1, 0, 0.9965, 0.8, 0, 1, -1, true);
        std::vector<float> y (24000);
        for (auto& s : y) { float l = 0, r = 0; v.tick (l, r); s = l; }
        int best = 0; double bestS = -1e9;
        for (int lag = 180; lag < 260; ++lag)
        { double s = 0; for (int i = 2000; i < 20000; ++i) s += y[(size_t) i] * y[(size_t) i + (size_t) lag]; if (s > bestS) { bestS = s; best = lag; } }
        CHECK (std::abs (best - 218) <= 1, "afinacao da corda fora");
        std::printf ("A3: periodo %d amostras (esperado 218)\n", best);
    }
    // 5) os 12 guitarristas: 8 compassos sem NaN, sem clipping e com som
    for (int p = 0; p < NumPlayers; ++p)
    {
        const Player& pl = player (p);
        static StringVoice voices[64];
        for (auto& v : voices) v.active = false;
        static Chain chain; chain.p = SoundParams(); chain.prepare (sr);
        SoundParams sp; sp.guitar = pl.guitar; sp.pickup = pl.pickup; sp.amp = pl.amp; sp.wahOn = pl.wah; sp.phOn = pl.phaser; sp.chOn = pl.chorus; sp.bpm = pl.tracks[0].bpm;
        chain.update (sp);
        GrooveSettings s; s.stroke = pl.stroke; s.mute = pl.mute;
        if (pl.chop) { s.steps[7] = 2; s.steps[15] = 2; }
        const VarPreset& vp = varPreset (pl.var); s.start = vp.start; for (int i = 0; i < 16; ++i) s.fill[i] = vp.s[i];
        const double b = pl.tracks[0].bpm, spb = 60.0 / b;
        const int len = (int) (8 * 4 * spb * sr) + (int) sr;
        std::vector<float> L ((size_t) len), R ((size_t) len);
        uint32_t seed = 1; Rng rng (99);
        auto freeVoice = [&] () -> StringVoice& { StringVoice* best = &voices[0]; for (auto& v : voices) { if (! v.active) return v; if (v.age > best->age) best = &v; } return *best; };
        std::vector<std::pair<int, Action>> sched;
        for (long long st = 0; st < 16 * 8; ++st)
            for (auto& a : grooveStep (s, st, b)) sched.push_back ({ (int) (st * 0.25 * spb * sr), a });
        size_t next = 0; const int block = 256;
        for (int pos = 0; pos < len; pos += block)
        {
            const int n = std::min (block, len - pos);
            while (next < sched.size() && sched[next].first < pos + n)
            {
                const auto& [t, a] = sched[next++];
                const int off = t - pos + (int) (a.offsetBeats * spb * sr);
                const std::array<int, 4> ch = chordVoicing ((t / (int) (4 * spb * sr)) % 2 ? 2 : 9, true);
                if (a.kind == Action::Strum)
                {
                    std::vector<int> notes (ch.begin(), ch.end()); std::sort (notes.begin(), notes.end());
                    if (a.top3) notes.erase (notes.begin());
                    if (a.up) std::reverse (notes.begin(), notes.end());
                    const double spr = a.spread >= 0 ? a.spread : 0.0048;
                    for (int take = 0; take < 2; ++take)
                        for (size_t i = 0; i < notes.size(); ++i)
                            freeVoice().start (sr, notes[i], a.vel * (1.0f - 0.09f * (float) i), (int) (std::max (0.008, a.holdSec) * sr),
                                               off + (take ? (int) (a.dbl * sr) : 0) + (int) (i * spr * sr), 0.13 + 0.02 * rng.uni(), pickupModel (pl.pickup).pos,
                                               a.mute == MuPalm ? 1.0f : 0.0f, a.mute == MuAberto ? 0.998 : 0.9965, a.mute == MuMaoEsq ? 0.7 : 0.8,
                                               take ? 0.35f : -0.35f, ++seed, -1, false);
                }
                else if (a.kind == Action::Lead)
                    freeVoice().start (sr, a.note, a.vel, (int) (a.holdSec * sr), off, 0.15, pickupModel (pl.pickup).pos, 1.0f, 0.998, 0.8, 0.1f, ++seed, a.note, false);
            }
            for (auto& v : voices) if (v.active) for (int i = 0; i < n; ++i) v.tick (L[(size_t) (pos + i)], R[(size_t) (pos + i)]);
            chain.process (&L[(size_t) pos], &R[(size_t) pos], n);
        }
        double peak = 0, rms = 0; bool nan = false;
        for (size_t i = 0; i < L.size(); ++i) { nan |= ! std::isfinite (L[i]) || ! std::isfinite (R[i]); peak = std::max (peak, (double) std::fabs (L[i])); rms += L[i] * L[i]; }
        rms = std::sqrt (rms / L.size());
        std::printf ("%-16s pico %.2f  rms %.3f\n", pl.name, peak, rms);
        CHECK (! nan, "NaN no audio");
        CHECK (peak < 1.0 && peak > 0.05, "nivel fora (clipping ou mudo)");
        if (p == 5 || p == 7)
            writeWav (std::string ("mrg68_") + (p == 5 ? "chinna" : "bingy") + ".wav", L, R, (int) sr);
    }
    std::printf (falhas ? "\n%d falha(s)\n" : "\nTudo certo: contagem, bang-bang, variacao, afinacao e os 12 guitarristas.\n", falhas);
    return falhas ? 1 : 0;
}
