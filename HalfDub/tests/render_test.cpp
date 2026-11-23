// Teste offline do DSP do Mastor Half Dub MHD-050 (sem JUCE):
//   g++ -std=c++17 -O2 -Wall -Wextra tests/render_test.cpp -o render_test && ./render_test
// Confere: meia velocidade de verdade (FITA baixa o tom, GROOVE mantém), REV, 1/4, DROP desligado = seco,
// POWER DOWN, pads de sound system, bandas, EQ/Big Knob, ausência de estalos e de NaN. Grava WAVs de demonstração.
#include "../Source/HalfDubEngine.h"
#include <cstdio>
#include <string>
#include <random>

static int falhas = 0;
static void check (bool ok, const char* what, double v)
{
    std::printf ("%s  %-58s %.4f\n", ok ? "ok   " : "FALHA", what, v);
    if (! ok) ++falhas;
}

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int fs)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto u32 = [&](uint32_t v){ std::fwrite (&v, 4, 1, f); };
    auto u16 = [&](uint16_t v){ std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) fs); u32 ((uint32_t) fs * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        const int16_t l = (int16_t) (std::clamp (L[i], -1.0f, 1.0f) * 32767), r = (int16_t) (std::clamp (R[i], -1.0f, 1.0f) * 32767);
        std::fwrite (&l, 2, 1, f); std::fwrite (&r, 2, 1, f);
    }
    std::fclose (f);
}

static const int FS = 48000;
static const double BPM = 120.0;

// roda o aparelho num sinal (host tocando a 120 BPM desde o tempo 0)
static void run (hd::Engine& e, const std::vector<float>& inL, const std::vector<float>& inR,
                 std::vector<float>& outL, std::vector<float>& outR, int block = 512)
{
    const size_t N = inL.size();
    outL = inL; outR = inR;
    const double spb = 60.0 / BPM * FS;
    for (size_t pos = 0; pos < N; pos += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, N - pos);
        hd::HostClock c; c.playing = true; c.bpm = BPM; c.ppq = (double) pos / spb;
        e.process (outL.data() + pos, outR.data() + pos, n, c);
    }
}

static std::vector<float> sine (float hz, int n, float amp = 0.3f)
{
    std::vector<float> v ((size_t) n);
    for (int i = 0; i < n; ++i) v[(size_t) i] = amp * std::sin (hd::kTau * hz * (float) i / FS);
    return v;
}

// frequência dominante por contagem de cruzamentos por zero numa janela
static double zcFreq (const std::vector<float>& x, size_t a, size_t b)
{
    int z = 0;
    for (size_t i = a + 1; i < b; ++i)
        if ((x[i - 1] < 0) != (x[i] < 0)) ++z;
    return z * 0.5 * FS / (double) (b - a);
}

static double rmsOf (const std::vector<float>& x, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i];
    return std::sqrt (s / (double) std::max<size_t> (1, b - a));
}

// maior salto entre amostras (estalos)
static double maxJump (const std::vector<float>& x, size_t a, size_t b)
{
    double m = 0; for (size_t i = a + 1; i < b; ++i) m = std::max (m, (double) std::abs (x[i] - x[i - 1]));
    return m;
}

static hd::EngineParams base()
{
    hd::EngineParams p;
    p.echoOn = false; p.echoSend = 0; p.wow = 0; p.sat = 0; p.washout = 0;
    p.bkMix = 0.0f;                        // Big Knob fora (OFF já é quase transparente)
    return p;
}

