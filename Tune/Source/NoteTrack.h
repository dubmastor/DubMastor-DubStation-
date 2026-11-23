#pragma once
// Editor de notas do Mastor Tune: guarda as notas cantadas (presas à timeline da DAW, em tempos)
// e as notas corrigidas à mão. Vive na thread de mensagens; o áudio só lê as correções prontas.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>

struct HopFrame
{
    double ppq = 0;     // posição na timeline (tempos)
    float  midi = 0;    // nota cantada (0 = sem nota)
    int    target = -1; // nota escolhida pelo afinador
    bool   doubt = false;
};

struct SungNote
{
    int    id = 0;
    double start = 0, end = 0;   // tempos
    int    target = 0;           // nota que o afinador escolheu
    float  sung = 0;             // média do que foi cantado
    bool   doubt = false;        // ficou entre duas notas / fora do acorde
    int    edit = -1;            // nota escolhida no editor (-1 = sem correção manual)
    int    shown() const { return edit >= 0 ? edit : target; }
};

// correções prontas para o áudio (cópia dupla: a thread de mensagens escreve na outra e troca)
struct OverrideSet
{
    static constexpr int kMax = 1024;
    struct O { double start, end; int midi; };
    O items[kMax];
    int count = 0;

    int find (double ppq) const
    {
        int lo = 0, hi = count - 1;
        while (lo <= hi)
        {
            const int mid = (lo + hi) / 2;
            if (ppq < items[mid].start) hi = mid - 1;
            else if (ppq >= items[mid].end) lo = mid + 1;
            else return items[mid].midi;
        }
        return -1;
    }
};

class NoteTrack
{
public:
    NoteTrack() { sets[0].count = sets[1].count = 0; }

    // ---- thread de áudio
    int overrideAt (double ppq) const
    {
        const auto& s = sets[active.load (std::memory_order_acquire)];
        return s.count > 0 ? s.find (ppq) : -1;
    }

    // ---- thread de mensagens
    // junta as análises novas em notas; devolve true se algo mudou
    bool addFrames (const HopFrame* f, int n)
    {
        bool changed = false;
        for (int i = 0; i < n; ++i)
        {
            const auto& h = f[i];
            // tocando de novo por cima: as notas sem correção manual daquele trecho saem
            for (int k = (int) notes.size() - 1; k >= 0; --k)
            {
                auto& nn = notes[(size_t) k];
                if (nn.edit < 0 && h.ppq >= nn.start - 1e-6 && h.ppq < nn.end && ! isCurrent (nn))
                { notes.erase (notes.begin() + k); changed = true; }
            }
            if (insideEdited (h.ppq)) { closeCurrent(); continue; }

            const bool jump = cur.active && (h.ppq < cur.lastPpq - 1e-6 || h.ppq - cur.lastPpq > 0.12);
            if (h.midi <= 0.0f || h.target < 0 || jump || (cur.active && h.target != cur.target))
            {
                changed |= closeCurrent();
                if (h.midi <= 0.0f || h.target < 0) continue;
            }
            if (! cur.active)
            {
                cur = {};
                cur.active = true; cur.start = h.ppq; cur.target = h.target;
            }
            cur.lastPpq = h.ppq;
            cur.sum += h.midi; cur.n++;
            if (h.doubt) cur.doubts++;
        }
        return changed;
    }

    bool closeCurrent()
    {
        if (! cur.active) return false;
        cur.active = false;
        const double end = cur.lastPpq + 0.03;
        if (end - cur.start < 0.06 || cur.n < 3) return false;   // pedacinho: não vira nota
        SungNote s;
        s.id = nextId++;
        s.start = cur.start; s.end = end; s.target = cur.target;
        s.sung = cur.sum / (float) cur.n;
        s.doubt = cur.doubts * 10 > cur.n * 4;   // mais de 40 % das análises em dúvida
        insertSorted (s);
        return true;
    }

    void setEdit (int id, int midi)
    {
        for (auto& n : notes) if (n.id == id) { n.edit = midi; n.doubt = n.doubt && midi < 0; }
        publish();
    }

    void clear() { notes.clear(); cur = {}; publish(); }

    const std::vector<SungNote>& getNotes() const { return notes; }
    int pendingDoubts() const { int c = 0; for (auto& n : notes) if (n.doubt && n.edit < 0) ++c; return c; }

    // ---- salvar e abrir junto com o projeto
    juce::ValueTree toTree() const
    {
        juce::ValueTree t ("Notes");
        for (auto& n : notes)
        {
            juce::ValueTree c ("N");
            c.setProperty ("s", n.start, nullptr); c.setProperty ("e", n.end, nullptr);
            c.setProperty ("t", n.target, nullptr); c.setProperty ("m", n.sung, nullptr);
            c.setProperty ("d", n.doubt, nullptr); c.setProperty ("x", n.edit, nullptr);
            t.appendChild (c, nullptr);
        }
        return t;
    }
    void fromTree (const juce::ValueTree& t)
    {
        notes.clear(); cur = {};
        for (auto c : t)
        {
            SungNote n;
            n.id = nextId++;
            n.start = c.getProperty ("s"); n.end = c.getProperty ("e");
            n.target = c.getProperty ("t"); n.sung = (float) (double) c.getProperty ("m");
            n.doubt = c.getProperty ("d"); n.edit = c.getProperty ("x", -1);
            if (n.end > n.start) insertSorted (n);
        }
        publish();
    }

    // MIDI da melodia afinada (para arrastar para a DAW ou para o MRM)
    juce::MidiFile toMidi (double bpm) const
    {
        juce::MidiMessageSequence seq;
        seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / juce::jmax (30.0, bpm))), 0);
        for (auto& n : notes)
        {
            const int note = juce::jlimit (0, 127, n.shown());
            seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), n.start * 960.0);
            seq.addEvent (juce::MidiMessage::noteOff (1, note), n.end * 960.0);
        }
        seq.updateMatchedPairs();
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (960);
        mf.addTrack (seq);
        return mf;
    }

private:
    struct Cur { bool active = false; double start = 0, lastPpq = 0; int target = -1; float sum = 0; int n = 0, doubts = 0; };

    bool isCurrent (const SungNote&) const { return false; }

    bool insideEdited (double ppq) const
    {
        for (auto& n : notes) if (n.edit >= 0 && ppq >= n.start && ppq < n.end) return true;
        return false;
    }

    void insertSorted (const SungNote& s)
    {
        auto it = std::lower_bound (notes.begin(), notes.end(), s, [] (const SungNote& a, const SungNote& b) { return a.start < b.start; });
        notes.insert (it, s);
        if (notes.size() > 4000) notes.erase (notes.begin());
    }

    void publish()
    {
        const int next = 1 - active.load();
        auto& s = sets[next];
        s.count = 0;
        for (auto& n : notes)
            if (n.edit >= 0 && s.count < OverrideSet::kMax)
                s.items[s.count++] = { n.start, n.end, n.edit };
        active.store (next, std::memory_order_release);
    }

    std::vector<SungNote> notes;
    Cur cur;
    int nextId = 1;
    OverrideSet sets[2];
    std::atomic<int> active { 0 };
};
