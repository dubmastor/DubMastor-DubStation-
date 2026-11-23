#pragma once
// Mastor Half Dub MHD-050 — o aparelho inteiro (sem JUCE): bandas + DROP + fita + pads + eco + EQ + Big Knob.
#include "HalfDubDSP.h"
#include "TapeEcho.h"
#include "TubbyFilter.h"
#include <atomic>

namespace hd
{
static constexpr float kEchoDivBeats[7] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };   // 1/16 .. 1/2
static constexpr float kXoverLo = 180.0f, kXoverHi = 2400.0f;

struct EngineParams
{
    // velocidade
    int   speed = 2;                    // seletor VELOCIDADE (0..5)
    bool  groove = false;               // motor
    bool  drop = true;                  // DROP ligado
    float powerDownBeats = 0.0f, powerUpBeats = 0.0f;
    int   smoothMode = 1;
    // bandas
    int   bandSpeed[3] { 0, 0, 0 };     // 0 = segue o seletor, 1..6 = 1/1 2/3 1/2 1/3 1/4 REV, 7 = OFF
    float bandMix[3] { 1, 1, 1 };
    float bandSmooth[3] { 0.3f, 0.3f, 0.3f };
    int   bandLoop[3] { 4, 4, 4 };      // índice em kLoopBeats (4 = 1 bar)
    bool  bandSolo[3] { false, false, false };
    // pads
    int   pad = 0;                      // SoundSystemPads::Pad
    float padDur = 0.5f;                // 0..1
    bool  padSync = true;
    // fita
    float wow = 0.2f, sat = 0.3f;
    // eco
    bool  echoOn = true, echoPP = false, echoMte = false;
    int   echoDiv = 5;
    float echoFb = 0.45f, echoSend = 0.25f, washout = 0.6f;
    // EQ
    bool  eqBypass = false;
    bool  eqOnTotal = false;            // false = só no sinal lento
    EqSettings eq;
    // Big Knob
    float bkCut = 0.0f, bkRes = 0.25f, bkDrive = 0.0f, bkMix = 1.0f;
    bool  bkStepped = true, bkLowPass = false, bk24 = false, bkBypass = false;
    // saída
    float dryWet = 1.0f, outDb = 0.0f;
    bool  bypass = false;
};

struct HostClock
{
    bool   playing = false;
    double ppq = 0.0;     // posição em semínimas no início do bloco
    double bpm = 120.0;
};

// padrões de duração dos pads: em tempos (SYNC) ou em segundos
inline float padDurationSamples (float v, bool sync, double bpm, float fs)
{
    if (sync) return (float) (0.25 * std::pow (2.0, 4.0 * v) * 60.0 / bpm * fs);   // 1/16 .. 1 bar
    return (float) (0.1 * std::pow (2.0, 5.0 * v) * fs);                            // 0,1 .. 3,2 s
}
inline float effectiveRatio (const EngineParams& p, int band)
{
    const int bs = p.bandSpeed[band];
    if (bs == 0) return kSpeedRatio[std::clamp (p.speed, 0, kNumSpeeds - 1)];
    if (bs >= 1 && bs <= kNumSpeeds) return kSpeedRatio[bs - 1];
    return 1.0f;
}