// loop de "riddim" sintético: bumbo no 3, aro, chimbal e linha de baixo
static void makeRiddim (std::vector<float>& L, std::vector<float>& R, int bars)
{
    const int spb = (int) (60.0 / BPM * FS), n = spb * 4 * bars;
    L.assign ((size_t) n, 0.0f); R.assign ((size_t) n, 0.0f);
    std::mt19937 rng (7); std::uniform_real_distribution<float> u (-1, 1);
    const float bass[8] = { 55.0f, 55.0f, 65.4f, 73.4f, 55.0f, 49.0f, 55.0f, 82.4f };
    float ph = 0;
    for (int i = 0; i < n; ++i)
    {
        const int beatI = i / spb, inBeat = i % spb;
        const float tb = (float) inBeat / FS;
        float y = 0;
        if (beatI % 4 == 2) y += 0.8f * std::exp (-tb * 18.0f) * std::sin (hd::kTau * (60.0f + 80.0f * std::exp (-tb * 40.0f)) * tb);   // one drop
        if (beatI % 2 == 1) y += 0.35f * std::exp (-tb * 30.0f) * u (rng);                                                             // aro
        const int e8 = (i / (spb / 2)) % 2; const float t8 = (float) (i % (spb / 2)) / FS;
        if (e8 == 1) y += 0.12f * std::exp (-t8 * 60.0f) * u (rng);                                                                       // chimbal
        const float f = bass[(i / (spb / 2)) % 8];
        ph += hd::kTau * f / FS; if (ph > hd::kTau) ph -= hd::kTau;
        y += 0.35f * std::sin (ph) * (0.6f + 0.4f * std::exp (-t8 * 6.0f));
        L[(size_t) i] = y; R[(size_t) i] = y;
    }
}

