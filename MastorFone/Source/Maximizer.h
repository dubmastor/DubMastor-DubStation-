#pragma once
// Maximizer do DubMastor Big Knob (mesmo DSP, copiado de BigKnob/Source/DubFX.h). Independente do JUCE.
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace dub
{
// MAXIMIZER (v2.0): compressor de "cola" suave + limiter brickwall com lookahead.
// Um único controle (PUSH) empurra o volume; o teto (CEILING) nunca é ultrapassado.
struct MaximizerSettings
{
    float pushDb    = 0.0f;   // 0..20 dB de ganho
    float ceilingDb = -0.3f;  // -3..0 dBFS
    bool  on        = true;
};

class Maximizer
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        N = std::max (1, (int) std::lround (0.0015 * sampleRate));     // 1.5 ms de lookahead
        for (auto& d : delay) d.assign ((size_t) N, 0.0f);
        reqRing.assign ((size_t) N + 1, 1.0f);
        boxRing.assign ((size_t) N, 1.0f);
        relA     = 1.0f - std::exp (-1.0f / (0.080f * fs));             // release do limiter
        cAtk     = 1.0f - std::exp (-1.0f / (0.005f * fs));             // compressor
        cRel     = 1.0f - std::exp (-1.0f / (0.150f * fs));
        smoothA  = 1.0f - std::exp (-1.0f / (0.020f * fs));
        reset();
    }

    void reset()
    {
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.0f);
        std::fill (reqRing.begin(), reqRing.end(), 1.0f);
        std::fill (boxRing.begin(), boxRing.end(), 1.0f);
        boxSum = (double) N; env = 1.0f; compEnv = 0.0f; di = ri = bi = 0;
        pushLin = std::pow (10.0f, s.pushDb / 20.0f);
        lastGrDb = 0.0f;
    }

    void setSettings (const MaximizerSettings& m) { s = m; }
    int  getLatency() const { return N; }
    float getLastGainReductionDb() const { return lastGrDb; } // <= 0

    inline void processSample (float& l, float& r)
    {
        const float ceil = std::pow (10.0f, s.ceilingDb / 20.0f);
        pushLin += smoothA * ((s.on ? std::pow (10.0f, s.pushDb / 20.0f) : 1.0f) - pushLin);

        float xl = l, xr = r, compGrDb = 0.0f;
        if (s.on)
        {
            xl *= pushLin; xr *= pushLin;

            // compressor de cola: 1.6:1, joelho de 6 dB, limiar 6 dB abaixo do teto
            const float pk = std::max (std::fabs (xl), std::fabs (xr));
            compEnv += (pk > compEnv ? cAtk : cRel) * (pk - compEnv);
            const float lvl = 20.0f * std::log10 (std::max (compEnv, 1e-9f));
            const float over = lvl - (s.ceilingDb - 6.0f), W = 6.0f, slope = 1.0f / 1.6f - 1.0f;
            if (2.0f * over > W)                    compGrDb = slope * over;
            else if (2.0f * std::fabs (over) <= W)  compGrDb = slope * (over + W * 0.5f) * (over + W * 0.5f) / (2.0f * W);
            const float cg = std::pow (10.0f, compGrDb / 20.0f);
            xl *= cg; xr *= cg;
        }

        // limiter: ganho necessário, mínimo na janela de lookahead, release, média móvel
        const float peak = std::max (std::fabs (xl), std::fabs (xr));
        const float req  = (s.on && peak > ceil) ? ceil / peak : 1.0f;
        reqRing[(size_t) ri] = req;
        if (++ri > N) ri = 0;
        float wmin = 1.0f;
        for (float v : reqRing) wmin = std::min (wmin, v);
        env = wmin < env ? wmin : env + (wmin - env) * relA;
        boxSum += (double) env - (double) boxRing[(size_t) bi];
        boxRing[(size_t) bi] = env;
        if (++bi >= N) bi = 0;
        const float g = (float) (boxSum / (double) N);

        // atraso de N amostras (latência informada ao Cubase)
        const float dl = delay[0][(size_t) di], dr = delay[1][(size_t) di];
        delay[0][(size_t) di] = xl; delay[1][(size_t) di] = xr;
        if (++di >= N) di = 0;
        // sai a amostra de N passos atrás; cada valor da média móvel veio de uma janela
        // de mínimo que contém essa amostra, então o ganho final nunca deixa passar do teto.

        if (s.on)
        {
            l = std::clamp (dl * g, -ceil, ceil);   // trava de segurança
            r = std::clamp (dr * g, -ceil, ceil);
            lastGrDb = compGrDb + 20.0f * std::log10 (std::max (g, 1e-6f));
        }
        else { l = dl; r = dr; lastGrDb = 0.0f; }
    }

private:
    float fs = 44100.0f, relA = 0, cAtk = 0, cRel = 0, smoothA = 0;
    float env = 1.0f, compEnv = 0.0f, pushLin = 1.0f, lastGrDb = 0.0f;
    double boxSum = 0.0;
    int N = 64, di = 0, ri = 0, bi = 0;
    std::array<std::vector<float>, 2> delay;
    std::vector<float> reqRing, boxRing;
    MaximizerSettings s;
};

} // namespace dub
