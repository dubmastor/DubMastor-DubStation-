// Teste offline do DSP do Mastor Tune MT-0420 (sem JUCE).
//   g++ -std=c++17 -O2 -Wall -Wextra tests/render_test.cpp -o render_test && ./render_test
// Confere: detecção de afinação, afinação no robô e no clean, transparência (voz afinada não muda),
// timbre preservado (formantes), notas forçadas pelo editor, escuta do riddim, EQ e saturador.
// Grava WAVs de antes/depois na pasta atual para ouvir.
#include "../Source/TuneEngine.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>

using namespace mt;

static int failures = 0;
static void check (bool ok, const char* what, double v)
{
    std::printf ("  [%s] %-58s %.3f\n", ok ? " OK " : "FALHOU", what, v);
    if (! ok) ++failures;
}

static void writeWav (const std::string& name, const std::vector<float>& x, int fs)
{
    FILE* f = std::fopen (name.c_str(), "wb");
    if (! f) return;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    const uint32_t bytes = (uint32_t) x.size() * 2;
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (1); w32 ((uint32_t) fs); w32 ((uint32_t) fs * 2); w16 (2); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (float v : x) { const int16_t s = (int16_t) std::lround (std::clamp (v, -1.0f, 1.0f) * 32767.0f); std::fwrite (&s, 2, 1, f); }
    std::fclose (f);
}

// "voz" sintética: pulso glotal (soma de harmônicos com queda) passando por 3 formantes de "a"
struct SynthVoice
{
    double fs; double ph = 0;
    Biquad f1, f2, f3;
    explicit SynthVoice (double sr) : fs (sr)
    {
        f1.peak (sr, 700, 4, 18); f2.peak (sr, 1220, 5, 14); f3.peak (sr, 2600, 6, 10);
    }
    float next (double hz)
    {
        ph += hz / fs; if (ph >= 1.0) ph -= 1.0;
        float s = 0;
        const int nh = std::min (60, (int) (0.45 * fs / hz));
        for (int k = 1; k <= nh; ++k) s += (float) (std::sin (2.0 * M_PI * k * ph) / std::pow (k, 1.1));
        s *= 0.08f;
        return 0.25f * f3.process (f2.process (f1.process (s))) * 0.3f;
    }
};

// afinação medida por autocorrelação normalizada num trecho
static double measureHz (const std::vector<float>& x, size_t start, size_t len, double fs, double lo = 70, double hi = 900)
{
    const int tmin = (int) (fs / hi), tmax = (int) (fs / lo);
    double best = -1; int bt = tmin;
    std::vector<double> r ((size_t) tmax + 2, 0.0);
    for (int t = tmin - 1; t <= tmax + 1; ++t)
    {
        double s = 0, e1 = 0, e2 = 0;
        for (size_t i = start; i < start + len; ++i) { s += x[i] * x[i + (size_t) t]; e1 += x[i] * x[i]; e2 += x[i + (size_t) t] * x[i + (size_t) t]; }
        r[(size_t) t] = s / std::sqrt (e1 * e2 + 1e-12);
    }
    for (int t = tmin; t <= tmax; ++t) if (r[(size_t) t] > best && r[(size_t) t] >= r[(size_t) t - 1] && r[(size_t) t] >= r[(size_t) t + 1]) { best = r[(size_t) t]; bt = t; }
    // primeiro pico com pelo menos 90 % do maior (evita oitava abaixo)
    for (int t = tmin; t <= tmax; ++t) if (r[(size_t) t] > 0.9 * best && r[(size_t) t] >= r[(size_t) t - 1] && r[(size_t) t] >= r[(size_t) t + 1]) { bt = t; break; }
    const double a = r[(size_t) bt - 1], b = r[(size_t) bt], c = r[(size_t) bt + 1], den = a - 2 * b + c;
    const double tt = bt + (std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0);
    return fs / tt;
}

static double cents (double a, double b) { return 1200.0 * std::log2 (a / b); }

// energia numa faixa (DFT direta)
static double bandEnergy (const std::vector<float>& x, size_t start, size_t len, double fs, double f0, double f1)
{
    double e = 0;
    for (double f = f0; f <= f1; f += 20.0)
    {
        double re = 0, im = 0;
        for (size_t i = 0; i < len; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2 * M_PI * i / (double) len);
            re += x[start + i] * w * std::cos (2 * M_PI * f * i / fs);
            im -= x[start + i] * w * std::sin (2 * M_PI * f * i / fs);
        }
        e += re * re + im * im;
    }
    return e;
}

static std::vector<float> runEngine (double fs, const std::vector<float>& in, TuneSettings st, std::function<int (int64_t)> ov = {}, int* lat = nullptr, std::vector<HopInfo>* hops = nullptr)
{
    TuneEngine e;
    e.setSettings (st);
    e.prepare (fs);
    e.setSettings (st);
    if (ov) e.overrideAt = ov;
    if (hops) e.onHop = [hops] (const HopInfo& h) { hops->push_back (h); };
    std::vector<float> out (in.size());
    for (size_t i = 0; i < in.size(); ++i) out[i] = e.process (in[i]);
    if (lat) *lat = e.latency();
    return out;
}

