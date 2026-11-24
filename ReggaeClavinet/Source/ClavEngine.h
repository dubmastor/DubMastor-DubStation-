// Mastor Reggae Clavinet MRCL-71 — o som: corda por modelo físico (Karplus-Strong) e a cadeia de efeitos.
// A tangente de metal bate na corda e a prende; a corda curta vibra até a ponte; os dois captadores perto da ponte
// filtram em pente (o som anasalado, o "quack"); ao soltar a tecla a lã abafa na hora com o "toc" da tangente.
// Mesmo modelo da versão para celular (site/web/reggaeclavinet/, função clavBuf). Sem JUCE: compila sozinho no teste.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mrcl
{
inline float clampf (float x, float a, float b) { return std::min (b, std::max (a, x)); }
inline float midiHz (float m) { return 440.0f * std::pow (2.0f, (m - 69.0f) / 12.0f); }

struct Noise { uint32_t s = 22222u; float next() { s = s * 1664525u + 1013904223u; return (float) (s >> 8) / 8388608.0f - 1.0f; } };

// ------------------------------------------------------------------ biquad (RBJ)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void set (int type, float f, float q, float gainDb, float sr)
    {
        const float w = 2.0f * 3.14159265f * clampf (f, 10.0f, 0.45f * sr) / sr, cw = std::cos (w), sw = std::sin (w);
        const float A = std::pow (10.0f, gainDb / 40.0f), alpha = sw / (2.0f * std::max (0.05f, q));
        float B0, B1, B2, A0, A1, A2;
        switch (type)
        {
            case 0: /* passa-baixas */ B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case 1: /* passa-altas */  B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case 2: /* pico */         B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A; A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
            case 3: /* agudos */ { const float sq = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) + (A - 1) * cw + sq); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sq);
                A0 = (A + 1) - (A - 1) * cw + sq; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - sq; break; }
            default: /* passa-tudo */  B0 = 1 - alpha; B1 = -2 * cw; B2 = 1 + alpha; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

struct ClavParams
{
    bool pickupCD = true, pickupAB = false;   // C/D: captador mais cheio · A/B: mais perto da ponte, mais fino
    float bright = 0.85f, decay = 0.5f, mute = 0.08f;
};

// ------------------------------------------------------------------ uma corda
class ClavVoice
{
public:
    static constexpr int RingSize = 8192;   // atraso de até ~5800 amostras (C-1 a 96 kHz)
    int note = -1; uint32_t tag = 0; bool active = false;

    void start (int n, float velocity, float sr, const ClavParams& p, uint32_t id)
    {
        note = n; tag = id; active = true; releasing = false; relGain = 1.0f; thunkPos = -1; age = 0;
        const float f = midiHz ((float) n), vel = clampf (velocity, 0.0f, 1.0f);
        const float T = clampf ((0.8f + 3.0f * p.decay) * std::pow (2.0f, -((float) n - 48.0f) / 20.0f), 0.3f, 5.0f) * (1.0f - 0.86f * p.mute);
        g = clampf (0.8f + 0.2f * clampf (((float) n - 48.0f) / 24.0f, -0.5f, 1.2f) - 0.25f * (1.0f - p.bright) - 0.3f * p.mute - 0.08f * (1.0f - vel), 0.3f, 0.97f);
        const float P = sr / f - (1.0f - g) / g;
        Ni = std::clamp ((int) std::floor (P), 2, RingSize - 8); fr = P - (float) Ni;
        loss = std::pow (0.001f, 1.0f / (f * T));
        lifeSamples = (int) (sr * std::min (4.5f, T * 1.15f + 0.05f));
        // excitação: a tangente bate seca (pulso curto e brilhante; mais forte = mais agudo)
        const float a = clampf (0.18f + 0.4f * vel * (0.5f + 0.5f * p.bright), 0.15f, 0.6f);
        exc.assign ((size_t) Ni + 1, 0.0f);
        float lp = 0, mean = 0;
        for (int i = 0; i <= Ni; ++i)
        {
            const float x = (i < std::max (2.0f, Ni * 0.04f) ? 0.8f : 0.0f) + noise.next() * 0.35f;
            lp += a * (x - lp); exc[(size_t) i] = lp; mean += lp;
        }
        mean /= (float) (Ni + 1);
        for (auto& e : exc) e -= mean;
        taps.clear();
        if (p.pickupAB) taps.push_back (std::max (1, (int) std::lround (P * 0.07f)));
        if (p.pickupCD || taps.empty()) taps.push_back (std::max (1, (int) std::lround (P * 0.15f)));
        // normalização: mede o pico do primeiro trecho (como a versão web, que normaliza o buffer inteiro)
        resetString();
        float pk = 1e-6f;
        const int pre = std::min (lifeSamples, 2 * (Ni + 1) + 64);
        for (int i = 0; i < pre; ++i) pk = std::max (pk, std::fabs (tick()));
        resetString();
        gain = 0.16f * (0.35f + 0.65f * vel) / pk;
        thunkAmp = 0.05f * vel;
    }

