// RootsKeysEngine.h
// Motor de síntese header-only (sem dependências) para o plugin "Dub Station Roots Keys".
//  - Piano estilo Korg M1 (síntese aditiva com inarmonicidade, ataque brilhante,
//    decaimento em dois estágios, ruído de martelo e chorus estéreo)
//  - Órgão tonewheel estilo Hammond B3 (9 drawbars, percussão single-trigger,
//    key click, overdrive valvulado e simulação de caixa Leslie)
//  - Reverb estilo Freeverb no master
//  - Eco de fita FitaEco TE-12 (o mesmo do Dub Station 2.1: 3 cabeças + mola)
//  - Maximizer com lookahead no final da cadeia
// Nenhum sample é usado: todo o timbre é sintetizado.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "TapeEchoDSP.h"

namespace rk
{
constexpr float kTwoPi = 6.28318530718f;

inline float clampf (float x, float lo, float hi) { return std::min (hi, std::max (lo, x)); }
inline float midiToHz (float n) { return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f); }

// ---------------------------------------------------------------- utilidades
struct SineTable
{
    static constexpr int N = 4096;
    float t[N + 1];
    SineTable() { for (int i = 0; i <= N; ++i) t[i] = std::sin (kTwoPi * (float) i / (float) N); }
    inline float operator() (float ph) const   // ph em [0,1)
    {
        const float x = ph * (float) N;
        const int i = (int) x;
        const float f = x - (float) i;
        return t[i] + f * (t[i + 1] - t[i]);
    }
};
inline const SineTable& sine() { static SineTable s; return s; }

struct Rng
{
    uint32_t s = 0x12345678u;
    inline float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 8388608.0f - 1.0f; }
    inline float uni() { return 0.5f * (next() + 1.0f); }
};

// State-variable filter (Chamberlin/TPT simplificado)
struct SVF
{
    float g = 0, k = 1, ic1 = 0, ic2 = 0;
    void set (float fc, float q, float sr)
    {
        g = std::tan (3.14159265f * clampf (fc, 20.0f, 0.45f * sr) / sr);
        k = 1.0f / q;
    }
    void reset() { ic1 = ic2 = 0; }
    // retorna band-pass; lp/hp via ponteiros opcionais
    inline float bp (float x, float* lp = nullptr, float* hp = nullptr)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + g * a1 * v3;
        const float v2 = ic2 + g * v1;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        if (lp) *lp = v2;
        if (hp) *hp = x - k * v1 - v2;
        return v1;
    }
};

struct DelayLine
{
    std::vector<float> buf;
    int w = 0, mask = 0;
    void init (int minSize)
    {
        int n = 1;
        while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float delaySamples) const   // leitura fracionária
    {
        float rp = (float) w - 1.0f - delaySamples;
        while (rp < 0) rp += (float) (mask + 1);
        const int i0 = (int) rp;
        const float f = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        return a + f * (b - a);
    }
};

