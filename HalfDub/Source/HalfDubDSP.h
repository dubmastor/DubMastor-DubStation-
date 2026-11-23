#pragma once
// Mastor Half Dub MHD-050 — DSP de meia velocidade (independente do JUCE, para o teste offline).
//
// Cadeia: entrada -> 3 bandas (Linkwitz-Riley 4ª ordem) -> cada banda com a sua velocidade
//         (motor FITA = reamostragem, o tom desce; motor GROOVE = grãos, o tom fica)
//         -> DROP (entra/sai com POWER DOWN/UP de fita) -> soma -> cor de fita (wow/flutter, saturação)
//         -> pads de sound system (rewind, tape stop, reverse, spin-back)
//
// A velocidade é medida em "loops" presos ao compasso do DAW, como no HalfTime: em cada loop
// a banda toca o começo do loop mais devagar (1/2 = a primeira metade esticada no loop inteiro).
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>

namespace hd
{
static constexpr float kPi = 3.14159265358979f;
static constexpr float kTau = 6.28318530717959f;

// velocidades do seletor e das bandas (índice 0 da banda = segue o seletor VELOCIDADE)
static constexpr int kNumSpeeds = 6;                                   // 1/1 2/3 1/2 1/3 1/4 REV
static constexpr float kSpeedRatio[kNumSpeeds] = { 1.0f, 2.0f / 3.0f, 0.5f, 1.0f / 3.0f, 0.25f, -1.0f };
// tamanho do loop em tempos (semínimas): 1/16, 1/8, 1/4, 1/2, 1 bar, 2, 4, 8 bars
static constexpr int kNumLoops = 8;
static constexpr float kLoopBeats[kNumLoops] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f };

//==============================================================================
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void copyCoefs (const Biquad& o) { b0 = o.b0; b1 = o.b1; b2 = o.b2; a1 = o.a1; a2 = o.a2; }

    // RBJ cookbook
    void setLowPass (float f, float q, float fs)
    {
        const float w = kTau * std::clamp (f, 10.0f, 0.49f * fs) / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        const float a0 = 1.0f + al;
        b0 = (1.0f - c) * 0.5f / a0; b1 = (1.0f - c) / a0; b2 = b0; a1 = -2.0f * c / a0; a2 = (1.0f - al) / a0;
    }
    void setHighPass (float f, float q, float fs)
    {
        const float w = kTau * std::clamp (f, 10.0f, 0.49f * fs) / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        const float a0 = 1.0f + al;
        b0 = (1.0f + c) * 0.5f / a0; b1 = -(1.0f + c) / a0; b2 = b0; a1 = -2.0f * c / a0; a2 = (1.0f - al) / a0;
    }
    void setPeak (float f, float q, float gainDb, float fs)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w = kTau * std::clamp (f, 10.0f, 0.49f * fs) / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        const float a0 = 1.0f + al / A;
        b0 = (1.0f + al * A) / a0; b1 = -2.0f * c / a0; b2 = (1.0f - al * A) / a0; a1 = -2.0f * c / a0; a2 = (1.0f - al / A) / a0;
    }
    void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    // módulo da resposta em dB (para desenhar a curva do EQ)
    float magnitudeDb (float f, float fs) const
    {
        const double w = 2.0 * 3.14159265358979 * f / fs;
        const double cr = std::cos (w), sr = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2, ni = -(b1 * sr + b2 * s2);
        const double dr = 1.0 + a1 * cr + a2 * c2, di = -(a1 * sr + a2 * s2);
        const double m = (nr * nr + ni * ni) / (dr * dr + di * di + 1e-30);
        return (float) (10.0 * std::log10 (m + 1e-30));
    }
};

// Linkwitz-Riley 4ª ordem = duas Butterworth de 2ª ordem em série
struct LR4
{
    Biquad a, b;
    void setLP (float f, float fs) { a.setLowPass (f, 0.70710678f, fs); b.copyCoefs (a); }
    void setHP (float f, float fs) { a.setHighPass (f, 0.70710678f, fs); b.copyCoefs (a); }
    inline float process (float x) { return b.process (a.process (x)); }
    void reset() { a.reset(); b.reset(); }
};