    void release (float sr)
    {
        if (! active || releasing) return;
        releasing = true; relCoef = std::exp (-1.0f / (0.009f * sr)); thunkPos = 0;
    }

    void kill() { active = false; note = -1; }

    // soma a corda em out[0..n)
    void render (float* out, int n, float sr)
    {
        if (! active) return;
        const int thunkLen = (int) (0.02f * sr);
        for (int i = 0; i < n; ++i)
        {
            float s = tick() * gain;
            if (releasing) { relGain *= relCoef; s *= relGain; }
            if (thunkPos >= 0 && thunkPos < thunkLen)   // o "toc" da tangente voltando
            {
                th += 0.15f * (noise.next() - th);
                const float e = 1.0f - (float) thunkPos / (float) thunkLen;
                s += th * e * e * e * 3.0f * thunkAmp;
                ++thunkPos;
            }
            out[i] += s;
            if (++age > lifeSamples || (releasing && relGain < 1e-4f && (thunkPos < 0 || thunkPos >= thunkLen))) { kill(); return; }
        }
    }

private:
    std::array<float, RingSize> ring {};
    std::vector<float> exc;
    std::vector<int> taps;
    int Ni = 100, w = 0, n0 = 0, age = 0, lifeSamples = 0, thunkPos = -1;
    float fr = 0, g = 0.8f, loss = 0.999f, z = 0, gain = 1, relGain = 1, relCoef = 1, th = 0, thunkAmp = 0;
    bool releasing = false;
    Noise noise;

    void resetString() { ring.fill (0.0f); w = 0; n0 = 0; z = 0; }
    float at (int back) const { return ring[(size_t) ((w - back) & (RingSize - 1))]; }
    float tick()
    {
        const float a1 = n0 >= Ni ? at (Ni) : 0.0f, a2 = n0 > Ni ? at (Ni + 1) : 0.0f;
        z += g * (a1 * (1.0f - fr) + a2 * fr - z);
        const float y = (n0 <= Ni ? exc[(size_t) n0] : 0.0f) + z * loss;
        ring[(size_t) w] = y;
        float o = 0;
        for (int d : taps) o += y - (n0 >= d ? at (d) : 0.0f);
        w = (w + 1) & (RingSize - 1); ++n0;
        return o;
    }
};

// ------------------------------------------------------------------ o instrumento inteiro (vozes + amp + efeitos)
struct FxParams
{
    std::array<bool, 4> filters { true, false, true, false };   // BRILL., TREBLE, MEDIUM, SOFT
    float volume = 0.8f;
    bool wahOn = false; float wahSens = 0.6f, wahRange = 0.7f;
    bool phOn = false;  float phRate = 0.4f, phDepth = 0.55f;
    bool echoOn = false; int echoTime = 1; float echoFb = 0.4f, echoMix = 0.45f;   // Tape Echo começa sempre desligado
    bool maxOn = true; float reverb = 0.2f, master = 0.8f;
    bool m1 = true;   // timbre M1 CLAV (estilo Korg M1): brilho digital, agudos +3,5 dB em 5 kHz
    double bpm = 76;
};

