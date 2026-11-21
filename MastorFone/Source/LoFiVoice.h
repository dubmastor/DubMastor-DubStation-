#pragma once
// Simulação de aparelhos de voz: RÁDIO AM, TELEFONE antigo e MEGAFONE.
// Independente do JUCE. Mono por dentro (os três aparelhos são mono).

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <iterator>

namespace rf
{

static constexpr float kPi = 3.14159265359f;
static constexpr float kTau = 6.28318530718f;

//==============================================================================
// SVF (Simper/Cytomic, trapezoidal): estável ao girar os controles ao vivo
struct SVF
{
    enum Type { LP, HP, BP, Bell };
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, m0 = 0, m1 = 0, m2 = 0;

    void set (Type t, float hz, float q, float fs, float gainDb = 0.0f)
    {
        hz = std::clamp (hz, 10.0f, 0.45f * fs);
        const float g = std::tan (kPi * hz / fs);
        float k = 1.0f / q;
        if (t == Bell)
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            k = 1.0f / (q * A);
            m0 = 1; m1 = k * (A * A - 1.0f); m2 = 0;
        }
        else if (t == LP) { m0 = 0; m1 = 0;  m2 = 1; }
        else if (t == HP) { m0 = 1; m1 = -k; m2 = -1; }
        else              { m0 = 0; m1 = k;  m2 = 0; }  // BP com pico unitário
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    inline float tick (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        return m0 * x + m1 * v1 + m2 * v2;
    }
    void reset() { ic1 = ic2 = 0; }
};

// gerador de ruído rápido (xorshift)
struct Noise
{
    uint32_t s = 0x9E3779B9u;
    inline float white() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * (1.0f / 2147483648.0f); }
    inline float uni()   { return 0.5f * (white() + 1.0f); }
};

// seguidor de envelope (ataque/soltura)
struct Env
{
    float z = 0, a = 0, r = 0;
    void prepare (float fs, float atkMs, float relMs) { a = 1.0f - std::exp (-1.0f / (atkMs * 0.001f * fs)); r = 1.0f - std::exp (-1.0f / (relMs * 0.001f * fs)); }
    inline float tick (float x) { x = std::fabs (x); z += (x > z ? a : r) * (x - z); return z; }
};

//==============================================================================
struct VoiceSettings
{
    int   mode   = 0;      // 0 rádio AM, 1 telefone, 2 megafone
    float tune   = 0.0f;   // SINTONIA -1..1: desloca a faixa (±1 oitava)
    float band   = 0.5f;   // BANDA 0..1: estreita <-> larga
    float drive  = 0.35f;  // 0..1
    float noise  = 0.25f;  // CHIADO 0..1
    float defect = 0.2f;   // DEFEITO 0..1 (fading do AM, linha falhando, amplificador vagabundo)
    float mix    = 1.0f;   // 0..1
};

// Um aparelho. Os três rodam sempre, e a troca de modo é um crossfade (sem estalo).
class Device
{
public:
    explicit Device (int m) : mode (m) {}

    void prepare (float sampleRate)
    {
        fs = sampleRate;
        env.prepare (fs, 2.0f, 120.0f);
        comp.prepare (fs, 4.0f, 90.0f);
        noiseRng.s = 0x1234567u + (uint32_t) mode * 7919u;
        reset();
    }

    void reset()
    {
        for (auto* f : { &hp1, &hp2, &lp1, &lp2, &pk1, &pk2, &post1, &post2, &nz1, &nz2, &humLp })
            f->reset();
        env.z = comp.z = 0; hold = 0; holdPh = 0; fadePh = 0; whistlePh = 0; humPh = 0; humPh2 = 0;
        dropT = 0; dropGain = 1; crackle = 0;
        std::fill (std::begin (horn), std::end (horn), 0.0f); hornW = 0;
    }

