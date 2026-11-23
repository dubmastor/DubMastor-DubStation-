// =============================================================================
//  Mastor Dub Horns — motor de som (naipe de metais anos 70 + Mastor Tape Echo)
//  C++17 puro, sem JUCE, para ser testado offline (tests/engine_test.cpp).
//
//  Voz de metal: osciladores com poly-BLEP (dente-de-serra / quadrada / triângulo),
//  "scoop" de afinação no ataque, vibrato atrasado, filtro com envelope que abre
//  com a força (o "blat" do metal), dois formantes por instrumento e sopro filtrado.
//  Barramento: corte de grave -> saturação de fita -> banda limitada (console anos 70)
//  -> EQ gráfico de 10 bandas -> Filtro Tubby (passa-altas ressonante) -> dry + eco.
//  Mastor Tape Echo: 3 cabeças (1x 2x 3x), tempo livre ou em sync, intensidade,
//  agudos/graves na realimentação, wow & flutter, saturação, throw e auto-oscilação.
// =============================================================================
#pragma once
#include <cmath>
#include <cstdint>
#include <complex>
#include <vector>
#include <array>
#include <algorithm>
#include "Music.h"
#include "Phrases.h"

namespace horns
{
constexpr float kPi = 3.14159265358979f;
constexpr double kPiD = 3.14159265358979323846;

// ------------------------------------------------------------------ utilidades
struct Rng
{
    uint32_t s = 0x2468aceu;
    float next()     { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * (1.0f / 2147483648.0f); } // [-1,1)
    float uniform()  { return 0.5f * (next() + 1.0f); }                                                              // [0,1)
};

inline float coefFor (float seconds, float sr) { return 1.0f - std::exp (-1.0f / std::max (1.0e-5f, seconds * sr)); }
inline float clampF (float f, float sr) { return std::clamp (f, 10.0f, sr * 0.45f); }

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void setNorm (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
    void lowpass (float f, float q, float sr)
    {
        const double w = 2 * kPiD * clampF (f, sr) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (float f, float q, float sr)
    {
        const double w = 2 * kPiD * clampF (f, sr) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandpass (float f, float q, float sr)
    {
        const double w = 2 * kPiD * clampF (f, sr) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void peaking (float f, float q, float dB, float sr)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2 * kPiD * clampF (f, sr) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        setNorm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (float f, float dB, float sr)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2 * kPiD * clampF (f, sr) / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al;
        setNorm (A * ((A + 1) - (A - 1) * c + sA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sA),
                 (A + 1) + (A - 1) * c + sA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sA);
    }
    void highShelf (float f, float dB, float sr)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2 * kPiD * clampF (f, sr) / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al;
        setNorm (A * ((A + 1) + (A - 1) * c + sA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sA),
                 (A + 1) - (A - 1) * c + sA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sA);
    }
    // resposta em dB (para desenhar a curva do EQ)
    double magnitudeDb (double f, double sr) const
    {
        const std::complex<double> z1c = std::polar (1.0, -2 * kPiD * f / sr), z2c = z1c * z1c;
        const auto num = (double) b0 + (double) b1 * z1c + (double) b2 * z2c;
        const auto den = 1.0 + (double) a1 * z1c + (double) a2 * z2c;
        return 20.0 * std::log10 (std::max (1.0e-9, std::abs (num / den)));
    }
};

// Filtro de variáveis de estado (TPT), estável sob modulação rápida
struct Svf
{
    float a1 = 0, a2 = 0, a3 = 0, k = 1, ic1 = 0, ic2 = 0, lpOut = 0, bpOut = 0;
    void set (float fc, float q, float sr)
    {
        const float g = std::tan (kPi * clampF (fc, sr) / sr);
        k = 1.0f / std::max (0.3f, q);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void process (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        lpOut = v2; bpOut = v1;
    }
    void reset() { ic1 = ic2 = 0; }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

struct DelayLine
{
    std::vector<float> buf; int mask = 0, w = 0;
    void init (int minSize)
    {
        int n = 1; while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    int size() const { return mask + 1; }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float d) const // d = atraso em amostras (>= 2)
    {
        float rp = (float) w - 1.0f - d;
        while (rp < 0) rp += (float) size();
        const int i = (int) rp; const float f = rp - (float) i;
        const float x0 = buf[(size_t) ((i - 1) & mask)], x1 = buf[(size_t) (i & mask)];
        const float x2 = buf[(size_t) ((i + 1) & mask)], x3 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (x2 - x0), c2 = x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3, c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
        return ((c3 * f + c2) * f + c1) * f + x1;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
};

// ------------------------------------------------------------------ parâmetros
struct InstSettings { bool on = false; float level = 0.75f; float pan = 0.0f; int oct = 0; int harm = 0; };

struct Params
{
    int key = 9, scale = 0;                 // Lá menor
    std::array<InstSettings, kNumInst> inst {{
        { true,  0.80f, -0.30f,  0, 0 },     // trompete
        { false, 0.70f, -0.15f,  0, 2 },     // flugelhorn
        { false, 0.70f,  0.20f,  0, 2 },     // sax alto
        { true,  0.75f,  0.15f, -1, 0 },     // sax tenor
        { true,  0.70f,  0.35f,  0, 2 },     // trombone
        { false, 0.65f,  0.00f, -1, 3 },     // sax barítono
    }};
    // timbre
    float bright = 0.55f, breath = 0.40f, vib = 0.30f, scoop = 0.40f, attack = 1.0f, release = 0.30f;
    float drive = 0.35f, band = 9000.0f, lowcut = 80.0f, hiss = 0.15f, human = 0.50f, width = 0.70f;
    float growl = 0.30f, room = 0.25f;     // v1.2: rouco do sax e sala do estúdio
    // Mastor Tape Echo
    bool echoOn = true; int sync = 5; float echoMs = 375.0f;
    float fb = 0.45f, send = 0.30f, ret = 0.75f, tone = 3200.0f, bass = 220.0f, wow = 0.30f, sat = 0.50f;
    std::array<bool, 3> head { true, false, false };
    bool throwOn = false, selfOsc = false;
    // EQ
    std::array<float, 10> eq {};
    float tubby = 20.0f, tubbyRes = 0.0f;
    float master = 0.8f;
    double bpm = 74.0;
};

inline const std::array<float, 10>& eqFreqs()
{
    static const std::array<float, 10> f { 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
    return f;
}
inline void setEqBand (Biquad& b, int i, float dB, float sr)
{
    const float f = eqFreqs()[(size_t) i];
    if (i == 0)      b.lowShelf (f, dB, sr);
    else if (i == 9) b.highShelf (f, dB, sr);
    else             b.peaking (f, 1.1f, dB, sr);
}
inline float tubbyQ (float res) { return 0.707f + res * 7.0f; }

inline double echoMs (const Params& p)
{
    const auto& d = syncDivs()[(size_t) std::clamp (p.sync, 0, (int) syncDivs().size() - 1)];
    if (d.beats <= 0.0) return p.echoMs;
    return d.beats * 60000.0 / std::clamp (p.bpm, 30.0, 300.0);
}

// saturação com ganho unitário em sinal baixo (não "acelera" a realimentação)
inline float softSat (float x, float g) { return std::tanh (x * g) / g; }

// ------------------------------------------------------------------ voz
//  v1.2: o fim da nota é o que entrega se é gente ou máquina. Aqui:
//   • fall / doit: a nota segura um instante, depois cai (ou sobe) acelerando, com o volume
//     sumindo JUNTO — a parte de baixo do fall quase não se ouve, como num músico de verdade.
//     Profundidade, duração e curva variam a cada nota; o sax cai "pelas chaves" (meio cromático),
//     o trombone desliza liso pela vara. O filtro fecha e aparece o ar do sopro.
//   • fim normal: o lábio relaxa e a afinação cede uns poucos cents enquanto o som some.
//   • vibrato: começa depois do ataque, varia de velocidade e de fundura, mexe também no volume e
//     no brilho, e PARA quando o músico solta a nota (nada de vibrato "de máquina" na cauda).
//   • afinação viva: cada músico oscila uns cents devagar; o ataque assenta na nota.
//   • golpe de língua no ataque e "rouco" (growl) para os saxes estilo Dean Fraser.
struct Voice
{
    bool active = false, releasing = false;
    int inst = 0, src = 0, srcNote = -1, note = 60, artic = Ten;
    uint32_t age = 0;
    int delay = 0;
    float sr = 44100.0f;

    int numOsc = 1; Wave wave[3] { Saw, Saw, Saw }; float oscGain[3] {}, detRatio[3] { 1, 1, 1 }, ph[3] {};
    float baseFreq = 440.0f, dt = 0.01f;
    float pOff = 0, pCoef = 0;                                   // scoop (semitons)
    float settle = 0, settleCoef = 0;                            // ataque assentando na nota
    float drift = 0, driftTarget = 0, driftAmt = 0; int driftCount = 0;
    float vibPh = 0, vibRate = 5, vibRateCur = 5, vibDepth = 0, vibDepthCur = 0, vibRamp = 0, vibInc = 0, vibVal = 0;
    int vibWait = 0;

    Svf lp; Biquad f1, f2, f3, bell; float q = 1;
    float nlG = 1, nlK = 0, buzzAmt = 0, brightK = 1;   // v1.3: dinâmica dos harmônicos e zumbido do lábio/palheta
    float cut = 500, cutBase = 500, cutPeak = 1000, cutTarget = 500, cutCoef16 = 0, cutInc16 = 0; int cutStage = 0;

    float amp = 0, ampPeak = 1, ampSus = 0.7f, ampInc = 0, ampCoef = 0, relCoef = 0; int ampStage = 0;
    // final da nota
    int endMode = 0;                     // 0 normal, 1 fall, 2 doit
    float relT = 0, relHold = 0, relLen = 1, endDepth = 0, endExpo = 2, endU = 0, ampRel0 = 1, sagSemis = 0;
    float endGain = 1, endGainStep = 0, stairMix = 0, stairSm = 0, wobble = 0;
    float trem = 1, tremStep = 0;
    Svf bf; float bAmp = 0, bPeak = 0, bSus = 0, bInc = 0, bCoef = 0, bRel = 0, bEnd = 0; bool bUp = true;
    Svf tf; float tAmp = 0, tCoef = 0;                           // golpe de língua
    float growlAmt = 0, gz1 = 0, gz2 = 0, gK = 0, gNorm = 1;
    float gL = 0.7f, gR = 0.7f;
    int ctr = 0;
    Rng rng;

    void start (int instIdx, int midi, float vel, int articIn, int srcId, int srcNoteIn,
                const Params& P, float sampleRate, uint32_t ageIn, Rng& r, float delaySec = -1.0f, float sessionCents = 0.0f)
    {
        const auto& D = instruments()[(size_t) instIdx];
        const auto& S = P.inst[(size_t) instIdx];
        sr = sampleRate; active = true; releasing = false;
        inst = instIdx; note = midi; artic = articIn; src = srcId; srcNote = srcNoteIn; age = ageIn;
        rng.s = 0x9e3779b9u ^ (uint32_t) (ageIn * 2654435761u) ^ (uint32_t) midi;
        if (rng.s == 0) rng.s = 1;

        const float hum = P.human;
        delay = (int) ((delaySec >= 0.0f ? delaySec : r.uniform() * 0.02f * hum) * sr);
        const float det = r.next() * 4.0f * hum + sessionCents;
        numOsc = D.numOsc;
        for (int i = 0; i < 3; ++i)
        {
            wave[i] = D.osc[i].wave; oscGain[i] = i < numOsc ? D.osc[i].gain : 0.0f;
            detRatio[i] = std::exp2 ((det + D.osc[i].detuneCents) / 1200.0f);
            ph[i] = r.uniform();
        }
        baseFreq = 440.0f * std::exp2 ((float) (midi - 69) / 12.0f);
        vel = std::clamp (vel, 0.05f, 1.0f);

        const float atk = D.atk * (0.4f + P.attack * 0.9f);
        // scoop: cada nota entra um pouco diferente (e quase nada nas pontadas secas)
        const float scoopVar = 0.65f + 0.7f * rng.uniform();
        const float scoopC = (P.scoop * D.scoop * scoopVar * (artic == Stac ? 0.45f : 1.0f)) + (artic == Scoop ? 200.0f + 120.0f * rng.uniform() : 0.0f);
        const float scoopT = 0.035f + P.scoop * 0.045f + (artic == Scoop ? 0.08f + 0.04f * rng.uniform() : 0.0f);
        pOff = -scoopC / 100.0f; pCoef = coefFor (scoopT / 3.0f, sr);
        settle = (rng.next() * 9.0f * (0.3f + hum)) / 100.0f; settleCoef = coefFor (0.05f, sr) * 16.0f;
        driftAmt = D.drift * (0.25f + hum) / 100.0f; drift = rng.next() * driftAmt * 0.5f; driftTarget = drift; driftCount = 0;

        // vibrato atrasado, com a velocidade do músico
        vibRate = D.vibRate * (0.92f + 0.16f * rng.uniform()); vibRateCur = vibRate; vibPh = rng.uniform();
        vibDepth = 0.20f * P.vib * D.vib; vibDepthCur = vibDepth * 0.8f; vibRamp = 0.0f; vibVal = 0.0f;
        vibWait = (int) ((0.16f + 0.14f * rng.uniform()) * sr / 16.0f); vibInc = 1.0f / (0.45f * sr / 16.0f);

        const float kt = std::pow (baseFreq / 440.0f, 0.45f);
        cutBase = D.cut * kt * (0.55f + P.bright * 0.9f) * (0.45f + 0.75f * vel);   // pp fecha, ff abre (como no metal real)
        cutPeak = std::min (16000.0f, cutBase + D.env * kt * P.bright * (0.08f + vel * vel * 1.37f));
        cut = cutBase * 0.5f; cutStage = 0;
        cutInc16 = (cutPeak - cut) / std::max (1.0f, atk * 1.3f * sr) * 16.0f;
        cutTarget = cutBase + (cutPeak - cutBase) * 0.5f;
        cutCoef16 = 1.0f - std::pow (1.0f - coefFor (0.18f, sr), 16.0f);
        q = D.q; lp.reset(); lp.set (cut, q, sr);
        f1.peaking (D.f1, 1.4f, D.g1, sr); f1.reset();
        f2.peaking (D.f2, 2.0f, D.g2, sr); f2.reset();
        f3.peaking (D.f3, 1.6f, D.g3, sr); f3.reset();
        bell.highpass (D.bellHp, 0.6f, sr); bell.reset();
        // metal de verdade: em pp o som é escuro, em ff os harmônicos agudos crescem muito (o "blat")
        nlK = D.nl; brightK = 0.4f + 0.8f * P.bright; nlG = 1.0f;
        buzzAmt = D.buzz * (0.35f + 0.65f * P.breath) * 0.32f;

        amp = 0.0f; ampPeak = vel * 0.95f; ampSus = vel * (0.68f + 0.08f * rng.uniform()); ampStage = 0;
        ampInc = ampPeak / std::max (1.0f, atk * sr); ampCoef = coefFor (0.12f, sr);
        bPeak = P.breath * D.breath * 0.22f * (0.5f + vel); bAmp = 0.0f; bUp = true;
        bInc = bPeak / std::max (1.0f, 0.7f * atk * sr); bSus = bPeak * 0.3f; bCoef = coefFor (0.08f, sr); bEnd = 0.0f;
        bf.reset(); bf.set (D.breathF, 1.2f, sr);
        // golpe de língua: um "t" curtinho de ar no começo da nota
        tAmp = (0.05f + 0.11f * vel) * (0.4f + P.breath) * (artic == Stac ? 1.3f : 1.0f) * (D.breath + 0.4f);
        tCoef = coefFor (0.007f, sr); tf.reset(); tf.set (std::min (D.breathF * 1.6f, 7000.0f), 0.9f, sr);
        // rouco (growl): modulação áspera vinda de um ruído grave, como a garganta do saxofonista
        growlAmt = P.growl * D.growl;
        gK = coefFor (1.0f / (2.0f * kPi * (55.0f + 25.0f * rng.uniform())), sr); gz1 = gz2 = 0.0f;
        gNorm = 1.0f / std::sqrt (std::max (1.0e-6f, gK * 0.5f)) * 0.55f;

        endMode = 0; relT = 0; endGain = 1; endGainStep = 0; stairSm = 0; wobble = 0; endU = 0; trem = 1; tremStep = 0;
        stairMix = D.stair;

        const float p = std::clamp (S.pan * (0.3f + P.width), -1.0f, 1.0f);
        const float lvl = S.level * D.gain;
        gL = std::cos ((p + 1.0f) * kPi * 0.25f) * lvl;
        gR = std::sin ((p + 1.0f) * kPi * 0.25f) * lvl;
        ctr = 0;
    }

    void release (float relParam)
    {
        if (releasing || ! active) return;
        releasing = true;
        const auto& D = instruments()[(size_t) inst];
        const float rel = 0.05f + relParam * 0.45f;
        bRel = coefFor (rel / 4.0f, sr);
        ampRel0 = std::max (1.0e-4f, amp);
        relT = 0;
        if (artic == Fall || artic == Doit)
        {
            endMode = artic == Fall ? 1 : 2;
            const float var = 0.8f + 0.4f * rng.uniform();
            // segura um pouco a nota e então cai / sobe acelerando; o volume vai junto
            relHold = (0.035f + 0.05f * rng.uniform()) * sr;
            if (endMode == 1)
            {
                endDepth = -D.fallDepth * var;
                relLen = (0.24f + 0.14f * rng.uniform() + relParam * 0.18f) * sr;
                endExpo = 1.9f + 0.6f * rng.uniform();
            }
            else
            {
                endDepth = (D.fallDepth * 0.45f + 0.8f) * var;
                relLen = (0.16f + 0.10f * rng.uniform() + relParam * 0.12f) * sr;
                endExpo = 1.6f + 0.5f * rng.uniform();
            }
            relCoef = 0.0f;
            bEnd = bPeak * (endMode == 1 ? 0.55f : 0.4f);   // o ar aparece quando o lábio solta
            if (ampStage < 2) amp = std::max (amp, ampSus * 0.8f);
        }
        else
        {
            endMode = 0;
            relCoef = coefFor (rel / 3.5f, sr);
            sagSemis = -(0.06f + 0.10f * rng.uniform()) * (0.4f + 0.8f * D.drift / 6.0f);   // lábio relaxando: poucos cents
            cutTarget = cutBase * 0.45f; cutCoef16 = 1.0f - std::pow (1.0f - coefFor (rel / 3.0f, sr), 16.0f);
        }
        if (cutStage == 1) cutStage = 2;
    }

    void kill() { active = false; }

    void updateCoefs()
    {
        // afinação viva (passeio lento)
        if (--driftCount <= 0) { driftTarget = rng.next() * driftAmt; driftCount = (int) ((0.25f + 0.4f * rng.uniform()) * sr / 16.0f); }
        drift += (driftTarget - drift) * (16.0f / (0.35f * sr));
        settle -= settle * settleCoef;

        // vibrato humano
        if (! releasing)
        {
            if (vibWait > 0) --vibWait; else vibRamp = std::min (1.0f, vibRamp + vibInc);
            vibRateCur += (vibRate * (1.0f + 0.07f * rng.next()) - vibRateCur) * 0.004f;
            vibDepthCur += (vibDepth * (1.0f + 0.25f * rng.next()) - vibDepthCur) * 0.003f;
        }
        else vibRamp *= 0.985f;                                 // o músico para o vibrato ao soltar
        vibPh += vibRateCur * 16.0f / sr; if (vibPh >= 1.0f) vibPh -= 1.0f;
        vibVal = std::sin (2.0f * kPi * vibPh) * vibDepthCur * vibRamp;

        // final da nota
        float endSemis = 0.0f, gTarget = 1.0f;
        if (releasing)
        {
            relT += 16.0f;
            if (endMode != 0)
            {
                endU = std::clamp ((relT - relHold) / relLen, 0.0f, 1.0f);
                float semis = endDepth * std::pow (endU, endExpo);
                // chaves do sax: meio cromático; trombone: liso
                stairSm += (std::round (semis) - stairSm) * 0.35f;
                semis = semis * (1.0f - stairMix * 0.6f) + stairSm * stairMix * 0.6f;
                wobble += (rng.next() * 0.35f - wobble) * 0.08f;
                endSemis = semis + wobble * endU;
                gTarget = std::pow (1.0f - endU, 1.7f);
                if (endU >= 1.0f) { active = false; return; }
            }
            else
                endSemis = sagSemis * (1.0f - std::min (1.0f, amp / ampRel0));
        }
        endGainStep = (gTarget - endGain) / 16.0f;
        const float tTarget = 1.0f + vibVal * 0.45f;            // vibrato também mexe no volume
        tremStep = (tTarget - trem) / 16.0f;

        const float semis = pOff + settle + drift + vibVal + endSemis;
        const float f = baseFreq * std::exp2 (semis / 12.0f);
        dt = std::min (0.45f, f / sr);
        if (cutStage == 0)
        {
            cut += cutInc16;
            if (cut >= cutPeak) { cut = cutPeak; cutStage = releasing ? 2 : 1; }
        }
        else cut += (cutTarget - cut) * cutCoef16;
        // metal de verdade: o brilho acompanha o volume (fica escuro quando some)
        const float lvl = std::clamp ((amp / std::max (1.0e-4f, ampPeak)) * endGain, 0.0f, 1.0f);
        lp.set (std::max (120.0f, cut * (0.45f + 0.55f * lvl) * (1.0f + vibVal * 0.6f)), q, sr);
        nlG = 1.0f + nlK * 2.6f * lvl * ampPeak * brightK;
    }

    // soma a voz em L/R por n amostras
    void render (float* L, float* R, int n)
    {
        for (int s = 0; s < n; ++s)
        {
            if (delay > 0) { --delay; continue; }
            if ((ctr++ & 15) == 0) { updateCoefs(); if (! active) return; }

            pOff -= pOff * pCoef;
            endGain += endGainStep; trem += tremStep;

            float x = 0.0f;
            for (int i = 0; i < numOsc; ++i)
            {
                const float d = std::min (0.45f, dt * detRatio[i]);
                float& p = ph[i];
                p += d; if (p >= 1.0f) p -= 1.0f;
                float v;
                switch (wave[i])
                {
                    case Saw:    v = 2.0f * p - 1.0f - polyBlep (p, d); break;
                    case Square: { float p2 = p + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
                                   v = (p < 0.5f ? 1.0f : -1.0f) + polyBlep (p, d) - polyBlep (p2, d); } break;
                    case Pulse:  { float p2 = p + 0.65f; if (p2 >= 1.0f) p2 -= 1.0f;
                                   v = (p < 0.35f ? 1.0f : -1.0f) + polyBlep (p, d) - polyBlep (p2, d) - 0.3f; } break;
                    case Tri:
                    default:     v = 4.0f * std::fabs (p - 0.5f) - 1.0f; break;
                }
                x += v * oscGain[i];
            }
            // zumbido do lábio / da palheta: ruído que abre e fecha no ritmo de cada ciclo da nota
            if (buzzAmt > 0.0f && ph[0] < 0.2f) x += buzzAmt * rng.next() * (1.0f - ph[0] * 5.0f) * 2.0f;
            // rouco: aspereza na palheta antes do filtro (fica dentro do timbre do instrumento)
            if (growlAmt > 0.001f)
            {
                gz1 += (rng.next() - gz1) * gK; gz2 += (gz1 - gz2) * gK;
                const float gm = std::clamp (gz2 * gNorm, -1.0f, 1.0f);
                x *= 1.0f + growlAmt * 0.7f * gm;
                x += growlAmt * 0.22f * (x * std::fabs (x) - 0.3f * x);
            }
            lp.process (x);
            float y0 = lp.lpOut;
            if (nlG > 1.001f) y0 = std::tanh (nlG * y0) / nlG * (1.0f + 0.35f * (nlG - 1.0f));
            float y = bell.process (f3.process (f2.process (f1.process (y0))));

            bf.process (rng.next());
            if (! releasing)
            {
                if (bUp) { bAmp += bInc; if (bAmp >= bPeak) { bAmp = bPeak; bUp = false; } }
                else bAmp += (bSus - bAmp) * bCoef;
            }
            else if (endMode != 0) bAmp += (bEnd * (1.0f - endU) - bAmp) * bCoef;
            else bAmp -= bAmp * bRel;
            y += bf.bpOut * bAmp;

            if (tAmp > 1.0e-4f) { tf.process (rng.next()); y += tf.bpOut * tAmp; tAmp -= tAmp * tCoef; }

            switch (ampStage)
            {
                case 0: amp += ampInc; if (amp >= ampPeak) { amp = ampPeak; ampStage = releasing ? 2 : 1; } break;
                case 1: amp += (ampSus - amp) * ampCoef; if (releasing) ampStage = 2; break;
                default:
                    if (endMode != 0) amp += (ampSus - amp) * ampCoef;     // o fall/doit cuida do volume
                    else amp -= amp * relCoef;
                    if (amp < 1.0e-4f) { active = false; return; }
                    break;
            }
            const float out = y * amp * std::max (0.0f, endGain) * trem;
            L[s] += out * gL;
            R[s] += out * gR;
        }
    }
};

// ------------------------------------------------------------------ motor
class Engine
{
public:
    static constexpr int kMaxVoices = 96;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate;
        busL.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        busR.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        line.init ((int) (sr * 6.0f) + 16);
        for (auto& v : voices) v.active = false;
        dSm = (float) (echoMs (P) * 0.001 * sr);
        hissL.bandpass (5000, 0.4f, sr); hissR.bandpass (5200, 0.4f, sr);
        smoothK = coefFor (0.03f, sr);
        glideK = coefFor (0.12f, sr);
        compAtk = coefFor (0.01f, sr); compRel = coefFor (0.2f, sr);
        for (auto& t : instTune) t = rng.next() * 7.0f;        // cada músico afina um pouco diferente na sessão
        // sala do estúdio (4 linhas com matriz de Hadamard): a sala de gravação de Kingston
        static const float roomMs[4] = { 31.7f, 41.3f, 49.9f, 57.1f };   // v1.3: mais perto de uma plate de estúdio
        for (int i = 0; i < 2; ++i) { apLen[i] = std::max (4, (int) ((i == 0 ? 4.7f : 7.3f) * 0.001f * sr)); apBuf[i].assign ((size_t) apLen[i], 0.0f); apPos[i] = 0; }
        for (int i = 0; i < 4; ++i) { roomLen[i] = std::max (8, (int) (roomMs[i] * 0.001f * sr)); roomBuf[i].assign ((size_t) roomLen[i], 0.0f); roomPos[i] = 0; }
        roomDamp = coefFor (1.0f / (2.0f * kPi * 4200.0f), sr);
        roomHpK = coefFor (1.0f / (2.0f * kPi * 180.0f), sr);
        dirtyCoefs = true;
        updateCoefs();
        reset();
    }

    void reset()
    {
        line.clear();
        for (auto* b : { &bumpL, &bumpR, &hpL, &hpR, &lp1L, &lp1R, &lp2L, &lp2R, &tubL, &tubR, &eHpL, &eHpR, &eLpL, &eLpR, &fHp, &fLp, &hissL, &hissR }) b->reset();
        for (auto& b : eqL) b.reset();
        for (auto& b : eqR) b.reset();
        compEnv = 0.0f;
        for (int i = 0; i < 4; ++i) { std::fill (roomBuf[i].begin(), roomBuf[i].end(), 0.0f); roomLp[i] = 0.0f; }
        for (auto& b : apBuf) std::fill (b.begin(), b.end(), 0.0f);
        roomHpL = roomHpR = 0.0f;
    }

    void setParams (const Params& p)
    {
        const bool changed = p.drive != P.drive || p.lowcut != P.lowcut || p.band != P.band || p.eq != P.eq || p.tubby != P.tubby
                          || p.tubbyRes != P.tubbyRes || p.tone != P.tone || p.bass != P.bass;
        P = p;
        if (changed) dirtyCoefs = true;
        updateCoefs();
    }
    const Params& params() const { return P; }

    void noteOn (int midi, float vel, int artic, int src)
    {
        // o naipe tem um líder (o primeiro a entrar); os outros chegam alguns ms depois, cada um com a sua afinação
        int lead = -1;
        for (int i : { 0, 3, 2, 1, 4, 5 }) if (P.inst[(size_t) i].on) { lead = i; break; }
        for (int i = 0; i < kNumInst; ++i)
        {
            const auto& S = P.inst[(size_t) i];
            if (! S.on) continue;
            const int m = sectionNote (midi, S.harm, S.oct, i, P.key, P.scale);
            // reataque: solta a mesma nota da mesma fonte
            for (auto& v : voices)
                if (v.active && ! v.releasing && v.src == src && v.srcNote == midi && v.inst == i) v.release (P.release);
            Voice& v = allocVoice();
            const float hum = P.human;
            const float dl = i == lead ? rng.uniform() * 0.006f * hum : (0.004f + rng.uniform() * 0.022f) * hum;
            v.start (i, m, vel * (i == lead ? 1.0f : 0.92f + 0.08f * rng.uniform()), artic, src, midi, P, sr, ++ageCounter, rng, dl,
                     instTune[(size_t) i] * (0.3f + hum));
        }
    }

    void noteOff (int midi, int src)
    {
        for (auto& v : voices)
            if (v.active && ! v.releasing && v.src == src && v.srcNote == midi) v.release (P.release);
    }

    void allNotesOff (int src = -1)
    {
        for (auto& v : voices)
            if (v.active && (src < 0 || v.src == src)) v.release (P.release);
    }

    int activeVoices() const { int n = 0; for (auto& v : voices) n += v.active ? 1 : 0; return n; }
    float currentEchoMs() const { return (float) echoMs (P); }

    void render (float* outL, float* outR, int n)
    {
        int pos = 0;
        while (pos < n)
        {
            const int chunk = std::min (n - pos, (int) busL.size());
            renderChunk (outL + pos, outR + pos, chunk);
            pos += chunk;
        }
    }

private:
    Voice& allocVoice()
    {
        for (auto& v : voices) if (! v.active) return v;
        Voice* best = nullptr;               // rouba: primeiro a mais antiga já soltando, senão a mais antiga
        for (auto& v : voices) if (v.releasing && (! best || v.age < best->age)) best = &v;
        if (! best) for (auto& v : voices) if (! best || v.age < best->age) best = &v;
        best->kill();
        return *best;
    }

    void updateCoefs()
    {
        if (! dirtyCoefs) return;
        dirtyCoefs = false;
        hpL.highpass (P.lowcut, 0.7f, sr);  hpR.highpass (P.lowcut, 0.7f, sr);
        bumpL.lowShelf (95.0f, 1.0f + 2.5f * P.drive, sr); bumpR.lowShelf (95.0f, 1.0f + 2.5f * P.drive, sr);   // "head bump" da fita
        lp1L.lowpass (P.band, 0.6f, sr);    lp1R.lowpass (P.band, 0.6f, sr);
        lp2L.lowpass (std::min (20000.0f, P.band * 1.4f), 0.5f, sr); lp2R.lowpass (std::min (20000.0f, P.band * 1.4f), 0.5f, sr);
        for (int i = 0; i < 10; ++i) { setEqBand (eqL[(size_t) i], i, P.eq[(size_t) i], sr); setEqBand (eqR[(size_t) i], i, P.eq[(size_t) i], sr); }
        tubL.highpass (P.tubby, tubbyQ (P.tubbyRes), sr); tubR.highpass (P.tubby, tubbyQ (P.tubbyRes), sr);
        eHpL.highpass (P.bass, 0.6f, sr); eHpR.highpass (P.bass, 0.6f, sr); fHp.highpass (P.bass, 0.6f, sr);
        eLpL.lowpass (P.tone, 0.6f, sr);  eLpR.lowpass (P.tone, 0.6f, sr);  fLp.lowpass (P.tone, 0.6f, sr);
    }

    void renderChunk (float* outL, float* outR, int n)
    {
        std::fill (busL.begin(), busL.begin() + n, 0.0f);
        std::fill (busR.begin(), busR.begin() + n, 0.0f);
        for (auto& v : voices) if (v.active) v.render (busL.data(), busR.data(), n);

        const float driveG = 1.0f + P.drive * 3.0f, driveMake = 1.0f + P.drive * 1.2f;
        const float dTarget = std::clamp ((float) (echoMs (P) * 0.001 * sr), 2.0f, (float) line.size() / 3.0f - 64.0f);
        int nHeads = 0; for (bool h : P.head) nHeads += h ? 1 : 0;
        const float fbTarget = (P.selfOsc ? 1.12f : P.fb) / std::sqrt ((float) std::max (1, nHeads));
        const float fbK = P.selfOsc ? coefFor (0.25f, sr) : smoothK;
        const float satG = 1.0f + P.sat * 1.5f;
        const float throwK = P.throwOn ? coefFor (0.01f, sr) : coefFor (0.05f, sr);
        const float hissG = P.hiss * P.hiss * 0.03f;
        const float wowA = P.wow * 0.0035f * sr, flutA = P.wow * 0.0006f * sr;
        const float wowInc = 0.55f / sr, flutInc = 6.3f / sr;
        const float masterG = P.master * 1.2f;
        const float thr = 0.2f; // ~ -14 dB
        const float roomG = 0.64f + P.room * 0.22f, roomWet = P.room * 0.40f;
        float peak = 0.0f, sumSq = 0.0f;

        for (int s = 0; s < n; ++s)
        {
            // barramento do naipe
            float l = busL[(size_t) s] * 0.34f, r = busR[(size_t) s] * 0.34f;
            l = hpL.process (l); r = hpR.process (r);
            l = softSat (l, driveG) * driveMake; r = softSat (r, driveG) * driveMake;
            l = lp2L.process (lp1L.process (bumpL.process (l))); r = lp2R.process (lp1R.process (bumpR.process (r)));
            for (int b = 0; b < 10; ++b) { l = eqL[(size_t) b].process (l); r = eqR[(size_t) b].process (r); }
            l = tubL.process (l); r = tubR.process (r);

            // Mastor Tape Echo
            sendSm  += (P.send - sendSm) * smoothK;
            gateSm  += ((P.echoOn ? 1.0f : 0.0f) - gateSm) * smoothK;
            throwSm += ((P.throwOn ? 1.0f : 0.0f) - throwSm) * throwK;
            retSm   += (P.ret - retSm) * smoothK;
            fbSm    += (fbTarget - fbSm) * fbK;
            for (int h = 0; h < 3; ++h) gH[h] += ((P.head[(size_t) h] ? 1.0f : 0.0f) - gH[h] * 1.0f) * smoothK;
            dSm += (dTarget - dSm) * glideK;
            wowPh += wowInc;  if (wowPh >= 1.0f) wowPh -= 1.0f;
            flutPh += flutInc; if (flutPh >= 1.0f) flutPh -= 1.0f;
            const float mod = std::sin (2 * kPi * wowPh) * wowA + std::sin (2 * kPi * flutPh) * flutA;

            const float in = 0.5f * (l + r) * (sendSm + throwSm) * gateSm;
            const float maxD = (float) line.size() - 8.0f;
            const float h1 = line.read (std::clamp (dSm + mod, 2.0f, maxD));
            const float h2 = line.read (std::clamp (2.0f * (dSm + mod), 2.0f, maxD));
            const float h3 = line.read (std::clamp (3.0f * (dSm + mod), 2.0f, maxD));
            const float a = gH[0] * h1, b = gH[1] * h2, c = gH[2] * h3;
            const float fbSig = softSat (fLp.process (fHp.process (a + b + c)), satG);
            line.push (in + fbSig * fbSm);
            const float wl = eLpL.process (eHpL.process (a + 0.6f * b + c)) * retSm * 1.1f * gateSm;
            const float wr = eLpR.process (eHpR.process (0.6f * a + b + c)) * retSm * 1.1f * gateSm;

            // chiado de fita
            const float hl = hissL.process (rngHiss.next()) * hissG, hr = hissR.process (rngHiss.next()) * hissG;

            // sala: as paredes da sala de gravação, bem curtinho, só para o naipe respirar
            float rl = 0.0f, rr = 0.0f;
            if (roomWet > 0.0005f)
            {
                roomHpL += (l - roomHpL) * roomHpK; roomHpR += (r - roomHpR) * roomHpK;
                float il = (l - roomHpL) * 0.5f, ir = (r - roomHpR) * 0.5f;
                for (int k = 0; k < 2; ++k)       // difusão (allpass) antes da plate
                {
                    const float d = apBuf[k][(size_t) apPos[k]];
                    const float ain = 0.5f * (il + ir), w = ain + 0.6f * d;
                    apBuf[k][(size_t) apPos[k]] = w; if (++apPos[k] >= apLen[k]) apPos[k] = 0;
                    const float o = d - 0.6f * w;
                    il = 0.6f * o + 0.4f * il; ir = 0.6f * o + 0.4f * ir;
                }
                float o[4];
                for (int i = 0; i < 4; ++i)
                {
                    o[i] = roomBuf[i][(size_t) roomPos[i]];
                    roomLp[i] += (o[i] - roomLp[i]) * roomDamp;
                    o[i] = roomLp[i];
                }
                const float q0 = 0.5f * (o[0] + o[1] + o[2] + o[3]), q1 = 0.5f * (o[0] - o[1] + o[2] - o[3]);
                const float q2 = 0.5f * (o[0] + o[1] - o[2] - o[3]), q3 = 0.5f * (o[0] - o[1] - o[2] + o[3]);
                const float in4[4] = { il, ir, il, ir }, hv[4] = { q0, q1, q2, q3 };
                for (int i = 0; i < 4; ++i)
                {
                    roomBuf[i][(size_t) roomPos[i]] = in4[i] + roomG * hv[i];
                    if (++roomPos[i] >= roomLen[i]) roomPos[i] = 0;
                }
                rl = (o[0] + 0.6f * o[2] - 0.3f * o[3]) * roomWet;
                rr = (o[1] + 0.6f * o[3] - 0.3f * o[2]) * roomWet;
            }
            else { roomHpL = roomHpR = 0.0f; }

            float ol = (l + wl + hl + rl) * masterG, or_ = (r + wr + hr + rr) * masterG;

            // compressor de barramento + limitador suave
            const float lv = std::max (std::fabs (ol), std::fabs (or_));
            compEnv += (lv - compEnv) * (lv > compEnv ? compAtk : compRel);
            float g = 1.0f;
            if (compEnv > thr) g = std::pow (compEnv / thr, 1.0f / 3.0f - 1.0f);
            ol = limit (ol * g); or_ = limit (or_ * g);
            if (! std::isfinite (ol) || ! std::isfinite (or_)) { ol = or_ = 0.0f; reset(); }
            outL[s] = ol; outR[s] = or_;
            peak = std::max (peak, std::max (std::fabs (ol), std::fabs (or_)));
            sumSq += 0.5f * (ol * ol + or_ * or_);
        }
        lastPeak = peak; lastRms = std::sqrt (sumSq / (float) std::max (1, n));
    }

    static float limit (float x)
    {
        const float a = std::fabs (x);
        if (a <= 0.9f) return x;
        return std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), x);
    }

public:
    float lastPeak = 0.0f, lastRms = 0.0f;

private:
    Params P;
    std::array<Voice, kMaxVoices> voices {};
    uint32_t ageCounter = 0;
    Rng rng, rngHiss { 0x13579bdu };
    float sr = 44100.0f;
    std::vector<float> busL, busR;
    bool dirtyCoefs = true;

    Biquad hpL, hpR, lp1L, lp1R, lp2L, lp2R, tubL, tubR, bumpL, bumpR;
    std::array<float, kNumInst> instTune {};
    std::array<Biquad, 10> eqL, eqR;

    DelayLine line;
    Biquad eHpL, eHpR, eLpL, eLpR, fHp, fLp;
    float dSm = 1000, sendSm = 0, gateSm = 1, throwSm = 0, retSm = 0.75f, fbSm = 0.0f, gH[3] { 1, 0, 0 };
    float wowPh = 0, flutPh = 0.37f, smoothK = 0.001f, glideK = 0.001f;
    Biquad hissL, hissR;
    float compEnv = 0, compAtk = 0.01f, compRel = 0.001f;
    std::vector<float> roomBuf[4]; int roomLen[4] { 8, 8, 8, 8 }, roomPos[4] {}; float roomLp[4] {};
    float roomDamp = 0.5f, roomHpK = 0.02f, roomHpL = 0, roomHpR = 0;
    std::vector<float> apBuf[2]; int apLen[2] { 4, 4 }, apPos[2] {};
};
} // namespace horns
