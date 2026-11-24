// Teste offline do motor da Monstro Machine MM-2500 (sem JUCE).
//   g++ -std=c++17 -O2 -I Source tests/render_test.cpp -o mm_render && ./mm_render [saida.wav]
// Confere: 33 kits, contagem das famílias, o mesmo aleatório da versão web, todos os pads de todos os kits
// soando sem NaN nem estouro, grooves de reggae com caixa/aro no 2 e no 4, e saturador/eco em bypass de fábrica.
#include "MonstroEngine.h"
#include <cstdio>
#include <cmath>
#include <fstream>

static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { std::printf ("FALHOU: " __VA_ARGS__); std::printf ("\n"); ++fails; } } while (0)

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    std::ofstream f (path, std::ios::binary);
    auto u32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto u16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    const uint32_t n = (uint32_t) L.size();
    f.write ("RIFF", 4); u32 (36 + n * 4); f.write ("WAVEfmt ", 8); u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    f.write ("data", 4); u32 (n * 4);
    for (uint32_t i = 0; i < n; ++i)
        for (float x : { L[i], R[i] }) { const int16_t s = (int16_t) std::lround (std::max (-1.0f, std::min (1.0f, x)) * 32767); f.write ((const char*) &s, 2); }
}

int main (int argc, char** argv)
{
    const int sr = 48000;
    CHECK (mm::numKits() == 33, "esperava 33 kits, veio %d", mm::numKits());

    // mesmo hash e mesmo aleatório da versão web (valores conferidos no node)
    CHECK (mm::hashStr ("DubMastor") == 648870692u, "hash diferente da web");
    CHECK (std::abs (mm::charOf (0).kStart - 138.896) < 1e-6, "charOf diferente da web");
    const auto v3 = mm::varOf (0, 0, 3);
    CHECK (std::abs (v3.t + 0.6924) < 1e-6 && std::abs (v3.d - 0.84112) < 1e-6, "variação diferente da web");
    const auto v0 = mm::varOf (0, 0, 0);
    CHECK (v0.t == 0 && v0.d == 1 && v0.b == 1, "o timbre 01 tem que ser o original");

    // famílias do kit DubMastor (lista publicada do pack)
    CHECK (mm::famCount (0, mm::KICK) == 81, "kicks do DubMastor: %d", mm::famCount (0, mm::KICK));
    CHECK (mm::famCount (0, mm::SNARE) == 338, "snares do DubMastor: %d", mm::famCount (0, mm::SNARE));
    CHECK (mm::famCount (0, mm::HIHAT) == 78, "hihats do DubMastor: %d", mm::famCount (0, mm::HIHAT));
    for (int k = 0; k < mm::numKits(); ++k)
        for (int f = 0; f < mm::numFams; ++f) CHECK (mm::famCount (k, f) >= 1, "kit %d família %d vazia", k, f);

    // todos os pads de todos os kits, em 3 timbres de cada família
    mm::Synth s; s.prepare (sr);
    std::vector<float> L ((size_t) sr * 4), R ((size_t) sr * 4);
    for (int k = 0; k < mm::numKits(); ++k)
    {
        const auto c = mm::charOf (k);
        for (int p = 0; p < mm::numPads; ++p)
            for (int v : { 0, 1, 7 })
            {
                std::fill (L.begin(), L.end(), 0.0f); std::fill (R.begin(), R.end(), 0.0f);
                s.prepare (sr);
                s.hit (p, k, c, mm::varOf (k, p, v), 0.9, 0, 100, nullptr);
                s.render (L.data(), R.data(), (int) L.size());
                double pk = 0, e = 0; bool bad = false;
                for (float x : L) { if (! std::isfinite (x)) bad = true; pk = std::max (pk, (double) std::abs (x)); e += x * x; }
                CHECK (! bad, "NaN no kit %d pad %d timbre %d", k, p, v);
                CHECK (pk > 0.01, "pad mudo: kit %d pad %d timbre %d (pico %f)", k, p, v, pk);
                CHECK (pk < 4.0, "pad estourado: kit %d pad %d timbre %d (pico %f)", k, p, v, pk);
                // o som termina (a voz é liberada)
                double tail = 0; for (size_t i = L.size() - 4800; i < L.size(); ++i) tail = std::max (tail, (double) std::abs (L[i]));
                CHECK (tail < 1e-3, "pad sem fim: kit %d pad %d (%f)", k, p, tail);
            }
    }

    // grooves: 4 SEQs por estilo, nenhum vazio; reggae com caixa ou aro nos tempos 2 e 4 (passos 4 e 12)
    for (const char* st : mm::styleKeys())
    {
        const auto& pats = mm::patterns (st);
        for (int q = 0; q < mm::numSeqs; ++q)
        {
            int n = 0; for (auto& r : pats[(size_t) q]) for (auto x : r) n += x ? 1 : 0;
            CHECK (n > 3, "%s SEQ %d quase vazia", st, q);
        }
    }
    for (int q = 0; q < mm::numSeqs; ++q)
    {
        const auto& g = mm::patterns ("reggae")[(size_t) q];
        for (int step : { 4, 12 })
        {
            const bool back = g[2][(size_t) step] || g[4][(size_t) step];
            CHECK (back, "reggae SEQ %c sem caixa/aro no passo %d (tempo %d)", 'A' + q, step, step / 4 + 1);
        }
        CHECK (! g[2][8] && ! g[4][8], "reggae SEQ %c com caixa no 3 (one drop errado)", 'A' + q);
    }

    // efeitos: padrão de fábrica em bypass; ligados, continuam estáveis
    mm::FxSettings def;
    CHECK (! def.satOn && ! def.echoOn, "saturador e eco têm que começar em bypass");

    // um compasso do kit DubMastor no groove A, com eco e saturador ligados (arquivo para ouvir)
    {
        const int bars = 4; const double bpm = mm::kits()[0].bpm, stepDur = 60.0 / bpm / 4;
        const int total = (int) (bars * 16 * stepDur * sr) + sr;
        std::vector<float> OL ((size_t) total), OR ((size_t) total);
        s.prepare (sr); mm::Fx fx; fx.prepare (sr);
        mm::FxSettings fs; fs.satOn = true; fs.echoOn = true; fs.stepDur = stepDur;
        const auto c = mm::charOf (0);
        const auto& g = mm::patterns (mm::kits()[0].style)[0];
        int pos = 0;
        for (int k = 0; k < bars * 16; ++k)
        {
            const int st = k % 16;
            const int at = (int) ((k * stepDur + (st % 2 ? 0.08 * stepDur : 0)) * sr);
            if (at > pos) { s.render (OL.data() + pos, OR.data() + pos, at - pos); pos = at; }
            for (int p = 0; p < mm::numPads; ++p) if (g[(size_t) p][(size_t) st]) s.hit (p, 0, c, mm::varOf (0, p, 0), g[(size_t) p][(size_t) st] == 2 ? 1.0 : 0.75, 0, 100, nullptr);
        }
        s.render (OL.data() + pos, OR.data() + pos, total - pos);
        fx.process (OL.data(), OR.data(), total, fs);
        double pk = 0; bool bad = false;
        for (float x : OL) { if (! std::isfinite (x)) bad = true; pk = std::max (pk, (double) std::abs (x)); }
        CHECK (! bad && pk > 0.05 && pk < 1.5, "groove com efeitos: pico %f", pk);
        if (argc > 1) writeWav (argv[1], OL, OR, sr);
    }

    // ---- 1.1: vários kits por produtor e o groove de cada produtor (iguais aos da web)
    auto grooveHash = [] (int k)
    {
        uint32_t h = 2166136261u;
        for (auto& s : mm::groove (k)) for (auto& p : s) for (auto v : p) h = (h ^ (uint32_t) ('0' + v)) * 16777619u;
        return h;
    };
    CHECK (std::string (mm::kits()[(size_t) mm::defaultKit()].name) == "DJ Coala", "preset inicial tem que ser o DJ Coala");
    CHECK (grooveHash (0) == 1595767603u, "groove do DubMastor diferente da web (%u)", grooveHash (0));
    CHECK (grooveHash (1) == 2340979147u, "groove do Nox diferente da web (%u)", grooveHash (1));
    for (int k = 0; k < mm::numKits(); ++k)
    {
        const auto& x = mm::kits()[(size_t) k];
        bool hasProf = false;
        for (auto& p : mm::profs()) hasProf |= std::string (p.name) == x.name;
        CHECK (hasProf, "%s sem perfil de groove", x.name);
        const auto g = mm::groove (k);
        for (int q = 0; q < mm::numSeqs; ++q)
        {
            int notes = 0;
            for (auto& p : g[(size_t) q]) for (auto v : p) notes += v ? 1 : 0;
            CHECK (notes >= 4, "%s SEQ %c quase vazio", x.name, 'A' + q);
            if (std::string (x.style) == "reggae")   // caixa/aro só no 2 e no 4 (passos 5 e 13): nunca o drop no 3
                for (int pad : { 2, 4 })
                    for (int st = 0; st < mm::numSteps; ++st)
                        if (g[(size_t) q][(size_t) pad][(size_t) st]) CHECK (st == 4 || st == 12, "%s: caixa/aro fora do 2 e do 4 (passo %d)", x.name, st + 1);
        }
        const int nk = mm::kitCountF (k);
        CHECK (nk >= 3 && nk <= 8, "%s: %d kits", x.name, nk);
        const auto k1 = mm::kitVars (k, 0);
        for (int v : k1) CHECK (v == 0, "%s: o KIT 1 tem que ser o original", x.name);
        for (int a = 0; a < nk; ++a)
            for (int b = a + 1; b < nk; ++b) CHECK (mm::kitVars (k, a) != mm::kitVars (k, b), "%s: KIT %d igual ao KIT %d", x.name, a + 1, b + 1);
        for (int n = 0; n < nk; ++n)
        {
            const auto v = mm::kitVars (k, n);
            for (int p = 0; p < mm::numPads; ++p) CHECK (v[(size_t) p] >= 0 && v[(size_t) p] < mm::famCount (k, mm::padFam()[(size_t) p]), "%s: timbre fora da família", x.name);
        }
    }

    if (fails) { std::printf ("%d falha(s)\n", fails); return 1; }
    std::printf ("OK: %d produtores, kits por produtor, 16 pads, timbres por família, grooves de cada produtor e efeitos\n", mm::numKits());
    return 0;
}