// ---------------------------------------------------------------- parâmetros
struct Params
{
    int   mode = 0;            // 0 = Piano, 1 = Órgão
    // Piano
    float pianoLevel  = 0.8f;
    float pianoBright = 0.6f;
    float pianoDecay  = 0.5f;
    float pianoChorus = 0.5f;
    // Órgão
    float drawbar[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 }; // 16' 5⅓' 8' 4' 2⅔' 2' 1⅗' 1⅓' 1'
    bool  percOn   = false;
    bool  perc3rd  = true;     // false = 2º harmônico
    bool  percFast = true;
    float click    = 0.35f;
    bool  leslieFast = false;
    float leslieMix  = 0.85f;
    float drive      = 0.2f;
    float organLevel = 0.7f;
    // Tape Echo — FitaEco TE-12 (mesmo DSP do Dub Station 2.1)
    bool  echoOn        = false;
    int   echoMode      = 7;       // 1..12 (1-4 só eco, 5-11 eco + mola, 12 só mola)
    float echoRate      = 0.5f;    // velocidade da fita
    float echoIntensity = 0.45f;   // feedback (>~0.85 auto-oscila)
    float echoVol       = 0.6f;
    float echoSpring    = 0.3f;
    float echoWow       = 0.35f;
    float echoAge       = 0.3f;
    // Flanger estilo dub (o "avião a jato" do Mad Professor) — na mistura inteira
    bool  flOn       = false;
    float flRate     = 0.12f;   // Hz (0.02..4)
    float flDepth    = 0.8f;    // amplitude da varredura
    float flManual   = 0.3f;    // atraso base (posição do "pente")
    float flFeedback = 0.65f;   // -0.95..0.95 (regeneração; negativo = timbre oco)
    float flMix      = 0.5f;
    // Master / Maximizer
    float reverb = 0.18f;
    float master = 0.8f;
    bool  maxOn      = true;
    float maxGainDb  = 3.0f;
    float maxCeilDb  = -0.3f;
};

// ---------------------------------------------------------------- MAXIMIZER
struct Maximizer
{
    DelayLine dlL, dlR;
    std::vector<float> req;
    int la = 64, ri = 0;
    float env = 1, attCoef = 0, relCoef = 0;
    float grDb = 0;

    void prepare (float sr)
    {
        la = std::max (8, (int) (0.0015f * sr));
        dlL.init (la + 8); dlR.init (la + 8);
        req.assign ((size_t) la, 1.0f); ri = 0; env = 1;
        attCoef = 1.0f - std::exp (-3.0f / (float) la);
        relCoef = 1.0f - std::exp (-1.0f / (0.08f * sr));
    }

    inline void process (float& L, float& R, bool on, float gain, float ceil)
    {
        float r = 1.0f;
        if (on)
        {
            L *= gain; R *= gain;
            const float pk = std::max (std::fabs (L), std::fabs (R));
            if (pk > ceil) r = ceil / pk;
        }
        req[(size_t) ri] = r; if (++ri >= la) ri = 0;
        float m = 1.0f;
        for (float v : req) m = std::min (m, v);
        env += (m - env) * (m < env ? attCoef : relCoef);

        dlL.push (L); dlR.push (R);
        L = dlL.read ((float) la);
        R = dlR.read ((float) la);
        if (on)
        {
            L = clampf (L * env, -ceil, ceil);
            R = clampf (R * env, -ceil, ceil);
        }
        grDb = 20.0f * std::log10 (std::max (env, 1.0e-4f));
    }
};

// ---------------------------------------------------------------- PIANO (estilo M1)
// ---------------------------------------------------------------- FLANGER (jet / dub)
struct Flanger
{
    DelayLine dlL, dlR;
    float sr = 44100, ph = 0, fbL = 0, fbR = 0, lpL = 0, lpR = 0, wet = 0, lfoSmL = 0, lfoSmR = 0;

    void prepare (float s)
    {
        sr = s;
        dlL.init ((int) (0.03f * s)); dlR.init ((int) (0.03f * s));
        ph = 0; fbL = fbR = lpL = lpR = wet = lfoSmL = lfoSmR = 0;
    }

    static inline float tri (float x) { x -= std::floor (x); return x < 0.5f ? 2.0f * x : 2.0f - 2.0f * x; }

