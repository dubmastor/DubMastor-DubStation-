// Teste do motor de harmonia (sem JUCE): voicings, padrões e agendamento por blocos sem notas presas
// g++ -std=c++17 -O2 -Wall tests/engine_test.cpp -o engine_test && ./engine_test
#include "../Source/DubEngine.h"
#include <cstdio>
#include <cstdlib>
using namespace dub;

static int failures = 0;
#define CHECK(c, ...) do { if (! (c)) { std::printf ("FALHOU: " __VA_ARGS__); std::printf ("\n"); ++failures; } } while (0)

int main()
{
    // 1) voicings com condução de vozes: Am -> Dm -> Em -> G
    std::vector<ChordSlot> slots { { 9, Min, 8 }, { 2, Min, 4 }, { 4, Min, 4 }, { 7, Maj, 2 } };   // inclui 1/2 compasso
    std::vector<int> prev;
    for (auto& c : slots)
    {
        auto v = voiceChord (c.root, c.quality, 62, prev);
        std::printf ("%-4s", chordLabel (c).c_str());
        for (int n : v) std::printf (" %d", n);
        std::printf ("\n");
        CHECK (v.size() == 3, "triade com 3 notas");
        for (int n : v) CHECK (n >= 48 && n <= 80, "nota %d fora do registro", n);
        if (! prev.empty())
        {
            int move = 0;
            for (size_t k = 0; k < 3; ++k) move += std::abs (v[k] - prev[k]);
            CHECK (move <= 9, "salto grande demais entre acordes (%d semitons)", move);
        }
        prev = v;
    }

    // 1b) inversões: a sugerida é a de menor movimento; as manuais saem como pedido
    {
        auto movement = [] (const std::vector<int>& a, const std::vector<int>& b)
        { int m = 0; for (size_t k = 0; k < std::min (a.size(), b.size()); ++k) m += std::abs (a[k] - b[k]); return m; };

        const std::vector<std::vector<ChordSlot>> progs {
            { { 9, Min, 8 }, { 2, Min, 4 }, { 4, Min, 4 }, { 7, Maj, 2 } },
            { { 0, Maj, 4 }, { 5, Maj, 4 }, { 7, Maj, 4 }, { 9, Min, 4 } },
            { { 2, Min, 4 }, { 7, Maj, 4 }, { 0, Maj, 4 }, { 9, Min, 4 } },
            { { 4, Min, 8 }, { 9, Min, 8 }, { 11, Min, 4 }, { 4, Min, 4 } },
        };
        for (const auto& prog : progs)
        {
            const auto v = voiceProgression (prog, 60);
            std::printf ("sugestao:");
            for (size_t i = 0; i < prog.size(); ++i)
            {
                const auto& prev = v.notes[(i + prog.size() - 1) % prog.size()];
                const int got = movement (v.notes[i], prev);
                int bestAny = 1000;
                for (int inv = 0; inv < 3; ++inv)
                    bestAny = std::min (bestAny, movement (voiceChord (prog[i].root, prog[i].quality, 60, prev, inv), prev));
                std::printf ("  %s=%s(%d st)", chordLabel (prog[i]).c_str(), inversionName (v.suggested[i]), got);
                CHECK (got <= bestAny + 1, "%s: sugerida move %d, existe opcao com %d", chordLabel (prog[i]).c_str(), got, bestAny);
                CHECK (v.used[i] == v.suggested[i], "AUTO deveria usar a sugerida");
            }
            std::printf ("\n");
        }
        auto forced = progs[0];
        for (int inv = 0; inv < 3; ++inv)
        {
            for (auto& c : forced) c.inversion = inv;
            const auto v = voiceProgression (forced, 60);
            for (size_t i = 0; i < forced.size(); ++i)
                CHECK (v.used[i] == inv, "inversao manual %d virou %d", inv, v.used[i]);
        }
    }

    // 2) duas mãos + bubble uma oitava abaixo
    {
        Settings s; double L = 0;
        const auto ev = buildLoop (slots, s, L);
        std::vector<int> sk, org;
        for (auto& e : ev)
            if (e.start < 1.0) { if (e.channel == ChSkank) sk.push_back (e.note); if (e.channel == ChBubble) org.push_back (e.note); }
        std::sort (sk.begin(), sk.end()); sk.erase (std::unique (sk.begin(), sk.end()), sk.end());
        std::sort (org.begin(), org.end()); org.erase (std::unique (org.begin(), org.end()), org.end());
        std::printf ("skank:"); for (int n : sk) std::printf (" %d", n);
        std::printf ("   bubble:"); for (int n : org) std::printf (" %d", n); std::printf ("\n");
        CHECK (sk.size() == 6, "skank deveria ter 6 notas (duas maos), tem %zu", sk.size());
        CHECK (org.size() == 6, "bubble deveria ter 6 notas (duas maos), tem %zu", org.size());
        if (sk.size() == 6) CHECK (sk[3] == sk[0] + 12 && sk[4] == sk[1] + 12 && sk[5] == sk[2] + 12, "mao direita deveria ser a esquerda + 12");
        if (sk.size() == 6 && org.size() == 6)
        {
            CHECK (org[3] == sk[0] && org[4] == sk[1] && org[5] == sk[2], "mao direita do bubble = mao esquerda do skank");
            CHECK (org[0] == sk[0] - 12, "mao esquerda do bubble uma oitava abaixo da direita");
        }
        // ritmo do bubble no 1º tempo: "e" (mão esquerda), "&" (mão direita), "a" (mão esquerda)
        std::vector<std::pair<double, int>> b1;
        for (auto& e : ev) if (e.channel == ChBubble && e.start < 1.0) b1.push_back ({ e.start, e.note });
        int le = 0, ri = 0;
        for (auto& [t, n] : b1)
        {
            const bool right = std::find (sk.begin(), sk.begin() + 3, n) != sk.begin() + 3;
            if (std::abs (t - 0.5) < 1e-9) { CHECK (right, "no & tem que ser a mao direita"); ++ri; }
            else if (std::abs (t - 0.25) < 1e-9 || std::abs (t - 0.75) < 1e-9) { CHECK (! right, "no e/a tem que ser a mao esquerda"); ++le; }
            else CHECK (false, "nota de bubble fora do e/&/a (t=%g)", t);
        }
        CHECK (ri == 3 && le == 6, "bubble do 1o tempo: esperado 3 notas da direita e 6 da esquerda (%d/%d)", ri, le);
        // meio compasso: o acorde 4 (G, 1/2 compasso) começa no tempo 16 e tem skank
        bool g = false;
        for (auto& e : ev) if (e.slot == 3 && e.channel == ChSkank && e.start >= 16.0 && e.start < 18.0) g = true;
        CHECK (g, "acorde de meio compasso sem skank");
    }

    // 2b) com o padrão do plugin (noteSize 0), TODAS as notas (skank e bubble) duram no máximo 1/32
    for (int pat = 0; pat < NumPatterns; ++pat)
    {
        Settings s; s.pattern = pat; s.bubble = true; s.swing = 0.5f;
        double L = 0; const auto ev = buildLoop (slots, s, L);
        double longest = 0;
        for (auto& e : ev) longest = std::max (longest, e.length);
        CHECK (longest <= 0.125 + 1e-9, "nota maior que 1/32 (%g tempos) no padrao %d", longest, pat);
    }

    // 3) padrões x gate x swing: simula a DAW tocando em blocos de 512 amostras
    for (int pat = 0; pat < NumPatterns; ++pat)
        for (int ns = 0; ns < numNoteSizes; ++ns)
            for (float swing : { 0.0f, 0.6f })
            {
                Settings s; s.pattern = pat; s.noteSize = ns; s.swing = swing;
                double L = 0; const auto loop = buildLoop (slots, s, L);
                CHECK (L == 18.0, "loop deveria ter 18 tempos (%g)", L);
                CHECK (! loop.empty(), "loop vazio");

                bool active[17][128] {}; long ons = 0, offs = 0, retrig = 0;
                const double sr = 48000, bpm = 75; const int bs = 512; const double bb = bs / sr * bpm / 60.0;
                double ppq = 0;
                for (int blk = 0; blk < (int) (L * 4 / bb); ++blk)
                {
                    const double end = ppq + bb;
                    struct P { int s; bool on; int ch, n; };
                    std::vector<P> pend;
                    auto col = [&] (double t, bool on, const NoteEvent& e)
                    {
                        double occ = t + std::ceil ((ppq - t) / L) * L;
                        for (; occ < end; occ += L)
                            if (occ >= ppq) pend.push_back ({ std::min (bs - 1, (int) ((occ - ppq) / bb * bs)), on, e.channel, e.note });
                    };
                    for (auto& e : loop) { col (e.start, true, e); col (e.start + e.length, false, e); }
                    std::sort (pend.begin(), pend.end(), [] (auto& a, auto& b) { return a.s != b.s ? a.s < b.s : (int) a.on < (int) b.on; });
                    for (auto& p : pend)
                    {
                        bool& a = active[p.ch][p.n];
                        if (p.on) { if (a) ++retrig; a = true; ++ons; }
                        else if (a) { a = false; ++offs; }
                    }
                    ppq = end;
                }
                int stillOn = 0;
                for (auto& c : active) for (bool b : c) stillOn += b;
                std::printf ("%-18s nota=%-4s swing=%.1f  notas=%ld  re-ataques=%ld\n", patternName (pat), noteSizeName (ns), swing, ons, retrig);
                CHECK (retrig == 0, "nota re-atacada sem note-off (%s)", patternName (pat));
                CHECK (ons - offs == stillOn && stillOn <= 12, "notas presas (%d)", stillOn);
            }

    std::printf (failures == 0 ? "\nTODOS OS TESTES PASSARAM\n" : "\n%d FALHA(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
