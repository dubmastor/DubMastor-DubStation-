// GroovePlayer.h — toca o GrooveEngine no tempo da DAW (header-only, sem JUCE)
// Recebe a posição em tempos (ppq) do começo de cada bloco e devolve comandos de nota com o sample exato.
// O mesmo código roda no plugin e no teste offline (tests/render_test.cpp).
#pragma once

#include <functional>
#include "GrooveEngine.h"
#include "OrganTrio.h"

namespace mro
{
struct NoteCmd
{
    int sample = 0, engine = 0, note = 60, velocity = 100, layer = 0;
    bool on = true;
};

inline int engineFor (const Event& e)
{
    if (e.layer == LayerBubble) return e.accent ? OrganTrio::Accent : OrganTrio::Ghost;
    return OrganTrio::Chop;
}

class GroovePlayer
{
public:
    // eventsFor(bar) devolve as notas do compasso (início em tempos dentro do compasso, 0..4):
    // geradas pelo GrooveEngine ou as editadas no visor MIDI
    using EventsFn = std::function<std::vector<Event> (int bar)>;

    void reset() { pending.clear(); expected = -1.0; running = false; }
    bool isRunning() const { return running; }

    // solta tudo (transporte parou ou pulou)
    void stop (std::vector<NoteCmd>& out, int sample = 0)
    {
        for (auto& p : pending) out.push_back ({ sample, p.engine, p.note, 0, p.layer, false });
        pending.clear();
        running = false;
        expected = -1.0;
    }

    void process (double ppq, double bpm, double sampleRate, int numSamples,
                  const EventsFn& eventsFor, std::vector<NoteCmd>& out)
    {
        if (numSamples <= 0 || bpm <= 0.0) return;
        const double beats = (double) numSamples / sampleRate * bpm / 60.0;
        const double end = ppq + beats;
        auto toSample = [&] (double t) { return std::clamp ((int) ((t - ppq) / beats * numSamples), 0, numSamples - 1); };

        // pulo no transporte (loop da DAW, reposicionamento): solta as notas presas
        if (running && std::abs (ppq - expected) > 1.0e-3) stop (out, 0);
        running = true;

        std::vector<NoteCmd> blk;
        // 1) note-offs pendentes que caem neste bloco
        for (size_t i = 0; i < pending.size();)
        {
            if (pending[i].off < end)
            {
                blk.push_back ({ toSample (pending[i].off), pending[i].engine, pending[i].note, 0, pending[i].layer, false });
                pending.erase (pending.begin() + (long) i);
            }
            else ++i;
        }
        // 2) note-ons dos compassos que este bloco toca
        const int b0 = (int) std::floor (std::max (0.0, ppq) / 4.0), b1 = (int) std::floor (std::max (0.0, end) / 4.0);
        for (int bar = b0; bar <= b1; ++bar)
        {
            for (const auto& e : eventsFor (bar))
            {
                const double t = bar * 4.0 + e.start;
                if (t < ppq || t >= end) continue;
                const int eng = engineFor (e);
                const int smp = toSample (t);
                // nota que já está soando no mesmo motor: solta antes (senão o note-off antigo cortaria a nova)
                for (size_t i = 0; i < pending.size(); ++i)
                    if (pending[i].engine == eng && pending[i].note == e.note)
                    {
                        blk.push_back ({ smp, eng, e.note, 0, e.layer, false });
                        pending.erase (pending.begin() + (long) i);
                        break;
                    }
                for (auto& cmd : blk)   // note-off da mesma nota já marcado para depois, dentro do bloco: antecipa
                    if (! cmd.on && cmd.engine == eng && cmd.note == e.note && cmd.sample > smp) cmd.sample = smp;
                blk.push_back ({ smp, eng, e.note, e.velocity, e.layer, true });
                const double off = t + e.length;
                if (off < end) blk.push_back ({ std::max (smp, toSample (off)), eng, e.note, 0, e.layer, false });
                else pending.push_back ({ off, eng, e.note, e.layer });
            }
        }
        std::stable_sort (blk.begin(), blk.end(), [] (const NoteCmd& a, const NoteCmd& b)
        {
            if (a.sample != b.sample) return a.sample < b.sample;
            return (int) a.on < (int) b.on;   // note-off antes de note-on
        });
        out.insert (out.end(), blk.begin(), blk.end());
        expected = end;
    }

private:
    struct Pending { double off; int engine, note, layer; };
    std::vector<Pending> pending;
    double expected = -1.0;
    bool running = false;
};
} // namespace mro

namespace mro
{
// notas de um compasso a partir de um loop (editado ou gerado), com início relativo ao compasso
inline std::vector<Event> eventsOfBar (const std::vector<Event>& loop, int loopBarsCount, int bar)
{
    std::vector<Event> v;
    if (loopBarsCount <= 0) return v;
    const int b = ((bar % loopBarsCount) + loopBarsCount) % loopBarsCount;
    for (auto e : loop)
        if (e.start >= 4.0 * b && e.start < 4.0 * (b + 1)) { e.start -= 4.0 * b; v.push_back (e); }
    return v;
}

// acordes dos 4 tempos de um compasso
inline void chordsOfBar (const std::vector<Chord>& prog, int bar, Chord out[4])
{
    for (int i = 0; i < 4; ++i) out[i] = chordForBeat (prog, bar * 4 + i);
}
} // namespace mro