    // processa no lugar; "on" abre/fecha suavemente o efeito
    inline void process (float& L, float& R, const Params& p)
    {
        const float target = p.flOn ? p.flMix : 0.0f;
        wet += (target - wet) * 0.0015f;
        if (! p.flOn && wet < 1.0e-4f) { dlL.push (L); dlR.push (R); fbL = fbR = 0; return; }

        ph += clampf (p.flRate, 0.02f, 4.0f) / sr;
        if (ph >= 1.0f) ph -= 1.0f;
        // triângulo suavizado: varredura "lenta e larga" típica de dub
        lfoSmL += (tri (ph) - lfoSmL) * 0.01f;
        lfoSmR += (tri (ph + 0.25f) - lfoSmR) * 0.01f;

        const float dMin = (0.15f + clampf (p.flManual, 0.0f, 1.0f) * 2.5f) * 0.001f * sr;
        const float dMax = std::min (dMin * (1.0f + clampf (p.flDepth, 0.0f, 1.0f) * 22.0f), 0.012f * sr);
        const float dL = dMin * std::pow (dMax / dMin, lfoSmL);
        const float dR = dMin * std::pow (dMax / dMin, lfoSmR);

        const float fb = clampf (p.flFeedback, -0.95f, 0.95f);
        dlL.push (L + std::tanh (fbL * fb));
        dlR.push (R + std::tanh (fbR * fb));
        const float yL = dlL.read (dL), yR = dlR.read (dR);
        lpL += (yL - lpL) * 0.6f; lpR += (yR - lpR) * 0.6f;   // leve escurecimento na regeneração
        fbL = lpL; fbR = lpR;

        const float makeup = 1.0f + 0.3f * wet;
        L = (L * (1.0f - wet) + 0.5f * wet * (L + yL)) * makeup;
        R = (R * (1.0f - wet) + 0.5f * wet * (R + yR)) * makeup;
    }
};

struct PianoVoice
{
    static constexpr int MaxP = 28;
    bool active = false, held = false, releasing = false;
    int note = -1, np = 0;
    uint64_t age = 0;
    float ph[MaxP], inc[MaxP], e1[MaxP], e2[MaxP], c1[MaxP], c2[MaxP];
    float att = 0, attInc = 0, rel = 1, relCoef = 1;
    float nzEnv = 0, nzCoef = 0, nzLvl = 0;
    SVF nzFilt;
    Rng rng;

    void start (int n, float vel, float sr, const Params& p, uint64_t stamp)
    {
        active = true; held = true; releasing = false; note = n; age = stamp;
        rng.s = 0x9e3779b9u * (uint32_t) (n + 1) + (uint32_t) stamp;

        const float f    = midiToHz ((float) n);
        const float v    = clampf (vel, 0.0f, 1.0f);
        const float br   = p.pianoBright;
        const float B    = 0.00012f * std::pow (2.0f, ((float) n - 60.0f) / 16.0f); // inarmonicidade
        const float pw   = clampf (2.0f - 0.9f * br - 0.7f * v, 0.55f, 2.2f);       // inclinação espectral
        const float tBase = clampf ((1.2f + 10.0f * p.pianoDecay) * std::pow (2.0f, -((float) n - 60.0f) / 18.0f), 0.25f, 22.0f);
        const float gain = 0.16f * (0.2f + 0.8f * std::pow (v, 1.3f));

        np = 0;
        for (int k = 1; k <= MaxP; ++k)
        {
            const float fk = (float) k * f * std::sqrt (1.0f + B * (float) (k * k));
            if (fk > std::min (0.42f * sr, 15000.0f)) break;
            float a = std::pow ((float) k, -pw);
            // "presença" característica do piano do M1 (brilho na região 2–4 kHz)
            const float d = (fk - 2800.0f) / 1600.0f;
            a *= 1.0f + 1.1f * br * std::exp (-d * d);
            // leve cancelamento de harmônicos pela posição do martelo (~1/8 da corda)
            a *= 0.55f + 0.45f * std::fabs (std::sin (3.14159265f * (float) k / 8.3f));

            const float t2 = tBase / (1.0f + 0.12f * (float) (k - 1) + 1.6f * std::pow (fk / 1000.0f, 1.25f));
            const float t1 = std::min (0.35f, 0.04f + t2 * 0.08f);
            const float w  = clampf (0.30f + 0.045f * (float) k, 0.0f, 0.93f);

            ph[np]  = rng.uni();
            inc[np] = fk / sr;
            e1[np]  = gain * a * w;
            e2[np]  = gain * a * (1.0f - w);
            c1[np]  = std::exp (-1.0f / (t1 * sr));
            c2[np]  = std::exp (-1.0f / (t2 * sr));
            ++np;
        }

        att = 0; attInc = 1.0f / (0.0012f * sr);
        rel = 1; relCoef = 1;
        nzEnv = 1; nzCoef = std::exp (-1.0f / (0.010f * sr));
        nzLvl = 0.10f * v * v * (0.4f + 0.6f * br);
        nzFilt.reset();
        nzFilt.set (std::min (6.0f * f, 7000.0f), 0.9f, sr);
    }