// separa em grave / médio / agudo; a soma das três dá a entrada (com a fase de um passa-tudo)
struct ThreeBandSplit
{
    LR4 lp1, hp1, lp2, hp2, apLo, apHi;   // apLo/apHi: compensação de fase do grave na 2ª divisão
    void prepare (float f1, float f2, float fs)
    {
        lp1.setLP (f1, fs); hp1.setHP (f1, fs);
        lp2.setLP (f2, fs); hp2.setHP (f2, fs);
        apLo.setLP (f2, fs); apHi.setHP (f2, fs);
        reset();
    }
    void reset() { for (auto* f : { &lp1, &hp1, &lp2, &hp2, &apLo, &apHi }) f->reset(); }
    inline void process (float x, float& lo, float& mid, float& hi)
    {
        const float l = lp1.process (x);
        lo = apLo.process (l) + apHi.process (l);
        const float h = hp1.process (x);
        mid = lp2.process (h);
        hi  = hp2.process (h);
    }
};

//==============================================================================
// Uma banda: buffer circular + "cabeças" de leitura (seco, grade de meia velocidade, varispeed de fita)
class SlowBand
{
public:
    struct Settings
    {
        float ratio = 0.5f;        // 1, 2/3, 1/2, 1/3, 1/4 ou -1 (REV)
        bool  groove = false;      // motor GROOVE (mantém o tom)
        int   smoothMode = 1;      // 0 PERC, 1 SUST, 2 FX, 3 SWELL
        float smooth = 0.3f;       // 0..1
        bool  drop = true;         // efeito ligado
        float downSamples = 0;     // POWER DOWN (amostras); 0 = troca imediata
        float upSamples = 0;       // POWER UP
    };

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        const int want = (int) std::min (fs * 20.0f, (float) (1 << 21));
        int n = 1; while (n < want) n <<= 1;
        N = n; mask = N - 1;
        for (auto& b : buf) b.assign ((size_t) N, 0.0f);
        grainLen = std::max (256, (int) std::lround (0.045 * fs));     // grão de ~45 ms
        shortXf = std::max (16, (int) (0.02f * fs));
        reset();
    }

    void reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        now = 0;
        cur = { Head::Dry, 0, 1, 1, 0 }; prev = cur; xf = 1.0f; xfStep = 0;
        lastDrop = false; rampLeft = 0; pendingAfterRamp = 0;
        grid = { 0, 1.0f }; gridOld = grid; gridXf = 1.0f; gridXfStep = 0; gridSwell = false;
        grainsValid = false; segInit = false;
    }

    void setSettings (const Settings& s) { set = s; }

    // Avança uma amostra. inL/inR = entrada da banda; segStartBeats/... vêm do relógio do DAW.
    // newSegment: true quando começa um loop novo (posição do loop voltou a zero)
    // posInSegSamples: posição dentro do loop, em amostras; segLenSamples: tamanho do loop
    inline void process (float inL, float inR, bool newSegment, double posInSegSamples, double segLenSamples, float& outL, float& outR)
    {
        buf[0][(size_t) (now & mask)] = inL;
        buf[1][(size_t) (now & mask)] = inR;

        // --- grade de meia velocidade: um "trajeto" por loop (início + razão)
        const float r = set.ratio;
        const int64_t segStart = now - (int64_t) std::llround (posInSegSamples);
        if (! segInit) { grid = { segStart, r }; segInit = true; }
        else if (newSegment || std::abs (r - grid.r) > 1e-4f || (r != 1.0f && segStart != grid.s && std::llabs (segStart - grid.s) > 64))
        {
            gridOld = grid;
            grid = { segStart, r };
            int len;
            if (set.smoothMode == 3)                       // SWELL: transição longa, proporcional ao loop
                len = (int) (segLenSamples * (0.05 + 0.45 * set.smooth));
            else
            {
                const float baseMs = set.smoothMode == 0 ? 4.0f : set.smoothMode == 1 ? 25.0f : 0.6f;
                len = (int) (fs * 0.001f * baseMs * (0.3f + 1.7f * set.smooth));
            }
            len = std::max (8, len);
            gridXf = 0.0f; gridXfStep = 1.0f / (float) len; gridSwell = set.smoothMode == 3;
        }

        // --- DROP: troca de cabeça de leitura com POWER DOWN / POWER UP
        if (set.drop != lastDrop)
        {
            lastDrop = set.drop;
            const bool tape = ! set.groove && std::abs (r - 1.0f) > 1e-3f;
            if (set.drop)
            {
                if (tape && set.downSamples > 32)
                {   // a fita perde velocidade até a razão escolhida e depois engata na grade
                    Head v { Head::Vari, (double) now, 1.0, (double) r, (double) (r - 1.0f) / set.downSamples };
                    startHead (v, shortXf / 4);
                    rampLeft = (int) set.downSamples; pendingAfterRamp = 1;
                }
                else
                {
                    startHead ({ Head::Grid, 0, 1, 1, 0 }, std::max (shortXf / 4, (int) set.downSamples));
                    rampLeft = 0; pendingAfterRamp = 0;
                }
            }
            else
            {
                if (tape && set.upSamples > 32)
                {   // a fita volta a acelerar até a velocidade normal e depois solta no seco
                    const double p = cur.kind == Head::Vari ? cur.p : gridPos (grid, now);
                    const double v0 = cur.kind == Head::Vari ? cur.v : (double) r;
                    Head v { Head::Vari, p, v0, 1.0, (1.0 - v0) / set.upSamples };
                    startHead (v, shortXf / 4);
                    rampLeft = (int) set.upSamples; pendingAfterRamp = 2;
                }
                else
                {
                    startHead ({ Head::Dry, 0, 1, 1, 0 }, std::max (shortXf / 4, (int) set.upSamples));
                    rampLeft = 0; pendingAfterRamp = 0;
                }
            }
        }
        if (rampLeft > 0 && --rampLeft == 0)
        {
            if (pendingAfterRamp == 1) startHead ({ Head::Grid, 0, 1, 1, 0 }, shortXf);
            else if (pendingAfterRamp == 2) startHead ({ Head::Dry, 0, 1, 1, 0 }, shortXf);
            pendingAfterRamp = 0;
        }

        float yl, yr;
        evalHead (cur, inL, inR, yl, yr);
        if (xf < 1.0f)
        {
            float pl, pr;
            evalHead (prev, inL, inR, pl, pr);
            const float a = std::sin (0.5f * kPi * xf), b = std::cos (0.5f * kPi * xf);
            yl = yl * a + pl * b; yr = yr * a + pr * b;
            xf = std::min (1.0f, xf + xfStep);
        }
        if (gridXf < 1.0f) gridXf = std::min (1.0f, gridXf + gridXfStep);

        outL = yl; outR = yr;
        ++now;
    }

    // para o visor
    bool isWet() const { return cur.kind != Head::Dry; }
    float currentRatio() const { return cur.kind == Head::Vari ? (float) cur.v : cur.kind == Head::Grid ? grid.r : 1.0f; }