    void setSettings (const VoiceSettings& s)
    {
        st = s;
        const float shift = std::exp2 (std::clamp (s.tune, -1.0f, 1.0f));
        const float b = std::clamp (s.band, 0.0f, 1.0f);
        switch (mode)
        {
            case 0: // RÁDIO AM: ~150 Hz–4.5 kHz, falante de gabinete com médio na cara
            {
                const float lo = 260.0f * std::exp2 (-1.2f * b), hi = 2800.0f * std::exp2 (1.1f * b);
                hp1.set (SVF::HP, lo * shift, 0.707f, fs); hp2.set (SVF::HP, lo * shift, 0.9f, fs);
                lp1.set (SVF::LP, hi * shift, 0.707f, fs); lp2.set (SVF::LP, hi * shift, 1.0f, fs);
                pk1.set (SVF::Bell, 1300.0f * shift, 0.9f, fs, 4.0f);
                pk2.set (SVF::Bell, 180.0f * shift, 1.2f, fs, 2.5f);   // corpo do gabinete de madeira
                post1.set (SVF::LP, hi * shift * 1.15f, 0.707f, fs); post2.set (SVF::HP, 90.0f, 0.707f, fs);
                break;
            }
            case 1: // TELEFONE: 300–3400 Hz bem fechado, cápsula de carvão
            {
                const float lo = 420.0f * std::exp2 (-0.8f * b), hi = 2600.0f * std::exp2 (0.65f * b);
                hp1.set (SVF::HP, lo * shift, 0.8f, fs); hp2.set (SVF::HP, lo * shift, 1.1f, fs);
                lp1.set (SVF::LP, hi * shift, 0.8f, fs); lp2.set (SVF::LP, hi * shift, 1.2f, fs);
                pk1.set (SVF::Bell, 1800.0f * shift, 1.4f, fs, 6.0f);
                pk2.set (SVF::Bell, 900.0f * shift, 1.0f, fs, 2.0f);
                post1.set (SVF::LP, hi * shift * 1.1f, 0.9f, fs); post2.set (SVF::HP, lo * shift * 0.8f, 0.707f, fs);
                holdRate = (6000.0f + 6000.0f * b) * shift;              // reamostragem de central antiga
                break;
            }
            default: // MEGAFONE: corneta com ressonâncias fortes, amplificador estourado
            {
                const float lo = 650.0f * std::exp2 (-1.0f * b), hi = 3600.0f * std::exp2 (0.8f * b);
                hp1.set (SVF::HP, lo * shift, 0.9f, fs); hp2.set (SVF::HP, lo * shift, 1.3f, fs);
                lp1.set (SVF::LP, hi * shift, 0.9f, fs); lp2.set (SVF::LP, hi * shift, 1.4f, fs);
                pk1.set (SVF::Bell, 1150.0f * shift, 2.2f, fs, 9.0f);
                pk2.set (SVF::Bell, 2500.0f * shift, 3.0f + 5.0f * s.defect, fs, 6.0f + 4.0f * s.defect);
                post1.set (SVF::LP, hi * shift * 1.2f, 0.9f, fs); post2.set (SVF::HP, lo * shift * 0.7f, 0.707f, fs);
                hornLen = std::clamp ((int) (0.00085f * fs / shift), 4, kHornMax - 1);
                break;
            }
        }
        // o chiado passa por uma faixa parecida com a do aparelho
        nz1.set (SVF::BP, (mode == 0 ? 2500.0f : mode == 1 ? 1700.0f : 3000.0f) * shift, 0.6f, fs);
        nz2.set (SVF::HP, 300.0f, 0.707f, fs);
        humLp.set (SVF::LP, 900.0f, 0.707f, fs);
    }

