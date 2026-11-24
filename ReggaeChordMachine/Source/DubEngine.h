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
// O 1/64 entrou na 2.5 no FIM da lista (os projetos guardam o índice: 0 continua sendo 1/32)
inline constexpr int numNoteSizes = 6;
inline constexpr int NoteSize64 = 5;
inline const char* noteSizeName (int n)
{
    static const char* names[numNoteSizes] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1/64" };
    return names[std::clamp (n, 0, numNoteSizes - 1)];
}
inline double noteSizeBeats (int n)
{
    static const double beats[numNoteSizes] = { 0.125, 0.25, 0.5, 1.0, 2.0, 0.0625 };
    return beats[std::clamp (n, 0, numNoteSizes - 1)];
}

//==============================================================================
// PIANISTAS (chops de piano inspirados nos tecladistas jamaicanos; não são transcrições).
// Contagem da Dub Station: caixa no 2 e no 4 (passos 4 e 12 da grade de 16), skank no "&" (passos 2, 6, 10, 14).
// part: 0 = acorde inteiro (as duas mãos), 1 = só a mão direita (oitava de cima), 2 = só a mão esquerda.
// lenMul multiplica o tamanho de nota escolhido. Os fills tomam o lugar dos chops nos tempos 3 e 4 do último
// compasso de cada frase de 4 compassos (ou do loop, se ele for mais curto).
enum Pianist { PnClassico, PnBeckford, PnGladdy, PnSterling, PnDownie, PnLyn, PnSteely, PnLenky, NumPianists };
// FILLS: seletor separado do pianista. O fill toma o lugar do skank nos tempos 3 e 4 do último compasso da frase.
enum PianoFill { FillNone, FillBeckford, FillGladdy, FillSterling, FillDownie, FillLyn, FillSteely, FillLenky, NumFills };

inline const char* fillName (int f)
{
    static const char* n[NumFills] = { "SEM FILL", "BECKFORD", "GLADDY", "STERLING", "DOWNIE", "LYN", "STEELY", "LENKY" };
    return n[std::clamp (f, 0, NumFills - 1)];
}
inline const char* fillWho (int f)
{
    static const char* n[NumFills] = {
        "S\xc3\x93 O SKANK",
        "CORRIDA DO SKA \xc2\xb7 COLCHEIAS CRESCENDO",
        "FRAGMENTOS GRAVES DESCENDO",
        "UM ACORDE SEGURADO E UMA NOTA NO ALTO",
        "RESPOSTAS SINCOPADAS",
        "ARPEJO SUBINDO (LOVERS ROCK)",
        "SEMICOLCHEIAS DIGITAIS NO 4",
        "STABS NOS \"E\" E NOS \"A\"" };
    return n[std::clamp (f, 0, NumFills - 1)];
}
inline bool fillNoSwing (int f) { return f == FillSteely || f == FillLenky; }

inline const char* pianistName (int p)
{
    static const char* n[NumPianists] = { "CLASSICO", "BECKFORD", "GLADDY", "STERLING", "DOWNIE", "LYN", "STEELY", "LENKY" };
    return n[std::clamp (p, 0, NumPianists - 1)];
}
inline const char* pianistWho (int p)
{
    static const char* n[NumPianists] = {
        "SKANK NO CONTRATEMPO",
        "THEOPHILUS BECKFORD \xc2\xb7 SHUFFLE DO SKA",
        "GLADSTONE ANDERSON \xc2\xb7 FRAGMENTOS GRAVES",
        "KEITH STERLING \xc2\xb7 SECO, COM ESPA\xc3\x87O",
        "TYRONE DOWNIE \xc2\xb7 RESPOSTAS SINCOPADAS",
        "ROBBIE LYN \xc2\xb7 LOVERS ROCK",
        "STEELY \xc2\xb7 DIGITAL, GRADE PRESA",
        "LENKY \xc2\xb7 STABS DOS ANOS 2000" };
    return n[std::clamp (p, 0, NumPianists - 1)];
}

struct PianoHit { int step; float vel; int part; float lenMul; };
struct PianistRecipe
{
    std::vector<PianoHit> barA, barB;   // compassos ímpares e pares da frase
    bool noSwing = false;               // grade presa (digital)
    double layBack = 0.0;               // atraso fixo dos chops, em tempos
};

