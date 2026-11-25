// MRM-0420 2.7: ARRANJO (partes da música e a ordem delas), SHOW (10 riddims salvos), MIXER e a sirene
// com o Mastor Tape Echo MTE-12. Igual à versão para celular (site/riddim/): ao mudar um, mude o outro.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <cmath>
#include <memory>
#include <vector>
#include "TapeEchoDSP.h"   // motor do Mastor Tape Echo MTE-12 (ReggaeKeys/Source, o mesmo do TapeEcho/Source)

namespace mrm
{
constexpr int numMachines = 3;   // Drums, Chords (riddim · keys), Bass — a mesma ordem do enum do MrmProcessor

//==============================================================================
// ARRANJO: cada parte tem nome, cor, compassos e quais máquinas tocam nela; a ordem é uma lista de partes.
// Tocando o arranjo, a harmonia recomeça no início de cada parte (a intro da bateria, se ligada, só no começo da música).
struct Section
{
    juce::String id, name;
    juce::uint32 colour = 0xffe6b83a;
    int bars = 8;
    std::array<bool, numMachines> on { true, true, true };
};

struct Arrangement
{
    bool on = false, loop = false;
    std::vector<Section> sections;
    std::vector<int> order;            // índices em sections

    int totalBars() const
    {
        int t = 0;
        for (int i : order) if (i >= 0 && i < (int) sections.size()) t += sections[(size_t) i].bars;
        return t;
    }
    bool active() const { return on && ! order.empty() && totalBars() > 0; }

    // posição dentro da música (em semínimas, sem a intro): parte da ordem, início dela e onde termina
    struct Where { int slot = -1; double start = 0.0, end = 0.0; bool ended = false; };
    Where find (double beat) const
    {
        Where w;
        const double total = totalBars() * 4.0;
        if (total <= 0.0) { w.ended = true; return w; }
        if (beat < 0.0) beat = 0.0;
        if (loop) beat = std::fmod (beat, total);
        else if (beat >= total) { w.ended = true; w.start = total; w.end = 1.0e12; return w; }
        double acc = 0.0;
        for (int k = 0; k < (int) order.size(); ++k)
        {
            const int i = order[(size_t) k];
            if (i < 0 || i >= (int) sections.size()) continue;
            const double len = sections[(size_t) i].bars * 4.0;
            if (beat < acc + len) { w.slot = k; w.start = acc; w.end = acc + len; return w; }
            acc += len;
        }
        w.ended = true;
        return w;
    }
    const Section* sectionAt (int slot) const
    {
        if (slot < 0 || slot >= (int) order.size()) return nullptr;
        const int i = order[(size_t) slot];
        return i >= 0 && i < (int) sections.size() ? &sections[(size_t) i] : nullptr;
    }

    static Arrangement makeDefault()
    {
        Arrangement a;
        auto add = [&a] (const char* id, const char* name, juce::uint32 c, int bars, bool d, bool ch, bool b)
        {
            Section s; s.id = id; s.name = juce::String::fromUTF8 (name); s.colour = c; s.bars = bars; s.on = { d, ch, b };
            a.sections.push_back (s);
        };
        // as mesmas partes da versão para celular: Intro e Outro sem baixo, Dub só bateria e baixo
        add ("intro",  "Intro",  0xff7fae6a, 4, true, true,  false);
        add ("verso",  "Verso",  0xffe6b83a, 8, true, true,  true);
        add ("refrao", "Refr\xc3\xa3o", 0xffd9573f, 8, true, true, true);
        add ("ponte",  "Ponte",  0xff3fa89a, 4, true, true,  true);
        add ("dub",    "Dub",    0xff5b7fc4, 8, true, false, true);
        add ("outro",  "Outro",  0xffa07a52, 4, true, true,  false);
        a.order = { 0, 1, 2, 1, 2, 4, 2, 5 };
        return a;
    }