class ClavInstrument
{
public:
    static constexpr int NumVoices = 24;
    ClavParams clav; FxParams fx;

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& v : voices) v.kill();
        hp.set (1, 28, 0.7f, 0, sr); nasal.set (2, 1500, 1.1f, 4, sr);
        tone[0].set (3, 3200, 0.7f, 6, sr); tone[1].set (2, 2200, 1.0f, 4, sr); tone[2].set (2, 650, 0.9f, 4, sr); tone[3].set (0, 1400, 0.7f, 0, sr); m1Shelf.set (3, 5000, 0.7f, 3.5f, sr);
        echoBuf.assign ((size_t) (sr * 2.5f), 0.0f); echoW = 0; echoLp.set (0, 2600, 0.7f, 0, sr);
        revBufL.assign ((size_t) (sr * 0.12f), 0.0f); revW = 0;
        for (auto& c : combs) c.init (sr);
        for (auto& a : aps) a.init (sr);
        env = 0; wahEnv = 0;
        setTimeConstants();
    }

    void noteOn (int n, float vel)
    {
        ClavVoice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.tag < v->tag) v = &x; }   // rouba a mais antiga
        v->start (n, vel, sr, clav, ++nextTag);
        wahEnv = std::max (wahEnv, 0.4f + 0.6f * fx.wahSens * vel);   // o wah abre com o ataque
    }
    void noteOff (int n) { for (auto& x : voices) if (x.active && x.note == n) x.release (sr); }
    void allOff() { for (auto& x : voices) if (x.active) x.release (sr); }
    int activeVoices() const { int c = 0; for (auto& x : voices) c += x.active ? 1 : 0; return c; }

    // mono -> saída estéreo
    void render (float* L, float* R, int n)
    {
        std::fill (L, L + n, 0.0f);
        for (auto& v : voices) v.render (L, n, sr);
        const float shp = std::tanh (1.6f);
        const float phase0 = (float) (2.0 * 3.14159265 * fx.phRate / sr);
        const double beatSec = 60.0 / std::max (30.0, fx.bpm);
        static const double times[4] = { 0.5, 0.75, 1.0, 1.5 };
        const int echoDelay = std::clamp ((int) (beatSec * times[std::clamp (fx.echoTime, 0, 3)] * sr), 1, (int) echoBuf.size() - 1);
        const float wahDecay = std::exp (-1.0f / (0.09f * sr));
        for (int i = 0; i < n; ++i)
        {
            float x = L[i];
            x = hp.process (x); x = nasal.process (x);
            x = std::tanh (1.6f * x) / shp;                              // amp do clavinet, leve saturação
            for (int k = 0; k < 4; ++k) if (fx.filters[(size_t) k]) x = tone[k].process (x);   // botões de filtro
            if (fx.m1) x = m1Shelf.process (x);
            x *= fx.volume;
            if (fx.wahOn)
            {
                if ((i & 15) == 0) { const float fc = 350.0f + 2600.0f * fx.wahRange * wahEnv; wah.set (0, fc, 6.0f, 0, sr); }
                x = 0.25f * x + wah.process (x);
                wahEnv *= wahDecay;
            }
            else wahEnv *= wahDecay;
            if (fx.phOn)
            {
                phase += phase0; if (phase > 6.2831853f) phase -= 6.2831853f;
                if ((i & 15) == 0) { const float fc = 900.0f + 700.0f * fx.phDepth * std::sin (phase); for (auto& a : ph) a.set (4, fc, 0.6f, 0, sr); }
                float y = x; for (auto& a : ph) y = a.process (y);
                x = 0.7f * x + 0.7f * y;
            }
            // tape echo
            const float d = echoBuf[(size_t) ((echoW - echoDelay + (int) echoBuf.size()) % (int) echoBuf.size())];
            const float dl = echoLp.process (d);
            echoBuf[(size_t) echoW] = x + (fx.echoOn ? fx.echoFb : 0.0f) * dl;
            echoW = (echoW + 1) % (int) echoBuf.size();
            float out = x + (fx.echoOn ? fx.echoMix * dl : 0.0f);
            // reverb curto (4 pentes + 2 passa-tudo)
            float rv = 0; for (auto& c : combs) rv += c.process (out * 0.25f);
            for (auto& a : aps) rv = a.process (rv);
            out += 0.5f * fx.reverb * rv;
            // maximizer: compressor rápido (-18 dB, 6:1) + ganho
            const float lvl = std::fabs (out);
            env = lvl > env ? env + (lvl - env) * atkC : env * relC;
            float gr = 1.0f;
            if (fx.maxOn && env > 0.126f) gr = std::pow (env / 0.126f, 1.0f / 6.0f - 1.0f);
            out *= gr * (fx.maxOn ? 1.3f : 1.0f) * fx.master;
            out = std::tanh (out * 1.1f) / 1.1f;   // limitador suave (nunca passa de 0 dBFS)
            L[i] = out; R[i] = out;
        }
    }

private:
    struct Comb { std::vector<float> b; int w = 0; float fb = 0.78f, lp = 0; void init (float sr) { static int k = 0; const float t[4] = { 0.0297f, 0.0371f, 0.0411f, 0.0437f }; b.assign ((size_t) (sr * t[k++ % 4]), 0.0f); w = 0; lp = 0; }
        float process (float x) { const float y = b[(size_t) w]; lp += 0.3f * (y - lp); b[(size_t) w] = x + fb * lp; w = (w + 1) % (int) b.size(); return y; } };
    struct Ap { std::vector<float> b; int w = 0; void init (float sr) { static int k = 0; const float t[2] = { 0.005f, 0.0017f }; b.assign ((size_t) (sr * t[k++ % 2]), 0.0f); w = 0; }
        float process (float x) { const float y = b[(size_t) w]; const float o = -x + y; b[(size_t) w] = x + 0.5f * y; w = (w + 1) % (int) b.size(); return o; } };

    float sr = 48000;
    std::array<ClavVoice, NumVoices> voices;
    uint32_t nextTag = 0;
    Biquad hp, nasal, tone[4], m1Shelf, wah, ph[4], echoLp;
    std::vector<float> echoBuf, revBufL; int echoW = 0, revW = 0;
    Comb combs[4]; Ap aps[2];
    float env = 0, wahEnv = 0, phase = 0;
    float atkC = 0.0f, relC = 0.0f;
public:
    void setTimeConstants() { atkC = 1.0f - std::exp (-1.0f / (0.003f * sr)); relC = std::exp (-1.0f / (0.12f * sr)); }
};
} // namespace mrcl