    void release (float sr)
    {
        held = false; releasing = true;
        const float t = note > 88 ? 1.2f : 0.11f;   // notas agudas sem abafador
        relCoef = std::exp (-1.0f / (t * sr));
    }

    inline float tick()
    {
        const SineTable& S = sine();
        float s = 0;
        for (int i = 0; i < np; ++i)
        {
            s += S (ph[i]) * (e1[i] + e2[i]);
            e1[i] *= c1[i]; e2[i] *= c2[i];
            ph[i] += inc[i]; if (ph[i] >= 1.0f) ph[i] -= 1.0f;
        }
        const float nz = nzFilt.bp (rng.next()) * nzEnv * nzLvl;
        nzEnv *= nzCoef;
        att = std::min (1.0f, att + attInc);
        const float out = (s + nz) * att * rel;
        if (releasing) rel *= relCoef;
        if (rel < 1.0e-4f || (np > 0 && e2[0] < 1.0e-6f && nzEnv < 1.0e-4f)) active = false;
        return out;
    }
};

struct StereoChorus
{
    DelayLine dl;
    float ph = 0, rate = 0.0f, sr = 44100;
    void prepare (float s) { sr = s; dl.init ((int) (0.05f * s)); rate = 0.55f / s; }
    inline void process (float x, float mix, float& l, float& r)
    {
        dl.push (x);
        const SineTable& S = sine();
        float pr = ph + 0.25f; if (pr >= 1) pr -= 1;
        const float base = 0.007f * sr, depth = 0.0022f * sr;
        const float wl = dl.read (base + depth * S (ph));
        const float wr = dl.read (base + depth * S (pr));
        ph += rate; if (ph >= 1) ph -= 1;
        l = x * (1.0f - 0.5f * mix) + wl * mix * 0.7f;
        r = x * (1.0f - 0.5f * mix) + wr * mix * 0.7f;
    }
};

// ---------------------------------------------------------------- ÓRGÃO (estilo Hammond)
constexpr float kDrawbarRatio[9] = { 0.5f, 1.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };

struct OrganVoice
{
    bool active = false, held = false;
    int note = -1;
    uint64_t age = 0;
    float ph[9], inc[9];
    float env = 0, envTarget = 0, envCoefA = 0, envCoefR = 0;
    float clickEnv = 0, clickCoef = 0, clickLvl = 0;
    float percPh = 0, percInc = 0, percEnv = 0, percCoef = 0;
    SVF clickFilt;
    Rng rng;

    static float fold (float fk)
    {
        while (fk > 5900.0f) fk *= 0.5f;   // foldback das rodas fônicas no topo
        while (fk < 32.0f) fk *= 2.0f;     // e no grave
        return fk;
    }

