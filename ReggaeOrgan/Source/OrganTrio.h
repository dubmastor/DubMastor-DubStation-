// OrganTrio.h — o som do Mastor Reggae Organ MRO-68 (header-only, sem JUCE)
//
// Usa o MESMO órgão tonewheel do Mastor Reggae Keys MRK-80 (ReggaeKeys/Source/RootsKeysEngine.h),
// em três instâncias, como dois organistas dividindo as partes (Tyrone Downie e Wire Lindo nos Wailers):
//   [0] BUBBLE acento   registro próprio, sem Leslie (o bubble é seco e grave)
//   [1] BUBBLE fantasma  mesmo registro, volume do knob GHOST (o Hammond não tem velocity:
//                        a nota fantasma precisa de um nível próprio para soar mais fraca)
//   [2] CHOP + FILLS     registro cheio (888), Leslie, drive, percussão opcional, eco de fita
// Depois da soma: reverb (de cada motor), master e o mesmo maximizer do MRK-80.
#pragma once

#include "RootsKeysEngine.h"

namespace mro
{
struct SoundParams
{
    float bubbleDb[9] = { 0, 0, 8, 2, 0, 0, 0, 0, 0 };
    float chopDb[9]   = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
    float bubbleLevel = 0.85f, ghostLevel = 0.5f, chopLevel = 0.75f;
    int   leslie = 0;          // 0 desligada, 1 lenta, 2 rápida (mão direita)
    bool  perc = false;
    float drive = 0.2f, click = 0.35f;
    bool  echoOn = false;
    float echoVol = 0.5f, echoIntensity = 0.4f, echoRate = 0.5f;
    float reverb = 0.15f, master = 0.8f;
    bool  maxOn = true;
    int   instr = 0;           // 0 Hammond B3 · 1 Rhodes Stage · 2 Rhodes Suitcase · 3 Wurlitzer 200A · 4 FM EP (DX7)
};

// instrumento -> timbre de piano elétrico do motor do MRK-80 (o 0 é o órgão)
inline int epPianoType (int instr)
{
    static const int t[5] = { 0, 10, 11, 12, 3 };
    return t[instr < 0 ? 0 : (instr > 4 ? 4 : instr)];
}

class OrganTrio
{
public:
    enum { Accent = 0, Ghost = 1, Chop = 2, NumEngines = 3 };

    void prepare (double sr)
    {
        for (auto& e : eng) e.prepare (sr);
        maxi.prepare ((float) sr);
    }

    // latência total: o maximizer interno de cada motor (sempre na cadeia) + o maximizer final
    int latencySamples() const { return eng[0].latencySamples() + maxi.la; }
    float gainReductionDb() const { return maxi.grDb; }

    rk::Engine& engine (int i) { return eng[i]; }

    void setParams (const SoundParams& sp)
    {
        for (int i = 0; i < NumEngines; ++i)
        {
            rk::Params& p = eng[i].params;
            const bool rh = (i == Chop);
            // HAMMOND: o órgão de sempre. Os pianos elétricos tocam o MESMO groove (bubble, chop e fills)
            // com o timbre do MRK-80; os drawbars, a Leslie e a percussão só valem no Hammond.
            p.mode = sp.instr == 0 ? 1 : 0;
            p.pianoType   = epPianoType (sp.instr);
            p.pianoBright = rh ? 0.6f : 0.45f;     // bubble mais redondo, chop mais brilhante
            p.pianoDecay  = rh ? 0.45f : 0.3f;
            p.pianoChorus = sp.instr == 2 ? 0.15f : (rh ? 0.35f : 0.2f);
            for (int d = 0; d < 9; ++d) p.drawbar[d] = rh ? sp.chopDb[d] : sp.bubbleDb[d];
            p.percOn     = rh && sp.perc;
            p.perc3rd    = true;
            p.percFast   = true;
            p.click      = sp.click;
            p.drive      = rh ? sp.drive : sp.drive * 0.5f;
            p.leslieFast = rh && sp.leslie == 2;
            p.leslieMix  = (rh && sp.leslie > 0) ? 0.85f : 0.0f;
            p.organLevel = i == Accent ? sp.bubbleLevel
                         : i == Ghost  ? sp.bubbleLevel * (0.12f + 0.88f * sp.ghostLevel)
                                       : sp.chopLevel;
            p.pianoLevel = p.organLevel;
            // eco de fita só na mão direita (o bubble seco segura o groove), como num rack de dub
            p.echoOn        = rh && sp.echoOn;
            p.echoMode      = 7;
            p.echoVol       = sp.echoVol;
            p.echoIntensity = sp.echoIntensity;
            p.echoRate      = sp.echoRate;
            p.echoSpring    = 0.3f;
            p.echoWow       = 0.3f;
            p.echoAge       = 0.3f;
            p.flOn   = false;
            p.reverb = rh ? sp.reverb : sp.reverb * 0.5f;
            p.master = 1.0f;
            p.maxOn  = false;                 // a limitação é feita uma vez só, depois da soma
        }
        master = sp.master;
        maxOn  = sp.maxOn;
    }

    void render (float* L, float* R, int n)
    {
        while (n > 0)
        {
            const int m = std::min (n, kChunk);
            std::fill (L, L + m, 0.0f);
            std::fill (R, R + m, 0.0f);
            for (auto& e : eng)
            {
                e.render (tL, tR, m);
                for (int s = 0; s < m; ++s) { L[s] += tL[s]; R[s] += tR[s]; }
            }
            for (int s = 0; s < m; ++s)
            {
                float l = L[s] * master, r = R[s] * master;
                maxi.process (l, r, maxOn, gain, 0.966f);   // ganho +1 dB, teto -0.3 dB
                if (! maxOn) { l = std::tanh (l); r = std::tanh (r); }
                L[s] = l; R[s] = r;
            }
            L += m; R += m; n -= m;
        }
    }

    void allNotesOff() { for (auto& e : eng) e.allNotesOff(); }

private:
    static constexpr int kChunk = 256;
    rk::Engine eng[NumEngines];
    rk::Maximizer maxi;
    float tL[kChunk] = {}, tR[kChunk] = {};
    float master = 0.8f, gain = 1.122f;
    bool maxOn = true;
};
} // namespace mro