class Engine
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        for (auto& s : split) s.prepare (kXoverLo, kXoverHi, fs);
        for (auto& b : bands) b.prepare (sampleRate);
        tape.prepare (sampleRate);
        pads.prepare (sampleRate);
        echo.prepare (sampleRate);
        eq.prepare (sampleRate);
        bigKnob.prepare (sampleRate, 2);
        sm = 1.0f - std::exp (-1.0f / (0.015f * fs));
        for (int b = 0; b < 3; ++b) { bandGain[b] = 1; bandMixSm[b] = 1; }
        dwSm = p.dryWet; outSm = 1; bypSm = p.bypass ? 1.0f : 0.0f; bkBypSm = p.bkBypass ? 1.0f : 0.0f;
        eqBypSm = p.eqBypass ? 1.0f : 0.0f; echoOnSm = p.echoOn ? 1.0f : 0.0f;
        washEnv = 0; lastDrop = p.drop; internalPpq = 0; lastSeg[0] = lastSeg[1] = lastSeg[2] = -1;
        for (auto& h : hist) for (auto& v : h) v.store (0.0f);
        histPos.store (0); histAcc[0] = histAcc[1] = histAcc[2] = 0; histN = 0;
    }

    void setParams (const EngineParams& np) { p = np; }
    const EngineParams& params() const { return p; }

    // processa um bloco estéreo no lugar
    void process (float* L, float* R, int n, const HostClock& clk)
    {
        const double bpm = std::clamp (clk.bpm, 30.0, 300.0);
        const double spb = 60.0 / bpm * fs;                   // amostras por tempo
        double ppq0 = clk.playing ? clk.ppq : internalPpq;
        internalPpq = ppq0 + n / spb;

        // bandas
        const bool anySolo = p.bandSolo[0] || p.bandSolo[1] || p.bandSolo[2];
        for (int b = 0; b < 3; ++b)
        {
            SlowBand::Settings s;
            s.ratio = effectiveRatio (p, b);
            s.groove = p.groove;
            s.smoothMode = p.smoothMode;
            s.smooth = p.bandSmooth[b];
            s.drop = p.drop;
            s.downSamples = (float) (p.powerDownBeats * spb);
            s.upSamples = (float) (p.powerUpBeats * spb);
            bands[b].setSettings (s);
            bandTarget[b] = (p.bandSpeed[b] == 7 || (anySolo && ! p.bandSolo[b])) ? 0.0f : 1.0f;
        }

        // DROP soltou: WASHOUT joga o sinal lento no eco por ~1 compasso
        if (lastDrop && ! p.drop) washEnv = p.washout;
        lastDrop = p.drop;
        const float washDecay = std::exp (-1.0f / (float) (4.0 * spb * 0.5));

        dub::EchoSettings es;
        es.delaySec = kEchoDivBeats[std::clamp (p.echoDiv, 0, 6)] * (float) (60.0 / bpm);
        es.feedback = p.echoFb;
        es.toneHz = p.echoMte ? 1800.0f : 3600.0f;
        es.wow = p.echoMte ? 0.55f : 0.15f;
        es.pingPong = p.echoPP;
        es.heads = p.echoMte ? 3 : 1;

        tape.set (p.wow, p.sat);
        pads.setPad (p.pad, padDurationSamples (p.padDur, p.padSync, bpm, fs));
        eq.update (p.eq);

        dub::Settings bk;
        bk.knob = p.bkCut; bk.stepped = p.bkStepped; bk.highPass = ! p.bkLowPass; bk.steep = p.bk24;
        bk.resonance = p.bkRes; bk.driveDb = p.bkDrive * 24.0f; bk.mix = p.bkMix;
        bigKnob.setSettings (bk);

        const float outT = std::pow (10.0f, p.outDb / 20.0f);
        float pk[2] = { 0, 0 }; double sq = 0;

        for (int i = 0; i < n; ++i)
        {
            const double ppq = ppq0 + i / spb;
            const float xl = L[i], xr = R[i];

            float lo[2], mid[2], hi[2];
            split[0].process (xl, lo[0], mid[0], hi[0]);
            split[1].process (xr, lo[1], mid[1], hi[1]);
            const float* in[3][2] = { { &lo[0], &lo[1] }, { &mid[0], &mid[1] }, { &hi[0], &hi[1] } };

            float dryL = lo[0] + mid[0] + hi[0], dryR = lo[1] + mid[1] + hi[1];   // seco com a mesma fase
            float wl = 0, wr = 0;
            for (int b = 0; b < 3; ++b)
            {
                const double loopB = kLoopBeats[std::clamp (p.bandLoop[b], 0, kNumLoops - 1)];
                const int64_t seg = (int64_t) std::floor (ppq / loopB);
                const bool newSeg = seg != lastSeg[b];
                lastSeg[b] = seg;
                const double posIn = (ppq - (double) seg * loopB) * spb;
                float yl, yr;
                bands[b].process (*in[b][0], *in[b][1], newSeg, posIn, loopB * spb, yl, yr);
                bandMixSm[b] += sm * (p.bandMix[b] - bandMixSm[b]);
                bandGain[b] += sm * (bandTarget[b] - bandGain[b]);
                const float m = bandMixSm[b];
                wl += (*in[b][0] * (1.0f - m) + yl * m) * bandGain[b];
                wr += (*in[b][1] * (1.0f - m) + yr * m) * bandGain[b];
                histAcc[b] = std::max (histAcc[b], std::abs (yl) + std::abs (yr));
            }

            // cor de fita e EQ "só no sinal lento": entram junto com o DROP
            const float dropOn = bands[0].isWet() || bands[1].isWet() || bands[2].isWet() ? 1.0f : 0.0f;
            wetAmt += sm * (dropOn - wetAmt);
            eqBypSm += sm * ((p.eqBypass ? 1.0f : 0.0f) - eqBypSm);
            {
                float tl = wl, tr = wr;
                tape.process (tl, tr);
                if (! p.eqOnTotal)
                {
                    float el = tl, er = tr;
                    eq.process (el, er);
                    tl = el * (1.0f - eqBypSm) + tl * eqBypSm;
                    tr = er * (1.0f - eqBypSm) + tr * eqBypSm;
                }
                wl = wl * (1.0f - wetAmt) + tl * wetAmt;
                wr = wr * (1.0f - wetAmt) + tr * wetAmt;
            }

            dwSm += sm * (p.dryWet - dwSm);
            float yl = dryL * (1.0f - dwSm) + wl * dwSm;
            float yr = dryR * (1.0f - dwSm) + wr * dwSm;

            // eco: recebe o sinal lento enquanto o DROP está ligado (+ o WASHOUT quando solta)
            echoOnSm += sm * ((p.echoOn ? 1.0f : 0.0f) - echoOnSm);
            washEnv *= washDecay;
            es.send = std::max (p.echoSend * dropOn, washEnv) * (p.echoOn ? 1.0f : 0.0f);
            es.feedback = std::min (1.1f, p.echoFb + 0.3f * washEnv);
            echo.setSettings (es);
            float el, er;
            echo.processSample (wl, wr, el, er);
            yl += el * echoOnSm; yr += er * echoOnSm;

            // pads de sound system no som todo
            pads.process (yl, yr);

            if (p.eqOnTotal)
            {
                float ql = yl, qr = yr;
                eq.process (ql, qr);
                yl = ql * (1.0f - eqBypSm) + yl * eqBypSm;
                yr = qr * (1.0f - eqBypSm) + yr * eqBypSm;
            }

            // Mastor Big Knob
            {
                float bl = yl, br = yr;
                float* ch[2] = { &bl, &br };
                bigKnob.process (ch, 2, 1);
                bkBypSm += sm * ((p.bkBypass ? 1.0f : 0.0f) - bkBypSm);
                yl = bl * (1.0f - bkBypSm) + yl * bkBypSm;
                yr = br * (1.0f - bkBypSm) + yr * bkBypSm;
            }

            outSm += sm * (outT - outSm);
            yl *= outSm; yr *= outSm;
            // proteção: nunca deixa passar NaN nem picos absurdos
            if (! std::isfinite (yl)) yl = 0;
            if (! std::isfinite (yr)) yr = 0;
            yl = std::clamp (yl, -4.0f, 4.0f); yr = std::clamp (yr, -4.0f, 4.0f);

            bypSm += sm * ((p.bypass ? 1.0f : 0.0f) - bypSm);
            yl = yl * (1.0f - bypSm) + xl * bypSm;
            yr = yr * (1.0f - bypSm) + xr * bypSm;

            L[i] = yl; R[i] = yr;
            pk[0] = std::max (pk[0], std::abs (yl)); pk[1] = std::max (pk[1], std::abs (yr));
            sq += 0.5 * (yl * yl + yr * yr);

            if (++histN >= kHistStep)
            {
                histN = 0;
                const int hp = histPos.load (std::memory_order_relaxed);
                for (int b = 0; b < 3; ++b) { hist[b][(size_t) hp].store (histAcc[b] * 0.5f, std::memory_order_relaxed); histAcc[b] = 0; }
                histPos.store ((hp + 1) % kHist, std::memory_order_release);
            }
        }

        peak[0].store (pk[0]); peak[1].store (pk[1]);
        rms.store ((float) std::sqrt (sq / std::max (1, n)));
        for (int b = 0; b < 3; ++b)
        {
            const double loopB = kLoopBeats[std::clamp (p.bandLoop[b], 0, kNumLoops - 1)];
            const double ppqEnd = ppq0 + n / spb;
            progress[b].store ((float) ((ppqEnd - std::floor (ppqEnd / loopB) * loopB) / loopB));
            ratioNow[b].store (bands[b].currentRatio());
            wetNow[b].store (bands[b].isWet() ? 1.0f : 0.0f);
        }
        bpmNow.store ((float) bpm);
        padNow.store (pads.currentPad());
        bkHz.store (bigKnob.currentFrequency());
    }

    // ---- dados para a interface
    static constexpr int kHist = 512, kHistStep = 256;
    std::array<std::array<std::atomic<float>, kHist>, 3> hist;
    std::atomic<int> histPos { 0 };
    std::atomic<float> peak[2] { { 0 }, { 0 } }, rms { 0 }, progress[3] { { 0 }, { 0 }, { 0 } };
    std::atomic<float> ratioNow[3] { { 1 }, { 1 }, { 1 } }, wetNow[3] { { 0 }, { 0 }, { 0 } }, bpmNow { 120 }, bkHz { 20 };
    std::atomic<int> padNow { 0 };

private:
    float fs = 48000.0f, sm = 0.01f;
    EngineParams p;
    ThreeBandSplit split[2];
    SlowBand bands[3];
    TapeColour tape;
    SoundSystemPads pads;
    dub::TapeEcho echo;
    GraphicEq eq;
    dub::TubbyFilter bigKnob;
    float bandGain[3] { 1, 1, 1 }, bandMixSm[3] { 1, 1, 1 }, bandTarget[3] { 1, 1, 1 };
    float wetAmt = 0, dwSm = 1, outSm = 1, bypSm = 0, bkBypSm = 0, eqBypSm = 0, echoOnSm = 1, washEnv = 0;
    bool lastDrop = true;
    double internalPpq = 0;
    int64_t lastSeg[3] { -1, -1, -1 };
    float histAcc[3] { 0, 0, 0 };
    int histN = 0;
};

} // namespace hd
