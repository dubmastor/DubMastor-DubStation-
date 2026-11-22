#pragma once
// Eco de fita do DubMastor Big Knob (mesmo DSP, copiado de BigKnob/Source/DubFX.h). Independente do JUCE.
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <complex>

namespace dub
{

static constexpr float kTwoPi = 6.28318530718f;

struct OnePole
{
    float z = 0.0f;
    inline float lp (float x, float a) { z += a * (x - z); return z; }
    static float coef (float hz, float fs) { return 1.0f - std::exp (-kTwoPi * hz / fs); }
};

//==============================================================================
// Eco de fita: mudança de tempo com "glide" de fita, saturação e filtro no loop,
// wow/flutter, ping-pong e envio suavizado (para o THROW não estalar).
struct EchoSettings
{
    float delaySec  = 0.375f;
    float feedback  = 0.55f;  // 0..1.2 (acima de 1 = autooscilação controlada)
    float toneHz    = 2500.0f;
    float wow       = 0.25f;  // 0..1
    float send      = 0.0f;   // 0..1
    bool  pingPong  = false;
    int   heads     = 1;      // bits: 1 = cabeçote 1 (1x), 2 = cabeçote 2 (2x), 4 = cabeçote 3 (3x)
};

class TapeEcho
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        size = (int) (6.0f * fs) + 8;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        glideCoef = std::exp (-1.0f / (0.25f * fs));  // glide de ~250 ms ao mudar o tempo
        sendCoef  = std::exp (-1.0f / (0.01f * fs));  // 10 ms
        hpA = OnePole::coef (110.0f, fs);
        reset();
    }

    void reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        for (auto& f : toneLP) f.z = 0;
        for (auto& f : hpLP)   f.z = 0;
        curDelay = std::clamp (s.delaySec * fs, 4.0f, (float) size - 8.0f);
        sendSm = s.send; wowPh = flutPh = 0.0f; w = 0;
    }

    void setSettings (const EchoSettings& e) { s = e; }

    // in: entrada (L,R); out: retorno molhado (L,R)
    inline void processSample (float inL, float inR, float& outL, float& outR)
    {
        const float target = std::clamp (s.delaySec * fs, 4.0f, ((float) size - 8.0f) / 3.0f);
        curDelay = target + glideCoef * (curDelay - target);
        sendSm   = s.send + sendCoef * (sendSm - s.send);

        wowPh  += kTwoPi * 0.55f / fs; if (wowPh  > kTwoPi) wowPh  -= kTwoPi;
        flutPh += kTwoPi * 6.3f  / fs; if (flutPh > kTwoPi) flutPh -= kTwoPi;
        const float mod = s.wow * (0.0018f * fs * std::sin (wowPh) + 0.00015f * fs * std::sin (flutPh));

        // cabeçotes estilo Space Echo: 1x, 2x e 3x o tempo base
        float y[2] = { 0.0f, 0.0f };
        int active = 0;
        for (int h = 0; h < 3; ++h)
        {
            if (! (s.heads & (1 << h))) continue;
            ++active;
            const float d = std::min (curDelay * (float) (h + 1), (float) size - 8.0f);
            for (int ch = 0; ch < 2; ++ch)
                y[ch] += read (ch, d + mod + (ch ? 0.0007f * fs * s.wow : 0.0f));
        }
        if (active == 0) { active = 1; for (int ch = 0; ch < 2; ++ch) y[ch] = read (ch, curDelay + mod); }
        const float fbNorm = 1.0f / (float) active;
        const float outNorm = 1.0f / std::sqrt ((float) active);

        const float toneA = OnePole::coef (s.toneHz, fs);
        const float g = s.feedback;
        float fb[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            const float src = (s.pingPong ? y[1 - ch] : y[ch]) * fbNorm;
            float f = toneLP[ch].lp (src, toneA);
            f -= hpLP[ch].lp (f, hpA);                  // tira o grave acumulado
            fb[ch] = 0.9f * std::tanh (f * g / 0.9f);   // saturação de fita
        }

        float in0 = inL * sendSm, in1 = inR * sendSm;
        if (s.pingPong) { in0 = 0.5f * (in0 + in1); in1 = 0.0f; }

        buf[0][(size_t) w] = in0 + fb[0];
        buf[1][(size_t) w] = in1 + fb[1];
        if (++w >= size) w = 0;

        outL = y[0] * outNorm; outR = y[1] * outNorm;
    }

private:
    inline float read (int ch, float d) const
    {
        float pos = (float) w - d;
        while (pos < 0) pos += (float) size;
        const int i1 = (int) pos; const float t = pos - (float) i1;
        auto at = [&](int i) { i %= size; if (i < 0) i += size; return buf[ch][(size_t) i]; };
        const float y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
        // Hermite cúbica
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    float fs = 44100.0f, curDelay = 1000.0f, glideCoef = 0, sendCoef = 0, sendSm = 0, hpA = 0;
    float wowPh = 0, flutPh = 0;
    int size = 1, w = 0;
    EchoSettings s;
    std::array<std::vector<float>, 2> buf;
    std::array<OnePole, 2> toneLP, hpLP;
};

} // namespace dub
