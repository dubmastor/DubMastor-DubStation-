// Teste offline do motor do Mastor Dub Horns (sem JUCE):
//   g++ -std=c++17 -O2 -Wall -Wextra tests/engine_test.cpp -o engine_test && ./engine_test
// Verifica harmonia diatônica, sugestões de frase, estabilidade do som (sem NaN, sem estouro),
// a cauda do Mastor Tape Echo, a auto-oscilação limitada e que o eco não dispara sozinho.
// v1.2: o fall/doit some junto com a queda de afinação (nada de sirene) e o vibrato para ao soltar.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../Source/HornEngine.h"
#include "../Source/PhraseStyles.h"
#include "../Source/Transcriber.h"

using namespace horns;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { std::printf ("FALHOU: " __VA_ARGS__); std::printf ("\n"); ++failures; } else { std::printf ("ok: " __VA_ARGS__); std::printf ("\n"); } } while (0)

struct Stats { float peak = 0, rms = 0; bool finite = true; };

static Stats run (Engine& e, int samples, int block, std::vector<float>* rec = nullptr)
{
    std::vector<float> L ((size_t) block), R ((size_t) block);
    Stats st; double sum = 0;
    for (int pos = 0; pos < samples; pos += block)
    {
        const int n = std::min (block, samples - pos);
        e.render (L.data(), R.data(), n);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) st.finite = false;
            st.peak = std::max (st.peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i])));
            sum += 0.5 * (L[(size_t) i] * L[(size_t) i] + R[(size_t) i] * R[(size_t) i]);
            if (rec) { rec->push_back (L[(size_t) i]); rec->push_back (R[(size_t) i]); }
        }
    }
    st.rms = (float) std::sqrt (sum / std::max (1, samples));
    return st;
}

