// Teste offline do Mastor Pick Guitar MPG-11 (sem JUCE):
//   g++ -std=c++17 -O2 -Wall -Wextra -I Source -I ../HalfDub/Source tests/render_test.cpp -o render_test && ./render_test
// Confere o padrão de fábrica, a contagem (grade de 16 semicolcheias = 1 compasso), os três modos, o pica-pau
// (dose, edições, repetições), o MIDI do baixo, o som (sem NaN, sem estouro) e grava um WAV por guitarrista em /tmp.
#include "PickEngine.h"
#include <cstdio>
#include <string>
#include <fstream>

using namespace mpg;
static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { ++fails; std::printf ("FALHOU: "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    std::ofstream f (path, std::ios::binary);
    auto w32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto w16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    const uint32_t n = (uint32_t) L.size();
    f.write ("RIFF", 4); w32 (36 + n * 4); f.write ("WAVEfmt ", 8); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    f.write ("data", 4); w32 (n * 4);
    for (uint32_t i = 0; i < n; ++i) { w16 ((uint16_t) (int16_t) std::lround (L[i] * 32000)); w16 ((uint16_t) (int16_t) std::lround (R[i] * 32000)); }
}

static double gEnd = 1e9;
struct Render { std::vector<float> L, R; std::vector<NoteLog> log; float peak = 0; bool finite = true; };

static Render render (const Params& p, int bars, double bpm = 76, int sr = 48000, std::vector<MidiIn> firstMidi = {})
{
    Render r;
    gEnd = bars * 4.0;
    Engine e; e.log = &r.log;
    e.prepare (sr); e.setParams (p);
    const int block = 480;
    const double totalBeats = bars * 4.0;
    const int total = (int) (totalBeats * 60.0 / bpm * sr);
    std::vector<float> l (block), rr (block);
    double ppq = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        std::fill (l.begin(), l.end(), 0.0f); std::fill (rr.begin(), rr.end(), 0.0f);
        Transport t; t.playing = true; t.ppq = ppq; t.bpm = bpm;
        std::vector<MidiIn> midi;
        if (pos == 0) midi = firstMidi;
        e.process (l.data(), rr.data(), block, t, midi);
        for (int i = 0; i < block; ++i)
        {
            if (! std::isfinite (l[(size_t) i]) || ! std::isfinite (rr[(size_t) i])) r.finite = false;
            r.peak = std::max (r.peak, std::max (std::abs (l[(size_t) i]), std::abs (rr[(size_t) i])));
            r.L.push_back (l[(size_t) i]); r.R.push_back (rr[(size_t) i]);
        }
        ppq += block / (double) sr * bpm / 60.0;
    }
    return r;
}

static int picks (const Render& r) { int n = 0; for (auto& x : r.log) if (! x.bass && x.beat < gEnd - 1e-9) ++n; return n; }

