// Teste offline do Mastor Orch Hit MOH-88 (sem JUCE):
//   g++ -std=c++17 -O2 -Wall -Wextra -I Source tests/render_test.cpp -o render_test && ./render_test
// Confere os 7 timbres (tamanho, normalização, bits do sampler), o sampler (transposição, duas mãos, tamanho da nota,
// nenhuma nota presa) e os grooves/MIDIs na contagem com a caixa no 2 e no 4.
#include <chrono>
#include <cstdio>
#include <set>
#include "HitEngine.h"

static int fails = 0;
static void check (bool c, const char* msg) { std::printf ("%s %s\n", c ? "ok    " : "FALHOU", msg); if (! c) ++fails; }

int main()
{
    using namespace moh;
    char m[200];
    std::vector<std::vector<float>> samples;
    for (const auto& p : presets())
    {
        const auto t0 = std::chrono::steady_clock::now();
        auto s = renderHit (p);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        float pk = 0; double rms = 0; bool finite = true;
        for (float v : s) { pk = std::max (pk, std::abs (v)); rms += v * v; finite = finite && std::isfinite (v); }
        rms = std::sqrt (rms / (double) s.size());
        const int expect = (int) (p.cut * kSampleRate) + (int) (0.08 * kSampleRate);
        std::snprintf (m, sizeof m, "%-8s %4zu ms de sample, pico %.2f, RMS %.3f (%.0f ms para gerar)", p.name, s.size() * 1000 / 44100, pk, rms, ms);
        check (finite && (int) s.size() == expect && pk > 0.99f && pk <= 1.0001f && rms > 0.05, m);
        // o fim do sample morre (sem clique)
        float tail = 0; for (size_t i = s.size() - 200; i < s.size(); ++i) tail = std::max (tail, std::abs (s[i]));
        std::snprintf (m, sizeof m, "%-8s fim do sample sem clique (%.4f)", p.name, tail);
        check (tail < 0.05f, m);
        if (p.bits == 8)
        {   // ORCH5: antes do último filtro só cabem 256 níveis; depois dele o som ainda é "degrau" (sample-and-hold)
            int steps = 0; for (size_t i = 1; i < s.size(); ++i) steps += s[i] != s[i - 1];
            check (steps > 1000, "ORCH5: sampler de 8 bits gerou o sample");
        }
        samples.push_back (std::move (s));
    }

    // ---- sampler
    const double sr = 48000;
    Sampler smp; smp.prepare (sr); smp.setSample (&samples[0]);
    auto renderLen = [&] (int note, bool hands, int64_t offAfter, double secs, double* rmsOut = nullptr)
    {
        smp.allOff(); smp.twoHands = hands; smp.noteOn (note, 1.0f, offAfter);
        std::vector<float> b (512);
        int64_t last = 0, n = 0; double acc = 0;
        for (int64_t pos = 0; pos < (int64_t) (secs * sr); pos += 512)
        {
            std::fill (b.begin(), b.end(), 0.0f); smp.render (b.data(), 512);
            for (int i = 0; i < 512; ++i) { if (std::abs (b[(size_t) i]) > 1e-4f) last = pos + i; acc += b[(size_t) i] * b[(size_t) i]; ++n; }
        }
        if (rmsOut) *rmsOut = std::sqrt (acc / (double) n);
        return last / sr;
    };
    smp.env = { .002, 1.0, 1.0, .01 };   // envelope aberto: dura o sample inteiro
    const double lenRoot = renderLen (kRootNote, false, -1, 2), lenLow = renderLen (kRootNote - 12, false, -1, 2), lenHigh = renderLen (kRootNote + 12, false, -1, 2);
    std::snprintf (m, sizeof m, "sampler antigo: grave longo, agudo curto (%.2f s / %.2f s / %.2f s)", lenLow, lenRoot, lenHigh);
    check (std::abs (lenLow / lenRoot - 2) < 0.1 && std::abs (lenRoot / lenHigh - 2) < 0.1, m);
    double r1 = 0, r2 = 0; renderLen (38, false, -1, 0.3, &r1); renderLen (38, true, -1, 0.3, &r2);
    std::snprintf (m, sizeof m, "DUAS MÃOS soma a oitava de cima (RMS %.3f -> %.3f)", r1, r2); check (r2 > r1 * 1.05, m);

    // tamanho da nota: 1/64 a 84 BPM = 60/84/16 s
    smp.env = { .002, .25, .6, .06 };
    const int64_t tick = (int64_t) (60.0 / 84 / 16 * sr);
    const double l64 = renderLen (38, true, lengthTicks (0) * tick, 2), l4 = renderLen (38, true, lengthTicks (4) * tick, 2);
    std::snprintf (m, sizeof m, "TAMANHO DA NOTA: 1/64 solta em %.3f s, 1/4 em %.3f s", l64, l4);
    check (l64 < 0.15 && l4 > 0.4 && l64 < l4, m);
    check (smp.activeVoices() == 0, "nenhuma nota presa depois do tamanho da nota");
    smp.allOff(); smp.noteOn (40, 1, -1); smp.noteOff (40);
    { std::vector<float> b (48000); smp.render (b.data(), 48000); }
    check (smp.activeVoices() == 0, "LIVRE: solta com o note off");

    // ---- grooves e MIDIs na contagem (caixa no 2 = tick 16, no 4 = tick 48)
    const auto g0 = grooveGrid (0);
    bool on24 = g0.size() == 8;
    for (auto& n : g0) on24 = on24 && (n.tick % 64 == 16 || n.tick % 64 == 48);
    check (on24, "groove PONTUAÇÃO: 8 hits, todos na caixa (2 e 4)");
    std::set<int> notes; for (auto& n : g0) notes.insert (n.note);
    check (notes == std::set<int> { 34, 36, 38 }, "progressão Ré – Ré – Si♭ – Dó");
    bool inside = true;
    for (int i = 0; i < (int) grooves().size(); ++i) for (auto& n : grooveGrid (i)) inside = inside && n.tick >= 0 && n.tick < kTotalTicks && n.len == 1;
    check (inside, "todos os grooves cabem nos 4 compassos, notas de 1/64");
    Grid g; placeClip (g, clips()[1].notes, 64, 40);
    check (g.size() == 5 && g[0].tick == 80 && g[0].note == 40 && g[3].tick == 172, "MIDI arrastado: começa no compasso certo e na nota base");
    Grid cut; placeClip (cut, clips()[3].notes, 192, 38);
    check (cut.size() == 2, "MIDI que passa do fim da grade é cortado");

    // ---- bateria do preview
    PreviewDrums dr; dr.prepare (sr);
    std::vector<float> b (4800, 0.0f); dr.trigger (16); dr.render (b.data(), 4800);
    float pk = 0; for (float v : b) pk = std::max (pk, std::abs (v));
    check (pk > 0.2f && pk < 2.0f, "bateria do PREVIEW (caixa no 2)");

    std::printf (fails == 0 ? "\nTUDO OK\n" : "\n%d FALHA(S)\n", fails);
    return fails == 0 ? 0 : 1;
}
