// Teste offline do Mastor Reggae Clavinet MRCL-71 (sem JUCE):
//   g++ -std=c++17 -O2 -I Source tests/render_test.cpp -o /tmp/mrcl_test && /tmp/mrcl_test [pasta para os WAVs]
// Confere: afinação das cordas, sem estouro, sem nota presa, as linhas na contagem (caixa no 2 e no 4), a oitava e o MIDI.
#include "ClavEngine.h"
#include "LineEngine.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <map>

static int fails = 0;
#define CHECK(c, msg) do { if (c) std::printf ("ok     %s\n", msg); else { std::printf ("FALHOU %s\n", msg); ++fails; } } while (0)

static void writeWav (const std::string& path, const std::vector<float>& x, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const int n = (int) x.size(), bytes = n * 2;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f); w32 (16); w16 (1); w16 (1); w32 (sr); w32 (sr * 2); w16 (2); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (float v : x) { const int16_t s = (int16_t) std::lround (mrcl::clampf (v, -1, 1) * 32767); std::fwrite (&s, 2, 1, f); }
    std::fclose (f);
}

// frequência pela autocorrelação (afinação da corda)
static double pitchOf (const std::vector<float>& x, int sr, double guess)
{
    const int a = (int) (sr / (guess * 1.06)), b = (int) (sr / (guess / 1.06)), n = std::min ((int) x.size() - b - 1, sr / 2);
    double best = -1e9; int lag = a;
    for (int l = a; l <= b; ++l) { double s = 0; for (int i = 0; i < n; ++i) s += x[(size_t) i] * x[(size_t) (i + l)]; if (s > best) { best = s; lag = l; } }
    // interpolação parabólica
    auto ac = [&] (int l) { double s = 0; for (int i = 0; i < n; ++i) s += x[(size_t) i] * x[(size_t) (i + l)]; return s; };
    const double y0 = ac (lag - 1), y1 = best, y2 = ac (lag + 1), d = (y0 - y2) / (2 * (y0 - 2 * y1 + y2));
    return sr / (lag + d);
}

