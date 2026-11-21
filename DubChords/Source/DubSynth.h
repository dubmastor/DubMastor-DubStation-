// Som interno do Dub Chords: os mesmos timbres do Dub Station Roots Keys
//  - skank (canal 1): piano estilo Korg M1 com chorus estéreo
//  - bubble (canal 3): órgão tonewheel estilo Hammond B3, drawbars 888000000 do reggae,
//    key click, overdrive valvulado e Leslie lento
//  - baixo guia (canal 2): seno com um pouco de 2º harmônico
// Tudo sintetizado (sem samples), header-only, sem JUCE.
#pragma once
#include "RootsKeysEngine.h"

class DubSoundEngine
{
public:
    float pianoLevel = 0.8f, organLevel = 0.6f, bassLevel = 0.7f, master = 0.8f;

    DubSoundEngine()
    {
        // piano do M1 com cara de skank: brilhante e de decaimento curto
        p.pianoBright = 0.65f;
        p.pianoDecay  = 0.30f;
        p.pianoChorus = 0.55f;
        p.pianoLevel  = 1.0f;
        // Hammond clássico do bubble: 16' 5⅓' 8' puxados, sem percussão, Leslie lento
        const float db[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
        for (int i = 0; i < 9; ++i) p.drawbar[i] = db[i];
        p.percOn = false;
        p.click = 0.45f;
        p.leslieFast = false;
        p.leslieMix = 0.85f;
        p.drive = 0.28f;
        p.organLevel = 1.0f;
    }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        chorus.prepare (sr);
        leslie.prepare (sr);
        reverb.prepare (sr);
        for (auto& v : piano) v.active = false;
        for (auto& v : organ) v.active = false;
        for (auto& b : bass) b.active = false;
        smoothCoef = 1.0f - std::exp (-1.0f / (0.01f * sr));
        for (int i = 0; i < 9; ++i)
        {
            const float d = p.drawbar[i];
            dbGain[i] = d < 0.01f ? 0.0f : std::pow (10.0f, (d - 8.0f) * 3.0f / 20.0f);
        }
    }

    void noteOn (int channel, int note, float vel)
    {
        ++stamp;
        if (channel == 3)
        {
            auto& v = steal (organ, note);
            v.start (note, sr, p, false, stamp);
        }
        else if (channel == 2)
        {
            auto& b = stealBass (note);
            b.start (note, vel, sr);
        }
        else
        {
            auto& v = steal (piano, note);
            v.start (note, vel, sr, p, stamp);
        }
    }

    void noteOff (int channel, int note)
    {
        if (channel == 3)
        {
            for (auto& v : organ) if (v.active && v.held && v.note == note) v.release (p.click);
        }
        else if (channel == 2)
        {
            for (auto& b : bass) if (b.active && b.held && b.note == note) b.held = false;
        }
        else
        {
            for (auto& v : piano) if (v.active && v.held && v.note == note) { v.held = false; v.release (sr); }
        }
    }

    // corta todas as vozes na hora (usado quando o SOM INTERNO liga/desliga:
    // enquanto desligado o motor não recebe os note-offs, então nada pode ficar segurado)
    void killAll()
    {
        for (auto& v : piano) v.active = false;
        for (auto& v : organ) v.active = false;
        for (auto& b : bass)  b.active = false;
    }

    void allNotesOff()
    {
        for (auto& v : piano) if (v.active) { v.held = false; v.release (sr); }
        for (auto& v : organ) if (v.active) v.release (0.0f);
        for (auto& b : bass) b.held = false;
    }

    // soma no buffer (não limpa)
    void render (float* L, float* R, int n)
    {
        for (int s = 0; s < n; ++s)
        {
            float pin = 0, oin = 0, bin = 0;
            for (auto& v : piano) if (v.active) pin += v.tick();
            for (auto& v : organ) if (v.active) oin += v.tick (dbGain, false);
            for (auto& b : bass)  if (b.active) bin += b.tick();

            float pl, pr;
            chorus.process (pin, p.pianoChorus, pl, pr);

            const float drv = 1.0f + p.drive * 9.0f;
            const float od = std::tanh (oin * drv * 0.5f) / std::tanh (drv * 0.5f);
            float ll, lr;
            leslie.process (od, p.leslieFast, ll, lr);
            const float m = p.leslieMix;
            const float ol = (od * (1 - m) + ll * m), orr = (od * (1 - m) + lr * m);

            float outL = pl * pianoLevel + ol * organLevel + bin * bassLevel;
            float outR = pr * pianoLevel + orr * organLevel + bin * bassLevel;

            float rl, rr;
            reverb.process (outL, outR, rl, rr);
            outL = (outL + rl * 0.16f) * master;
            outR = (outR + rr * 0.16f) * master;

            L[s] += soft (outL);
            R[s] += soft (outR);
        }
    }

private:
    struct BassVoice
    {
        bool active = false, held = false;
        int note = -1;
        float ph = 0, inc = 0, env = 0, level = 0, att = 0, rel = 0;
        void start (int n, float vel, float sr)
        {
            active = true; held = true; note = n; ph = 0; env = 0;
            inc = rk::midiToHz ((float) n) / sr;
            level = 0.35f * vel;
            att = 1.0f - std::exp (-1.0f / (0.004f * sr));
            rel = std::exp (-1.0f / (0.06f * sr));
        }
        inline float tick()
        {
            if (held) env += (1.0f - env) * att;
            else { env *= rel; if (env < 1.0e-4f) active = false; }
            const auto& S = rk::sine();
            float p2 = ph * 2.0f; if (p2 >= 1.0f) p2 -= 1.0f;
            const float s = S (ph) + 0.22f * S (p2);
            ph += inc; if (ph >= 1.0f) ph -= 1.0f;
            return s * env * level;
        }
    };

    static inline float soft (float x)
    {
        if (x > 1.5f) return 1.0f;
        if (x < -1.5f) return -1.0f;
        return x - (4.0f / 27.0f) * x * x * x;
    }

    template <typename V, size_t N>
    V& steal (std::array<V, N>& pool, int note)
    {
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
    BassVoice& stealBass (int note)
    {
        for (auto& b : bass) if (b.active && b.note == note) return b;
        for (auto& b : bass) if (! b.active) return b;
        return bass[0];
    }

    rk::Params p;
    float sr = 44100.0f, smoothCoef = 0.01f;
    float dbGain[9] {};
    std::array<rk::PianoVoice, 24> piano;
    std::array<rk::OrganVoice, 16> organ;
    std::array<BassVoice, 4> bass;
    rk::StereoChorus chorus;
    rk::Leslie leslie;
    rk::Reverb reverb;
    uint64_t stamp = 0;
};