private:
    struct Traj { int64_t s; float r; };
    struct Head { enum Kind { Dry, Grid, Vari } kind; double p, v, vTarget, dv; };

    void startHead (Head h, int xfLen)
    {
        prev = cur; cur = h;
        xf = 0.0f; xfStep = 1.0f / (float) std::max (1, xfLen);
        if (h.kind == Head::Grid) grainsValid = false;
    }

    inline double gridPos (const Traj& t, int64_t at) const
    {
        const double el = (double) (at - t.s);
        if (t.r >= 0.999f) return (double) at;
        if (t.r < 0.0f) return (double) t.s - el;            // REV: toca o loop anterior de trás para frente
        return (double) t.s + el * (double) t.r;
    }

    inline double clampPast (double p) const
    {
        const double minP = (double) (now - (int64_t) N + grainLen + 8);
        return std::clamp (p, minP, (double) now - 2.0);
    }

    inline float rd (int ch, double p) const
    {
        const double fl = std::floor (p);
        const int64_t i = (int64_t) fl;
        const float t = (float) (p - fl);
        const auto& b = buf[(size_t) ch];
        const float y0 = b[(size_t) ((i - 1) & mask)], y1 = b[(size_t) (i & mask)];
        const float y2 = b[(size_t) ((i + 1) & mask)], y3 = b[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    // leitura de um trajeto da grade (FITA) — p <= now sempre (a leitura nunca passa a gravação)
    inline void readTraj (const Traj& t, float& l, float& r) const
    {
        if (t.r >= 0.999f) { l = buf[0][(size_t) (now & mask)]; r = buf[1][(size_t) (now & mask)]; return; }
        const double p = clampPast (gridPos (t, now) - 1.0);
        l = rd (0, p); r = rd (1, p);
    }

    inline void evalGrid (float& l, float& r)
    {
        if (set.groove && grid.r > 0.0f && grid.r < 0.999f)
        {
            grooveRead (l, r);
            return;
        }
        readTraj (grid, l, r);
        if (gridXf < 1.0f)
        {
            float ol, orr;
            readTraj (gridOld, ol, orr);
            float a, b;
            if (gridSwell) { a = gridXf; b = 1.0f - gridXf; }
            else { a = std::sin (0.5f * kPi * gridXf); b = std::cos (0.5f * kPi * gridXf); }
            l = l * a + ol * b; r = r * a + orr * b;
        }
    }

    // GROOVE: dois grãos com janela de Hann a 50% (o tom não muda); cada grão nasce na posição da grade
    inline void grooveRead (float& l, float& r)
    {
        const int G = grainLen, hop = G / 2;
        if (! grainsValid)
        {
            grainsValid = true;
            gStart[0] = clampPast (gridPos (grid, now) - G - 3); gAge[0] = 0;
            gStart[1] = clampPast (gridPos (grid, now) - G - 3 - hop); gAge[1] = hop;
        }
        l = r = 0.0f;
        for (int k = 0; k < 2; ++k)
        {
            if (gAge[k] >= G)
            {
                gAge[k] = 0;
                gStart[k] = clampPast (gridPos (grid, now) - G - 3);
            }
            const float w = 0.5f - 0.5f * std::cos (kTau * (float) gAge[k] / (float) G);
            const double p = gStart[k] + (double) gAge[k];
            l += w * rd (0, p); r += w * rd (1, p);
            ++gAge[k];
        }
    }

    inline void evalHead (Head& h, float inL, float inR, float& l, float& r)
    {
        switch (h.kind)
        {
            case Head::Dry:  l = inL; r = inR; return;
            case Head::Grid: evalGrid (l, r); return;
            case Head::Vari:
            {
                h.p = clampPast (h.p);
                l = rd (0, h.p); r = rd (1, h.p);
                h.p += h.v;
                if ((h.dv < 0 && h.v > h.vTarget) || (h.dv > 0 && h.v < h.vTarget)) h.v += h.dv;
                if (h.p > (double) now - 2.0) h.p = (double) now - 2.0;
                return;
            }
        }
    }

    float fs = 48000.0f;
    int N = 1, mask = 0, grainLen = 2048, shortXf = 960;
    std::array<std::vector<float>, 2> buf;
    int64_t now = 0;
    Settings set;
    Head cur { Head::Dry, 0, 1, 1, 0 }, prev { Head::Dry, 0, 1, 1, 0 };
    float xf = 1.0f, xfStep = 0.0f;
    bool lastDrop = false;
    int rampLeft = 0, pendingAfterRamp = 0;
    Traj grid { 0, 1.0f }, gridOld { 0, 1.0f };
    float gridXf = 1.0f, gridXfStep = 0.0f;
    bool gridSwell = false, segInit = false;
    bool grainsValid = false;
    double gStart[2] { 0, 0 };
    int gAge[2] { 0, 0 };
};

//==============================================================================
// Cor de fita no sinal lento: wow/flutter (atraso modulado) + saturação e leve corte de agudo
class TapeColour
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        for (auto& b : d) b.assign (8192, 0.0f);
        w = 0; ph1 = ph2 = 0; lp[0] = lp[1] = 0;
    }
    void set (float wow01, float sat01) { wow = wow01; sat = sat01; }
    inline void process (float& l, float& r)
    {
        ph1 += kTau * 0.55f / fs; if (ph1 > kTau) ph1 -= kTau;
        ph2 += kTau * 5.7f / fs;  if (ph2 > kTau) ph2 -= kTau;
        const float base = 0.004f * fs;
        const float mod = wow * (0.0016f * fs * std::sin (ph1) + 0.00012f * fs * std::sin (ph2));
        d[0][(size_t) w] = l; d[1][(size_t) w] = r;
        float y[2];
        for (int c = 0; c < 2; ++c)
        {
            float pos = (float) w - (base + mod + (c ? 0.0003f * fs * wow : 0.0f));
            while (pos < 0) pos += 8192.0f;
            const int i = (int) pos; const float t = pos - (float) i;
            const float a = d[(size_t) c][(size_t) (i & 8191)], b = d[(size_t) c][(size_t) ((i + 1) & 8191)];
            y[c] = wow > 0.001f ? a + t * (b - a) : (c ? r : l);
        }
        w = (w + 1) & 8191;
        if (sat > 0.001f)
        {
            const float drive = 1.0f + 5.0f * sat;
            const float a = 1.0f - std::exp (-kTau * (17000.0f - 9000.0f * sat) / fs);
            for (int c = 0; c < 2; ++c)
            {
                lp[c] += a * (y[c] - lp[c]);
                y[c] = std::tanh (lp[c] * drive) / drive * (1.0f + 0.6f * sat);
            }
        }
        l = y[0]; r = y[1];
    }
    // atraso fixo do wow (para alinhar o seco)
    int latency() const { return (int) (0.004f * fs); }