    // retorna o som do aparelho (sem o seco)
    inline float process (float x)
    {
        const float e = env.tick (x);
        const float d = st.drive;
        const float pre = std::pow (10.0f, (6.0f + 30.0f * d) / 20.0f);  // +6..+36 dB antes da saturação

        float y = hp2.tick (hp1.tick (x));
        y = pk2.tick (pk1.tick (y));
        y = lp2.tick (lp1.tick (y));

        // --- ruído característico de cada aparelho
        const float voiceGate = 0.25f + 0.75f * std::min (1.0f, e * 8.0f);
        float nz = 0.0f;
        if (st.noise > 0.0005f)
        {
            float hiss = nz2.tick (nz1.tick (noiseRng.white())) * 0.05f;
            // estalos (crackle)
            const float crackleProb = (mode == 1 ? 18.0f : mode == 0 ? 10.0f : 3.0f) * st.noise / fs;
            if (noiseRng.uni() < crackleProb) crackle = (noiseRng.white() > 0 ? 1.0f : -1.0f) * (0.2f + 0.6f * noiseRng.uni());
            crackle *= 0.86f;
            float hum = 0.0f;
            if (mode != 0)
            {
                humPh += (mode == 1 ? 60.0f : 120.0f) / fs; if (humPh >= 1.0f) humPh -= 1.0f;
                const float saw = 2.0f * humPh - 1.0f;   // zumbido rico em harmônicos
                hum = humLp.tick (mode == 1 ? std::sin (kTau * humPh) : saw) * (mode == 1 ? 0.012f : 0.02f);
            }
            nz = (hiss * (mode == 2 ? 0.4f : 1.0f) + crackle * 0.07f) * voiceGate + hum;
            nz *= st.noise * st.noise * 6.0f;
        }

        // --- DEFEITO
        float gain = 1.0f;
        if (mode == 0)
        {
            // fading do AM (onda que vai e volta) + assobio de heterodino bem baixinho
            fadePh += kTau * (0.17f + 0.11f * std::sin (fadePh * 0.31f)) / fs; if (fadePh > 1000.0f * kTau) fadePh -= 1000.0f * kTau;
            const float f = 0.5f + 0.5f * std::sin (fadePh) * std::sin (fadePh * 0.43f + 1.0f);
            gain = 1.0f - 0.7f * st.defect * f;
            whistlePh += kTau * (3100.0f + 400.0f * std::sin (fadePh * 0.2f)) / fs; if (whistlePh > kTau) whistlePh -= kTau;
            nz += std::sin (whistlePh) * 0.006f * st.defect * st.defect;
        }
        else if (mode == 1)
        {
            // linha falhando: quedas curtas de volume
            if (dropT <= 0 && noiseRng.uni() < st.defect * st.defect * 1.6f / fs)
                dropT = (int) ((0.03f + 0.15f * noiseRng.uni()) * fs);
            if (dropT > 0) --dropT;
            const float target = dropT > 0 ? 0.08f : 1.0f;
            dropGain += (dropT > 0 ? 0.02f : 0.002f) * (target - dropGain);
            gain = dropGain;
        }
        else
        {
            // amplificador vagabundo: ondulação da fonte (120 Hz) modulando o volume
            humPh2 += 120.0f / fs; if (humPh2 >= 1.0f) humPh2 -= 1.0f;
            gain = 1.0f - 0.35f * st.defect * (0.5f + 0.5f * std::sin (kTau * humPh2));
        }

        y *= gain;

        // --- saturação de cada aparelho
        float s = y * pre;
        if (mode == 0)
        {
            // válvula: assimétrica e macia
            const float bias = 0.25f;
            s = (std::tanh (s + bias) - std::tanh (bias)) / (1.0f - std::tanh (bias) * std::tanh (bias));
        }
        else if (mode == 1)
        {
            // cápsula de carvão: cortante, depois reamostragem + mu-law 8 bits
            s = s / (1.0f + std::fabs (s));
            s = std::clamp (s * 1.6f, -1.0f, 1.0f);
            holdPh += holdRate / fs;
            if (holdPh >= 1.0f) { holdPh -= std::floor (holdPh); hold = s; }
            s = muLaw8 (hold);
        }
        else
        {
            // megafone: clipa forte e duro
            s = std::clamp (s * 1.5f, -1.0f, 1.0f);
            s = 1.5f * s - 0.5f * s * s * s;
            // reflexão curta dentro da corneta (som de lata)
            const int r = (hornW - hornLen + kHornMax) % kHornMax;
            const float refl = horn[r];
            horn[hornW] = s + refl * 0.45f;
            hornW = (hornW + 1) % kHornMax;
            s = s + 0.55f * refl;
        }
        s = post2.tick (post1.tick (s + nz));   // o chiado entra depois da saturação: o DRIVE não o amplifica

        // compressor (os aparelhos são bem achatados) + compensação de ganho da saturação
        const float c = comp.tick (s);
        const float thr = 0.12f;
        const float cg = c > thr ? std::pow (thr / c, 0.6f) : 1.0f;
        const float out = s * cg * makeup() * std::pow (10.0f, (mode == 2 ? -2.0f : -5.0f) * d / 20.0f);
        return softLimit (out);
    }

private:
    static float muLaw8 (float x)
    {
        constexpr float mu = 255.0f;
        const float sgn = x < 0 ? -1.0f : 1.0f;
        float v = sgn * std::log1p (mu * std::fabs (x)) / std::log1p (mu);
        v = std::round (v * 127.0f) / 127.0f;              // 8 bits
        return (v < 0 ? -1.0f : 1.0f) * (std::pow (1.0f + mu, std::fabs (v)) - 1.0f) / mu;
    }
    float makeup() const { return mode == 0 ? 1.15f : mode == 1 ? 1.05f : 0.8f; }
    // teto macio em ~0,9 (os picos que o compressor deixa passar)
    static float softLimit (float x)
    {
        const float a = std::fabs (x);
        if (a < 0.6f) return x;
        const float o = 0.6f + 0.3f * std::tanh ((a - 0.6f) / 0.3f);
        return x < 0 ? -o : o;
    }

