// Teste do motor de linhas de baixo (sem JUCE): estilos, registro, monofonia, determinismo
// e agendamento por blocos sem notas presas.
// g++ -std=c++17 -O2 -Wall tests/engine_test.cpp -o engine_test && ./engine_test
#include "../Source/BassEngine.h"
#include "../Source/AccompEngine.h"
#include "../Source/RiddimPresets.h"
#include <cstdio>
#include <cstdlib>
#include <set>
using namespace mbm;

static int failures = 0;
#define CHECK(c, ...) do { if (! (c)) { std::printf ("FALHOU: " __VA_ARGS__); std::printf ("\n"); ++failures; } } while (0)

static void printLine (const char* title, const std::vector<NoteEvent>& ev)
{
    std::printf ("%s\n", title);
    static const char* names[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    int bar = -1;
    for (const auto& e : ev)
    {
        const int b = (int) (e.start / 4.0);
        if (b != bar) { std::printf ("\n  comp %d:", b + 1); bar = b; }
        std::printf (" %s%d@%.2f", names[e.note % 12], e.note / 12 - 1, e.start - b * 4.0);
    }
    std::printf ("\n\n");
}

int main()
{
    // Am (2 comp.) | Dm (1) | Em (1) | G (1/2)
    const std::vector<ChordSlot> slots { { 9, Min, 8 }, { 2, Min, 4 }, { 4, Min7, 4 }, { 7, Maj, 2 } };

    for (int style = 0; style < NumStyles; ++style)
    {
        Settings s; s.style = style; s.seed = 42;
        double L = 0;
        const auto ev = buildLoop (slots, s, L);
        printLine (styleName (style), ev);
        CHECK (L == 18.0, "loop deveria ter 18 tempos (%g)", L);
        CHECK (ev.size() >= 10, "%s: poucas notas (%zu)", styleName (style), ev.size());
    }

    // varredura: estilos x ideias x densidade x variação x swing x registro
    for (int style = 0; style < NumStyles; ++style)
    {
        std::set<std::string> distinct;
        for (int seed = 0; seed < 60; ++seed)
            for (float dens : { 0.0f, 0.6f, 1.0f })
                for (float var : { 0.0f, 1.0f })
                    for (float sw : { 0.0f, 1.0f })
                        for (int base : { 28, 31, 40 })
                        {
                            Settings s; s.style = style; s.seed = seed; s.density = dens; s.variation = var;
                            s.swing = sw; s.base = base; s.gate = 1.0f;
                            double L = 0;
                            const auto ev = buildLoop (slots, s, L);
                            CHECK (! ev.empty(), "loop vazio");
                            std::string sig;
                            for (size_t i = 0; i < ev.size(); ++i)
                            {
                                const auto& e = ev[i];
                                CHECK (e.note >= 28 && e.note <= 67, "nota %d fora do registro de baixo", e.note);
                                CHECK (e.start >= 0.0 && e.start < L, "nota fora do loop (%g)", e.start);
                                CHECK (e.length >= 0.04, "nota curta demais");
                                CHECK (e.velocity >= 1 && e.velocity <= 127, "velocity invalida");
                                const double nextStart = i + 1 < ev.size() ? ev[i + 1].start : ev[0].start + L;
                                CHECK (e.start + e.length <= nextStart + 1e-9, "%s: notas sobrepostas (baixo e monofonico)", styleName (style));
                                // Family Man: o tempo 1 de cada compasso fica vazio (one drop)
                                if (style == FamilyMan)
                                    CHECK (std::fmod (e.start, 4.0) > 1e-6, "Family Man tocou no tempo 1 (%g)", e.start);
                                sig += std::to_string (e.note) + "@" + std::to_string ((int) (e.start * 100)) + " ";
                            }
                            if (dens == 0.6f && var == 1.0f && sw == 0.0f && base == 31) distinct.insert (sig);

                            // determinismo
                            double L2 = 0;
                            const auto ev2 = buildLoop (slots, s, L2);
                            CHECK (ev2.size() == ev.size(), "mesma ideia gerou linha diferente");
                        }
        std::printf ("%-12s %zu linhas diferentes em 60 ideias\n", styleName (style), distinct.size());
        CHECK (distinct.size() >= 15, "%s: ideias pouco variadas", styleName (style));
    }

    // Flabba: fundamental no 1 de todo compasso que começa um acorde
    {
        Settings s; s.style = Flabba; s.seed = 7; s.density = 0.0f;
        double L = 0;
        const auto ev = buildLoop (slots, s, L);
        for (double barStart : { 0.0, 4.0, 8.0, 12.0, 16.0 })
        {
            bool found = false;
            for (const auto& e : ev) if (std::abs (e.start - barStart) < 1e-6) found = true;
            CHECK (found, "Flabba sem nota no 1 do compasso em %g", barStart);
        }
    }

    // padrão de início do plugin: Am e Dm de meio compasso cada (loop de 1 compasso)
    for (int style = 0; style < NumStyles; ++style)
    {
        Settings s; s.style = style;
        double L = 0;
        const auto ev = buildLoop ({ { 9, Min, 2 }, { 2, Min, 2 } }, s, L);
        CHECK (L == 4.0 && ev.size() >= 2, "%s: padrao de inicio (2 x 1/2 compasso)", styleName (style));
    }

    // um acorde só, meio compasso, e nenhum acorde
    {
        Settings s; double L = 0;
        CHECK (buildLoop ({ { 0, Maj, 2 } }, s, L).size() > 0 && L == 2.0, "meio compasso sozinho");
        CHECK (buildLoop ({ { 0, Maj, 0 } }, s, L).empty() && L == 0.0, "sem acordes deveria dar loop vazio");
    }

    // agendamento como na DAW: blocos de 512 amostras, nenhuma nota presa
    for (int style = 0; style < NumStyles; ++style)
        for (int seed : { 1, 99 })
        {
            Settings s; s.style = style; s.seed = seed; s.swing = 0.6f; s.gate = 1.0f;
            double L = 0; const auto loop = buildLoop (slots, s, L);
            bool active[128] {}; long ons = 0, offs = 0, retrig = 0, maxPoly = 0;
            const double sr = 48000, bpm = 75; const int bs = 512; const double bb = bs / sr * bpm / 60.0;
            double ppq = 0;
            for (int blk = 0; blk < (int) (L * 4 / bb); ++blk)
            {
                const double end = ppq + bb;
                struct P { int s; bool on; int n; };
                std::vector<P> pend;
                auto collect = [&] (double t, bool on, int n)
                {
                    double occ = t + std::ceil ((ppq - t) / L) * L;
                    for (; occ < end; occ += L)
                        if (occ >= ppq) pend.push_back ({ (int) ((occ - ppq) / bb * bs), on, n });
                };
                for (const auto& e : loop) { collect (e.start, true, e.note); collect (e.start + e.length, false, e.note); }
                std::sort (pend.begin(), pend.end(), [] (const P& a, const P& b) { return a.s != b.s ? a.s < b.s : (int) a.on < (int) b.on; });
                for (const auto& p : pend)
                {
                    if (p.on) { if (active[p.n]) ++retrig; active[p.n] = true; ++ons; }
                    else if (active[p.n]) { active[p.n] = false; ++offs; }
                    long poly = 0; for (bool a : active) poly += a;
                    maxPoly = std::max (maxPoly, poly);
                }
                ppq = end;
            }
            CHECK (retrig == 0, "%s: nota religada sem note-off", styleName (style));
            CHECK (maxPoly <= 1, "%s: mais de uma nota ao mesmo tempo (%ld)", styleName (style), maxPoly);
            CHECK (std::abs (ons - offs) <= 1, "%s: note-ons %ld x note-offs %ld", styleName (style), ons, offs);
        }

    // acompanhamento: bateria nos 4 estilos e teclado nos 3 modos, em loops de vários tamanhos
    for (double L : { 2.0, 4.0, 16.0, 18.0, 34.0 })
        for (int ds = 0; ds < NumDrumStyles; ++ds)
            for (float sw : { 0.0f, 1.0f })
            {
                const auto d = buildDrums (L, ds, sw);
                CHECK (! d.empty(), "%s: bateria vazia", drumStyleName (ds));
                int kicks = 0;
                for (size_t i = 0; i < d.size(); ++i)
                {
                    CHECK (d[i].start >= 0 && d[i].start < L, "bateria fora do loop");
                    CHECK (i == 0 || d[i].start >= d[i - 1].start, "bateria fora de ordem");
                    CHECK (d[i].target == (isDigitalKit (ds) ? AccDigitalDrum : AccAcousticDrum), "kit errado");
                    kicks += d[i].note == drum::Kick;
                }
                CHECK (kicks >= 1 || (ds == DrumOneDrop && L < 4.0), "%s sem bumbo", drumStyleName (ds));
                for (int bar = 0; bar < (int) std::floor (L / 4.0 + 1e-9); ++bar)
                    for (double bt : { 1.0, 3.0 })
                    {
                        bool hit = false; int wrong = 0;
                        for (const auto& e : d)
                            if (e.note == drum::Snare || e.note == drum::Rim)
                            {
                                const double rel = e.start - bar * 4.0;
                                if (rel < -0.01 || rel >= 4.0 - 0.01) continue;
                                if (std::abs (rel - bt) < 0.05) hit = true;
                                if (e.note == drum::Snare && std::abs (rel - 1.0) > 0.05 && std::abs (rel - 3.0) > 0.05) ++wrong;
                            }
                        CHECK (hit, "%s: sem caixa no tempo %d", drumStyleName (ds), (int) bt + 1);
                        CHECK (wrong == 0, "%s: caixa fora do 2 e 4", drumStyleName (ds));
                    }
                if (ds == DrumOneDrop)
                    for (const auto& e : d) CHECK (! (e.note == drum::Kick && std::fmod (e.start, 4.0) < 1e-6), "one drop com bumbo no 1");
            }
    for (int km = 0; km < NumKeysModes; ++km)
    {
        const auto k = buildKeys (slots, km, 0.3f);
        bool piano = false, organ = false;
        for (const auto& e : k)
        {
            CHECK (e.note >= 36 && e.note <= 84, "teclado fora do registro (%d)", e.note);
            CHECK (e.start < 18.0, "teclado fora do loop");
            piano |= e.target == AccPiano; organ |= e.target == AccOrgan;
        }
        CHECK (piano == (km != KeysOrgan) && organ == (km != KeysPiano), "modo do teclado errado (%s)", keysModeName (km));
    }

    // RIDDIMS CLÁSSICOS: 20 por estilo, valores dentro dos parâmetros, linha gerada em cima da harmonia
    {
        using namespace mbmriddims;
        CHECK (numRiddims == numFolders * 20, "deveriam ser 20 riddims por estilo (%d)", numRiddims);
        int perFolder[numFolders] = {};
        for (int i = 0; i < numRiddims; ++i)
        {
            const auto& r = riddims[i];
            CHECK (r.folder >= 0 && r.folder < numFolders, "%s: pasta", r.name);
            if (r.folder >= 0 && r.folder < numFolders) ++perFolder[r.folder];
            CHECK (r.numChords >= 1 && r.numChords <= 4, "%s: acordes", r.name);
            CHECK (r.bpm >= 50 && r.bpm <= 100 && r.style >= 0 && r.style < NumStyles && r.drum >= 0 && r.drum < NumDrumStyles && r.timbre >= 0 && r.timbre < 3,
                   "%s: valores fora dos parametros", r.name);
            CHECK (r.year >= 1964 && r.year <= 2010, "%s: ano", r.name);
            std::vector<ChordSlot> sl;
            for (int c = 0; c < r.numChords; ++c)
            {
                CHECK (r.chords[c].root >= 0 && r.chords[c].root < 12 && r.chords[c].quality >= 0 && r.chords[c].quality < NumQualities
                       && r.chords[c].dur >= 0 && r.chords[c].dur < numDurations, "%s: acorde %d", r.name, c);
                sl.push_back ({ r.chords[c].root, r.chords[c].quality, durationBeats (r.chords[c].dur) });
            }
            Settings s; s.style = r.style; s.seed = 1; s.density = r.density; s.variation = r.variation; s.swing = r.swing;
            double L = 0;
            const auto ev = buildLoop (sl, s, L);
            CHECK (! ev.empty() && L >= 2.0, "%s: linha vazia", r.name);
            for (int j = 0; j < i; ++j) CHECK (std::string (riddims[j].name) != r.name, "%s repetido", r.name);
        }
        for (int f = 0; f < numFolders; ++f) CHECK (perFolder[f] == 20, "%s: %d riddims", folders[f], perFolder[f]);
        std::printf ("riddims classicos: %d em %d estilos OK\n", numRiddims, numFolders);
    }

    // edição: escala pentatônica maior/menor por acorde e encaixe na nota permitida mais próxima
    {
        const ChordSlot am { 9, Min, 2 }, c7 { 0, Dom7, 2 }, g { 7, Maj, 4 };
        CHECK (inPentatonic (57, am) && inPentatonic (60, am) && inPentatonic (62, am) && ! inPentatonic (59, am) && ! inPentatonic (61, am), "penta menor de A");
        CHECK (inPentatonic (64, c7) && inPentatonic (69, c7) && ! inPentatonic (70, c7) && ! inPentatonic (65, c7), "penta maior de C7");
        for (int n = 24; n <= 72; ++n)
            for (const auto& c : { am, c7, g })
            {
                const int sn = snapToPentatonic (n, c, 24, 72);
                CHECK (inPentatonic (sn, c) && std::abs (sn - n) <= 2, "encaixe na penta (%d -> %d)", n, sn);
            }
        const std::vector<ChordSlot> sl { am, { 2, Min, 0 }, c7, g };
        CHECK (chordAtBeat (sl, 0.0).root == 9 && chordAtBeat (sl, 2.5).root == 0 && chordAtBeat (sl, 7.9).root == 7 && chordAtBeat (sl, 99).root == 7, "acorde no tempo");
        std::printf ("edicao: so pentatonica do acorde OK\n");
    }

    std::printf (failures == 0 ? "\nTODOS OS TESTES PASSARAM\n" : "\n%d FALHAS\n", failures);
    return failures == 0 ? 0 : 1;
}
