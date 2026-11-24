// Teste offline do Mastor Reggae Organ MRO-68 (sem JUCE, sem DAW)
//   g++ -std=c++17 -O2 -I Source -I ../ReggaeKeys/Source tests/render_test.cpp -o render_test && ./render_test
// Contagem da Dub Station: caixa no 2 e no 4. Confere as regras do groove (posições, acentos, duração 1/32,
// fills, determinismo, notas presas) e grava um WAV de 8 compassos por músico em out/
// (Am | Dm | Am | Em, com a bateria de referência da batida do músico).
#include <cstdio>
#include <cstring>
#include <array>
#include <map>
#include <memory>
#include <string>
#include "GroovePlayer.h"

using namespace mro;
static int fails = 0;
#define CHECK(c, msg) do { if (! (c)) { std::printf ("  FALHOU: %s\n", msg); ++fails; } } while (0)

static int frac16 (double t) { return (int) std::lround (std::fmod (t, 1.0) * 4.0) % 4; }

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path, "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) L.size(), data = n * 4, riff = 36 + data;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (riff); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (data);
    for (uint32_t i = 0; i < n; ++i)
        for (float x : { L[i], R[i] })
        {
            const int16_t v = (int16_t) std::lround (std::clamp (x, -1.0f, 1.0f) * 32767.0f);
            std::fwrite (&v, 2, 1, f);
        }
    std::fclose (f);
}