    void start (int n, float sr, const Params& p, bool percTrig, uint64_t stamp)
    {
        active = true; held = true; note = n; age = stamp;
        rng.s = 0x85ebca6bu * (uint32_t) (n + 7) + (uint32_t) stamp;
        const float f = midiToHz ((float) n);
        for (int i = 0; i < 9; ++i)
        {
            inc[i] = fold (f * kDrawbarRatio[i]) / sr;
            ph[i]  = rng.uni();        // rodas fônicas giram livremente: fase aleatória
        }
        envTarget = 1;
        envCoefA = 1.0f - std::exp (-1.0f / (0.0015f * sr));
        envCoefR = 1.0f - std::exp (-1.0f / (0.0060f * sr));

        clickLvl = p.click * (0.6f + 0.4f * rng.uni());
        clickEnv = 1; clickCoef = std::exp (-1.0f / (0.0035f * sr));
        clickFilt.reset();
        clickFilt.set (2500.0f + 2500.0f * rng.uni(), 0.7f, sr);

        if (p.percOn && percTrig)
        {
            percPh = 0;
            percInc = fold (f * (p.perc3rd ? 3.0f : 2.0f)) / sr;
            percEnv = 1;
            percCoef = std::exp (-1.0f / ((p.percFast ? 0.20f : 1.0f) * sr));
        }
        else percEnv = 0;
    }

    void release (float click)
    {
        held = false; envTarget = 0;
        clickEnv = 0.4f; clickLvl = click * 0.35f;  // contato soltando também estala
    }

    inline float tick (const float* dbGain, bool percOn)
    {
        const SineTable& S = sine();
        float s = 0;
        for (int i = 0; i < 9; ++i)
        {
            if (dbGain[i] > 0.0f) s += S (ph[i]) * dbGain[i];
            ph[i] += inc[i]; if (ph[i] >= 1.0f) ph[i] -= 1.0f;
        }
        env += (envTarget - env) * (envTarget > env ? envCoefA : envCoefR);

        float perc = 0;
        if (percOn && percEnv > 1.0e-4f)
        {
            perc = S (percPh) * percEnv * 0.9f;
            percPh += percInc; if (percPh >= 1) percPh -= 1;
            percEnv *= percCoef;
        }

        float clk = 0;
        if (clickEnv > 1.0e-3f)
        {
            float hp; clickFilt.bp (rng.next(), nullptr, &hp);
            clk = hp * clickEnv * clickLvl * 0.6f;
            clickEnv *= clickCoef;
        }

        if (! held && env < 1.0e-4f && clickEnv < 1.0e-3f) active = false;
        return (s + perc) * env * 0.075f + clk;
    }
};

struct Leslie
{
    DelayLine hornDl, drumDl;
    float sr = 44100, lp = 0, lpCoef = 0;
    float hornPh = 0, drumPh = 0, hornHz = 0.8f, drumHz = 0.65f;
    float hornAcc = 0, drumAcc = 0;

    void prepare (float s)
    {
        sr = s;
        hornDl.init ((int) (0.01f * s)); drumDl.init ((int) (0.01f * s));
        lpCoef = 1.0f - std::exp (-kTwoPi * 800.0f / s);   // crossover ~800 Hz
        hornAcc = 1.0f / (0.8f * s);    // corneta acelera rápido
        drumAcc = 1.0f / (3.5f * s);    // tambor é pesado e demora
    }

    inline void process (float x, bool fast, float& l, float& r)
    {
        const float hT = fast ? 6.8f : 0.83f;
        const float dT = fast ? 5.9f : 0.67f;
        hornHz += (hT - hornHz) * hornAcc * 4.0f;
        drumHz += (dT - drumHz) * drumAcc * 4.0f;
        hornPh += hornHz / sr; if (hornPh >= 1) hornPh -= 1;
        drumPh += drumHz / sr; if (drumPh >= 1) drumPh -= 1;

        lp += (x - lp) * lpCoef;
        const float low = lp, high = x - lp;

        const SineTable& S = sine();
        auto wrap = [] (float p) { return p >= 1.0f ? p - 1.0f : p; };

        // Corneta: Doppler (delay modulado) + AM, dois microfones a 90°
        hornDl.push (high);
        const float hpL = hornPh, hpR = wrap (hornPh + 0.25f);
        const float base = 0.0012f * sr, dep = 0.00035f * sr;
        const float hL = hornDl.read (base + dep * S (hpL)) * (0.62f + 0.38f * S (wrap (hpL + 0.25f)));
        const float hR = hornDl.read (base + dep * S (hpR)) * (0.62f + 0.38f * S (wrap (hpR + 0.25f)));

        // Tambor (graves): AM mais suave + pequeno Doppler
        drumDl.push (low);
        const float dpL = drumPh, dpR = wrap (drumPh + 0.25f);
        const float dB = 0.0010f * sr, dD = 0.00015f * sr;
        const float dL = drumDl.read (dB + dD * S (dpL)) * (0.82f + 0.18f * S (wrap (dpL + 0.25f)));
        const float dR = drumDl.read (dB + dD * S (dpR)) * (0.82f + 0.18f * S (wrap (dpR + 0.25f)));

        l = hL + dL;
        r = hR + dR;
    }
};

