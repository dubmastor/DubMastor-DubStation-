#pragma once
// Mastor Tune MT-0420 — o afinador inteiro (sem o JUCE): detecção + escolha da nota + PSOLA.
// O PluginProcessor só passa os ajustes, a harmonia e as notas corrigidas no editor.
#include "TuneDSP.h"
#include <functional>

namespace mt
{
struct TuneSettings
{
    float    amount  = 0.3f;    // 0 = clean .. 1 = robô
    uint16_t allowed = 0x0fff;  // notas da escala
    uint16_t chord   = 0;       // notas do acorde do momento (0 = sem acorde)
    int      voice   = 1;       // 0 grave, 1 média, 2 aguda
    float    mix     = 1.0f;    // 0 = seco .. 1 = afinado
};

// uma análise (a cada ~5,8 ms) — vai para o editor de notas
struct HopInfo
{
    int64_t time = 0;      // tempo (amostras de entrada) do centro da janela analisada
    float   midi = 0.0f;   // nota cantada (0 = sem nota)
    int     target = -1;   // nota escolhida
    bool    doubt = false; // a voz ficou entre duas notas / fora do acorde
    float   ratio = 1.0f;
};

class TuneEngine
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        tracker.prepare (sampleRate);
        tracker.setVoice (s.voice);
        psola.prepare (sampleRate);
        corrector.prepare ((double) tracker.hopFullRate() / sampleRate);
        n = 0; wetMix = s.mix; activity = 0.0f; lastTarget = -1;
    }

    int  latency() const { return psola.latency(); }
    void setSettings (const TuneSettings& ns)
    {
        if (ns.voice != s.voice) tracker.setVoice (ns.voice);
        s = ns;
    }

    // nota forçada pelo editor (devolve -1 quando não há correção manual neste tempo)
    std::function<int (int64_t)> overrideAt;
    // chamada a cada análise (thread de áudio: não pode alocar nem travar)
    std::function<void (const HopInfo&)> onHop;

    inline float process (float x)
    {
        PitchResult pr;
        if (tracker.push (x, pr))
        {
            const int64_t t = n - tracker.analysisDelay();
            const bool voiced = pr.hz > 0.0f && pr.hz * 1.0f < 2000.0f;
            HopInfo h; h.time = t;
            float ratio = 1.0f;
            if (voiced)
            {
                const float midi = hzToMidi (pr.hz);
                h.midi = midi;
                const int forced = overrideAt ? overrideAt (t) : -1;
                if (forced >= 0)
                {
                    // nota escolhida no editor: vai para ela, na oitava que está mais perto
                    int tg = forced;
                    while ((float) tg - midi > 6.0f) tg -= 12;
                    while (midi - (float) tg > 6.0f) tg += 12;
                    h.target = tg;
                }
                else
                {
                    const NoteChoice c = Corrector::choose (midi, s.allowed, s.chord, lastTarget);
                    h.target = c.target; h.doubt = c.doubt;
                }
                ratio = corrector.update (midi, h.target, s.amount, true);
                lastTarget = h.target;
                psola.setControl (t, (float) (fs / pr.hz), ratio, true);
            }
            else
            {
                ratio = corrector.update (0.0f, -1, s.amount, false);
                lastTarget = -1;
                psola.setControl (t, 0.0f, 1.0f, false);
            }
            h.ratio = ratio;
            // quanto o afinador está mexendo (para cruzar com o sinal seco quando não há correção)
            const float cents = std::abs (1200.0f * std::log2 (ratio));
            activityTarget = (voiced && cents > (s.amount < 0.7f ? 3.0f : 0.5f)) ? 1.0f : 0.0f;
            if (onHop) onHop (h);
        }
        ++n;
        const float wet = psola.process (x);
        const float dry = psola.dryDelayed();
        // sem correção (ou sem nota) sai o próprio sinal, atrasado: transparência total
        activity += (activityTarget - activity) * actCoef();
        wetMix += (s.mix - wetMix) * 0.002f;
        const float tuned = dry + activity * (wet - dry);
        return dry + wetMix * (tuned - dry);
    }

private:
    float actCoef() const { return (float) (1.0 - std::exp (-1.0 / (0.012 * fs))); }

    double fs = 44100.0;
    TuneSettings s;
    PitchTracker tracker;
    Psola psola;
    Corrector corrector;
    int64_t n = 0;
    int lastTarget = -1;
    float wetMix = 1.0f, activity = 0.0f, activityTarget = 0.0f;
};
} // namespace mt
