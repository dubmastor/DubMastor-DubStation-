// LeslieDSP.h — a Leslie de dois rotores do Mastor Reggae Organ MRO-68 (header-only, sem JUCE)
//
// Igual à versão para celular (site/web/reggaeorgan/): crossover em 800 Hz; a CORNETA (agudos) e o TAMBOR (graves)
// giram separados, cada um com vibrato (atraso variável), tremolo (volume) e giro no estéreo. Ao trocar LENTA/RÁPIDA
// ou soltar o FREIO, cada rotor chega na velocidade nova no seu tempo: a corneta é leve e acelera rápido,
// o tambor é pesado e demora (curva exponencial, ~95% no tempo da tabela).
// Ao mudar a tabela, mude também a constante LES da versão para celular.
#pragma once

#include <atomic>
#include <cmath>
#include <vector>

namespace mro
{
struct LesliePreset
{
    const char* name;
    const char* sub;
    bool  on;
    float horn[2], drum[2];   // rotações por segundo: lenta, rápida
    float tH[2], tD[2];       // segundos para chegar na lenta / na rápida
    float depth;
    int   speed;              // velocidade que o preset liga (0 lenta, 1 rápida)
};

enum { NumLesliePresets = 6 };

inline const LesliePreset& lesliePreset (int i)
{
    static const LesliePreset t[NumLesliePresets] = {
        { "SEM LESLIE",   "som direto",            false, { 0, 0 },       { 0, 0 },       { 1, 1 },       { 1, 1 },       0.0f, 0 },
        { "122 CL\xc3\x81SSICA", "a do Hammond B3", true, { 0.83f, 6.7f }, { 0.67f, 5.8f }, { 0.9f, 0.6f },  { 3.5f, 2.4f }, 0.80f, 0 },
        { "ROOTS SUAVE",  "lenta, macia no chop",  true,  { 0.62f, 5.9f }, { 0.50f, 5.1f }, { 1.1f, 0.8f },  { 4.0f, 3.0f }, 0.55f, 0 },
        { "147 GRAVE",    "tambor mais presente",  true,  { 0.80f, 6.3f }, { 0.72f, 6.0f }, { 1.0f, 0.7f },  { 3.0f, 2.2f }, 0.90f, 0 },
        { "CHORALE",      "meio-termo, coral",     true,  { 1.60f, 6.7f }, { 1.30f, 5.8f }, { 0.8f, 0.6f },  { 2.6f, 2.0f }, 0.75f, 0 },
        { "DUB TREMOLO",  "r\xc3\xa1pida e funda", true,  { 0.83f, 7.2f }, { 0.67f, 6.4f }, { 0.7f, 0.45f }, { 2.8f, 1.8f }, 1.00f, 1 },
    };
    return t[i < 0 ? 0 : (i >= NumLesliePresets ? NumLesliePresets - 1 : i)];
}

// leslie dos músicos (0 sem, 1 lenta, 2 rápida) -> preset
inline int leslieFromMusician (int v) { return v == 2 ? 5 : (v == 1 ? 1 : 0); }

class LeslieDSP
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        const int len = (int) (0.02f * sr) + 8;
        for (auto& r : rot) { r.buf.assign ((size_t) len, 0.0f); r.w = 0; r.phase = 0; r.rate = 0; r.z1 = r.z2 = 0; }
        setXover();
        wetGain = 0.0f;
    }

    void set (int presetIndex, int speed, bool brake, float depth)
    {
        preset = presetIndex; spd = speed ? 1 : 0; brk = brake; dep = depth < 0 ? 0 : (depth > 1 ? 1 : depth);
    }

    // processa no lugar (estéreo)
    void process (float* L, float* R, int n)
    {
        const LesliePreset& P = lesliePreset (preset);
        const float wetTarget = P.on ? 1.0f : 0.0f;
        const float smooth = 1.0f - std::exp (-1.0f / (0.03f * sr));
        float tgt[2], coef[2];
        for (int k = 0; k < 2; ++k)
        {
            const float* rates = k == 0 ? P.horn : P.drum;
            const float* times = k == 0 ? P.tH : P.tD;
            tgt[k]  = (! P.on || brk) ? 0.0f : rates[spd];
            const float tau = std::fmax (0.01f, times[brk ? 0 : spd] / 3.0f);
            coef[k] = 1.0f - std::exp (-1.0f / (tau * sr));
        }
        const float d = P.on ? dep : 0.0f;
        static const float vib[2] = { 0.0007f, 0.00025f }, am[2] = { 0.38f, 0.22f }, pan[2] = { 0.75f, 0.3f }, base[2] = { 0.002f, 0.003f };
        const float twoPi = 6.28318530718f;

        for (int s = 0; s < n; ++s)
        {
            wetGain += (wetTarget - wetGain) * smooth;
            const float x = 0.5f * (L[s] + R[s]);
            float ol = 0, orr = 0;
            for (int k = 0; k < 2; ++k)
            {
                Rotor& r = rot[k];
                r.rate += (tgt[k] - r.rate) * coef[k];
                r.phase += twoPi * r.rate / sr;
                if (r.phase > twoPi) r.phase -= twoPi;
                const float sn = std::sin (r.phase);

                // crossover (biquad passa-altas na corneta, passa-baixas no tambor), forma direta II transposta
                const Bq& c = k == 0 ? hp : lp;
                const float y = c.b0 * x + r.z1;
                r.z1 = c.b1 * x - c.a1 * y + r.z2;
                r.z2 = c.b2 * x - c.a2 * y;

                // vibrato: atraso variável com interpolação linear
                const int len = (int) r.buf.size();
                r.buf[(size_t) r.w] = y;
                const float delay = (base[k] + vib[k] * d * sn) * sr;
                float rp = (float) r.w - delay;
                while (rp < 0) rp += (float) len;
                const int i0 = (int) rp; const float fr = rp - (float) i0;
                const float v = r.buf[(size_t) i0] * (1.0f - fr) + r.buf[(size_t) ((i0 + 1) % len)] * fr;
                if (++r.w >= len) r.w = 0;

                const float g = 1.0f - am[k] * d + am[k] * d * sn;          // tremolo
                const float p = pan[k] * d * sn;                              // -1..1
                const float a = (p + 1.0f) * 0.785398163f;                    // pan de potência constante
                ol  += v * g * std::cos (a) * 1.41421356f;
                orr += v * g * std::sin (a) * 1.41421356f;
            }
            L[s] = L[s] * (1.0f - wetGain) + ol * wetGain;
            R[s] = R[s] * (1.0f - wetGain) + orr * wetGain;
        }
        hornRate.store (rot[0].rate); drumRate.store (rot[1].rate);
        hornAngle.store (rot[0].phase); drumAngle.store (rot[1].phase);
    }

    // para a animação da tela
    std::atomic<float> hornRate { 0 }, drumRate { 0 }, hornAngle { 0 }, drumAngle { 0 };

private:
    struct Bq { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct Rotor { std::vector<float> buf; int w = 0; float phase = 0, rate = 0, z1 = 0, z2 = 0; };

    void setXover()
    {
        const float w0 = 6.28318530718f * 800.0f / sr, cw = std::cos (w0), al = std::sin (w0) / (2.0f * 0.6f);
        const float a0 = 1.0f + al;
        lp = { (1 - cw) / 2 / a0, (1 - cw) / a0, (1 - cw) / 2 / a0, -2 * cw / a0, (1 - al) / a0 };
        hp = { (1 + cw) / 2 / a0, -(1 + cw) / a0, (1 + cw) / 2 / a0, -2 * cw / a0, (1 - al) / a0 };
    }

    float sr = 48000.0f;
    Rotor rot[2];
    Bq lp, hp;
    int preset = 0, spd = 0;
    bool brk = false;
    float dep = 0.8f, wetGain = 0.0f;
};
} // namespace mro