int main()
{
    const std::vector<Chord> prog { { 9, true, 4 }, { 2, true, 4 }, { 9, true, 4 }, { 4, true, 4 } };
    auto barChords = [&] (int bar) { std::array<Chord, 4> c; chordsOfBar (prog, bar, c.data()); return c; };

    // ---------------- regras do groove (contagem com caixa no 2 e no 4)
    {
        Settings s; s.human = 0; s.swing = 0; s.fillsOn = false;
        const auto bc = barChords (0);
        for (int d = 0; d < NumDensities; ++d)
            for (int f = 0; f < NumFeels; ++f)
            {
                s.grid = makeGrid (f, d);
                std::map<int, int> pos;
                for (auto& e : barEvents (0, bc.data(), s))
                {
                    const int p = frac16 (e.start);
                    if (e.layer == LayerBubble) pos[p]++;
                    CHECK (p != 0, "nada na cabe\xc3\xa7" "a do tempo (o 1, 2, 3 e 4 ficam para a bateria e o baixo)");
                    if (e.layer == LayerChop) CHECK (p == 2, "chop (m\xc3\xa3o direita) sempre no contratempo &");
                    if (e.layer == LayerBubble) CHECK (p == 1 || p == 3, "bubble (m\xc3\xa3o esquerda) no e e no a");
                    CHECK (std::abs (e.length - 0.125) < 1e-9, "nota de 1/32 por padr\xc3\xa3o");
                    if (e.layer == LayerBubble)
                    {
                        const bool want = f == FeelRocksteady ? true : (f == FeelSka ? false : p == 1);
                        CHECK (e.accent == want, "FEEL: ska ki-koo-ki, rocksteady koo-ki-koo, reggae e forte / a fantasma");
                    }
                }
                CHECK ((pos.count (1) == 1) == (d != DenA), "densidade controla o e");
                CHECK ((pos.count (3) == 1) == (d != DenE), "densidade controla o a");
            }
        // tríade de Lá menor no & do tempo 1
        s.grid = makeGrid (FeelReggae, DenEA);
        std::vector<int> notes;
        for (auto& e : barEvents (0, bc.data(), s)) if (e.layer == LayerChop && std::abs (e.start - 0.5) < 1e-9) notes.push_back (e.note % 12);
        std::sort (notes.begin(), notes.end());
        CHECK ((notes == std::vector<int> { 0, 4, 9 }), "acorde de L\xc3\xa1 menor = A C E");
        // nota maior: NOTA 1/16
        s.noteSize = 1;
        for (auto& e : barEvents (0, bc.data(), s)) CHECK (std::abs (e.length - 0.25) < 1e-9, "NOTA 1/16");
        s.noteSize = 0;
        // grade editada: célula na cabeça do 1 também funciona
        s.grid = Grid(); s.grid.rh[0] = Accent; s.grid.lh[7] = Ghost;
        auto ev = barEvents (0, bc.data(), s);
        CHECK (ev.size() == 4 && ev[0].start == 0.0, "a grade manda: c\xc3\xa9lula editada toca onde foi posta");
        // swing: só o e e o a andam
        s.grid = makeGrid (FeelReggae, DenEA); s.swing = 1.0f;
        for (auto& e : barEvents (0, bc.data(), s))
        {
            const double f = std::fmod (e.start, 1.0);
            CHECK (std::abs (f - 0.25 - 1.0 / 12) < 1e-9 || std::abs (f - 0.5) < 1e-9 || std::abs (f - 0.75 - 1.0 / 12) < 1e-9,
                   "swing atrasa s\xc3\xb3 o e e o a");
        }
        // acorde de meio compasso troca no tempo 3
        const std::vector<Chord> half { { 9, true, 2 }, { 2, true, 2 } };
        Chord hc[4]; chordsOfBar (half, 0, hc);
        CHECK (hc[0].root == 9 && hc[1].root == 9 && hc[2].root == 2 && hc[3].root == 2, "acorde de 1/2 compasso");
        // fills só nos compassos certos, nos tempos 3 e 4
        s = Settings(); s.fillEvery = 4;
        for (int st = 0; st < NumFillStyles; ++st)
        {
            s.fillStyle = st;
            for (int b = 0; b < 8; ++b)
            {
                int nf = 0;
                const auto c = barChords (b);
                for (auto& e : barEvents (b, c.data(), s)) if (e.layer == LayerFill) { ++nf; CHECK (e.start >= 1.97, "fill nos tempos 3 e 4"); }
                CHECK ((nf > 0) == (b % 4 == 3), "fill s\xc3\xb3 no 4\xc2\xba compasso");
            }
        }
        // determinismo e variação
        const auto c5 = barChords (5), c7 = barChords (7);
        auto a1 = barEvents (5, c5.data(), s), b1 = barEvents (5, c5.data(), s);
        bool same = a1.size() == b1.size();
        for (size_t i = 0; same && i < a1.size(); ++i) same = a1[i].start == b1[i].start && a1[i].note == b1[i].note && a1[i].velocity == b1[i].velocity;
        CHECK (same, "mesmo compasso = mesmo groove (loop est\xc3\xa1vel)");
        s.seed = 2; auto c = barEvents (7, c7.data(), s); s.seed = 1; auto d = barEvents (7, c7.data(), s);
        bool differ = c.size() != d.size();
        for (size_t i = 0; ! differ && i < c.size(); ++i) differ = c[i].note != d[i].note || c[i].start != d[i].start;
        CHECK (differ, "VARIA\xc3\x87\xc3\x83O muda o groove");
        // músicos: grades válidas, sempre com o contratempo da mão direita, e fills dentro do loop
        for (int m = 0; m < NumMusicians; ++m)
        {
            const auto& mu = musician (m);
            CHECK (std::strlen (mu.rh) == 16 && std::strlen (mu.lh) == 16, "grade do m\xc3\xbasico com 16 semicolcheias");
            const Grid g = gridFrom (mu.rh, mu.lh);
            for (int b = 0; b < 4; ++b) CHECK (g.rh[b * 4 + 2] != Off, "todo m\xc3\xbasico faz o contratempo da m\xc3\xa3o direita");
            CHECK (mu.bpm <= 100.0f, "BPM na contagem com caixa no 2 e no 4 (at\xc3\xa9 100)");
        }
        for (int e = 0; e < numEpochs; ++e) CHECK (epoch (e).bpm <= 100.0f, "BPM das \xc3\xa9pocas at\xc3\xa9 100");
        // bateria de referência: caixa no 2 e no 4
        CHECK (snareAt (4) && snareAt (12) && ! snareAt (0) && ! snareAt (8), "caixa no 2 e no 4");
        CHECK (kickAt (BeatOneDrop, 4) && kickAt (BeatOneDrop, 12) && ! kickAt (BeatOneDrop, 0), "one drop: bumbo junto com a caixa no 2 e no 4");
        CHECK (kickAt (BeatRockers, 0) && kickAt (BeatRockers, 8) && ! kickAt (BeatRockers, 4), "rockers: bumbo no 1 e no 3");
        // detecção de acorde pelo teclado
        bool held[128] = {};
        held[57] = held[60] = held[64] = true;
        Chord dc; CHECK (detectChord (held, dc) && dc.root == 9 && dc.minor, "teclado: A C E = L\xc3\xa1 menor");
        std::memset (held, 0, sizeof held); held[64] = held[67] = held[72] = true;
        CHECK (detectChord (held, dc) && dc.root == 0 && ! dc.minor, "teclado: invers\xc3\xa3o E G C = D\xc3\xb3 maior");
        // loop
        int lb = 0; s.fillEvery = 8; buildLoop (prog, s, lb);
        CHECK (lb == 8, "loop de 4 acordes com fill a cada 8 = 8 compassos");
        s.fillEvery = 4; buildLoop (half, s, lb);
        CHECK (lb == 4, "2 acordes de meio compasso com fill a cada 4 = 4 compassos");
        // loop editado: notas por compasso
        std::vector<Event> ed { { 0.5, 0.125, 60, 100, LayerChop, true }, { 4.75, 0.125, 45, 90, LayerBubble, false } };
        CHECK (eventsOfBar (ed, 2, 3).size() == 1 && std::abs (eventsOfBar (ed, 2, 3)[0].start - 0.75) < 1e-9, "loop editado toca no compasso certo");
    }

    // ---------------- player: todas as notas ligadas são desligadas, em blocos de tamanhos variados
    {
        GroovePlayer pl; Settings s;
        std::vector<NoteCmd> cmds;
        std::map<std::pair<int, int>, int> on;
        double ppq = 0; const double sr = 48000, bpm = 80;
        int blocks[] = { 64, 512, 333, 1024, 7 };
        auto evf = [&] (int bar) { const auto c = barChords (bar); return barEvents (bar, c.data(), s); };
        for (int i = 0; i < 2000; ++i)
        {
            const int n = blocks[i % 5];
            cmds.clear();
            pl.process (ppq, bpm, sr, n, evf, cmds);
            int last = -1;
            for (auto& c : cmds)
            {
                CHECK (c.sample >= last && c.sample < n, "comandos em ordem e dentro do bloco");
                last = c.sample;
                on[std::make_pair (c.engine, c.note)] += c.on ? 1 : -1;
                { const int cnt = on[std::make_pair (c.engine, c.note)]; CHECK (cnt >= 0 && cnt <= 1, "nunca duas notas iguais presas no mesmo motor"); }
            }
            ppq += n / sr * bpm / 60.0;
        }
        cmds.clear(); pl.stop (cmds);
        for (auto& c : cmds) on[std::make_pair (c.engine, c.note)] += c.on ? 1 : -1;
        int stuck = 0; for (auto& kv : on) stuck += kv.second;
        CHECK (stuck == 0, "nenhuma nota presa depois de parar");
    }

    // ---------------- áudio: 8 compassos por músico
    if (std::system ("mkdir -p out") != 0) return 1;
    for (int m = 0; m <= NumMusicians; ++m)   // o último: o groove do Mittoo com o TIMBRE MIDNITE (1.3)
    {
        const bool midnite = m == NumMusicians;
        const Musician& mu = musician (midnite ? (int) MusMittoo : m);
        Settings s;
        s.grid = gridFrom (mu.rh, mu.lh); s.beat = mu.beat; s.swing = mu.swing; s.human = mu.human;
        s.ghost = mu.ghost; s.fillStyle = mu.fillStyle; s.fillEvery = mu.fillEvery;
        SoundParams sp;
        for (int d = 0; d < 9; ++d) { sp.bubbleDb[d] = (float) mu.bubbleDb[d]; sp.chopDb[d] = (float) mu.chopDb[d]; }
        sp.ghostLevel = mu.ghost; sp.lesPreset = leslieFromMusician (mu.leslie); sp.lesSpeed = mu.leslie == 2 ? 1 : 0; sp.drive = mu.drive; sp.perc = mu.perc; sp.echoOn = true; sp.echoVol = 0.3f;
        if (midnite)
        {
            const OrganTimbre& t = midniteTimbre();
            for (int d = 0; d < 9; ++d) { sp.bubbleDb[d] = (float) t.bubbleDb[d]; sp.chopDb[d] = (float) t.chopDb[d]; }
            sp.perc = t.perc; sp.drive = t.drive; sp.click = t.click; sp.reverb = t.reverb; sp.echoOn = false;
            sp.lesPreset = t.lesPreset; sp.lesSpeed = t.lesSpeed; sp.lesDepth = t.lesDepth; sp.eq = t.eq;
        }

        const int sr = 48000, blk = 256;
        OrganTrio trio; trio.prepare (sr); trio.setParams (sp);
        GroovePlayer pl;
        const double bpm = mu.bpm, total = 8 * 4 * 60.0 / bpm + 1.5;
        const int N = (int) (total * sr);
        std::vector<float> L ((size_t) N), R ((size_t) N), keys ((size_t) N);
        std::vector<NoteCmd> cmds;
        auto evf = [&] (int bar) { const auto c = barChords (bar); return barEvents (bar, c.data(), s); };
        double ppq = 0; int pos = 0, notesOn = 0;
        while (pos < N)
        {
            const int n = std::min (blk, N - pos);
            cmds.clear();
            if (ppq < 32.0) pl.process (ppq, bpm, sr, n, evf, cmds);
            else if (pl.isRunning()) pl.stop (cmds);
            int done = 0;
            for (auto& c : cmds)
            {
                if (c.sample > done) { trio.render (&L[(size_t) (pos + done)], &R[(size_t) (pos + done)], c.sample - done); done = c.sample; }
                if (c.on) { trio.engine (c.engine).noteOn (c.note, c.velocity / 127.0f); ++notesOn; }
                else trio.engine (c.engine).noteOff (c.note);
            }
            if (n > done) trio.render (&L[(size_t) (pos + done)], &R[(size_t) (pos + done)], n - done);
            ppq += n / (double) sr * bpm / 60.0;
            pos += n;
        }
        std::memcpy (keys.data(), L.data(), sizeof (float) * (size_t) N);

        // bateria de referência na contagem da Dub Station: caixa no 2 e no 4, bumbo conforme a batida, chimbal em colcheias
        const double spb = 60.0 / bpm * sr;
        uint32_t rs = 1;
        auto noise = [&] { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (float) (rs & 0xffff) / 32768.0f - 1.0f; };
        for (int b = 0; b < 32; ++b)
            for (int half = 0; half < 2; ++half)
            {
                const int at = (int) ((b + 0.5 * half + (half ? s.swing / 6.0 : 0.0)) * spb);
                const int p16 = (b % 4) * 4 + half * 2;
                for (int i = 0; i < (int) (0.4 * sr) && at + i < N; ++i)
                {
                    const float t = (float) i / sr;
                    float y = 0;
                    if (half == 0 && kickAt (s.beat, p16)) y += 0.6f * std::exp (-t * 9.0f) * std::sin (6.2832f * (48.0f * t + 1.8f * (1 - std::exp (-t * 30.0f))));
                    if (half == 0 && snareAt (p16) && t < 0.12f) y += 0.3f * std::exp (-t * 35.0f) * noise() + 0.15f * std::exp (-t * 25.0f) * std::sin (6.2832f * 190.0f * t);
                    if (t < 0.04f) y += 0.05f * std::exp (-t * 90.0f) * noise();
                    L[(size_t) (at + i)] += y; R[(size_t) (at + i)] += y;
                }
            }

        float pk = 0; double ms = 0;
        for (int i = 0; i < N; ++i) { pk = std::max (pk, std::fabs (keys[(size_t) i])); ms += keys[(size_t) i] * keys[(size_t) i]; }
        const double rmsDb = 10 * std::log10 (ms / N + 1e-12);
        std::printf ("%-9s %5.1f BPM  notas %4d  pico %.2f  RMS %5.1f dB\n", midnite ? "MIDNITE" : mu.name, bpm, notesOn, pk, rmsDb);
        CHECK (pk > 0.05f && pk <= 1.0f, "\xc3\xb3rg\xc3\xa3o soa e n\xc3\xa3o passa do teto");
        CHECK (rmsDb > -40.0 && std::isfinite (rmsDb), "n\xc3\xadvel do \xc3\xb3rg\xc3\xa3o razo\xc3\xa1vel, sem NaN");

        float mx = 0;
        for (int i = 0; i < N; ++i) mx = std::max (mx, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i])));
        if (mx > 0.98f) for (int i = 0; i < N; ++i) { L[(size_t) i] *= 0.98f / mx; R[(size_t) i] *= 0.98f / mx; }
        const std::string name = "out/MRO-68_" + std::to_string (m + 1) + "_" + (midnite ? "MIDNITE" : mu.name) + ".wav";
        writeWav (name.c_str(), L, R, sr);
    }

    // ---- pianos elétricos no lugar do Hammond: o mesmo groove soa em todos, sem NaN e sem passar do teto
    {
        const char* nomes[5] = { "B3", "Rhodes Stage", "Rhodes Suitcase", "Wurlitzer", "DX EP" };
        for (int ins = 0; ins < 5; ++ins)
        {
            const float sr = 48000.0f; const int N = (int) (6.0f * sr);
            SoundParams sp; sp.instr = ins;
            auto trioP = std::make_unique<OrganTrio>(); OrganTrio& trio = *trioP;   // no heap: 3 motores não cabem na pilha junto com o outro teste
            trio.prepare (sr); trio.setParams (sp);
            std::vector<float> L ((size_t) N), R ((size_t) N);
            const int chord[3] = { 57, 60, 64 };
            int pos = 0;
            for (int k = 0; k < 8; ++k)   // skank/chop no contratempo: o "e" de cada tempo a 100 BPM
            {
                const int on = (int) ((k + 0.5) * 0.6 * sr), off = on + (int) (0.6 * sr / 16);
                trio.render (&L[(size_t) pos], &R[(size_t) pos], on - pos); pos = on;
                for (int nn : chord) trio.engine (OrganTrio::Chop).noteOn (nn, 0.8f);
                trio.render (&L[(size_t) pos], &R[(size_t) pos], off - pos); pos = off;
                for (int nn : chord) trio.engine (OrganTrio::Chop).noteOff (nn);
            }
            trio.render (&L[(size_t) pos], &R[(size_t) pos], N - pos);
            float pk = 0; double e = 0;
            for (int i = 0; i < N; ++i) { pk = std::max (pk, std::fabs (L[(size_t) i])); e += (double) L[(size_t) i] * L[(size_t) i]; }
            const double db = 10.0 * std::log10 (e / N + 1e-12);
            char msg[120]; std::snprintf (msg, sizeof msg, "instrumento %s soa (%.1f dB RMS) sem NaN e abaixo do teto", nomes[ins], db);
            CHECK (pk > 0.02f && pk <= 1.0f && std::isfinite (db) && db > -45.0, msg);
        }
    }
    // ---- Leslie de dois rotores: a corneta acelera antes do tambor, o freio para os dois, o som gira sem NaN
    {
        const float sr = 48000.0f; const int blk = 480;
        LeslieDSP les; les.prepare (sr);
        std::vector<float> L ((size_t) blk), R ((size_t) blk);
        auto run = [&] (double secs)
        {
            for (int b = 0; b < (int) (secs * sr / blk); ++b)
            {
                for (int i = 0; i < blk; ++i) { const float x = 0.3f * std::sin (6.2831853f * 440.0f * (float) (b * blk + i) / sr); L[(size_t) i] = R[(size_t) i] = x; }
                les.process (L.data(), R.data(), blk);
                for (int i = 0; i < blk; ++i) CHECK (std::isfinite (L[(size_t) i]) && std::fabs (L[(size_t) i]) < 1.0f, "Leslie sem NaN e sem estourar");
            }
        };
        les.set (1, 0, false, 0.8f); run (6.0);
        CHECK (std::fabs (les.hornRate.load() - 0.83f) < 0.05f && std::fabs (les.drumRate.load() - 0.67f) < 0.05f, "122 CL\xc3\x81SSICA lenta: corneta 0,83 e tambor 0,67 rot/s");
        les.set (1, 1, false, 0.8f); run (0.6);
        CHECK (les.hornRate.load() > 6.0f && les.drumRate.load() < 4.5f, "na R\xc3\x81PIDA a corneta chega antes do tambor");
        run (3.0);
        CHECK (les.drumRate.load() > 5.5f, "o tambor chega na R\xc3\x81PIDA em ~2,4 s");
        les.set (1, 1, true, 0.8f); run (6.0);
        CHECK (les.hornRate.load() < 0.1f && les.drumRate.load() < 0.2f, "FREIO para os rotores");
        for (int p = 0; p < NumLesliePresets; ++p) { les.set (p, 1, false, 1.0f); run (0.5); }
    }
    // ---- EQUALIZADOR (1.3): plano não muda nada; a curva da tela bate com o som; corte e realces onde devem
    {
        CHECK (std::fabs (OrganEQ::responseDb (eqPreset (0).p, 1000.0)) < 0.01, "EQ PLANO: 0 dB");
        EqParams e; e.midG = 6.0f; e.midF = 1000.0f;
        CHECK (std::fabs (OrganEQ::responseDb (e, 1000.0) - 6.0) < 0.1, "M\xc3\x89" "DIOS +6 dB em 1 kHz");
        e = EqParams(); e.hpf = 200.0f;
        CHECK (OrganEQ::responseDb (e, 50.0) < -20.0 && std::fabs (OrganEQ::responseDb (e, 2000.0)) < 0.2, "CORTE em 200 Hz: -20 dB em 50 Hz, 2 kHz intacto");
        e = EqParams(); e.lowG = -9.0f; e.highG = 6.0f;
        CHECK (OrganEQ::responseDb (e, 40.0) < -8.0 && OrganEQ::responseDb (e, 15000.0) > 5.5, "GRAVES e AGUDOS (shelves)");
        e.on = false;
        CHECK (OrganEQ::responseDb (e, 40.0) == 0.0, "EQ desligado: 0 dB");
        // a conta da tela = o som: seno de 1 kHz com +6 dB nos médios sai ~2x mais forte
        const double sr = 48000.0;
        for (double f : { 80.0, 1000.0, 8000.0 })
        {
            const EqParams& t = midniteTimbre().eq;
            OrganEQ q; q.prepare (sr); q.set (t);
            const int N = 48000;
            std::vector<float> L ((size_t) N), R ((size_t) N);
            for (int i = 0; i < N; ++i) L[(size_t) i] = R[(size_t) i] = 0.25f * (float) std::sin (6.283185307 * f * i / sr);
            q.process (L.data(), R.data(), N);
            double e2 = 0; for (int i = N / 2; i < N; ++i) e2 += (double) L[(size_t) i] * L[(size_t) i];
            const double got = 10 * std::log10 (e2 / (N / 2) / (0.25 * 0.25 / 2));
            char msg[120]; std::snprintf (msg, sizeof msg, "EQ Midnite em %.0f Hz: som %.2f dB = tela %.2f dB", f, got, OrganEQ::responseDb (t, f, sr));
            CHECK (std::fabs (got - OrganEQ::responseDb (t, f, sr)) < 0.15, msg);
        }
        const EqParams& t = midniteTimbre().eq;
        CHECK (OrganEQ::responseDb (t, 60.0) < -6.0 && OrganEQ::responseDb (t, 1100.0) > 1.5, "MIDNITE: grave fora do caminho do baixo, m\xc3\xa9" "dio na frente");
        CHECK (midniteTimbre().reverb == 0.0f && ! midniteTimbre().perc, "MIDNITE: seco (sem reverb) e sem percuss\xc3\xa3o");
    }
    std::printf (fails ? "\n%d FALHA(S)\n" : "\nTudo certo.\n", fails);
    return fails ? 1 : 0;
}