int main()
{
    // 1) padrão de fábrica
    {
        Params p;
        CHECK (p.mute == 100 && p.pos == 7 && p.atk == 100 && p.drive == 0 && p.tone == 62 && p.body == 96 && p.pickup == Ponte, "timbre de fábrica");
        CHECK (p.gate == 8 && p.swing == 10 && p.ppSwing == 10 && p.rep == 3 && p.dose == 5, "duração 8, swing 10, pica-pau em tercina, dose 5");
        CHECK (! p.echoOn && ! p.satOn, "Tape Echo e saturador começam em bypass");
        CHECK (p.eqOn && p.bk == 0, "EQ ligado (Pick clássico) e Big Knob neutro");
        CHECK (std::string (p.grid.data(), 16) == presets()[kDefaultPreset].grid, "grade inicial do Hux Brown");
    }

    // 2) contagem: a grade é de semicolcheias (0,25 tempo), 16 por compasso
    {
        Params p; applyPreset (p, 6);   // Bingy Bunny & Tony Chin: as 16 semicolcheias cheias
        p.source = SrcGuide; p.human = 0; p.swing = 0; p.art = Mantem; p.guideBass = false;
        auto r = render (p, 2);
        CHECK (picks (r) == 32, "Groove com grade cheia deu %d palhetadas em 2 compassos (esperado 32)", picks (r));
        bool grid = true;
        for (auto& n : r.log) { const double s = n.beat / kStepBeats; if (std::abs (s - std::round (s)) > 1e-6) grid = false; }
        CHECK (grid, "palhetadas fora das semicolcheias");
    }

    // 3) pica-pau: dose de 0 a 10% em 8 compassos, edições e repetições
    {
        Params p; // Hux Brown: 10 notas abafadas na grade -> 80 no ciclo de 8 compassos
        std::array<bool, kCycle> a {};
        auto count = [&] (float d) { autoPicaPau (p.grid, d, a); int n = 0; for (bool b : a) n += b; return n; };
        CHECK (count (0) == 0, "dose 0 deveria dar 0");
        CHECK (count (0.5f) == 1, "dose 0,5%% deveria dar 1 (deu %d)", count (0.5f));
        CHECK (count (5) == 4, "dose 5%% deveria dar 4 (deu %d)", count (5));
        CHECK (count (10) == 8, "dose 10%% deveria dar 8 (deu %d)", count (10));
        // nem todos na mesma nota do compasso
        autoPicaPau (p.grid, 10, a);
        int firstCol = -1; bool varied = false;
        for (int i = 0; i < kCycle; ++i) if (a[(size_t) i]) { if (firstCol < 0) firstCol = i % 16; else if (i % 16 != firstCol) varied = true; }
        CHECK (varied, "os pica-paus deveriam mudar de lugar no compasso");
        // edições: tirar um automático e pôr um manual
        int on = -1; for (int i = 0; i < kCycle; ++i) if (a[(size_t) i]) { on = i; break; }
        p.dose = 10; p.edits[(size_t) on] = 2;
        CHECK (! isPicaPau (p, a, on), "edição 'tirar' não funcionou");
        p.edits[2] = 1;
        CHECK (isPicaPau (p, a, 2), "edição 'pôr' não funcionou");
        p.art = Mantem;
        CHECK (isPicaPau (p, a, 2) && ! isPicaPau (p, a, on), "no Mantém só as edições manuais repetem");
        // pausa e nota aberta não recebem pica-pau
        p.edits[1] = 1; p.edits[15] = 1;
        CHECK (! isPicaPau (p, a, 1) && ! isPicaPau (p, a, 15), "pausa e nota aberta não podem ter pica-pau");

        // repetições: 3 palhetadas na semicolcheia
        Params q; q.source = SrcGuide; q.human = 0; q.swing = 0; q.ppSwing = 0; q.guideBass = false; q.art = Mantem;
        q.setGrid ("X..............."); q.edits.fill (0); q.edits[0] = 1;
        auto r = render (q, 1);
        CHECK (picks (r) == 3, "pica-pau em tercina deveria dar 3 palhetadas (deu %d)", picks (r));
        if (picks (r) == 3) CHECK (std::abs (r.log[1].beat - kStepBeats / 3) < 1e-6, "tercina fora do lugar");
    }

    // 4) FOLLOW: a nota do baixo pelo MIDI vira palhetada uma oitava acima
    {
        Params p; p.mode = Follow; p.source = SrcMidi; p.human = 0;
        auto r = render (p, 1, 76, 48000, { { 10, 45, 1.0f, true } });
        CHECK (picks (r) == 1 && r.log[0].midi == 57, "Follow pelo MIDI: %d palhetadas", picks (r));
    }

    // 5) GROOVE pelo MIDI: segura a nota do baixo e toca a grade
    {
        Params p; p.source = SrcMidi; p.art = Mantem; p.human = 0;
        auto none = render (p, 1);
        CHECK (picks (none) == 0, "sem nota do baixo o Groove não toca");
        auto r = render (p, 1, 76, 48000, { { 0, 45, 1.0f, true } });
        int want = 0; for (char c : p.grid) want += c != '.';
        CHECK (picks (r) >= want - 1, "Groove pelo MIDI deu %d palhetadas (esperado ~%d)", picks (r), want);
    }

    // 6) COUNTER: as notas mudam conforme o desenho
    {
        Params p; applyPreset (p, 4); p.source = SrcGuide; p.art = Mantem;
        auto r = render (p, 2);
        std::array<bool, 128> seen {}; int distinct = 0;
        for (auto& n : r.log) if (! n.bass && ! seen[(size_t) n.midi]) { seen[(size_t) n.midi] = true; ++distinct; }
        CHECK (distinct >= 4, "Counter com poucas notas diferentes (%d)", distinct);
    }

    // 7) todos os guitarristas: som limpo, sem estouro; WAV em /tmp; com eco, mola, saturador e Big Knob ligados também
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        Params p; applyPreset (p, i); p.source = SrcGuide;
        auto r = render (p, 8);
        CHECK (r.finite && r.peak > 0.02f && r.peak < 0.99f, "%s: pico %.3f", presets()[(size_t) i].name, r.peak);
        CHECK (picks (r) > 16, "%s: só %d palhetadas em 8 compassos", presets()[(size_t) i].name, picks (r));
        std::string name = "/tmp/mpg11_" + std::to_string (i) + ".wav";
        writeWav (name, r.L, r.R, 48000);
        std::printf ("%-26s %3d palhetadas · pico %.2f · WAV %s\n", presets()[(size_t) i].name, picks (r), r.peak, name.c_str());
        p.echoOn = true; p.satOn = true; p.satType = i % 3; p.bk = (i % 2) ? 60.0f : -60.0f; p.echoMix = 40; p.spring = 60;
        auto fx = render (p, 4, 90, 44100);
        CHECK (fx.finite && fx.peak <= 1.0f && fx.peak > 0.01f, "%s com efeitos: pico %.3f", presets()[(size_t) i].name, fx.peak);
    }

    // 8) curva do EQ: Pick clássico corta o grave
    {
        Params p;
        CHECK (responseDb (p, 50) < -6 && std::abs (responseDb (p, 1000)) < 6, "curva do EQ Pick clássico");
        p.bk = -100; CHECK (responseDb (p, 5000) < -20, "Big Knob à esquerda deveria fechar o agudo");
    }

    std::printf ("\n%s (%d falha(s))\n", fails ? "TESTE FALHOU" : "TESTE OK", fails);
    return fails ? 1 : 0;
}