    static constexpr int kHornMax = 512;
    const int mode;
    float fs = 44100.0f;
    VoiceSettings st;
    SVF hp1, hp2, lp1, lp2, pk1, pk2, post1, post2, nz1, nz2, humLp;
    Env env, comp;
    Noise noiseRng;
    float hold = 0, holdPh = 0, holdRate = 8000.0f;
    float fadePh = 0, whistlePh = 0, humPh = 0, humPh2 = 0, dropGain = 1, crackle = 0;
    int dropT = 0;
    float horn[kHornMax] {};
    int hornW = 0, hornLen = 40;
};

//==============================================================================
class LoFiVoice
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        for (auto& d : dev) d.prepare (fs);
        xfA = 1.0f - std::exp (-1.0f / (0.025f * fs));   // 25 ms entre aparelhos
        mixA = 1.0f - std::exp (-1.0f / (0.02f * fs));
        reset();
    }
    void reset()
    {
        for (auto& d : dev) d.reset();
        for (int i = 0; i < 3; ++i) w[i] = (i == s.mode ? 1.0f : 0.0f);
        mixSm = s.mix;
    }
    void setSettings (const VoiceSettings& n)
    {
        s = n; s.mode = std::clamp (n.mode, 0, 2);
        for (auto& d : dev) d.setSettings (s);
    }

    // mono in -> mono out (seco/molhado conforme MIX)
    inline float processSample (float x)
    {
        float y = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            w[i] += xfA * ((i == s.mode ? 1.0f : 0.0f) - w[i]);
            if (w[i] > 1e-4f || i == s.mode) y += w[i] * dev[i].process (x);
        }
        mixSm += mixA * (s.mix - mixSm);
        return x * (1.0f - mixSm) + y * mixSm;
    }

private:
    float fs = 44100.0f, xfA = 0, mixA = 0, mixSm = 1;
    float w[3] { 1, 0, 0 };
    VoiceSettings s;
    Device dev[3] { Device (0), Device (1), Device (2) };
};

} // namespace rf