int main (int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : "";
    const int sr = 48000;
    using namespace mrcl;

    // 1) afinação de uma corda sozinha (sem efeitos)
    for (int m : { 36, 48, 60, 72 })
    {
        ClavVoice v; ClavParams p;
        v.start (m, 1.0f, (float) sr, p, 1);
        std::vector<float> x ((size_t) sr, 0.0f);
        v.render (x.data(), sr, (float) sr);
        const double f = pitchOf (std::vector<float> (x.begin() + 2000, x.end()), sr, midiHz ((float) m));
        const double cents = 1200.0 * std::log2 (f / midiHz ((float) m));
        char msg[128]; std::snprintf (msg, sizeof msg, "afina\xc3\xa7\xc3\xa3o da nota %d: %.1f cents", m, cents);
        CHECK (std::fabs (cents) < 5.0, msg);
    }

    // 2) as linhas: cada clavinetista x cada sugestão x fill
    std::vector<Chord> cs { { 0, 9, true, 4 }, { 1, 2, true, 4 }, { 2, 7, false, 4 }, { 3, 4, true, 4 } };
    int total = 0, onBeat1or3Chords = 0;
    for (int mu = 0; mu < (int) musicians().size(); ++mu)
        for (int st = 0; st < 3; ++st)
            for (int fill = 0; fill < 4; ++fill)
            {
                Settings s; s.musician = mu; s.fill = fill; s.fillEvery = fill % 2 ? 8 : 4; s.center = 53;
                const auto line = generate (st, cs, s);
                bool okLen = true, okRange = true;
                for (auto& e : line.events)
                {
                    okLen &= e.start >= 0 && e.start + e.length <= line.bars * 4.0 + 1e-9 && e.length > 0;
                    okRange &= e.note >= 41 && e.note <= 77;
                    // o skank do clavinet nunca cai na cabeça do 1 nem do 3 (o espaço do baixo); a caixa é no 2 e no 4
                    if (st == 0 && fill == 0 && std::fmod (e.start, 2.0) < 1e-9) ++onBeat1or3Chords;
                }
                total += (int) line.events.size();
                if (! okLen || ! okRange || line.events.empty())
                {
                    char msg[160]; std::snprintf (msg, sizeof msg, "linha %s / %s / fill %s", musicians()[(size_t) mu].name, styleName (st), fillName (fill));
                    CHECK (false, msg);
                }
            }
    char msg[160]; std::snprintf (msg, sizeof msg, "7 clavinetistas x 3 sugest\xc3\xb5" "es x 4 fills: %d notas, todas dentro do loop e da faixa", total);
    CHECK (fails == 0, msg);
    CHECK (onBeat1or3Chords == 0, "skank duplo nunca na cabe\xc3\xa7" "a do 1 e do 3 (contagem com a caixa no 2 e no 4)");
    {
        Settings a; a.seed = 7; Settings b = a;
        CHECK (generate (2, cs, a).events.size() == generate (2, cs, b).events.size(), "mesma semente = mesma linha");
        Settings f8; f8.fill = 1; f8.fillEvery = 8;
        std::vector<Chord> two { { 0, 9, true, 4 }, { 1, 2, true, 4 } };
        CHECK (generate (0, two, f8).bars == 8, "frase de 8: loop de 2 compassos com fill vira 8 compassos");
        Settings lo; lo.center = 17; Settings hi; hi.center = 65;
        int mnLo = 127, mnHi = 127;
        for (auto& e : generate (0, cs, lo).events) mnLo = std::min (mnLo, e.note);
        for (auto& e : generate (0, cs, hi).events) mnHi = std::min (mnHi, e.note);
        CHECK (mnHi - mnLo == 48, "chave OITAVA: C-1 a C3 = 4 oitavas");
    }

    // 3) instrumento inteiro tocando uma linha: sem estouro, sem nota presa
    {
        ClavInstrument ins; ins.prepare ((float) sr);
        Settings s; s.musician = 1; s.center = 53;
        const auto line = generate (0, cs, s);
        const double bpm = 76, spb = 60.0 / bpm;
        const int len = (int) (line.bars * 4 * spb * sr) + sr;
        std::vector<float> L ((size_t) len), R ((size_t) len);
        std::multimap<int, std::pair<int, int>> evs;   // amostra -> (nota, vel ou -1 = off)
        for (auto& e : line.events)
        {
            evs.insert ({ (int) (e.start * spb * sr), { e.note, e.vel } });
            evs.insert ({ (int) ((e.start + e.length) * spb * sr), { e.note, -1 } });
        }
        auto it = evs.begin();
        float peak = 0; double rms = 0;
        for (int pos = 0; pos < len; pos += 64)
        {
            const int n = std::min (64, len - pos);
            while (it != evs.end() && it->first < pos + n) { if (it->second.second < 0) ins.noteOff (it->second.first); else ins.noteOn (it->second.first, it->second.second / 127.0f); ++it; }
            ins.render (L.data() + pos, R.data() + pos, n);
        }
        for (float v : L) { peak = std::max (peak, std::fabs (v)); rms += v * v; }
        rms = std::sqrt (rms / L.size());
        char m2[128]; std::snprintf (m2, sizeof m2, "linha tocada: pico %.2f, RMS %.1f dB", peak, 20 * std::log10 (rms + 1e-9));
        CHECK (peak < 1.0f && rms > 0.003, m2);
        CHECK (ins.activeVoices() == 0, "nenhuma nota presa depois da linha");
        if (! out.empty()) writeWav (out + "/mrcl71_linha.wav", L, sr);
        // escala de notas soltas para ouvir o timbre
        if (! out.empty())
        {
            ClavInstrument i2; i2.prepare ((float) sr);
            std::vector<float> l2 ((size_t) sr * 6), r2 ((size_t) sr * 6);
            const int notes[8] = { 41, 45, 48, 53, 55, 60, 65, 72 };
            for (int k = 0; k < 8; ++k)
            {
                const int a = k * sr * 3 / 4;
                i2.noteOn (notes[k], 0.9f);
                i2.render (l2.data() + a, r2.data() + a, sr / 2);
                i2.noteOff (notes[k]);
                i2.render (l2.data() + a + sr / 2, r2.data() + a + sr / 2, sr / 4);
            }
            writeWav (out + "/mrcl71_notas.wav", l2, sr);
        }
    }
    std::printf (fails == 0 ? "\nTUDO OK\n" : "\n%d FALHA(S)\n", fails);
    return fails == 0 ? 0 : 1;
}