// ---------------------------------------------------------------- REVERB (Freeverb)
struct Reverb
{
    struct Comb { std::vector<float> b; int i = 0; float store = 0; };
    struct AP   { std::vector<float> b; int i = 0; };
    Comb cl[8], cr[8];
    AP al[4], ar[4];

    void prepare (float sr)
    {
        static const int ct[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int at[4] = { 556, 441, 341, 225 };
        const float sc = sr / 44100.0f;
        for (int k = 0; k < 8; ++k)
        {
            cl[k].b.assign ((size_t) std::max (1, (int) (ct[k] * sc)), 0.0f); cl[k].i = 0; cl[k].store = 0;
            cr[k].b.assign ((size_t) std::max (1, (int) ((ct[k] + 23) * sc)), 0.0f); cr[k].i = 0; cr[k].store = 0;
        }
        for (int k = 0; k < 4; ++k)
        {
            al[k].b.assign ((size_t) std::max (1, (int) (at[k] * sc)), 0.0f); al[k].i = 0;
            ar[k].b.assign ((size_t) std::max (1, (int) ((at[k] + 23) * sc)), 0.0f); ar[k].i = 0;
        }
    }
    static inline float comb (Comb& c, float x)
    {
        const float fb = 0.84f, damp = 0.25f;
        float y = c.b[(size_t) c.i];
        c.store = y * (1 - damp) + c.store * damp;
        c.b[(size_t) c.i] = x + c.store * fb;
        if (++c.i >= (int) c.b.size()) c.i = 0;
        return y;
    }
    static inline float ap (AP& a, float x)
    {
        float bo = a.b[(size_t) a.i];
        float y = -x + bo;
        a.b[(size_t) a.i] = x + bo * 0.5f;
        if (++a.i >= (int) a.b.size()) a.i = 0;
        return y;
    }
    inline void process (float inL, float inR, float& outL, float& outR)
    {
        const float in = (inL + inR) * 0.015f;
        float l = 0, r = 0;
        for (int k = 0; k < 8; ++k) { l += comb (cl[k], in); r += comb (cr[k], in); }
        for (int k = 0; k < 4; ++k) { l = ap (al[k], l); r = ap (ar[k], r); }
        outL = l; outR = r;
    }
};

// ---------------------------------------------------------------- ENGINE
class Engine
{
public:
    Params params;
    int latencySamples() const { return maxi.la; }
    float gainReductionDb() const { return maxi.grDb; }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        chorus.prepare (sr);
        leslie.prepare (sr);
        reverb.prepare (sr);
        echo.prepare (sampleRate);
        flanger.prepare (sr);
        echoIdle = true;
        maxi.prepare (sr);
        for (auto& v : piano) v.active = false;
        for (auto& v : organ) v.active = false;
        for (float& g : dbSmooth) g = 0;
        smoothCoef = 1.0f - std::exp (-1.0f / (0.01f * sr));
        percWindow = 0;
        heldKeys = 0;
    }