inline PianistRecipe pianistRecipe (int p)
{
    PianistRecipe r;
    auto skank = [] (float v, float len) { return std::vector<PianoHit> { { 2, v, 0, len }, { 6, v, 0, len }, { 10, v, 0, len }, { 14, v, 0, len } }; };
    switch (std::clamp (p, 0, NumPianists - 1))
    {
        case PnBeckford:   // shuffle: o "&" de todo tempo forte e a mão esquerda leve na cabeça de cada tempo
            r.barA = { { 0, 0.45f, 2, 1.0f }, { 2, 1.0f, 1, 1.0f }, { 4, 0.5f, 2, 1.0f }, { 6, 1.0f, 1, 1.0f },
                       { 8, 0.45f, 2, 1.0f }, { 10, 1.0f, 1, 1.0f }, { 12, 0.5f, 2, 1.0f }, { 14, 1.0f, 1, 1.0f } };
            r.barB = r.barA;
            break;
        case PnGladdy:     // skank curto e, no fim da frase, fragmentos graves e lentos
            r.barA = skank (0.9f, 0.8f); r.barB = r.barA; break;
        case PnSterling:   // seco e com espaço: o acorde cheio no 2 e no 4, só a mão de cima (leve) no 1 e no 3
            r.barA = { { 2, 0.55f, 1, 0.8f }, { 6, 1.0f, 0, 0.8f }, { 10, 0.55f, 1, 0.8f }, { 14, 1.0f, 0, 0.8f } };
            r.barB = { { 6, 1.0f, 0, 0.8f }, { 10, 0.5f, 1, 0.8f }, { 14, 1.0f, 0, 0.8f } };
            break;
        case PnDownie:     // skank e respostas sincopadas no "a" do 3 e do 4 no segundo compasso
            r.barA = skank (0.95f, 1.0f);
            r.barB = { { 2, 0.95f, 0, 1.0f }, { 6, 0.95f, 0, 1.0f }, { 10, 0.95f, 0, 1.0f }, { 11, 0.7f, 1, 1.0f },
                       { 14, 0.95f, 0, 1.0f }, { 15, 0.65f, 1, 1.0f } };
            r.layBack = 0.015; break;
        case PnLyn:        // lovers rock: skank mais longo e mais suave, arpejo no fim da frase
            r.barA = skank (0.8f, 2.0f); r.barB = r.barA; break;
        case PnSteely:     // digital: grade presa, staccato, e o "a" do 4 na mão de cima
            r.barA = { { 2, 1.0f, 0, 0.75f }, { 6, 1.0f, 0, 0.75f }, { 10, 1.0f, 0, 0.75f }, { 14, 1.0f, 0, 0.75f }, { 15, 0.8f, 1, 0.75f } };
            r.barB = r.barA; r.noSwing = true; break;
        case PnLenky:      // stabs curtos fora do lugar esperado (o "e" do 3), caixa sempre no 2 e no 4
            r.barA = { { 2, 0.9f, 0, 0.75f }, { 6, 1.0f, 0, 0.75f }, { 9, 0.85f, 1, 0.75f }, { 14, 1.0f, 0, 0.75f } };
            r.barB = { { 2, 0.9f, 0, 0.75f }, { 6, 1.0f, 0, 0.75f }, { 11, 0.85f, 1, 0.75f }, { 14, 1.0f, 0, 0.75f } };
            r.noSwing = true; break;
        default:           // CLÁSSICO: o skank de sempre
            r.barA = skank (1.0f, 1.0f); r.barB = r.barA; break;
    }
    return r;
}

//==============================================================================
// Padrões do skank (grade de 16 semicolcheias por compasso)
// Só o contratempo (o skank clássico nos "e" de cada tempo); os outros padrões saíram na 1.7.
enum Pattern { SkankOffbeat, NumPatterns };

inline const char* patternName (int) { return "Contratempo"; }

struct Hit { int step; float vel; };