private:
    float fs = 48000.0f, wow = 0, sat = 0, ph1 = 0, ph2 = 0, lp[2] { 0, 0 };
    std::array<std::vector<float>, 2> d;
    int w = 0;
};

//==============================================================================
// Pads de SOUND SYSTEM: REWIND, TAPE STOP, REVERSE, SPIN-BACK
class SoundSystemPads
{
public:
    enum Pad { None = 0, Rewind, TapeStop, Reverse, SpinBack };

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        int n = 1; while (n < (int) (fs * 8.0f)) n <<= 1;
        N = n; mask = N - 1;
        for (auto& b : buf) b.assign ((size_t) N, 0.0f);
        xfLen = std::max (16, (int) (0.012f * fs));
        now = 0; pad = None; mix = 0; active = false;
    }

    // durSamples: duração do efeito (DURAÇÃO)
    void setPad (int newPad, float durSamples)
    {
        dur = std::max (64.0f, durSamples);
        if (newPad == pad) return;
        pad = newPad;
        if (pad != None)
        {
            p = (double) now - 1.0; v = 1.0; t = 0; active = true;
        }
    }

    int currentPad() const { return pad; }

    inline void process (float& l, float& r)
    {
        buf[0][(size_t) (now & mask)] = l;
        buf[1][(size_t) (now & mask)] = r;
        ++now;
        const float target = pad != None ? 1.0f : 0.0f;
        const float step = 1.0f / (float) xfLen;
        mix = target > mix ? std::min (target, mix + step) : std::max (target, mix - step);
        if (mix <= 0.0f) { active = false; return; }

        float g = 1.0f;
        const float k = std::min (1.0f, t / dur);
        switch (pad)
        {
            case TapeStop: v = std::pow (1.0f - k, 1.6f); g = std::min (1.0f, (float) v * 5.0f); break;
            case Rewind:   v = k < 0.25f ? 1.0 - 4.5 * (k / 0.25f) : -3.5; g = k < 0.7f ? 1.0f : std::max (0.0f, 1.0f - (k - 0.7f) / 0.3f); break;
            case Reverse:  v = -1.0; break;
            case SpinBack: v = k < 0.12f ? 1.0 - 6.0 * (k / 0.12f) : -5.0 * (1.0 - (k - 0.12) / 0.88); g = std::max (0.0f, 1.0f - std::max (0.0f, k - 0.4f) / 0.6f); break;
            default: break;
        }
        t += 1.0f;
        // a leitura nunca passa a gravação nem sai do buffer
        p = std::clamp (p + v, (double) now - (double) N + 8.0, (double) now - 1.0);
        float y[2];
        for (int c = 0; c < 2; ++c)
        {
            const double fl = std::floor (p);
            const int64_t i = (int64_t) fl; const float tt = (float) (p - fl);
            const float a = buf[(size_t) c][(size_t) (i & mask)], b = buf[(size_t) c][(size_t) ((i + 1) & mask)];
            y[c] = (a + tt * (b - a)) * g;
        }
        l = l * (1.0f - mix) + y[0] * mix;
        r = r * (1.0f - mix) + y[1] * mix;
    }