static void writeWav (const char* path, const std::vector<float>& lr, int sr)
{
    FILE* f = std::fopen (path, "wb"); if (! f) return;
    const uint32_t frames = (uint32_t) (lr.size() / 2), data = frames * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + data); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (data);
    for (float x : lr) { const int16_t s = (int16_t) std::lround (std::clamp (x, -1.0f, 1.0f) * 32767.0f); std::fwrite (&s, 2, 1, f); }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    // --- teoria
    CHECK (sectionNote (69, 2, 0, 0, 9, 0) == 65, "La menor: A4 com 3a abaixo vira F4 (%d)", sectionNote (69, 2, 0, 0, 9, 0));
    CHECK (diatonic (60, 2, 4, 0, 1) == 64, "Do maior: C4 + 3a acima = E4");
    CHECK (sectionNote (60, 3, 0, 4, 0, 1) == 53, "Do maior: C4 5a abaixo = F3");
    CHECK (degreeToMidi (4, 9, 0) == 76, "La menor: grau 4 (5a) = E5");
    CHECK (degreeToMidi (7, 9, 0) == 81, "La menor: grau 7 (oitava) = A5");
    CHECK (inScale (degreeToMidi (2, 9, 4), 9, 4), "pentatonica: sugestao encaixa na escala");
    CHECK (sectionNote (40, 0, -2, 0, 0, 1) >= instruments()[0].lo, "trompete nunca sai da extensao");
    {
        int total = 0; bool okLen = true;
        for (const auto* lib : { &stabPatterns(), &introPatterns() })
            for (const auto& p : *lib)
            {
                ++total;
                for (const auto& e : p.ev) okLen = okLen && e[0] >= 0 && e[0] + e[1] <= p.len && (int) p.ev.size() <= kMaxEvents;
                const auto ph = makePhrase (p, false, 9, 0);
                for (const auto& ev : ph.events) okLen = okLen && ev.note == -1;
            }
        CHECK (total == 14 && okLen, "14 sugestoes, eventos dentro do compasso e 'so ritmo' deixa as notas em aberto");
    }


    // --- v1.2: final natural. Sax tenor sozinho, seco: o fall cai JUNTO com o volume
    {
        const int sr = 48000;
        Engine e; Params p;
        for (auto& i : p.inst) i.on = false;
        p.inst[3].on = true; p.inst[3].oct = 0; p.echoOn = false; p.hiss = 0; p.room = 0;
        e.prepare (sr, 256); e.setParams (p);
        e.noteOn (69, 0.9f, Fall, 1);
        std::vector<float> rec; run (e, sr / 2, 64, &rec);
        auto rmsOf = [] (const std::vector<float>& lr, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += lr[2 * i] * lr[2 * i]; return std::sqrt (s / (double) std::max<size_t> (1, b - a)); };
        const double sus = rmsOf (rec, (size_t) (0.35 * sr), (size_t) (0.5 * sr));
        e.noteOff (69, 1);
        std::vector<float> rel; run (e, sr, 64, &rel);
        const double late = rmsOf (rel, (size_t) (0.25 * sr), (size_t) (0.35 * sr));
        CHECK (20 * std::log10 (late / sus) < -10.0, "fall: depois de 250 ms o volume ja caiu junto com a afinacao (%.1f dB)", 20 * std::log10 (late / sus));
        CHECK (e.activeVoices() == 0, "fall: a voz termina sozinha, sem cauda de sirene");
        const double early = rmsOf (rel, (size_t) (0.0 * sr), (size_t) (0.03 * sr));
        CHECK (20 * std::log10 (early / sus) > -4.0, "fall: segura a nota um instante antes de cair (%.1f dB)", 20 * std::log10 (early / sus));
    }


    // --- v1.3: dinâmica de metal real. Trompete: no pp o 10º harmônico fica bem abaixo do 1º; no ff ele cresce
    {
        auto tenth = [] (float vel)
        {
            const int sr = 48000; Engine e; Params p;
            for (auto& i : p.inst) i.on = false;
            p.inst[0].on = true; p.inst[0].oct = 0; p.echoOn = false; p.hiss = 0; p.drive = 0; p.band = 18000; p.lowcut = 20;
            p.human = 0; p.vib = 0; p.breath = 0; p.room = 0;
            e.prepare (sr, 256); e.setParams (p); e.noteOn (62, vel, Ten, 1);
            std::vector<float> rec; run (e, sr, 256, &rec);
            const double f0 = 440.0 * std::pow (2.0, (62 - 69) / 12.0);
            auto mag = [&] (double f) { double re = 0, im = 0; const int a = sr / 2, n = sr / 4;
                for (int i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * 3.14159265 * i / n), x = rec[2 * (size_t) (a + i)] * w;
                    re += x * std::cos (2 * 3.14159265 * f * i / sr); im += x * std::sin (2 * 3.14159265 * f * i / sr); }
                return std::sqrt (re * re + im * im); };
            return 20.0 * std::log10 (mag (10 * f0) / mag (f0));
        };
        const double pp = tenth (0.25f), ff = tenth (1.0f);
        CHECK (ff - pp > 15.0, "trompete: ff tem bem mais harmonicos agudos que pp (10a-1a: pp %.1f dB, ff %.1f dB)", pp, ff);
    }


    // --- v1.4: biblioteca por estilo e transcrição de áudio
    {
        int n = 0;
        for (const auto& st : phraseStyles())
            for (const auto& pd : st.pats)
            {
                ++n;
                bool ok = (pd.len == 32 || pd.len == 64) && ! pd.ev.empty() && (int) pd.ev.size() <= kMaxEvents;
                for (const auto& e : pd.ev) ok = ok && e[0] >= 0 && e[0] < pd.len && e[1] >= 1 && e[3] >= 0 && e[3] < kNumArtic;
                CHECK (ok, "frase '%s' (%s) dentro do formato", pd.name, st.name);
            }
        CHECK (phraseStyles().size() == 8 && n >= 40, "biblioteca: %zu estilos e %d frases", phraseStyles().size(), n);

        // o trompete toca uma frase conhecida; a transcrição tem que achar as notas
        const int sr = 44100; const double bpm = 74, s16 = 60.0 / bpm / 4.0;
        const auto ph = makePhrase (phraseStyles()[2].pats[1], true, 9, 1);
        Engine e; Params p; for (auto& i : p.inst) i.on = false;
        p.inst[0].on = true; p.inst[0].oct = 0; p.echoOn = false; p.room = 0.15f; p.hiss = 0.1f;
        e.prepare (sr, 256); e.setParams (p);
        const int N = (int) ((0.5 + ph.length * s16 + 1.5) * sr);
        std::vector<float> mono ((size_t) N);
        for (int i = 0; i < N; ++i)
        {
            const double t = i / (double) sr - 0.5;
            for (const auto& ev : ph.events)
            {
                if (std::abs (t - ev.start * s16) < 0.5 / sr) e.noteOn (ev.note, 0.85f, ev.artic, 1);
                if (std::abs (t - (ev.start + ev.dur * (ev.artic == Stac ? 0.5 : 0.9)) * s16) < 0.5 / sr) e.noteOff (ev.note, 1);
            }
            float l, r; e.render (&l, &r, 1); mono[(size_t) i] = 0.5f * (l + r);
        }
        const auto tr = transcribe (mono, sr, bpm, false);
        int best = 0;
        for (int sh = -4; sh <= 4; ++sh)
        {
            int ok = 0;
            for (const auto& ev : ph.events)
                for (const auto& t : tr.phrase.events) if (std::abs (t.start - ev.start - sh) <= 1 && t.note == ev.note) { ++ok; break; }
            best = std::max (best, ok);
        }
        CHECK (best * 10 >= (int) ph.events.size() * 8, "transcricao: %d de %zu notas certas", best, ph.events.size());
    }

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        Engine e;
        e.prepare (sr, 512);
        Params p;
        for (auto& i : p.inst) i.on = true;                    // naipe cheio
        p.inst[2].harm = 5;
        e.setParams (p);

        std::vector<float> rec;
        const int S = (int) sr;
        // pontadas + nota longa com fall
        e.noteOn (69, 0.9f, Stac, 1); auto a = run (e, S / 8, 97, &rec); e.noteOff (69, 1);
        auto b = run (e, S / 4, 128, &rec);
        e.noteOn (72, 1.0f, Scoop, 1); e.noteOn (76, 0.8f, Ten, 0);
        auto c = run (e, S, 1, &rec);                          // bloco de 1 amostra
        e.noteOff (72, 1); e.noteOff (76, 0);
        e.noteOn (81, 0.9f, Fall, 1); run (e, S / 2, 333, &rec); e.noteOff (81, 1);
        e.noteOn (64, 0.9f, Doit, 1); run (e, S / 2, 512, &rec); e.noteOff (64, 1);
        auto tail = run (e, S, 256, &rec);
        CHECK (a.finite && b.finite && c.finite && tail.finite, "%.0f Hz: sem NaN/inf", sr);
        CHECK (c.peak <= 1.0f && c.rms > 0.02f, "%.0f Hz: naipe soa (rms %.3f) sem estourar (pico %.3f)", sr, c.rms, c.peak);
        CHECK (tail.rms > 0.002f, "%.0f Hz: cauda do Tape Echo depois das notas (rms %.4f)", sr, tail.rms);
        CHECK (e.activeVoices() == 0, "%.0f Hz: todas as vozes terminaram (%d)", sr, e.activeVoices());
        if (sr == 48000.0 && argc > 1) writeWav (argv[1], rec, (int) sr);

        // o eco precisa morrer sozinho com intensidade 0.45 (nada de realimentação disparando)
        run (e, S * 15, 512);
        auto quiet = run (e, S * 5, 512);
        CHECK (quiet.rms < 5.0e-4f, "%.0f Hz: eco decai em silencio, so fica o chiado (rms %.6f)", sr, quiet.rms);

        // throw + auto-oscilação: fica limitado
        p.throwOn = true; p.selfOsc = true; p.head = { true, true, true }; p.sat = 1.0f; e.setParams (p);
        e.noteOn (60, 1.0f, Ten, 1); run (e, S / 4, 64); e.noteOff (60, 1);
        auto osc = run (e, S * 4, 512);
        CHECK (osc.finite && osc.peak <= 1.0f && osc.rms > 0.01f, "%.0f Hz: auto-oscilacao soa e fica limitada (pico %.3f rms %.3f)", sr, osc.peak, osc.rms);
        p.selfOsc = false; p.throwOn = false; p.fb = 0.3f; p.echoOn = false; e.setParams (p);
        run (e, S / 2, 512);
        auto off = run (e, S * 3, 512);
        CHECK (off.finite && off.peak < 0.01f, "%.0f Hz: eco desligado silencia (pico %.4f)", sr, off.peak);

        // parâmetros extremos
        Params x; x.bright = 1; x.breath = 1; x.vib = 1; x.scoop = 1; x.attack = 0.3f; x.release = 0; x.drive = 1;
        x.band = 18000; x.lowcut = 500; x.hiss = 1; x.human = 1; x.width = 1; x.fb = 0.95f; x.sat = 0; x.tubby = 2000; x.tubbyRes = 1;
        for (auto& g : x.eq) g = 12;
        x.sync = 0; x.echoMs = 40; for (auto& i : x.inst) { i.on = true; i.oct = 1; i.harm = 1; }
        e.setParams (x);
        for (int n = 40; n < 90; n += 3) e.noteOn (n, 1.0f, n % 5, 0);
        auto ext = run (e, S * 2, 480);
        e.allNotesOff();
        auto ext2 = run (e, S * 2, 480);
        CHECK (ext.finite && ext2.finite && ext.peak <= 1.0f && ext2.peak <= 1.0f, "%.0f Hz: parametros extremos estaveis (pico %.3f)", sr, ext.peak);
    }

    std::printf (failures ? "\n%d FALHA(S)\n" : "\nTUDO OK\n", failures);
    return failures ? 1 : 0;
}