int main()
{
    const int spb = (int) (60.0 / BPM * FS), bar = spb * 4;
    std::vector<float> oL, oR;

    // 1) DROP desligado = seco (fase de passa-tudo, mesmo nível)
    {
        hd::Engine e; auto p = base(); p.drop = false; e.setParams (p); e.prepare (FS);
        auto s = sine (440, bar * 2);
        run (e, s, s, oL, oR);
        const double r = rmsOf (oL, bar, bar * 2) / rmsOf (s, bar, bar * 2);
        check (std::abs (r - 1.0) < 0.05, "DROP desligado: nível igual ao seco (razão)", r);
        check (std::abs (zcFreq (oL, bar, bar * 2) - 440) < 3, "DROP desligado: mesma frequência (Hz)", zcFreq (oL, bar, bar * 2));
    }
    // 2) FITA 1/2: o tom cai uma oitava
    {
        hd::Engine e; auto p = base(); p.speed = 2; e.setParams (p); e.prepare (FS);
        auto s = sine (440, bar * 3);
        run (e, s, s, oL, oR);
        const double f = zcFreq (oL, bar + bar / 8, bar * 2 - bar / 8);
        check (std::abs (f - 220) < 6, "FITA 1/2: 440 Hz vira 220 Hz", f);
        const double r = rmsOf (oL, bar, bar * 2) / rmsOf (s, bar, bar * 2);
        check (r > 0.85 && r < 1.15, "FITA 1/2: nível preservado", r);
        // sem estalo na virada do loop (seno de 220 Hz a 0,3 tem salto máx ~0,0086)
        const double j = maxJump (oL, bar * 2 - 2000, bar * 2 + 2000);
        check (j < 0.05, "FITA 1/2: virada do loop sem estalo (salto máx)", j);
    }
    // 3) FITA 1/4 e 2/3
    {
        hd::Engine e; auto p = base(); p.speed = 4; e.setParams (p); e.prepare (FS);
        auto s = sine (440, bar * 3); run (e, s, s, oL, oR);
        const double f = zcFreq (oL, bar + bar / 8, bar * 2 - bar / 8);
        check (std::abs (f - 110) < 4, "FITA 1/4: 440 Hz vira 110 Hz", f);
    }
    {
        hd::Engine e; auto p = base(); p.speed = 1; e.setParams (p); e.prepare (FS);
        auto s = sine (450, bar * 3); run (e, s, s, oL, oR);
        const double f = zcFreq (oL, bar + bar / 8, bar * 2 - bar / 8);
        check (std::abs (f - 300) < 6, "FITA 2/3: 450 Hz vira 300 Hz", f);
    }
    // 4) GROOVE 1/2: o tom fica
    {
        hd::Engine e; auto p = base(); p.speed = 2; p.groove = true; e.setParams (p); e.prepare (FS);
        auto s = sine (440, bar * 3); run (e, s, s, oL, oR);
        const double f = zcFreq (oL, bar + bar / 8, bar * 2 - bar / 8);
        check (std::abs (f - 440) < 10, "GROOVE 1/2: 440 Hz continua 440 Hz", f);
        const double r = rmsOf (oL, bar, bar * 2) / rmsOf (s, bar, bar * 2);
        check (r > 0.6 && r < 1.3, "GROOVE 1/2: nível próximo", r);
    }
    // 5) REV: um seno com frequência subindo vira descendo
    {
        hd::Engine e; auto p = base(); p.speed = 5; e.setParams (p); e.prepare (FS);
        std::vector<float> s ((size_t) bar * 3); double ph = 0;
        for (size_t i = 0; i < s.size(); ++i)
        {
            const double t = (double) (i % (size_t) bar) / bar;   // rampa 300 -> 900 Hz por compasso
            ph += hd::kTau * (300.0 + 600.0 * t) / FS; s[i] = 0.3f * (float) std::sin (ph);
        }
        run (e, s, s, oL, oR);
        const double a = zcFreq (oL, bar + bar / 10, bar + bar / 4), b = zcFreq (oL, bar * 2 - bar / 4, bar * 2 - bar / 10);
        check (a > b + 200, "REV: a rampa do compasso anterior toca ao contrário (Hz início-fim)", a - b);
    }
    // 6) bandas: grave normal, agudo desligado
    {
        hd::Engine e; auto p = base(); p.speed = 2; p.bandSpeed[0] = 1; p.bandSpeed[2] = 7; e.setParams (p); e.prepare (FS);
        auto lo = sine (60, bar * 3, 0.3f), hi = sine (6000, bar * 3, 0.3f);
        std::vector<float> s (lo.size()); for (size_t i = 0; i < s.size(); ++i) s[i] = lo[i] + hi[i];
        run (e, s, s, oL, oR);
        // agudo OFF: quase nada acima de 3 kHz -> sinal resultante ~ seno de 60 Hz
        const double f = zcFreq (oL, bar + bar / 8, bar * 2 - bar / 8);
        check (std::abs (f - 60) < 3, "Bandas: GRAVE 1/1 fica em 60 Hz e o AGUDO em OFF some", f);
    }
    // 7) POWER DOWN de 2 tempos: no meio da rampa o tom está entre 440 e 220
    {
        hd::Engine e; auto p = base(); p.speed = 2; p.drop = false; p.powerDownBeats = 2; e.setParams (p); e.prepare (FS);
        auto s = sine (440, bar * 3);
        oL = s; oR = s;
        const double spbD = 60.0 / BPM * FS;
        for (size_t pos = 0; pos < s.size(); pos += 512)
        {
            if (pos >= (size_t) bar && ! p.drop) { p.drop = true; e.setParams (p); }
            const int n = (int) std::min<size_t> (512, s.size() - pos);
            hd::HostClock c; c.playing = true; c.bpm = BPM; c.ppq = (double) pos / spbD;
            e.process (oL.data() + pos, oR.data() + pos, n, c);
        }
        const double f = zcFreq (oL, (size_t) bar + spb / 2 + 2000, (size_t) bar + spb * 3 / 2 - 2000);
        check (f > 250 && f < 420, "POWER DOWN: a fita perde velocidade aos poucos (Hz no meio)", f);
        const double j = maxJump (oL, bar - 2000, bar * 2);
        check (j < 0.05, "POWER DOWN: sem estalo ao ligar o DROP", j);
    }
    // 8) TAPE STOP: termina em silêncio
    {
        hd::Engine e; auto p = base(); p.drop = false; p.padSync = true; p.padDur = 0.5f; e.setParams (p); e.prepare (FS);   // 1 tempo
        auto s = sine (440, bar * 2);
        oL = s; oR = s;
        const double spbD = 60.0 / BPM * FS;
        for (size_t pos = 0; pos < s.size(); pos += 512)
        {
            if (pos >= (size_t) bar && p.pad == 0) { p.pad = hd::SoundSystemPads::TapeStop; e.setParams (p); }
            const int n = (int) std::min<size_t> (512, s.size() - pos);
            hd::HostClock c; c.playing = true; c.bpm = BPM; c.ppq = (double) pos / spbD;
            e.process (oL.data() + pos, oR.data() + pos, n, c);
        }
        const double r = rmsOf (oL, (size_t) bar + spb + 4000, (size_t) bar * 2);
        check (r < 0.01, "TAPE STOP: depois de 1 tempo a fita para (RMS)", r);
        const double mid = zcFreq (oL, (size_t) bar + spb / 4, (size_t) bar + spb / 2);
        check (mid < 400 && mid > 100, "TAPE STOP: o tom vai caindo (Hz)", mid);
    }
    // 9) estabilidade: parâmetros aleatórios, ruído alto
    {
        std::mt19937 rng (42); std::uniform_real_distribution<float> u (0, 1);
        hd::Engine e; auto p = hd::EngineParams(); e.setParams (p); e.prepare (44100);
        std::vector<float> s ((size_t) 44100 * 30); for (auto& v : s) v = 0.9f * (u (rng) * 2 - 1);
        std::vector<float> L = s, R = s;
        double pk = 0; bool nan = false;
        for (size_t pos = 0; pos < s.size(); pos += 256)
        {
            if ((pos / 256) % 50 == 0)
            {
                p.speed = (int) (u (rng) * 6); p.groove = u (rng) > 0.5f; p.drop = u (rng) > 0.3f;
                for (int b = 0; b < 3; ++b) { p.bandSpeed[b] = (int) (u (rng) * 8); p.bandLoop[b] = (int) (u (rng) * 8); p.bandSmooth[b] = u (rng); }
                p.smoothMode = (int) (u (rng) * 4); p.powerDownBeats = u (rng) * 4; p.powerUpBeats = u (rng) * 4;
                p.pad = (int) (u (rng) * 5); p.padDur = u (rng); p.padSync = u (rng) > 0.5f;
                p.wow = u (rng); p.sat = u (rng); p.echoFb = u (rng) * 1.1f; p.echoSend = u (rng); p.echoPP = u (rng) > 0.5f; p.echoMte = u (rng) > 0.5f;
                p.eq.hp = 20 + u (rng) * 500; p.eq.lp = 1000 + u (rng) * 19000; for (int k = 0; k < 4; ++k) { p.eq.g[k] = u (rng) * 24 - 12; p.eq.q[k] = 0.3f + u (rng) * 3.7f; }
                p.bkCut = u (rng); p.bkRes = u (rng); p.bkDrive = u (rng); p.bk24 = u (rng) > 0.5f; p.bkLowPass = u (rng) > 0.5f;
                e.setParams (p);
            }
            hd::HostClock c; c.playing = (pos / 44100) % 7 != 3; c.bpm = 60 + 120 * ((pos / 88200) % 2); c.ppq = (double) pos / (60.0 / c.bpm * 44100);
            const int n = (int) std::min<size_t> (256, s.size() - pos);
            e.process (L.data() + pos, R.data() + pos, n, c);
            for (int i = 0; i < n; ++i)
            {
                if (! std::isfinite (L[pos + (size_t) i]) || ! std::isfinite (R[pos + (size_t) i])) nan = true;
                pk = std::max (pk, (double) std::abs (L[pos + (size_t) i]));
            }
        }
        check (! nan, "Estabilidade: 30 s de parâmetros aleatórios sem NaN", nan ? 1 : 0);
        check (pk <= 4.0, "Estabilidade: pico limitado", pk);
    }

    // demos em WAV: riddim seco / INIT (1/2 FITA + eco) / GROOVE / bandas
    {
        std::vector<float> L, R; makeRiddim (L, R, 8);
        writeWav ("riddim_seco.wav", L, R, FS);
        const char* names[] = { "init_fita_meia", "groove_meia", "grave_normal_agudo_rev", "quarto_com_bigknob" };
        for (int k = 0; k < 4; ++k)
        {
            hd::Engine e; hd::EngineParams p;
            if (k == 1) p.groove = true;
            if (k == 2) { p.bandSpeed[0] = 1; p.bandSpeed[2] = 6; }
            if (k == 3) { p.speed = 4; p.bkCut = 4.0f / 11.0f; p.bkRes = 0.5f; }
            e.setParams (p); e.prepare (FS);
            run (e, L, R, oL, oR);
            writeWav (std::string (names[k]) + ".wav", oL, oR, FS);
            double pk = 0; for (float v : oL) pk = std::max (pk, (double) std::abs (v));
            check (pk > 0.05 && pk < 2.0, (std::string ("Demo ") + names[k] + ": pico").c_str(), pk);
        }
    }

    std::printf ("\n%s (%d falha(s))\n", falhas ? "TESTE FALHOU" : "TUDO OK", falhas);
    return falhas ? 1 : 0;
}
