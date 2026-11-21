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
    std::vector<ChordSlot> slots { { 9, Min, 8 }, { 2, Min, 4 }, { 4, Min, 4 }, { 7, Maj, 2 } };
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

    // 2) padrões x gate x swing: simula a DAW tocando em blocos de 512 amostras
    for (int pat = 0; pat < NumPatterns; ++pat)
        for (float gate : { 0.1f, 0.7f, 1.0f })
            for (float swing : { 0.0f, 0.6f })
            {
                Settings s; s.pattern = pat; s.gate = gate; s.swing = swing;
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
                std::printf ("%-24s gate=%.1f swing=%.1f  notas=%ld  re-ataques=%ld\n", patternName (pat), gate, swing, ons, retrig);
                CHECK (retrig == 0, "nota re-atacada sem note-off (%s)", patternName (pat));
                CHECK (ons - offs == stillOn && stillOn <= 4, "notas presas (%d)", stillOn);
            }

    std::printf (failures == 0 ? "\nTODOS OS TESTES PASSARAM\n" : "\n%d FALHA(S)\n", failures);
    return failures == 0 ? 0 : 1;
}
