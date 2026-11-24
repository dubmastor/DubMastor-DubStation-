// Som interno do Mastor Reggae Chord Machine (MRCM-1240): toca pelo próprio motor do
// DubMastor Roots Keys (rk::Engine, ReggaeKeys/Source/RootsKeysEngine.h), com a mesma
// cadeia do Roots Keys (vozes -> chorus/Leslie -> reverb -> master -> Maximizer).
//  - skank (canal 1): piano estilo Korg M1 com o timbre INICIAL do Roots Keys
//    (= preset "Piano: M1 Classic", os valores padrão dos parâmetros do Roots Keys)
//  - bubble (canal 3): Hammond no preset "Organ: Roots Percussion" do Roots Keys
//
// (O DubSynth.h antigo continua no repositório só porque o Mastor Reggae Bass Machine MRBM-69 usa.)
//
// IMPORTANTE: os números em RootsKeysPiano / RootsKeysOrgan têm que ser iguais aos do
// Roots Keys. O teste tests/check_rootskeys_timbre.py (roda no CI) lê
// ReggaeKeys/Source/PluginProcessor.cpp e falha se alguém mudar o timbre lá e não aqui.
// Header-only, sem JUCE.
#pragma once
#include "RootsKeysEngine.h"

namespace rkdefaults
{
    // Piano inicial do Roots Keys (padrões dos parâmetros = preset "Piano: M1 Classic")
    struct RootsKeysPiano
    {
        static constexpr int   type   = 6;      // pianoType: 6 = M1 Grand Piano (o timbre inicial do Reggae Keys MRK-80 desde a 3.1)
        static constexpr float bright = 0.65f;
        static constexpr float decay  = 0.55f;
        static constexpr float chorus = 0.50f;
        static constexpr float level  = 0.80f;
        static constexpr float reverb = 0.20f;
        static constexpr float master = 0.80f;
        static constexpr bool  maxOn  = true;
        static constexpr float maxGainDb = 3.0f;
        static constexpr float maxCeilDb = -0.3f;
    };

    // Preset "Organ: Roots Percussion" do Roots Keys
    struct RootsKeysOrgan
    {
        static constexpr float drawbar[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
        static constexpr bool  percOn   = true;
        static constexpr bool  perc3rd  = true;
        static constexpr bool  percFast = true;
        static constexpr float click    = 0.50f;
        static constexpr float drive    = 0.30f;
        static constexpr float leslieMix = 0.80f;
        static constexpr float level    = 0.75f;
        static constexpr bool  leslieFast = false;
        static constexpr float reverb   = 0.18f;
    };
}

class RootsKeysSoundEngine
{
public:
    RootsKeysSoundEngine()
    {
        using P = rkdefaults::RootsKeysPiano;
        auto& pp = piano.params;
        pp.mode = 0;
        pp.pianoType   = P::type;
        pp.pianoBright = P::bright;
        pp.pianoDecay  = P::decay;
        pp.pianoChorus = P::chorus;
        pp.pianoLevel  = P::level;
        pp.reverb = P::reverb;
        pp.master = P::master;
        pp.maxOn = P::maxOn; pp.maxGainDb = P::maxGainDb; pp.maxCeilDb = P::maxCeilDb;
        pp.echoOn = false; pp.flOn = false;

        using O = rkdefaults::RootsKeysOrgan;
        auto& op = organ.params;
        op.mode = 1;
        for (int i = 0; i < 9; ++i) op.drawbar[i] = O::drawbar[i];
        op.percOn = O::percOn; op.perc3rd = O::perc3rd; op.percFast = O::percFast;
        op.click = O::click; op.drive = O::drive; op.leslieMix = O::leslieMix;
        op.organLevel = O::level; op.leslieFast = O::leslieFast;
        op.reverb = O::reverb;
        op.master = P::master;
        op.maxOn = P::maxOn; op.maxGainDb = P::maxGainDb; op.maxCeilDb = P::maxCeilDb;
        op.echoOn = false; op.flOn = false;
    }

    // não é tempo real (aloca): chamar em prepareToPlay
    void prepare (double sampleRate, int maxBlock)
    {
        piano.prepare (sampleRate);
        organ.prepare (sampleRate);
        mixLimiter.prepare ((float) sampleRate);
        const auto n = (size_t) std::max (maxBlock, 16);
        pL.assign (n, 0.0f); pR.assign (n, 0.0f);
        oL.assign (n, 0.0f); oR.assign (n, 0.0f);
    }

    int maxBlock() const { return (int) pL.size(); }
    int latencySamples() const { return piano.latencySamples(); }

    void noteOn (int channel, int note, float vel)
    {
        if (channel == 3) organ.noteOn (note, vel);
        else              piano.noteOn (note, vel);
    }

    void noteOff (int channel, int note)
    {
        if (channel == 3) organ.noteOff (note);
        else              piano.noteOff (note);
    }

    // solta tudo (com a cauda natural de cada timbre): usado quando o loop muda,
    // o preview para ou o SOM INTERNO liga/desliga
    void allNotesOff()
    {
        piano.allNotesOff();
        organ.allNotesOff();
    }

    // Renderiza n amostras (n <= maxBlock()) e SOMA em L/R, com rampas de volume
    // (pianoFrom -> pianoTo, organFrom -> organTo) para não estalar ao girar os knobs.
    void render (float* L, float* R, int n, float pianoFrom, float pianoTo, float organFrom, float organTo)
    {
        n = std::min (n, maxBlock());
        if (n <= 0) return;
        std::fill (pL.begin(), pL.begin() + n, 0.0f); std::fill (pR.begin(), pR.begin() + n, 0.0f);
        std::fill (oL.begin(), oL.begin() + n, 0.0f); std::fill (oR.begin(), oR.begin() + n, 0.0f);
        piano.render (pL.data(), pR.data(), n);
        organ.render (oL.data(), oR.data(), n);

        const float ceil = std::pow (10.0f, -0.3f / 20.0f);
        for (int s = 0; s < n; ++s)
        {
            const float t = (float) (s + 1) / (float) n;
            const float gp = pianoFrom + (pianoTo - pianoFrom) * t;
            const float go = organFrom + (organTo - organFrom) * t;
            float l = pL[(size_t) s] * gp + oL[(size_t) s] * go;
            float r = pR[(size_t) s] * gp + oR[(size_t) s] * go;
            mixLimiter.process (l, r, true, 1.0f, ceil);   // piano + órgão juntos nunca passam de -0.3 dB
            L[s] += l;
            R[s] += r;
        }
    }

private:
    rk::Engine piano, organ;
    rk::Maximizer mixLimiter;
    std::vector<float> pL, pR, oL, oR;
};
