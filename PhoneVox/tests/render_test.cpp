// Teste do DSP do RádioFone: resposta em frequência, nível, estabilidade e demos em WAV.
// g++ -std=c++17 -O2 -Wall -Wextra tests/render_test.cpp -o render_test && ./render_test
#include "../Source/LoFiVoice.h"
#include "../Source/TapeEcho.h"
#include <cstdio>
#include <vector>
#include <string>

static const char* kNames[] = { "radio_am", "telefone", "megafone" };

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

// resposta de um modo com sinal baixo (quase sem saturação), sem ruído/defeito
static float gainAt (int mode, float hz, float fs = 48000.0f)
{
    rf::LoFiVoice v; rf::VoiceSettings s; s.mode = mode; s.drive = 0.0f; s.noise = 0; s.defect = 0;
    v.setSettings (s); v.prepare (fs); v.setSettings (s);
    const int N = (int) (fs * 0.6f);
    double acc = 0; int cnt = 0;
    for (int i = 0; i < N; ++i)
    {
        const float y = v.processSample (0.002f * std::sin (rf::kTau * hz * (float) i / fs));
        if (i > N / 2) { acc += (double) y * y; ++cnt; }
    }
    return 10.0f * std::log10 ((float) (acc / cnt) / (0.002f * 0.002f * 0.5f) + 1e-12f);
}

// "voz" sintética: pulso glotal com vibrato passando por formantes que mudam de vogal
static std::vector<float> makeVoice (int fs, float seconds)
{
    const int N = (int) (fs * seconds);
    std::vector<float> out (N);
    const float formants[5][3] = { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 270, 2300, 3000 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
    rf::SVF f1, f2, f3; rf::Noise nz;
    float ph = 0;
    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / fs;
        const int syl = (int) (t / 0.32f);
        const float inSyl = std::fmod (t, 0.32f) / 0.32f;
        const float* F = formants[syl % 5];
        if (i % 64 == 0)
        {
            f1.set (rf::SVF::BP, F[0], 6, (float) fs); f2.set (rf::SVF::BP, F[1], 8, (float) fs); f3.set (rf::SVF::BP, F[2], 10, (float) fs);
        }
        const float pitch = 140.0f * (1.0f + 0.15f * std::sin (rf::kTau * 0.4f * t)) * (1.0f + 0.01f * std::sin (rf::kTau * 5.5f * t));
        ph += pitch / fs; float pulse = 0;
        if (ph >= 1.0f) { ph -= 1.0f; pulse = 1.0f; }
        const float src = pulse + 0.01f * nz.white();
        const float envl = std::sin (rf::kPi * std::min (1.0f, inSyl * 1.15f)) * ((syl % 7 == 6) ? 0.0f : 1.0f);
        out[(size_t) i] = (f1.tick (src) * 1.0f + f2.tick (src) * 0.5f + f3.tick (src) * 0.25f) * envl * 1.6f;
    }
    return out;
}

static float rmsDb (const std::vector<float>& x)
{
    double a = 0; for (float v : x) a += (double) v * v;
    return 10.0f * std::log10 ((float) (a / (double) x.size()) + 1e-12f);
}