int main()
{
    const int fsList[] = { 44100, 48000, 96000 };
    for (int fsi : fsList)
    {
        const double fs = fsi;
        std::printf ("\n== %d Hz\n", fsi);

        // --- 1. detecção
        for (double hz : { 98.0, 220.0, 440.0 })
        {
            PitchTracker tr; tr.prepare (fs); tr.setVoice (hz < 150 ? 0 : 1);
            SynthVoice v (fs); PitchResult pr; double last = 0;
            for (int i = 0; i < (int) (fs * 0.5); ++i) if (tr.push (v.next (hz), pr)) last = pr.hz;
            char msg[96]; std::snprintf (msg, sizeof msg, "detecta %.0f Hz (erro em cents)", hz);
            check (std::abs (cents (last, hz)) < 5.0, msg, cents (last, hz));
        }

        const uint16_t cMaior = scaleMask (0, Maior);
        const size_t N = (size_t) (fs * 2.0);

        // --- 2. robô: A3 desafinado +38 cents -> 220 Hz
        {
            SynthVoice v (fs); std::vector<float> in (N);
            const double hz = 220.0 * std::pow (2.0, 38.0 / 1200.0);
            for (size_t i = 0; i < N; ++i) in[i] = v.next (hz);
            TuneSettings st; st.amount = 1.0f; st.allowed = cMaior;
            int lat = 0;
            auto t0 = std::chrono::high_resolution_clock::now();
            auto out = runEngine (fs, in, st, {}, &lat);
            const double secs = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
            const double m = measureHz (out, (size_t) (fs * 1.0), (size_t) (fs * 0.25), fs);
            check (std::abs (cents (m, 220.0)) < 4.0, "robô: +38 cents vira A3 (erro em cents)", cents (m, 220.0));
            check (secs < 2.0 * 0.25, "CPU: 2 s de voz em menos de 0,5 s (s)", secs);
            check (lat > 0 && lat < (int) (0.04 * fs), "latência fixa < 40 ms (ms)", 1000.0 * lat / fs);
            if (fsi == 48000) { writeWav ("robo_antes.wav", in, fsi); writeWav ("robo_depois.wav", out, fsi); }

            // timbre: relação entre as faixas dos formantes 1 e 2 parecida antes e depois
            const size_t L = (size_t) (fs * 0.1), s0 = (size_t) (fs * 1.2);
            const double rIn  = 10 * std::log10 (bandEnergy (in,  s0, L, fs, 600, 800) / bandEnergy (in,  s0, L, fs, 1100, 1350));
            const double rOut = 10 * std::log10 (bandEnergy (out, s0 + (size_t) lat, L, fs, 600, 800) / bandEnergy (out, s0 + (size_t) lat, L, fs, 1100, 1350));
            check (std::abs (rIn - rOut) < 3.0, "timbre (formantes) preservado: diferença em dB", rIn - rOut);
        }

        // --- 3. clean: corrige quase tudo e sem degraus
        {
            SynthVoice v (fs); std::vector<float> in (N);
            for (size_t i = 0; i < N; ++i)
            {
                const double vib = 1.0 + 0.004 * std::sin (2 * M_PI * 5.5 * (double) i / fs);   // vibrato natural
                in[i] = v.next (329.63 * std::pow (2.0, -30.0 / 1200.0) * vib);                // E4 -30 cents
            }
            TuneSettings st; st.amount = 0.25f; st.allowed = cMaior;
            auto out = runEngine (fs, in, st);
            const double m = measureHz (out, (size_t) (fs * 1.2), (size_t) (fs * 0.5), fs);
            check (std::abs (cents (m, 329.63)) < 7.0, "clean: E4 -30 cents corrigido (erro em cents)", cents (m, 329.63));
            if (fsi == 48000) { writeWav ("clean_antes.wav", in, fsi); writeWav ("clean_depois.wav", out, fsi); }
        }

        // --- 4. transparência: voz já afinada sai igual (só atrasada)
        {
            SynthVoice v (fs); std::vector<float> in (N);
            for (size_t i = 0; i < N; ++i) in[i] = v.next (261.63);
            TuneSettings st; st.amount = 0.25f; st.allowed = cMaior;
            int lat = 0;
            auto out = runEngine (fs, in, st, {}, &lat);
            double e = 0, s = 0;
            for (size_t i = (size_t) fs; i < N - (size_t) lat; ++i) { const double d = out[i + (size_t) lat] - in[i]; e += d * d; s += (double) in[i] * in[i]; }
            const double snr = 10 * std::log10 (s / (e + 1e-12));
            check (snr > 30.0, "transparência: voz afinada quase intocada (SNR dB)", snr);
        }

        // --- 5. nota forçada no editor: canta D4, editor manda E
        {
            SynthVoice v (fs); std::vector<float> in (N);
            for (size_t i = 0; i < N; ++i) in[i] = v.next (293.66);
            TuneSettings st; st.amount = 0.6f; st.allowed = cMaior;
            auto out = runEngine (fs, in, st, [] (int64_t) { return 4; });
            const double m = measureHz (out, (size_t) (fs * 1.2), (size_t) (fs * 0.25), fs);
            check (std::abs (cents (m, 329.63)) < 6.0, "editor: D4 cantado vira E4 escolhido (erro em cents)", cents (m, 329.63));
        }

        // --- 6. dúvida: voz no meio de duas notas é marcada
        {
            SynthVoice v (fs); std::vector<float> in ((size_t) fs);
            for (size_t i = 0; i < in.size(); ++i) in[i] = v.next (220.0 * std::pow (2.0, 100.0 / 1200.0));   // A#3: entre A e B
            TuneSettings st; st.amount = 0.5f; st.allowed = cMaior;
            std::vector<HopInfo> hops;
            runEngine (fs, in, st, {}, nullptr, &hops);
            int doubts = 0, voiced = 0;
            for (auto& h : hops) if (h.midi > 0) { ++voiced; if (h.doubt) ++doubts; }
            check (voiced > 0 && doubts > voiced / 2, "dúvida marcada quando a voz cai entre duas notas (%)", voiced ? 100.0 * doubts / voiced : 0.0);
        }
    }

    // --- 7. escuta do riddim: Am - Dm - Em em loop, com baixo
    {
        const double fs = 48000;
        RiddimEar ear; ear.prepare (fs);
        const int prog[3][2] = { { 9, 1 }, { 2, 1 }, { 4, 1 } };
        double ph[4] = {};
        int chordOk = 0, chordN = 0;
        for (int bar = 0; bar < 24; ++bar)
        {
            const int r = prog[bar % 3][0];
            const int notes[1] = { r + 36 };   // baixo; tríade menor (raiz, 3ª m, 5ª) a partir do C4
            const int tri[4] = { 0, 0, 3, 7 };
            for (int i = 0; i < (int) (fs * 1.5); ++i)
            {
                float s = 0;
                for (int k = 0; k < 4; ++k)
                {
                    const double hz = 440.0 * std::pow (2.0, ((k == 0 ? notes[0] : 60 + r + tri[k]) - 69) / 12.0);
                    ph[k] += hz / fs; if (ph[k] > 1) ph[k] -= 1;
                    s += (float) (0.15 * std::sin (2 * M_PI * ph[k]) + 0.05 * std::sin (4 * M_PI * ph[k]));
                }
                if (ear.push (s) && i > (int) (fs * 0.7) && bar > 3)
                {
                    ++chordN;
                    if (ear.getChordRoot() == r % 12 && ear.getChordMinor()) ++chordOk;
                }
            }
        }
        const bool keyOk = (ear.getKeyRoot() == 9 && ear.getKeyMinor()) || (ear.getKeyRoot() == 0 && ! ear.getKeyMinor());
        std::printf ("\n== riddim: tom %d %s\n", ear.getKeyRoot(), ear.getKeyMinor() ? "menor" : "maior");
        check (keyOk, "riddim Am-Dm-Em: tom A menor (ou relativa C maior)", ear.getKeyRoot());
        check (chordN > 0 && chordOk > chordN * 0.8, "riddim: acorde do momento certo (%)", chordN ? 100.0 * chordOk / chordN : 0.0);
        check (ear.hasSignal(), "riddim: sinal presente", 1.0);
    }

    // --- 8. EQ e saturador estáveis
    {
        const double fs = 48000;
        VoiceEQ eq; eq.prepare (fs);
        EqSettings es; es.hp = 120; es.lp = 6000; es.g[0] = 6; es.g[1] = -6; es.g[2] = 9;
        eq.setSettings (es);
        Saturator sat; sat.prepare (fs);
        bool finite = true; float peak = 0;
        for (int m = 0; m < 3; ++m)
        {
            sat.set (m, 1.0f);
            for (int i = 0; i < 48000; ++i)
            {
                float x = 0.5f * (float) std::sin (2 * M_PI * 220 * i / fs);
                x = eq.process (0, x);
                x = sat.process (0, x);
                if (! std::isfinite (x)) finite = false;
                peak = std::max (peak, std::abs (x));
            }
        }
        check (finite && peak < 4.0f, "EQ + saturador (3 modos, drive 100%) estáveis: pico", peak);
        check (std::abs (VoiceEQ::responseDb (es, 30.0f)) > 20.0f, "curva do EQ: HP 120 Hz corta 30 Hz (dB)", VoiceEQ::responseDb (es, 30.0f));
    }

    std::printf ("\n%s (%d falha%s)\n", failures ? "TESTE FALHOU" : "TESTE OK", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
