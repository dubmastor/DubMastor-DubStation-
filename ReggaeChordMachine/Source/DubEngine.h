// Mastor Reggae Chord Machine (MRCM-1240) — motor de harmonia (C++ puro, sem JUCE, testável isoladamente)
//  - skank de piano "com as duas mãos": a mesma inversão em duas oitavas (canal 1)
//  - bubble de órgão com as duas mãos, uma oitava abaixo do skank (canal 3)
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace dub
{
//==============================================================================
enum Quality { Maj, Min, NumQualities };

inline const char* qualityName (int q) { return q == Min ? "m" : ""; }

inline const std::vector<int>& intervals (int q)
{
    static const std::vector<int> maj { 0, 4, 7 }, min { 0, 3, 7 };
    return q == Min ? min : maj;
}

inline const char* rootName (int r)
{
    static const char* names[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[((r % 12) + 12) % 12];
}

//==============================================================================
// Duração de cada acorde (em tempos de semínima, compasso 4/4)
inline constexpr int numDurations = 6;
inline const char* durationName (int d)
{
    static const char* names[numDurations] = { "1/2 comp.", "1 comp.", "2 comp.", "3 comp.", "4 comp.", "8 comp." };
    return names[std::clamp (d, 0, numDurations - 1)];
}
inline const char* durationShort (int d)
{
    static const char* names[numDurations] = { "1/2", "1", "2", "3", "4", "8" };
    return names[std::clamp (d, 0, numDurations - 1)];
}
inline double durationBeats (int d)
{
    static const double beats[numDurations] = { 2, 4, 8, 12, 16, 32 };
    return beats[std::clamp (d, 0, numDurations - 1)];
}

//==============================================================================
// Tamanho da nota (figura) de cada ataque do skank / bubble
inline constexpr int numNoteSizes = 5;
inline const char* noteSizeName (int n)
{
    static const char* names[numNoteSizes] = { "1/32", "1/16", "1/8", "1/4", "1/2" };
    return names[std::clamp (n, 0, numNoteSizes - 1)];
}
inline double noteSizeBeats (int n)
{
    static const double beats[numNoteSizes] = { 0.125, 0.25, 0.5, 1.0, 2.0 };
    return beats[std::clamp (n, 0, numNoteSizes - 1)];
}

//==============================================================================
// Padrões do skank (grade de 16 semicolcheias por compasso, contagem com a caixa no 2 e no 4)
//   Contratempo: o skank clássico nos "e" de cada tempo
//   Bang-bang (double chop, o mesmo do Mastor Reggae Guitar MRG-68): o acorde acentuado no "e" +
//   só a mão direita (a oitava de cima), mais fraca, na semicolcheia seguinte (1/16 depois)
enum Pattern { SkankOffbeat, SkankBangBang, NumPatterns };

inline const char* patternName (int p) { return p == SkankBangBang ? "Bang-bang" : "Contratempo"; }

struct Hit { int step; float vel; bool upper = false; };   // upper: só a mão direita do skank

inline std::vector<Hit> patternHits (int p)
{
    if (p == SkankBangBang)
        return { { 2, 1.0f }, { 3, 0.62f, true }, { 6, 1.0f }, { 7, 0.62f, true },
                 { 10, 1.0f }, { 11, 0.62f, true }, { 14, 1.0f }, { 15, 0.62f, true } };
    return { { 2, 1.0f }, { 6, 1.0f }, { 10, 1.0f }, { 14, 1.0f } };
}

// Bubble de órgão com as duas mãos (o "bubble" clássico do reggae), em cada tempo:
//   "e"  -> mão esquerda (grave, staccato)
//   "&"  -> mão direita (o acorde, junto com o skank, acentuado)
//   "a"  -> mão esquerda (mais leve)
// A mão esquerda fica uma oitava abaixo da direita; a direita fica uma oitava abaixo
// da mão direita do skank. Todas as notas duram 1/32 (fusa), como as do skank.
struct BubbleHit { int step; float vel; bool rightHand; };

inline std::vector<BubbleHit> bubbleHits()
{
    std::vector<BubbleHit> h;
    for (int beat = 0; beat < 4; ++beat)
    {
        h.push_back ({ beat * 4 + 1, 0.72f, false });
        h.push_back ({ beat * 4 + 2, 0.95f, true  });
        h.push_back ({ beat * 4 + 3, 0.62f, false });
    }
    return h;
}
inline constexpr double kBubbleLeftLen  = 0.125;  // 1/32 (em tempos)
inline constexpr double kBubbleRightLen = 0.125;  // 1/32

//==============================================================================
// inversion: -1 = AUTO (a mais próxima do acorde anterior), 0 = fundamental, 1 = 1ª, 2 = 2ª
struct ChordSlot { int root = 9; int quality = Min; double beats = 8; int inversion = -1; };

inline const char* inversionName (int inv)
{
    static const char* names[3] = { "FUND", "1\xc2\xaa INV", "2\xc2\xaa INV" };   // UTF-8: 1ª / 2ª
    return names[std::clamp (inv, 0, 2)];
}

enum Channel { ChSkank = 1, ChBass = 2, ChBubble = 3 };   // ChBass: baixo guia, removido na 1.3 (o MRM-0420 ainda separa a faixa)

struct NoteEvent
{
    double start;   // em tempos (semínimas) dentro do loop
    double length;  // em tempos
    int note;
    int velocity;   // 1..127
    int channel;    // instrumento: 1 = skank (piano), 3 = bubble (órgão)
    int slot;       // índice do acorde (0..3) — usado para colorir o MIDI exportado
};

struct Settings
{
    int pattern = SkankOffbeat;
    int centerNote = 60;     // centro da mão esquerda do skank (C4)
    float velocity = 0.8f;   // 0..1
    int noteSize = 0;        // índice em noteSizeBeats; o MRCM-1240 usa sempre 0 (1/32)
    float swing = 0.0f;      // 0..1 (atrasa as semicolcheias pares)
    bool bubble = true;      // órgão uma oitava abaixo do skank
};

//==============================================================================
// Voicing com condução de vozes: escolhe a inversão mais próxima do acorde anterior
inline std::vector<int> voiceChord (int root, int quality, int center, const std::vector<int>& prev, int forcedInversion = -1)
{
    const auto& iv = intervals (quality);
    const int n = (int) iv.size();

    std::vector<int> pcs;
    for (int i : iv) pcs.push_back (((root + i) % 12 + 12) % 12);

    double bestScore = 1e9;
    std::vector<int> best;

    for (int inv = 0; inv < n; ++inv)
    {
        if (forcedInversion >= 0 && inv != forcedInversion % n) continue;
        for (int bottom = center - 12; bottom <= center + 6; ++bottom)
        {
            if (((bottom % 12) + 12) % 12 != pcs[(size_t) inv]) continue;

            std::vector<int> cand { bottom };
            for (int k = 1; k < n; ++k)
            {
                const int pc = pcs[(size_t) ((inv + k) % n)];
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

// Qual inversão é um voicing (0 = fundamental no baixo, 1 = terça no baixo, 2 = quinta no baixo)
inline int inversionOf (const std::vector<int>& voicing, int root, int quality)
{
    if (voicing.empty()) return 0;
    const auto& iv = intervals (quality);
    const int bassPc = ((voicing.front() % 12) + 12) % 12;
    for (int k = 0; k < (int) iv.size(); ++k)
        if (((root + iv[(size_t) k]) % 12 + 12) % 12 == bassPc) return k;
    return 0;
}

// Voicings da progressão inteira (mão esquerda do skank).
// suggested[i] = inversão com as notas mais próximas do acorde anterior (o que o AUTO escolheria);
// used[i] = inversão que está tocando (a escolhida pelo usuário ou a sugerida, no AUTO).
// O primeiro acorde é sugerido em relação ao último, para o loop fechar sem salto.
struct Voicings
{
    std::vector<std::vector<int>> notes;   // vazio para acorde desligado
    std::vector<int> suggested, used;
};

inline Voicings voiceProgression (const std::vector<ChordSlot>& slots, int center)
{
    Voicings v;
    v.notes.assign (slots.size(), {});
    v.suggested.assign (slots.size(), 0);
    v.used.assign (slots.size(), 0);

    auto pass = [&] (std::vector<int> prev)
    {
        for (size_t i = 0; i < slots.size(); ++i)
        {
            const auto& s = slots[i];
            if (s.beats <= 0) continue;
            const auto best = voiceChord (s.root, s.quality, center, prev);
            v.suggested[i] = inversionOf (best, s.root, s.quality);
            v.notes[i] = s.inversion < 0 ? best : voiceChord (s.root, s.quality, center, prev, s.inversion);
            v.used[i] = inversionOf (v.notes[i], s.root, s.quality);
            prev = v.notes[i];
        }
        return prev;
    };

    // 1ª passada a partir do centro; depois recomeça pelo último acorde (o loop volta ao início)
    // até a progressão parar de mudar
    auto last = pass ({});
    for (int k = 0; k < 6; ++k)
    {
        const auto before = v.notes;
        last = pass (last);
        if (v.notes == before) break;
    }
    return v;
}

//==============================================================================
// Comprimento de um ataque: o tamanho de nota escolhido, mas nunca encostando no próximo ataque
template <typename HitT>
inline double hitLength (const std::vector<HitT>& hits, size_t index, double noteBeats, double swingOffset)
{
    const int step = hits[index].step;
    int gapSteps = 16;
    for (const auto& h : hits)
    {
        int d = h.step - step;
        if (d <= 0) d += 16;
        gapSteps = std::min (gapSteps, d);
    }
    const double maxLen = gapSteps * 0.25 - swingOffset - 0.02;
    return std::max (0.03, std::min (noteBeats, maxLen));
}

// Gera um loop completo de eventos a partir dos slots (beats == 0 → acorde desligado)
inline std::vector<NoteEvent> buildLoop (const std::vector<ChordSlot>& slots, const Settings& s, double& loopBeats)
{
    std::vector<NoteEvent> events;
    loopBeats = 0;

    const auto hits = patternHits (s.pattern);
    const auto bub  = bubbleHits();
    const double noteBeats = noteSizeBeats (s.noteSize);
    const auto voicings = voiceProgression (slots, s.centerNote);
    double pos = 0;   // posição em tempos

    for (int si = 0; si < (int) slots.size(); ++si)
    {
        const auto& slot = slots[(size_t) si];
        if (slot.beats <= 0) continue;

        // mão esquerda (voicing) + mão direita uma oitava acima
        const auto& left = voicings.notes[(size_t) si];
        std::vector<int> skank = left, skankUpper;
        for (int n : left) { skank.push_back (n + 12); skankUpper.push_back (n + 12); }
        const std::vector<int>& organRight = left;              // bubble: mão direita = mão esquerda do skank
        std::vector<int> organLeft;                              //         mão esquerda uma oitava abaixo
        for (int n : left) organLeft.push_back (n - 12);

        const int firstStep = (int) std::lround (pos * 4.0);
        const int numSteps  = (int) std::lround (slot.beats * 4.0);

        auto emit = [&] (const std::vector<Hit>& hs, const std::vector<int>& notes, int channel, float velScale, int st)
        {
            const int stepInBar = st % 16;
            for (size_t h = 0; h < hs.size(); ++h)
            {
                if (hs[h].step != stepInBar) continue;
                const double swingOffset = (stepInBar % 2 == 1) ? s.swing * 0.125 : 0.0;
                const double start = st * 0.25 + swingOffset;
                const double len = hitLength (hs, h, noteBeats, swingOffset);
                const int vel = std::clamp ((int) std::lround (hs[h].vel * s.velocity * velScale * 127.0f), 1, 127);
                for (int note : (hs[h].upper && channel == ChSkank ? skankUpper : notes))
                    events.push_back ({ start, len, std::clamp (note, 0, 127), vel, channel, si });
            }
        };

        auto emitBubble = [&] (int st)
        {
            const int stepInBar = st % 16;
            for (size_t h = 0; h < bub.size(); ++h)
            {
                if (bub[h].step != stepInBar) continue;
                const double swingOffset = (stepInBar % 2 == 1) ? s.swing * 0.125 : 0.0;
                const double start = st * 0.25 + swingOffset;
                const double len = hitLength (bub, h, bub[h].rightHand ? kBubbleRightLen : kBubbleLeftLen, swingOffset);
                const int vel = std::clamp ((int) std::lround (bub[h].vel * s.velocity * 0.9f * 127.0f), 1, 127);
                for (int note : (bub[h].rightHand ? organRight : organLeft))
                    events.push_back ({ start, len, std::clamp (note, 0, 127), vel, ChBubble, si });
            }
        };

        for (int st = firstStep; st < firstStep + numSteps; ++st)
        {
            emit (hits, skank, ChSkank, 1.0f, st);
            if (s.bubble) emitBubble (st);
        }
        pos += slot.beats;
    }

    loopBeats = pos;
    std::stable_sort (events.begin(), events.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.start < b.start; });
    return events;
}

inline std::string chordLabel (const ChordSlot& c)
{
    return std::string (rootName (c.root)) + qualityName (c.quality);
}

} // namespace dub
