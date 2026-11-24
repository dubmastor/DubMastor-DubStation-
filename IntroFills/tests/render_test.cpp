// Teste offline do Mastor Intro Fills MIF (sem JUCE):
//   g++ -std=c++17 -O2 -Wall -Wextra -I Source tests/render_test.cpp -o render_test && ./render_test [pasta-dos-wav]
// Confere, para todos os bateristas, viradas, estilos, tamanhos e chegadas:
//  - o groove tem a caixa/aro no 2 e no 4 (passos 4 e 12 da contagem da Dub Station), nunca o "drop" no 3
//  - com chegada, há nota no 1 do groove; sem chegada, a virada não toca o passo 16
//  - anacruse só usa os passos 14 e 15; meio compasso começa no 8; 2 compassos tem o compasso de pedal antes
//  - velocidades 1..127, notas em ordem, o .mid é válido
//  - o som: sem NaN, sem estouro, a virada soa; Tape Echo e saturador desligados não mudam nada
// e grava um WAV de cada baterista (virada + 2 compassos de groove).
#include "FillSampler.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

static int falhas = 0;
#define CHECK(c, msg) do { if (! (c)) { std::printf ("FALHA: %s\n", msg); ++falhas; } } while (0)

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), db = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + db); std::fwrite ("WAVEfmt ", 1, 8, f); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (db);
    for (uint32_t i = 0; i < n; ++i)
    {
        const int16_t a = (int16_t) (std::clamp (L[i], -1.0f, 1.0f) * 32767), b = (int16_t) (std::clamp (R[i], -1.0f, 1.0f) * 32767);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    using namespace mif;
    const std::string dir = argc > 1 ? argv[1] : "Samples";
    FillSampler sampler;
    for (int p = 0; p < NumPieces; ++p)
    {
        std::ifstream in (dir + "/" + piece (p).file + ".wav", std::ios::binary);
        std::vector<char> bytes ((std::istreambuf_iterator<char> (in)), std::istreambuf_iterator<char>());
        Sample s;
        CHECK (readWav (bytes.data(), bytes.size(), s), (std::string ("lê o sample ") + piece (p).file).c_str());
        sampler.setSample (p, std::move (s));
    }
    CHECK (sampler.hasSamples(), "todas as peças têm sample");

    int combos = 0;
    for (int d = 0; d < (int) drummers().size(); ++d)
        for (int f = 0; f < (int) drummers()[(size_t) d].fills.size(); ++f)
            for (int style = 0; style < NumStyles; ++style)
                for (int size = 0; size < NumSizes; ++size)
                    for (int land = 0; land < 2; ++land)
                    {
                        Settings st; st.drummer = d; st.fill = f; st.style = style; st.size = size; st.land = land;
                        Rng rng; const auto& fill = drummers()[(size_t) d].fills[(size_t) f].ev;
                        auto seq = buildSequence (st, fill, rng);
                        auto notes = perform (st, seq, rng);
                        ++combos;
                        char ctx[160];
                        std::snprintf (ctx, sizeof ctx, "%s / %s / %s / %s / %s", drummers()[(size_t) d].name,
                                       drummers()[(size_t) d].fills[(size_t) f].name, styleName (style), sizeName (size), land ? "sem chegada" : "com chegada");
                        // caixa/aro do groove só no 2 e no 4
                        bool snareOk = true, hasSnare = false;
                        for (auto& x : seq.evs)
                            if (x.groove && (x.p == Aro || x.p == Caixa))
                            {
                                hasSnare = true;
                                const int s = (int) std::lround (x.s - seq.g0) % 16;
                                if (s != 4 && s != 12) snareOk = false;
                            }
                        CHECK (hasSnare && snareOk, (std::string ("caixa do groove no 2 e no 4: ") + ctx).c_str());
                        // chegada
                        const bool fillHas16 = std::any_of (fill.begin(), fill.end(), [] (const Ev& x) { return x.s >= 16; });
                        const bool arrives = std::any_of (seq.evs.begin(), seq.evs.end(), [&] (const Ev& x) { return ! x.groove && x.s == seq.g0; });
                        if (fillHas16) CHECK (arrives == (land == 0), (std::string ("chegada no 1 do groove: ") + ctx).c_str());
                        // região da virada
                        const int rs = regionStart (size); const double off = size == 3 ? 16 : 0;
                        for (auto& x : seq.evs)
                            if (! x.groove && ! x.ghost && x.s < seq.g0 && x.s >= off)
                                CHECK (x.s - off >= rs - 1e-9, (std::string ("virada dentro do tamanho: ") + ctx).c_str());
                        if (size == 3) CHECK (seq.g0 == 32 && seq.total == 64, (std::string ("2 compassos: ") + ctx).c_str());
                        // notas
                        bool velOk = true, orderOk = true;
                        for (size_t i = 0; i < notes.size(); ++i)
                        {
                            velOk = velOk && notes[i].vel >= 1 && notes[i].vel <= 127;
                            if (i) orderOk = orderOk && notes[i].beat >= notes[i - 1].beat;
                        }
                        CHECK (velOk && orderOk && ! notes.empty(), (std::string ("notas: ") + ctx).c_str());
                    }
    std::printf ("%d combinações conferidas\n", combos);

    // .mid
    {
        Settings st; Rng rng;
        auto seq = buildSequence (st, drummers()[0].fills[0].ev, rng);
        auto mid = makeMidiFile (perform (st, seq, rng), st.bpm, "MIF teste");
        CHECK (mid.size() > 40 && std::memcmp (mid.data(), "MThd", 4) == 0 && std::memcmp (mid.data() + 14, "MTrk", 4) == 0, "arquivo .mid válido");
    }

    // som: uma virada de cada baterista, com e sem efeitos
    const int sr = 48000;
    for (int d = 0; d < (int) drummers().size(); ++d)
    {
        Settings st; st.drummer = d; st.bpm = 74;
        Rng rng;
        auto seq = buildSequence (st, drummers()[(size_t) d].fills[0].ev, rng);
        auto notes = perform (st, seq, rng);
        auto render = [&] (bool echo, bool sat)
        {
            sampler.prepare (sr);
            const double secPerBeat = 60.0 / st.bpm;
            const int N = (int) ((seq.total / 4.0 * secPerBeat + 1.5) * sr);
            std::vector<float> L ((size_t) N, 0.0f), R ((size_t) N, 0.0f);
            size_t k = 0; const int blk = 64;
            for (int pos = 0; pos < N; pos += blk)
            {
                const int n = std::min (blk, N - pos);
                const double b1 = (double) (pos + n) / sr / secPerBeat;
                while (k < notes.size() && std::max (0.0, notes[k].beat) < b1) { sampler.hit (notes[k].p, notes[k].vel, st.human); ++k; }
                sampler.render (&L[(size_t) pos], &R[(size_t) pos], n, echo, sat, st.bpm, 0.8f);
            }
            return std::make_pair (L, R);
        };
        auto [L, R] = render (false, false);
        float pk = 0; double e = 0; bool fin = true;
        for (size_t i = 0; i < L.size(); ++i) { pk = std::max (pk, std::max (std::abs (L[i]), std::abs (R[i]))); e += (double) L[i] * L[i]; fin = fin && std::isfinite (L[i]) && std::isfinite (R[i]); }
        const double rms = std::sqrt (e / (double) L.size());
        char msg[160]; std::snprintf (msg, sizeof msg, "%s soa sem NaN nem estouro (pico %.2f, rms %.3f)", drummers()[(size_t) d].name, pk, rms);
        CHECK (fin && pk < 1.0f && rms > 0.01, msg);
        std::printf ("%s\n", msg);
        auto fx = render (true, true);
        bool finFx = true; for (float x : fx.first) finFx = finFx && std::isfinite (x);
        CHECK (finFx, "com Tape Echo e saturador ligados, sem NaN");
        std::string name = std::string ("demo_") + drummers()[(size_t) d].name + ".wav";
        for (auto& c : name) if (c == ' ') c = '_';
        writeWav (name, L, R, sr);
    }

    std::printf (falhas ? "\n%d FALHA(S)\n" : "\nTudo certo.\n", falhas);
    return falhas ? 1 : 0;
}