inline std::vector<Hit> patternHits (int)
{
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
    int pianist = PnClassico;   // PIANISTAS: 0 = o skank clássico (igual às versões antigas)
    int fill = FillNone;        // FILLS (seletor separado): no fim de cada frase de 4 compassos
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

// Fill do pianista num passo dos tempos 3 e 4 (passos 8..15 do compasso)
inline void pianoFill (int fill, const std::vector<int>& left, int stepInBar, double start, int slot, const Settings& s,
                       std::vector<NoteEvent>& events)
{
    if (left.empty()) return;
    const double barEnd = start - (start - std::floor (start / 4.0) * 4.0) + 4.0;   // nenhuma nota passa do compasso
    auto add = [&] (int note, double len, float v)
    {
        const int vel = std::clamp ((int) std::lround (v * s.velocity * 127.0f), 1, 127);
        len = std::max (0.03, std::min (len, barEnd - start - 0.02));
        events.push_back ({ start, len, std::clamp (note, 0, 127), vel, ChSkank, slot });
    };
    const int n = (int) left.size();
    switch (fill)
    {
        case FillBeckford: // corrida do ska: acorde da mão direita em todas as colcheias do 3 e do 4, crescendo
            if (stepInBar % 2 == 0)
                for (int x : left) add (x + 12, 0.2, 0.7f + 0.1f * (float) ((stepInBar - 8) / 2));
            break;
        case FillSterling: // espaço: um acorde segurado no "&" do 3 e uma nota só, no alto, no "&" do 4
            if (stepInBar == 10) for (int x : left) add (x + 12, 1.4, 0.85f);
            if (stepInBar == 14) add (left[(size_t) (n - 1)] + 24, 0.4, 0.6f);
            break;
        case FillSteely:   // digital: semicolcheias secas no tempo 4, grade presa
            if (stepInBar >= 12)
                for (int x : left) add (x + 12, 0.1, stepInBar % 2 == 0 ? 1.0f : 0.75f);
            break;
        case FillLenky:    // stabs no "e" e no "a" (9, 11, 13) e uma nota no alto no "a" do 4
            if (stepInBar == 9 || stepInBar == 11 || stepInBar == 13) for (int x : left) add (x + 12, 0.15, 0.85f);
            if (stepInBar == 15) add (left[(size_t) (n - 1)] + 24, 0.2, 0.8f);
            break;
        case FillGladdy:   // fragmentos graves e lentos: notas do acorde descendo, uma por colcheia, duas oitavas abaixo
        {
            static const int steps[4] = { 8, 10, 12, 14 };
            for (int k = 0; k < 4; ++k)
                if (stepInBar == steps[k])
                    add (left[(size_t) ((n - 1 - k % n + n) % n)] - 12 - (k >= n ? 12 : 0), 0.45, 0.78f - 0.04f * (float) k);
            break;
        }
        case FillDownie:   // acordes sincopados respondendo ao canto: "&" do 3, "a" do 3 e "&" do 4
            if (stepInBar == 10 || stepInBar == 11 || stepInBar == 14)
                for (int x : left) add (x + 12, 0.2, stepInBar == 11 ? 0.7f : 0.9f);
            break;
        case FillLyn:      // arpejo subindo nas semicolcheias do 4
            if (stepInBar >= 12)
            {
                const int k = stepInBar - 12;
                add (k < n ? left[(size_t) k] + 12 : left[0] + 24, 0.22, 0.68f);
            }
            break;
        default: break;
    }
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

    // fills dos pianistas: no fim de cada frase de 4 compassos (ou no último compasso do loop, se ele for mais curto)
    double totalBeats = 0;
    for (const auto& sl : slots) if (sl.beats > 0) totalBeats += sl.beats;
    const int loopBars = std::max (1, (int) std::lround (std::ceil (totalBeats / 4.0)));
    const int fillEvery = std::min (4, loopBars);

    for (int si = 0; si < (int) slots.size(); ++si)
    {
        const auto& slot = slots[(size_t) si];
        if (slot.beats <= 0) continue;

        // mão esquerda (voicing) + mão direita uma oitava acima
        const auto& left = voicings.notes[(size_t) si];
        std::vector<int> skank = left;
        for (int n : left) skank.push_back (n + 12);
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
                for (int note : notes)
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

        if (s.pianist == PnClassico)
        {
            for (int st = firstStep; st < firstStep + numSteps; ++st)
            {
                const int stepInBar = st % 16;
                if (s.fill != FillNone && stepInBar >= 8 && (st / 16) % fillEvery == fillEvery - 1)
                {
                    const double so = (! fillNoSwing (s.fill) && stepInBar % 2 == 1) ? s.swing * 0.125 : 0.0;
                    pianoFill (s.fill, left, stepInBar, st * 0.25 + so, si, s, events);
                }
                else
                    emit (hits, skank, ChSkank, 1.0f, st);
                if (s.bubble) emitBubble (st);
            }
        }
        else
        {
            const auto rec = pianistRecipe (s.pianist);
            std::vector<int> rightHand, leftHand;
            for (int n : left) { leftHand.push_back (n); rightHand.push_back (n + 12); }
            for (int st = firstStep; st < firstStep + numSteps; ++st)
            {
                const int bar = st / 16, stepInBar = st % 16;
                const bool fillBar = s.fill != FillNone && stepInBar >= 8 && bar % fillEvery == fillEvery - 1;
                const double swingOffset = (! rec.noSwing && stepInBar % 2 == 1) ? s.swing * 0.125 : 0.0;
                if (fillBar)
                {
                    const double so = (! fillNoSwing (s.fill) && stepInBar % 2 == 1) ? s.swing * 0.125 : 0.0;
                    pianoFill (s.fill, left, stepInBar, st * 0.25 + so, si, s, events);
                }
                else
                {
                    const auto& bh = (bar % 2 == 0) ? rec.barA : rec.barB;
                    for (const auto& h : bh)
                    {
                        if (h.step != stepInBar) continue;
                        int gap = 16;
                        for (const auto& o : bh) { int d = o.step - h.step; if (d <= 0) d += 16; gap = std::min (gap, d); }
                        const double maxLen = gap * 0.25 - swingOffset - 0.02;
                        const double len = std::max (0.03, std::min (noteBeats * h.lenMul, maxLen));
                        const int vel = std::clamp ((int) std::lround (h.vel * s.velocity * 127.0f), 1, 127);
                        const auto& notes = h.part == 1 ? rightHand : (h.part == 2 ? leftHand : skank);
                        for (int note : notes)
                            events.push_back ({ st * 0.25 + swingOffset + rec.layBack, len, std::clamp (note, 0, 127), vel, ChSkank, si });
                    }
                }
                if (s.bubble) emitBubble (st);
            }
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