    juce::ValueTree toTree() const
    {
        juce::ValueTree t ("ARRANJO");
        t.setProperty ("on", on, nullptr);
        t.setProperty ("loop", loop, nullptr);
        for (const auto& s : sections)
        {
            juce::ValueTree c ("PARTE");
            c.setProperty ("id", s.id, nullptr);
            c.setProperty ("name", s.name, nullptr);
            c.setProperty ("colour", (juce::int64) s.colour, nullptr);
            c.setProperty ("bars", s.bars, nullptr);
            c.setProperty ("mask", (s.on[0] ? 1 : 0) | (s.on[1] ? 2 : 0) | (s.on[2] ? 4 : 0), nullptr);
            t.appendChild (c, nullptr);
        }
        juce::StringArray o;
        for (int i : order) o.add (juce::String (i));
        t.setProperty ("order", o.joinIntoString (","), nullptr);
        return t;
    }
    static Arrangement fromTree (const juce::ValueTree& t)
    {
        if (! t.isValid() || ! t.hasType ("ARRANJO")) return makeDefault();
        Arrangement a;
        a.on = (bool) t.getProperty ("on", false);
        a.loop = (bool) t.getProperty ("loop", false);
        for (const auto& c : t)
        {
            if (! c.hasType ("PARTE")) continue;
            Section s;
            s.id = c.getProperty ("id").toString();
            s.name = c.getProperty ("name").toString();
            s.colour = (juce::uint32) (juce::int64) c.getProperty ("colour", (juce::int64) 0xffe6b83a);
            s.bars = juce::jlimit (1, 64, (int) c.getProperty ("bars", 8));
            const int m = (int) c.getProperty ("mask", 7);
            s.on = { (m & 1) != 0, (m & 2) != 0, (m & 4) != 0 };
            a.sections.push_back (s);
        }
        for (auto& tok : juce::StringArray::fromTokens (t.getProperty ("order").toString(), ",", ""))
            if (tok.isNotEmpty())
            {
                const int i = tok.getIntValue();
                if (i >= 0 && i < (int) a.sections.size()) a.order.push_back (i);
            }
        if (a.sections.empty()) return makeDefault();
        return a;
    }
};

//==============================================================================
// Sirene do MRM: o oscilador da DUB SIREN do Mastor Big Knob (mesmo timbre, mesmos controles PITCH, RATE,
// DEPTH, FORMA e LEVEL), sem o delay próprio: o eco agora é o Mastor Tape Echo MTE-12, ligado de fábrica.
class Siren
{
public:
    struct Settings { float pitchHz = 520.0f, rateHz = 2.0f, depth = 0.5f, level = 0.5f; int wave = 4; bool gate = false; };

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        atk = 1.0f - std::exp (-1.0f / (0.004f * fs));
        rel = 1.0f - std::exp (-1.0f / (0.09f * fs));
        env = 0.0f; oscPh = 0.0f; lfoPh = 0.0f; lastGate = false;
    }
    void set (const Settings& n)
    {
        if (n.gate && ! lastGate) lfoPh = 0.0f;   // o LFO recomeça a cada disparo
        lastGate = n.gate;
        s = n;
    }
    bool silent() const { return ! s.gate && env < 1.0e-5f; }
    float next()
    {
        env += (s.gate ? atk : rel) * ((s.gate ? 1.0f : 0.0f) - env);
        if (env < 1.0e-5f) return 0.0f;
        constexpr float twoPi = 6.283185307f;
        lfoPh += s.rateHz / fs; if (lfoPh >= 1.0f) lfoPh -= 1.0f;
        float lfo;
        switch (s.wave)
        {
            case 0:  lfo = std::sin (twoPi * lfoPh); break;
            case 1:  lfo = 1.0f - 4.0f * std::fabs (lfoPh - 0.5f); break;
            case 2:  lfo = lfoPh < 0.5f ? 1.0f : -1.0f; break;
            case 3:  lfo = 2.0f * lfoPh - 1.0f; break;
            default: lfo = 1.0f - 2.0f * lfoPh; break;
        }
        const float f = std::min (s.pitchHz * std::exp2 (2.0f * s.depth * lfo), 0.45f * fs);
        oscPh += f / fs; if (oscPh >= 1.0f) oscPh -= 1.0f;
        return std::tanh (3.0f * std::sin (twoPi * oscPh)) * 0.6f * env * s.level;
    }

private:
    Settings s;
    float fs = 44100.0f, atk = 0, rel = 0, env = 0, oscPh = 0, lfoPh = 0;
    bool lastGate = false;
};
} // namespace mrm