    void noteOn (int note, float vel)
    {
        ++stamp;
        const int mode = params.mode;
        if (mode == 0)
        {
            PianoVoice& v = allocPiano (note);
            v.start (note, vel, sr, params, stamp);
        }
        if (mode == 1)
        {
            // percussão single-trigger: só dispara se nenhuma tecla estava pressionada
            // (com janela de 25 ms para acordes tocados juntos)
            const bool trig = (organHeld() == 0) || percWindow > 0;
            if (organHeld() == 0) percWindow = (int) (0.025f * sr);
            OrganVoice& v = allocOrgan (note);
            v.start (note, sr, params, trig, stamp);
        }
        ++heldKeys;
    }

    void noteOff (int note)
    {
        for (auto& v : piano)
            if (v.active && v.held && v.note == note)
            {
                v.held = false;
                if (! sustain) v.release (sr);
            }
        for (auto& v : organ)
            if (v.active && v.held && v.note == note)
                v.release (params.click);
        heldKeys = std::max (0, heldKeys - 1);
    }

    void setSustain (bool on)
    {
        sustain = on;
        if (! on)
            for (auto& v : piano)
                if (v.active && ! v.held && ! v.releasing) v.release (sr);
    }

    void setModWheelLeslie (bool fast) { modLeslie = fast; }

    void allNotesOff()
    {
        for (auto& v : piano) if (v.active) { v.held = false; v.release (sr); }
        for (auto& v : organ) if (v.active) v.release (0.0f);
        sustain = false; heldKeys = 0;
    }

    void render (float* outL, float* outR, int n)
    {
        while (n > 0)
        {
            const int m = std::min (n, kChunk);
            renderChunk (outL, outR, m);
            outL += m; outR += m; n -= m;
        }
    }

