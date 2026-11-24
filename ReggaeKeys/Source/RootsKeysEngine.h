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
#include "GuitarSkank.h"

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
    int   pianoType = 0;       // 0 = M1, 1 = Grand Piano, 2 = MT-40 (Casio), 3 = FM EP (Yamaha DX), 4 = Yamaha CP-80, 5 = Yamaha C7, 6 = M1 Grand Piano (Korg),
                               //   7 = M1 Piano 16 comprimido, 8 = Honky Tonk (piano de armário), 9 = Casio CZ-101 (phase distortion),
                               //   10 = Rhodes Stage, 11 = Rhodes Suitcase (tremolo), 12 = Wurlitzer 200A
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
    // GTR SKANK: a guitarra do skank (timbre do MRG-68) tocando junto com o piano — sempre começa desligada
    bool  gtrOn    = false;
    float gtrLevel = 0.6f;
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
    int type = 0;
    // Grand: batimento entre as cordas de cada nota
    float bph[MaxP], binc[MaxP], bdep[MaxP];
    // MT-40 (Casio) e FM EP (Yamaha DX)
    float oPh = 0, oInc = 0, oEnv = 0, oCoef = 0, oLp = 0, oLpCoef = 0, oHold = 0, oHoldCnt = 0, oHoldLen = 1;
    float pdD = 0.5f, pdCoef = 1, pdTarget = 0.5f;
    // pianos elétricos (Rhodes / Wurlitzer): 3 parciais + não linearidade do captador
    float epPh[3] {}, epInc[3] {}, epAmp[3] {}, epCoef[3] {};
    float epBark = 0, epGain = 0, epDcX = 0, epDcY = 0, epDcR = 0.997f, trPh = 0, trInc = 0, trDep = 0;   // CZ-101: "DCW" (quanto a fase entorta) e o seu envelope
    float cPh[2] {}, mPh[2] {}, cInc[2] {}, mInc[2] {}, idx[2] {}, idxCoef[2] {}, cAmp[2] {}, cE1 = 0, cE2 = 0, cC1 = 0, cC2 = 0;

    void start (int n, float vel, float sr, const Params& p, uint64_t stamp)
    {
        active = true; held = true; releasing = false; note = n; age = stamp;
        rng.s = 0x9e3779b9u * (uint32_t) (n + 1) + (uint32_t) stamp;
        type = juce_clampType (p.pianoType);
        if (type == 1 || (type >= 4 && type <= 6) || type == 8) { startGrand (n, vel, sr, p, type); return; }
        if (type == 2) { startCasio (n, sr, p); return; }
        if (type == 9) { startCz (n, sr, p); return; }
        if (type >= 10) { startEp (n, vel, sr, p, type); return; }
        const bool comp = type == 7;   // M1 Piano 16 "comprimido até o talo", como no dancehall dos anos 90
        if (type == 3) { startFm (n, vel, sr, p); return; }

        const float f    = midiToHz ((float) n);
        const float v    = clampf (vel, 0.0f, 1.0f);
        const float br   = p.pianoBright;
        const float B    = 0.00012f * std::pow (2.0f, ((float) n - 60.0f) / 16.0f); // inarmonicidade
        const float pw   = clampf ((comp ? 1.75f : 2.0f) - 0.9f * br - (comp ? 0.35f : 0.7f) * v, 0.5f, 2.2f);   // inclinação espectral
        const float tBase = clampf ((1.2f + 10.0f * p.pianoDecay) * std::pow (2.0f, -((float) n - 60.0f) / 18.0f), 0.25f, 22.0f);
        const float gain = comp ? 0.15f * (0.7f + 0.3f * v)                       // compressão: quase não muda com o toque
                                : 0.16f * (0.2f + 0.8f * std::pow (v, 1.3f));

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
            const float w  = clampf ((comp ? 0.12f : 0.30f) + 0.045f * (float) k, 0.0f, 0.93f);   // comprimido: a cauda fica alta

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

    static int juce_clampType (int t) { return t < 0 ? 0 : (t > 12 ? 12 : t); }

    // ---- PIANOS ELÉTRICOS
    //  10 Rhodes Stage    — tine (barra de metal) batida por martelo: fundamental redonda, "sino" inarmônico curto
    //                       e o "bark" do captador magnético (2º harmônico que cresce com a força do toque)
    //  11 Rhodes Suitcase — o mesmo com mais sino, mais doce, e o tremolo do amplificador da maleta
    //  12 Wurlitzer 200A  — palheta de aço: mais curto, nasal, harmônicos ímpares que rosnam quando se toca forte
    void startEp (int n, float vel, float sr, const Params& p, int kind)
    {
        const float f  = midiToHz ((float) n);
        const float v  = clampf (vel, 0.0f, 1.0f);
        const float br = p.pianoBright;
        const float ks = std::pow (2.0f, -((float) n - 60.0f) / 18.0f);
        const bool wurli = kind == 12, suit = kind == 11;
        const float tBody = clampf ((wurli ? 1.0f + 3.5f * p.pianoDecay : 2.2f + 8.0f * p.pianoDecay) * ks, 0.3f, 14.0f);
        const float ratio[3] = { 1.0f, 2.0f, wurli ? 5.4f : 7.1f };            // o 3º é o "sino" do tine / o ataque da palheta
        const float amp[3]   = { 1.0f,
                                 wurli ? 0.10f : 0.10f + 0.22f * v,
                                 (wurli ? 0.10f + 0.25f * v : (0.22f + 0.5f * v) * (suit ? 1.6f : 1.0f)) * (0.6f + 0.8f * br) };
        const float tau[3]   = { tBody, tBody * 0.4f, wurli ? 0.025f : (suit ? 0.11f : 0.06f) };
        for (int i = 0; i < 3; ++i)
        {
            const float fi = f * ratio[i];
            epPh[i] = i == 0 ? 0.0f : rng.uni();
            epInc[i] = fi < 0.45f * sr ? fi / sr : 0.0f;
            epAmp[i] = epInc[i] > 0 ? amp[i] : 0.0f;
            epCoef[i] = std::exp (-1.0f / (tau[i] * sr));
        }
        epBark = wurli ? 1.0f + 3.5f * v * v * (0.6f + 0.8f * br)                 // Wurli: drive do tanh
                       : (0.15f + 1.1f * v * v) * (0.5f + br) * (suit ? 0.7f : 1.0f);   // Rhodes: bark assimétrico
        epGain = (kind == 10 ? 0.11f : 0.13f) * (0.25f + 0.75f * std::pow (v, 1.2f));
        epDcX = epDcY = 0; epDcR = 1.0f - kTwoPi * 20.0f / sr;
        trPh = 0; trInc = 4.8f / sr; trDep = suit ? 0.35f : 0.0f;
        np = 0; nzEnv = 0; nzLvl = 0;
        att = 0; attInc = 1.0f / (0.0008f * sr);
        rel = 1; relCoef = 1;
    }

    // ---- CASIO CZ-101: piano por phase distortion — começa entortado (brilho de serra) e o DCW fecha para o seno
    void startCz (int n, float sr, const Params& p)
    {
        const float f = midiToHz ((float) n);
        oPh = 0; oInc = f / sr;
        oEnv = 0.17f;                                                   // o CZ-101 não tem velocity
        oCoef = std::exp (-1.0f / ((0.6f + 2.2f * p.pianoDecay) * sr));
        pdD = clampf (0.06f + 0.12f * (1.0f - p.pianoBright), 0.04f, 0.2f);   // fase bem entortada no ataque
        pdTarget = 0.5f;                                                // 0,5 = seno puro
        pdCoef = std::exp (-1.0f / ((0.10f + 0.25f * p.pianoBright) * sr));
        oLp = 0; oLpCoef = 1.0f - std::exp (-kTwoPi * 9000.0f / sr);
        np = 0; nzEnv = 0; nzLvl = 0;
        att = 0; attInc = 1.0f / (0.001f * sr);
        rel = 1; relCoef = 1;
    }

    // ---- GRAND PIANO: mais parciais e inarmonicidade, decaimento longo, cordas que batem
    // variante 4 = Yamaha CP-80 (piano de cauda elétrico: cordas curtas, captador piezo, som "honky" e curto)
    // variante 5 = Yamaha C7 (cauda de concerto: ataque rápido e brilho na região de 3 kHz, que corta a mistura)
    // variante 6 = M1 Grand Piano (Korg M1): cauda gravada da ROM, brilho de "presença" em 2,8 kHz,
    //              pouca inarmonicidade e decaimento um pouco mais curto (o loop da amostra), com chorus do M1
    void startGrand (int n, float vel, float sr, const Params& p, int variant = 1)
    {
    // variante 8 = Honky Tonk (piano de armário dos estúdios de Kingston, cordas desafinadas entre si, mais curto e escuro)
        const bool cp = variant == 4, c7 = variant == 5, m1g = variant == 6, ht = variant == 8;
        const float f  = midiToHz ((float) n);
        const float v  = clampf (vel, 0.0f, 1.0f);
        const float br = p.pianoBright;
        const float B  = (cp ? 0.0011f : c7 ? 0.00024f : m1g ? 0.00018f : 0.00028f) * std::pow (2.0f, ((float) n - 60.0f) / 14.0f);
        const float pw = clampf ((cp ? 1.9f : c7 ? 2.1f : m1g ? 2.0f : 2.4f) - 0.8f * br - 1.0f * v, 0.6f, 2.6f);
        const float tBase = clampf ((cp ? 1.4f + 6.0f * p.pianoDecay : ht ? 1.2f + 5.0f * p.pianoDecay : m1g ? 2.0f + 11.0f * p.pianoDecay : 2.5f + 14.0f * p.pianoDecay) * std::pow (2.0f, -((float) n - 60.0f) / 16.0f), 0.3f, 30.0f);
        const float gain = (m1g ? 0.125f : 0.15f) * (0.15f + 0.85f * std::pow (v, 1.5f));
        np = 0;
        for (int k = 1; k <= MaxP; ++k)
        {
            const float fk = (float) k * f * std::sqrt (1.0f + B * (float) (k * k));
            if (fk > std::min (0.42f * sr, 14000.0f)) break;
            float a = std::pow ((float) k, -pw);
            a *= 0.5f + 0.5f * std::fabs (std::sin (3.14159265f * (float) k / 7.6f));   // martelo a ~1/7.6
            if (cp)
            {
                const float d = (fk - 1200.0f) / 700.0f;                    // ressonância do captador piezo
                a *= (0.55f + 1.3f * std::exp (-d * d)) * (fk < 180.0f ? 0.6f : 1.0f);   // pouco grave, meio "honky"
            }
            else if (ht)
            {
                const float d = (fk - 900.0f) / 900.0f;                     // caixa do armário: meio e pouco agudo
                a *= (0.7f + 0.8f * std::exp (-d * d)) * (fk > 4000.0f ? 0.5f : 1.0f);
            }
            else if (m1g)
            {
                const float d = (fk - 2800.0f) / 1500.0f;                   // a "presença" do piano do M1
                a *= 1.0f + 1.0f * (0.5f + 0.5f * br) * std::exp (-d * d);
            }
            else if (c7)
            {
                const float d = (fk - 3000.0f) / 1500.0f;                   // presença do Yamaha
                a *= 1.0f + 0.9f * (0.4f + 0.6f * br) * std::exp (-d * d);
            }
            const float t2 = tBase / (1.0f + 0.08f * (float) (k - 1) + 1.1f * std::pow (fk / 1000.0f, 1.3f));
            const float t1 = std::min (0.5f, 0.06f + t2 * 0.1f);
            const float w  = clampf (0.22f + 0.035f * (float) k, 0.0f, 0.9f);
            ph[np] = rng.uni(); inc[np] = fk / sr;
            e1[np] = gain * a * w; e2[np] = gain * a * (1.0f - w);
            c1[np] = std::exp (-1.0f / (t1 * sr)); c2[np] = std::exp (-1.0f / (t2 * sr));
            bph[np] = rng.uni(); binc[np] = (ht ? 2.5f + 4.0f * rng.uni() : 0.35f + 1.6f * rng.uni()) / sr;   // honky: cordas ~10 cents fora
            bdep[np] = (k <= 10 ? 0.22f : 0.08f) * (cp ? 1.3f : c7 ? 0.7f : m1g ? 0.6f : ht ? 2.6f : 1.0f);
            ++np;
        }
        att = 0; attInc = 1.0f / (0.0015f * sr);
        rel = 1; relCoef = 1;
        nzEnv = 1; nzCoef = std::exp (-1.0f / (0.014f * sr));
        nzLvl = (cp ? 0.05f : c7 ? 0.09f : 0.07f) * v * v;
        nzFilt.reset();
        nzFilt.set (std::min ((c7 ? 4.0f : 3.0f) * f, c7 ? 3500.0f : 2500.0f), 0.8f, sr);
    }

    // ---- MT-40: teclado Casio de brinquedo — onda de pulso, sem sensibilidade ao toque, lo-fi
    void startCasio (int n, float sr, const Params& p)
    {
        const float f = midiToHz ((float) n);
        oPh = 0; oInc = f / sr;
        oEnv = 0.16f;                                                   // MT-40 não tem velocity
        oCoef = std::exp (-1.0f / ((0.35f + 1.4f * p.pianoDecay) * sr));
        oLp = 0; oLpCoef = 1.0f - std::exp (-kTwoPi * (1800.0f + 4500.0f * p.pianoBright) / sr);
        oHoldLen = std::max (1.0f, sr / 16000.0f);                      // ~16 kHz, como os chips da época
        oHoldCnt = 0; oHold = 0;
        np = 0; nzEnv = 0; nzLvl = 0;
        att = 0; attInc = 1.0f / (0.0008f * sr);
        rel = 1; relCoef = 1;
    }

    // ---- FM EP: piano elétrico FM estilo Yamaha DX — corpo (1:1) + "tine" brilhante (1:14)
    void startFm (int n, float vel, float sr, const Params& p)
    {
        const float f = midiToHz ((float) n);
        const float v = clampf (vel, 0.0f, 1.0f);
        const float ks = std::pow (2.0f, -((float) n - 60.0f) / 20.0f);
        cInc[0] = f / sr;          mInc[0] = f / sr;
        cInc[1] = (f + 0.7f) / sr; mInc[1] = 14.0f * f / sr;
        idx[0] = (0.4f + 1.6f * v) * (0.6f + 0.8f * p.pianoBright);
        idx[1] = (0.6f + 2.4f * v * v) * (0.5f + p.pianoBright);
        idxCoef[0] = std::exp (-1.0f / ((0.6f + 0.8f * p.pianoDecay) * sr));
        idxCoef[1] = std::exp (-1.0f / (0.045f * sr));
        cAmp[0] = 0.75f; cAmp[1] = 0.22f + 0.25f * v;
        const float tBody = clampf ((1.2f + 6.0f * p.pianoDecay) * ks, 0.3f, 12.0f);
        const float gain = 0.2f * (0.25f + 0.75f * v);
        cE1 = gain * 0.35f; cE2 = gain * 0.65f;
        cC1 = std::exp (-1.0f / (0.12f * sr)); cC2 = std::exp (-1.0f / (tBody * sr));
        for (int i = 0; i < 2; ++i) { cPh[i] = 0; mPh[i] = rng.uni(); }
        np = 0; nzEnv = 0; nzLvl = 0;
        att = 0; attInc = 1.0f / (0.001f * sr);
        rel = 1; relCoef = 1;
    }

    void release (float sr)
    {
        held = false; releasing = true;
        float t = note > 88 ? 1.2f : 0.11f;   // notas agudas sem abafador
        if (type == 1 || type == 5 || type == 6) t = note > 88 ? 1.8f : 0.16f;
        if (type == 4) t = note > 88 ? 0.9f : 0.12f;
        if (type == 8) t = note > 88 ? 0.8f : 0.13f;
        if (type == 9) t = 0.08f;
        if (type == 10 || type == 11) t = 0.16f;   // abafador de feltro no tine
        if (type == 12) t = 0.10f;
        if (type == 2) t = 0.06f;
        if (type == 3) t = 0.22f;
        relCoef = std::exp (-1.0f / (t * sr));
    }

    static inline float wrap1 (float x) { return x - std::floor (x); }

    // fm = multiplicador de frequência (pitch bend / vibrato); 1.0 = afinação normal
    inline float tick (float fm = 1.0f)
    {
        const SineTable& S = sine();
        if (type == 2)
        {
            oPh += oInc * fm; if (oPh >= 1.0f) oPh -= 1.0f;
            const float pulse = (oPh < 0.25f ? 0.75f : -0.25f) + 0.35f * (oPh < 0.5f ? 0.5f : -0.5f);
            oLp += (pulse - oLp) * oLpCoef;
            if (--oHoldCnt <= 0.0f) { oHold = std::round (oLp * 48.0f) / 48.0f; oHoldCnt += oHoldLen; }   // ~6 bits + 16 kHz
            att = std::min (1.0f, att + attInc);
            const float out = oHold * oEnv * att * rel;
            oEnv *= oCoef;
            if (releasing) rel *= relCoef;
            if (rel < 1.0e-4f || oEnv < 1.0e-4f) active = false;
            return out;
        }
        if (type >= 10)
        {
            float x = 0;
            for (int i = 0; i < 3; ++i)
            {
                x += S (epPh[i]) * epAmp[i];
                epPh[i] += epInc[i] * fm; if (epPh[i] >= 1.0f) epPh[i] -= 1.0f;
                epAmp[i] *= epCoef[i];
            }
            float y = type == 12 ? std::tanh (epBark * x) / std::tanh (epBark)   // palheta saturando no captador eletrostático
                                 : x + epBark * x * x;                            // captador magnético: assimetria = bark
            const float d = y - epDcX + epDcR * epDcY; epDcX = y; epDcY = d;     // tira o DC que a assimetria cria
            float out = d * epGain;
            if (trDep > 0.0f)
            {
                trPh += trInc; if (trPh >= 1.0f) trPh -= 1.0f;
                out *= 1.0f - trDep * (0.5f + 0.5f * S (trPh));
            }
            att = std::min (1.0f, att + attInc);
            out *= att * rel;
            if (releasing) rel *= relCoef;
            if (rel < 1.0e-4f || epAmp[0] < 1.0e-5f) active = false;
            return out;
        }
        if (type == 9)
        {
            oPh += oInc * fm; if (oPh >= 1.0f) oPh -= 1.0f;
            pdD = pdTarget + (pdD - pdTarget) * pdCoef;
            const float w = oPh < pdD ? 0.5f * oPh / pdD : 0.5f + 0.5f * (oPh - pdD) / (1.0f - pdD);   // phase distortion
            const float x = -S (wrap1 (w + 0.25f));                                                       // -cos
            oLp += (x - oLp) * oLpCoef;
            att = std::min (1.0f, att + attInc);
            const float out = oLp * oEnv * att * rel;
            oEnv *= oCoef;
            if (releasing) rel *= relCoef;
            if (rel < 1.0e-4f || oEnv < 1.0e-4f) active = false;
            return out;
        }
        if (type == 3)
        {
            float s = 0;
            for (int i = 0; i < 2; ++i)
            {
                const float m = S (mPh[i]) * idx[i] * 0.159155f;   // índice em radianos -> ciclos
                s += S (wrap1 (cPh[i] + m)) * cAmp[i];
                cPh[i] += cInc[i] * fm; if (cPh[i] >= 1.0f) cPh[i] -= 1.0f;
                mPh[i] += mInc[i] * fm; if (mPh[i] >= 1.0f) mPh[i] -= 1.0f;
                idx[i] *= idxCoef[i];
            }
            att = std::min (1.0f, att + attInc);
            const float env = cE1 + cE2;
            cE1 *= cC1; cE2 *= cC2;
            const float out = s * env * att * rel;
            if (releasing) rel *= relCoef;
            if (rel < 1.0e-4f || cE2 < 1.0e-6f) active = false;
            return out;
        }

        float s = 0;
        if (type == 1 || (type >= 4 && type <= 6) || type == 8)
        {
            for (int i = 0; i < np; ++i)
            {
                const float beat = 1.0f + bdep[i] * (S (bph[i]) - 0.3f);
                s += S (ph[i]) * (e1[i] + e2[i]) * beat;
                e1[i] *= c1[i]; e2[i] *= c2[i];
                ph[i] += inc[i] * fm; if (ph[i] >= 1.0f) ph[i] -= 1.0f;
                bph[i] += binc[i]; if (bph[i] >= 1.0f) bph[i] -= 1.0f;
            }
        }
        else
        {
            for (int i = 0; i < np; ++i)
            {
                s += S (ph[i]) * (e1[i] + e2[i]);
                e1[i] *= c1[i]; e2[i] *= c2[i];
                ph[i] += inc[i] * fm; if (ph[i] >= 1.0f) ph[i] -= 1.0f;
            }
        }
        const float nz = nzFilt.bp (rng.next()) * nzEnv * nzLvl;
        nzEnv *= nzCoef;
        att = std::min (1.0f, att + attInc);
        float out = (s + nz) * att * rel;
        if (type == 7) out = 0.42f * std::tanh (2.4f * out);   // M1 comprimido: segura o pico, sobe a cauda
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

    inline float tick (const float* dbGain, bool percOn, float fm = 1.0f)
    {
        const SineTable& S = sine();
        float s = 0;
        for (int i = 0; i < 9; ++i)
        {
            if (dbGain[i] > 0.0f) s += S (ph[i]) * dbGain[i];
            ph[i] += inc[i] * fm; if (ph[i] >= 1.0f) ph[i] -= 1.0f;
        }
        env += (envTarget - env) * (envTarget > env ? envCoefA : envCoefR);

        float perc = 0;
        if (percOn && percEnv > 1.0e-4f)
        {
            perc = S (percPh) * percEnv * 0.9f;
            percPh += percInc * fm; if (percPh >= 1) percPh -= 1;
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
        gtr.prepare (sr);
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
            if (params.gtrOn) gtr.noteOn (note, vel);
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
        gtr.noteOff (note);
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
    // pitch bend (-1..1, ±2 semitons) e mod wheel (0..1: vibrato no piano, Leslie rápido no órgão)
    void setPitchBend (float b) { bend = clampf (b, -1.0f, 1.0f); }
    void setModWheel (float m) { modWheel = clampf (m, 0.0f, 1.0f); modLeslie = modWheel >= 0.5f; }

    void allNotesOff()
    {
        gtr.allNotesOff();
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
        gtr.flush();   // palhetada com as notas do piano que chegaram juntas
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
        const float bendRatio = bend == 0.0f ? 1.0f : std::pow (2.0f, bend * 2.0f / 12.0f);
        const float vibInc = 5.5f / sr;

        for (int s = 0; s < n; ++s)
        {
            // pitch bend + vibrato do mod wheel (só no piano; no órgão o mod wheel liga a Leslie)
            float fmPiano = bendRatio;
            if (modWheel > 0.0f)
            {
                vibPh += vibInc; if (vibPh >= 1.0f) vibPh -= 1.0f;
                fmPiano *= 1.0f + modWheel * 0.012f * sine() (vibPh);
            }
            for (int i = 0; i < 9; ++i) dbSmooth[i] += (dbTarget[i] - dbSmooth[i]) * smoothCoef;
            if (percWindow > 0) --percWindow;

            float pin = 0;
            for (auto& v : piano) if (v.active) pin += v.tick (fmPiano);
            float oin = 0;
            for (auto& v : organ) if (v.active) oin += v.tick (dbSmooth, p.percOn, bendRatio);

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

            // guitarra do skank: um pouco para a direita, entra no eco e no reverb como o piano
            const float gt = gtr.tick() * p.gtrLevel;
            bufL[s] = pl + ol + gt * 0.75f;
            bufR[s] = pr + orr + gt;
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
    GuitarSkank gtr;
    fitaeco::TapeEcho echo;
    Flanger flanger;
    static constexpr int kChunk = 256;
    bool echoIdle = true;
    float bufL[kChunk] = {}, bufR[kChunk] = {}, zero[kChunk] = {}, ecL[kChunk] = {}, ecR[kChunk] = {};
    Maximizer maxi;
    float dbSmooth[9] = {};
    bool sustain = false, modLeslie = false;
    float bend = 0.0f, modWheel = 0.0f, vibPh = 0.0f;
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