private:
    float fs = 48000.0f, dur = 48000.0f, t = 0, mix = 0;
    double p = 0, v = 1;
    int N = 1, mask = 0, xfLen = 512, pad = None;
    int64_t now = 0;
    bool active = false;
    std::array<std::vector<float>, 2> buf;
};

//==============================================================================
// EQ gráfico: HPF + 4 sinos + LPF (12 dB/oit nos filtros de corte) e ganho
struct EqSettings
{
    float hp = 20.0f, lp = 20000.0f;
    float f[4] { 100.0f, 500.0f, 2000.0f, 7000.0f };
    float g[4] { 0, 0, 0, 0 };
    float q[4] { 1.0f, 1.2f, 1.2f, 1.0f };
    float gainDb = 0.0f;
};

class GraphicEq
{
public:
    void prepare (double sampleRate) { fs = (float) sampleRate; update (s, true); }
    void update (const EqSettings& ns, bool force = false)
    {
        if (! force && std::equal (std::begin (ns.f), std::end (ns.f), std::begin (s.f)) && std::equal (std::begin (ns.g), std::end (ns.g), std::begin (s.g))
            && std::equal (std::begin (ns.q), std::end (ns.q), std::begin (s.q)) && ns.hp == s.hp && ns.lp == s.lp && ns.gainDb == s.gainDb)
            return;
        s = ns;
        design (s, fs, coef);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 6; ++i) st[c][i].copyCoefs (coef[i]);
        gain = std::pow (10.0f, s.gainDb / 20.0f);
    }
    static void design (const EqSettings& s, float fs, Biquad* out)
    {
        if (s.hp > 20.5f) out[0].setHighPass (s.hp, 0.70710678f, fs); else out[0].setIdentity();
        for (int i = 0; i < 4; ++i)
            if (std::abs (s.g[i]) > 0.01f) out[1 + i].setPeak (s.f[i], s.q[i], s.g[i], fs); else out[1 + i].setIdentity();
        if (s.lp < 19900.0f) out[5].setLowPass (s.lp, 0.70710678f, fs); else out[5].setIdentity();
    }
    // curva para a interface
    static float responseDb (const EqSettings& s, float f, float fs = 48000.0f)
    {
        Biquad b[6]; design (s, fs, b);
        float d = s.gainDb;
        for (auto& x : b) d += x.magnitudeDb (f, fs);
        return d;
    }
    inline void process (float& l, float& r)
    {
        for (int i = 0; i < 6; ++i) { l = st[0][i].process (l); r = st[1][i].process (r); }
        l *= gain; r *= gain;
    }
private:
    float fs = 48000.0f, gain = 1.0f;
    EqSettings s;
    Biquad coef[6];
    Biquad st[2][6];
};

} // namespace hd