    void renderChunk (float* outL, float* outR, int n)
    {
        const Params& p = params;
        float dbTarget[9];
        for (int i = 0; i < 9; ++i)
        {
            const float d = clampf (p.drawbar[i], 0.0f, 8.0f);
            dbTarget[i] = d < 0.01f ? 0.0f : std::pow (10.0f, (d - 8.0f) * 3.0f / 20.0f); // ~3 dB por posição
        }
        if (p.percOn) dbTarget[8] = 0.0f;   // como no B3: percussão "rouba" o drawbar de 1'

        const bool lesFast = p.leslieFast || modLeslie;
        const float drv = 1.0f + p.drive * 9.0f;
        const float drvComp = 1.0f / std::tanh (drv * 0.5f) * 0.5f;

        const float maxGain = std::pow (10.0f, p.maxGainDb / 20.0f);
        const float maxCeil = std::pow (10.0f, p.maxCeilDb / 20.0f);

        for (int s = 0; s < n; ++s)
        {
            for (int i = 0; i < 9; ++i) dbSmooth[i] += (dbTarget[i] - dbSmooth[i]) * smoothCoef;
            if (percWindow > 0) --percWindow;

            float pin = 0;
            for (auto& v : piano) if (v.active) pin += v.tick();
            float oin = 0;
            for (auto& v : organ) if (v.active) oin += v.tick (dbSmooth, p.percOn);

            // piano: chorus estéreo
            float pl, pr;
            chorus.process (pin, p.pianoChorus, pl, pr);
            pl *= p.pianoLevel; pr *= p.pianoLevel;

            // órgão: pré de válvula (soft clip) -> Leslie
            const float od = std::tanh (oin * drv * 0.5f) * drvComp * 2.0f;
            float ll, lr;
            leslie.process (od, lesFast, ll, lr);
            const float m = p.leslieMix;
            float ol = (od * (1 - m) + ll * m) * p.organLevel;
            float orr = (od * (1 - m) + lr * m) * p.organLevel;

            bufL[s] = pl + ol;
            bufR[s] = pr + orr;
        }

        // eco de fita FitaEco TE-12 (antes do reverb, como num rack de dub).
        // Só o sinal molhado volta daqui; desligar o ECHO corta a entrada mas deixa a cauda tocar.
        fitaeco::Params fp;
        fp.mode       = std::max (1, std::min (12, p.echoMode));
        fp.repeatRate = p.echoRate;
        fp.intensity  = p.echoIntensity;
        fp.echoVol    = p.echoVol;
        fp.reverbVol  = p.echoSpring;
        fp.wowFlutter = p.echoWow;
        fp.tapeAge    = p.echoAge;
        fp.input      = 0.5f;
        fp.dry        = 0.0f;
        fp.output     = 0.8f;
        echo.setParams (fp);
        if (p.echoOn || ! echoIdle)
        {
            echo.process (p.echoOn ? bufL : zero, p.echoOn ? bufR : zero, ecL, ecR, n);
            if (p.echoOn) echoIdle = false;
            else
            {
                // com o ECHO desligado, para de rodar quando a cauda acabar (sem chiado de fita parado)
                float pk = 0;
                for (int s = 0; s < n; ++s) pk = std::max (pk, std::max (std::fabs (ecL[s]), std::fabs (ecR[s])));
                if (pk < 0.002f) echoIdle = true;
            }
        }
        else
        {
            std::fill (ecL, ecL + n, 0.0f);
            std::fill (ecR, ecR + n, 0.0f);
        }

        for (int s = 0; s < n; ++s)
        {
            float L = bufL[s] + ecL[s], R = bufR[s] + ecR[s];
            float rl, rr;
            reverb.process (L, R, rl, rr);
            L += rl * p.reverb * 1.4f;
            R += rr * p.reverb * 1.4f;

            // flanger na mistura inteira (instrumento + eco + reverb), como nos dubs do Mad Professor
            flanger.process (L, R, p);

            L *= p.master; R *= p.master;
            maxi.process (L, R, p.maxOn, maxGain, maxCeil);
            outL[s] = p.maxOn ? L : softClip (L);
            outR[s] = p.maxOn ? R : softClip (R);
        }
    }

private:
    static constexpr int NumPiano = 32, NumOrgan = 40;
    float sr = 44100, smoothCoef = 0.01f;
    std::array<PianoVoice, NumPiano> piano;
    std::array<OrganVoice, NumOrgan> organ;
    StereoChorus chorus;
    Leslie leslie;
    Reverb reverb;
    fitaeco::TapeEcho echo;
    Flanger flanger;
    static constexpr int kChunk = 256;
    bool echoIdle = true;
    float bufL[kChunk] = {}, bufR[kChunk] = {}, zero[kChunk] = {}, ecL[kChunk] = {}, ecR[kChunk] = {};
    Maximizer maxi;
    float dbSmooth[9] = {};
    bool sustain = false, modLeslie = false;
    int percWindow = 0, heldKeys = 0;
    uint64_t stamp = 0;

    static inline float softClip (float x)
    {
        if (x > 1.5f) return 1.0f;
        if (x < -1.5f) return -1.0f;
        return x - (4.0f / 27.0f) * x * x * x;   // cúbica suave, contínua em ±1.5
    }

    int organHeld() const
    {
        int c = 0;
        for (auto& v : organ) if (v.active && v.held) ++c;
        return c;
    }

    template <typename V, size_t N>
    static V& steal (std::array<V, N>& pool, int note)
    {
        // mesma nota -> reusa; senão voz livre; senão a mais antiga
        for (auto& v : pool) if (v.active && v.note == note) return v;
        for (auto& v : pool) if (! v.active) return v;
        V* oldest = &pool[0];
        for (auto& v : pool)
        {
            if (! v.held && oldest->held) { oldest = &v; continue; }
            if (v.held == oldest->held && v.age < oldest->age) oldest = &v;
        }
        return *oldest;
    }
    PianoVoice& allocPiano (int note) { return steal (piano, note); }
    OrganVoice& allocOrgan (int note) { return steal (organ, note); }
};

} // namespace rk
