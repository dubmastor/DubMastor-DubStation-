// Dub Chords — motor de harmonia (C++ puro, sem JUCE, testável isoladamente)
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace dub
{
//==============================================================================
// Qualidades de acorde
enum Quality { Maj, Min, Dom7, Min7, Maj7, Min6, Sus2, Sus4, Dim, Min9, NumQualities };

inline const char* qualityName (int q)
{
    static const char* names[NumQualities] = { "", "m", "7", "m7", "maj7", "m6", "sus2", "sus4", "dim", "m9" };
    return names[std::clamp (q, 0, NumQualities - 1)];
}

inline const std::vector<int>& intervals (int q)
{
    static const std::vector<int> table[NumQualities] = {
        { 0, 4, 7 },        // Maior
        { 0, 3, 7 },        // menor
        { 0, 4, 7, 10 },    // 7
        { 0, 3, 7, 10 },    // m7
        { 0, 4, 7, 11 },    // maj7
        { 0, 3, 7, 9 },     // m6
        { 0, 2, 7 },        // sus2
        { 0, 5, 7 },        // sus4
        { 0, 3, 6 },        // dim
        { 0, 3, 10, 14 },   // m9 (sem quinta, soa bem no skank)
    };
    return table[std::clamp (q, 0, NumQualities - 1)];
}

inline const char* rootName (int r)
{
    static const char* names[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[((r % 12) + 12) % 12];
}

//==============================================================================
// Durações (em tempos de semínima, compasso 4/4)
inline constexpr int numDurations = 6;
inline const char* durationName (int d)
{
    static const char* names[numDurations] = { "1/2 comp.", "1 comp.", "2 comp.", "3 comp.", "4 comp.", "8 comp." };
    return names[std::clamp (d, 0, numDurations - 1)];
}
inline double durationBeats (int d)
{
    static const double beats[numDurations] = { 2, 4, 8, 12, 16, 32 };
    return beats[std::clamp (d, 0, numDurations - 1)];
}

//==============================================================================
// Padrões rítmicos (grade de 16 semicolcheias por compasso)
enum Pattern { SkankOffbeat, Skank24, DoubleSkank, Bubble, Steppers, NumPatterns };

inline const char* patternName (int p)
{
    static const char* names[NumPatterns] = {
        "Skank contratempo", "Skank 2 e 4 (one drop)", "Double skank", "Bubble (orgao)", "Steppers skank"
    };
    return names[std::clamp (p, 0, NumPatterns - 1)];
}

struct Hit { int step; float vel; double lenSteps; };

inline std::vector<Hit> patternHits (int p)
{
    switch (p)
    {
        case Skank24:     return { { 4, 1.0f, 2 }, { 12, 1.0f, 2 } };
        case DoubleSkank: return { { 2, 1.0f, 1 }, { 3, 0.7f, 1 }, { 6, 1.0f, 1 }, { 7, 0.7f, 1 },
                                   { 10, 1.0f, 1 }, { 11, 0.7f, 1 }, { 14, 1.0f, 1 }, { 15, 0.7f, 1 } };
        case Bubble:
        {
            std::vector<Hit> h;
            for (int s = 0; s < 16; ++s)
                if (s % 4 != 0)
                    h.push_back ({ s, (s % 4 == 2) ? 1.0f : 0.55f, 1 });
            return h;
        }
        case Steppers:    return { { 2, 1.0f, 1 }, { 6, 0.85f, 1 }, { 10, 1.0f, 1 }, { 14, 0.85f, 1 }, { 15, 0.6f, 1 } };
        case SkankOffbeat:
        default:          return { { 2, 1.0f, 1 }, { 6, 1.0f, 1 }, { 10, 1.0f, 1 }, { 14, 1.0f, 1 } };
    }
}

//==============================================================================
struct ChordSlot { int root = 9; int quality = Min; double beats = 8; };

struct NoteEvent
{
    double start;   // em tempos (semínimas) dentro do loop
    double length;  // em tempos
    int note;
    int velocity;   // 1..127
    int channel;    // 1 = acordes, 2 = baixo guia
};

struct Settings
{
    int pattern = SkankOffbeat;
    int centerNote = 62;     // centro do registro dos acordes (D4)
    float velocity = 0.8f;   // 0..1
    float gate = 0.8f;       // 0.1..1 (fração do comprimento da nota)
    float swing = 0.0f;      // 0..1 (atrasa as semicolcheias pares)
    bool bass = true;
    int bassBase = 36;       // C2: a fundamental do baixo fica entre C2 e B2
};

//==============================================================================
// Voicing com condução de vozes: escolhe a inversão mais próxima do acorde anterior
inline std::vector<int> voiceChord (int root, int quality, int center, const std::vector<int>& prev)
{
    const auto& iv = intervals (quality);
    const int n = (int) iv.size();

    std::vector<int> pcs;
    for (int i : iv) pcs.push_back (((root + i) % 12 + 12) % 12);

    double bestScore = 1e9;
    std::vector<int> best;

    for (int inv = 0; inv < n; ++inv)
    {
        for (int bottom = center - 12; bottom <= center + 6; ++bottom)
        {
            if (((bottom % 12) + 12) % 12 != pcs[(size_t) inv]) continue;

            std::vector<int> cand { bottom };
            for (int k = 1; k < n; ++k)
            {
                int pc = pcs[(size_t) ((inv + k) % n)];
                int note = cand.back() + 1;
                while (((note % 12) + 12) % 12 != pc) ++note;
                cand.push_back (note);
            }

            double mean = 0;
            for (int x : cand) mean += x;
            mean /= n;

            double score = 0.35 * std::abs (mean - center);     // não deixa o registro "derivar"
            if (! prev.empty())
            {
                const size_t m = std::min (prev.size(), cand.size());
                for (size_t k = 0; k < m; ++k) score += std::abs (cand[k] - prev[k]);
                double pm = 0;
                for (int x : prev) pm += x;
                pm /= (double) prev.size();
                score += 0.5 * std::abs (mean - pm);
            }
            if (cand.back() - cand.front() > 14) score += 4;    // evita voicings muito abertos

            if (score < bestScore) { bestScore = score; best = cand; }
        }
    }
    return best;
}

//==============================================================================
// Gera um loop completo de eventos a partir dos slots ativos
inline std::vector<NoteEvent> buildLoop (const std::vector<ChordSlot>& slots, const Settings& s, double& loopBeats)
{
    std::vector<NoteEvent> events;
    loopBeats = 0;

    const auto hits = patternHits (s.pattern);
    std::vector<int> prev;
    double pos = 0;   // posição em tempos

    for (const auto& slot : slots)
    {
        if (slot.beats <= 0) continue;

        const auto voicing = voiceChord (slot.root, slot.quality, s.centerNote, prev);
        prev = voicing;

        const int firstStep = (int) std::lround (pos * 4.0);
        const int numSteps  = (int) std::lround (slot.beats * 4.0);

        for (int st = firstStep; st < firstStep + numSteps; ++st)
        {
            const int stepInBar = st % 16;

            for (const auto& h : hits)
            {
                if (h.step != stepInBar) continue;
                double start = st * 0.25;
                const double swingOffset = (stepInBar % 2 == 1) ? s.swing * 0.125 : 0.0;
                start += swingOffset;
                // termina antes do próximo passo da grade, mesmo com swing
                const double maxLen = h.lenSteps * 0.25 - swingOffset - 0.01;
                const double len = std::max (0.03, std::min (maxLen, h.lenSteps * 0.25 * s.gate));
                const int vel = std::clamp ((int) std::lround (h.vel * s.velocity * 127.0f), 1, 127);
                for (int note : voicing)
                    events.push_back ({ start, len, std::clamp (note, 0, 127), vel, 1 });
            }

            // Baixo guia: fundamental no 1 e no "e" do 3 (estilo one drop simples)
            if (s.bass && (stepInBar == 0 || stepInBar == 10))
            {
                const int note = s.bassBase + ((slot.root % 12) + 12) % 12;
                const double len = (stepInBar == 0 ? 1.5 : 1.0) * std::max (0.5f, s.gate);
                const int vel = std::clamp ((int) std::lround (s.velocity * 127.0f * 0.9f), 1, 127);
                events.push_back ({ st * 0.25, len, note, vel, 2 });
            }
        }
        pos += slot.beats;
    }

    loopBeats = pos;
    std::sort (events.begin(), events.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });
    return events;
}

inline std::string chordLabel (const ChordSlot& c)
{
    return std::string (rootName (c.root)) + qualityName (c.quality);
}

} // namespace dub