int main()
{
    int fails = 0;
    std::printf ("== Resposta em frequência (drive 0, dB relativo à entrada)\n");
    for (int m = 0; m < 3; ++m)
    {
        std::printf ("%-9s", kNames[m]);
        for (float hz : { 80.f, 200.f, 400.f, 1000.f, 2000.f, 3400.f, 5000.f, 8000.f }) std::printf (" %5gHz=%6.1f", hz, gainAt (m, hz));
        std::printf ("\n");
    }
    // telefone precisa cortar forte fora de 300–3400 Hz
    if (gainAt (1, 100.0f) > gainAt (1, 1500.0f) - 24.0f) { std::printf ("FALHA: telefone deixa passar grave\n"); ++fails; }
    if (gainAt (1, 8000.0f) > gainAt (1, 1500.0f) - 24.0f) { std::printf ("FALHA: telefone deixa passar agudo\n"); ++fails; }

    const int fs = 44100;
    const auto voice = makeVoice (fs, 8.0f);
    // normaliza a voz para -18 dBFS RMS
    std::vector<float> vin = voice; { const float g = std::pow (10.0f, (-18.0f - rmsDb (vin)) / 20.0f); for (auto& v : vin) v *= g; }
    std::printf ("\n== Nível com a voz em %.1f dBFS RMS (padrões: drive 35%%, chiado 25%%, defeito 20%%)\n", rmsDb (vin));
    writeWav ("voz_original.wav", vin, vin, fs);

    for (int m = 0; m < 3; ++m)
    {
        for (float drive : { 0.0f, 0.35f, 1.0f })
        {
            rf::LoFiVoice v; rf::VoiceSettings s; s.mode = m; s.drive = drive;
            v.prepare (fs); v.setSettings (s); v.reset();
            std::vector<float> y (vin.size());
            float peak = 0; bool bad = false;
            for (size_t i = 0; i < vin.size(); ++i)
            {
                y[i] = v.processSample (vin[i]);
                if (! std::isfinite (y[i])) bad = true;
                peak = std::max (peak, std::fabs (y[i]));
            }
            const float lvl = rmsDb (y);
            std::printf ("%-9s drive %3d%%: RMS %6.1f dBFS  pico %5.2f\n", kNames[m], (int) (drive * 100), lvl, peak);
            if (bad || peak > 2.0f || lvl < -32.0f || lvl > -6.0f) { std::printf ("FALHA: nível fora do esperado\n"); ++fails; }
            if (drive == 0.35f)
            {
                // demo: aparelho + eco de fita (cabeçote 1+2, 3/8 a 100 BPM)
                dub::TapeEcho echo; echo.prepare (fs);
                dub::EchoSettings es; es.delaySec = 0.45f; es.feedback = 0.55f; es.send = 0.5f; es.heads = 3; es.pingPong = true;
                echo.setSettings (es); echo.reset();
                std::vector<float> L (y.size() + (size_t) fs * 3), R (L.size());
                for (size_t i = 0; i < L.size(); ++i)
                {
                    const float x = i < y.size() ? y[i] : 0.0f;
                    float el, er; echo.processSample (x, x, el, er);
                    L[i] = x + el; R[i] = x + er;
                }
                writeWav (std::string ("demo_") + kNames[m] + ".wav", L, R, fs);
            }
        }
    }

    // troca de modo no meio do som: sem salto grande entre amostras
    {
        rf::LoFiVoice v; rf::VoiceSettings s; v.prepare (fs); v.setSettings (s); v.reset();
        float maxJump = 0, last = 0;
        for (size_t i = 0; i < vin.size(); ++i)
        {
            if (i % (size_t) (fs / 2) == 0) { s.mode = (s.mode + 1) % 3; v.setSettings (s); }
            const float y = v.processSample (vin[i]);
            maxJump = std::max (maxJump, std::fabs (y - last)); last = y;
        }
        std::printf ("\n== Troca de modo a cada 0,5 s: maior salto entre amostras %.3f\n", maxJump);
        if (maxJump > 1.2f) { std::printf ("FALHA: estalo na troca de modo\n"); ++fails; }
    }

    // silêncio com chiado no máximo não pode explodir
    {
        for (int m = 0; m < 3; ++m)
        {
            rf::LoFiVoice v; rf::VoiceSettings s; s.mode = m; s.noise = 1; s.defect = 1; s.drive = 1;
            v.prepare (fs); v.setSettings (s); v.reset();
            float peak = 0;
            for (int i = 0; i < fs * 4; ++i) peak = std::max (peak, std::fabs (v.processSample (0.0f)));
            std::printf ("%-9s silêncio, chiado/defeito/drive no máximo: pico %.3f\n", kNames[m], peak);
            if (! std::isfinite (peak) || peak > 1.5f) { std::printf ("FALHA\n"); ++fails; }
        }
    }

    std::printf (fails ? "\n%d FALHA(S)\n" : "\nTudo certo.\n", fails);
    return fails ? 1 : 0;
}
