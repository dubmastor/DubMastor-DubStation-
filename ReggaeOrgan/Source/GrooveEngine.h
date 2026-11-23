// GrooveEngine.h — motor de grooves do Mastor Reggae Organ MRO-68 (header-only, sem JUCE)
//
// CONTAGEM DA DUB STATION: compasso 4/4 com a CAIXA NO 2 E NO 4 (a mesma do MRDM-12, do MRCM-1240 e do
// banco de riddims; BPM até 100). Tudo aqui — posições, durações das notas e dos acordes — é nessa contagem.
//
// As três partes do órgão de reggae, estudadas nos organistas jamaicanos:
//   CHOP    mão direita: o acorde no contratempo "&" de todo tempo, junto com o skank (Theophilus Beckford, ska)
//   BUBBLE  mão esquerda, grave: no "e" e no "a", em volta do chop (Jackie Mittoo, Studio One, 1968)
//   FILLS   no fim das frases (tempos 3 e 4 do compasso de fill): licks curtos (Mittoo), melodia em oitavas
//           (Winston Wright), acordes sincopados (Tyrone Downie) ou fragmentos graves e lentos (Gladstone Anderson)
// Bubble e chop saem de uma GRADE de 16 semicolcheias (Grid), que cada MÚSICO traz pronta e que pode ser editada.
// Notas de 1/32 por padrão, como o skank e o bubble do MRCM-1240 (NOTA: 1/32, 1/16 ou 1/8).
//
// Ajustes:
//   grade     vazio / fantasma / acento em cada semicolcheia, para a mão direita e a esquerda
//             atalhos de FEEL: SKA "ki-koo-ki" · ROCKSTEADY "koo-ki-koo" · REGGAE (bubble clássico)
//   beat      ONE DROP / ROCKERS / STEPPERS: contraste entre o acento e a nota fantasma
//   swing     atraso do "e" e do "a" até a quiáltera do shuffle
//   human     variação de tempo e de força, determinística (o mesmo loop sempre igual)
// Unidade de tempo: tempos (semínimas). Compasso = 4 tempos.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mro
{
enum Layer   { LayerBubble = 0, LayerChop = 1, LayerFill = 2, NumLayers };
enum Feel    { FeelSka = 0, FeelRocksteady, FeelReggae, NumFeels };
enum Beat    { BeatOneDrop = 0, BeatRockers, BeatSteppers, NumBeats };
enum Density { DenE = 0, DenA, DenEA, NumDensities };
enum Fill    { FillMittoo = 0, FillWright, FillDownie, FillGladdy, NumFillStyles };
enum Level   { Off = 0, Ghost = 1, Accent = 2 };   // célula da grade

inline const char* feelName (int f)    { static const char* n[] = { "SKA", "ROCKSTEADY", "REGGAE" }; return n[std::clamp (f, 0, 2)]; }
inline const char* beatName (int b)    { static const char* n[] = { "ONE DROP", "ROCKERS", "STEPPERS" }; return n[std::clamp (b, 0, 2)]; }
inline const char* densityName (int d) { static const char* n[] = { "e", "a", "e + a" }; return n[std::clamp (d, 0, 2)]; }
inline const char* fillName (int f)    { static const char* n[] = { "LICKS MITTOO", "MELODIA WRIGHT", "ACORDES DOWNIE", "FRAGMENTOS GLADDY" }; return n[std::clamp (f, 0, 3)]; }
inline const char* rootName (int r)    { static const char* n[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }; return n[((r % 12) + 12) % 12]; }

// duração das notas (figura), em tempos
constexpr int numNoteSizes = 3;
inline const char* noteSizeName (int n) { static const char* s[] = { "1/32", "1/16", "1/8" }; return s[std::clamp (n, 0, 2)]; }
inline double noteSizeBeats (int n)     { static const double b[] = { 0.125, 0.25, 0.5 }; return b[std::clamp (n, 0, 2)]; }

// duração dos acordes: 1/2, 1, 2, 3, 4 e 8 compassos (os mesmos índices do MRCM-1240 e do banco de riddims)
constexpr int numChordDurations = 6;
inline const char* chordDurationName (int d) { static const char* s[] = { "1/2 COMP.", "1 COMP.", "2 COMP.", "3 COMP.", "4 COMP.", "8 COMP." }; return s[std::clamp (d, 0, 5)]; }
inline int chordDurationBeats (int d)        { static const int b[] = { 2, 4, 8, 12, 16, 32 }; return b[std::clamp (d, 0, 5)]; }

// ------------------------------------------------------------------ a GRADE do groove (1 compasso, 16 semicolcheias)
// passo = tempo * 4 + {0 = cabeça, 1 = "e", 2 = "&", 3 = "a"}. Caixa da bateria no 2 e no 4 (passos 4 e 12).
//   rh = MÃO DIREITA (o acorde)      Accent = forte, Ghost = leve (só muda a velocity do MIDI: o Hammond não tem velocity)
//   lh = MÃO ESQUERDA (o bubble)     Accent = tônica + oitava (órgão do acento), Ghost = só a oitava (órgão da nota fantasma)
struct Grid
{
    uint8_t rh[16] {}, lh[16] {};
    bool operator== (const Grid& o) const
    {
        for (int i = 0; i < 16; ++i) if (rh[i] != o.rh[i] || lh[i] != o.lh[i]) return false;
        return true;
    }
};

// grade a partir de texto: '.' vazio, 'x' fantasma, 'X' acento (16 caracteres, 4 por tempo)
inline Grid gridFrom (const char* rh, const char* lh)
{
    Grid g;
    for (int i = 0; i < 16; ++i)
    {
        g.rh[i] = rh[i] == 'X' ? Accent : (rh[i] == 'x' ? Ghost : Off);
        g.lh[i] = lh[i] == 'X' ? Accent : (lh[i] == 'x' ? Ghost : Off);
    }
    return g;
}

struct Chord
{
    int  root  = 9;      // 0 = C ... 11 = B
    bool minor = true;   // o MRO-68 usa só acordes maiores e menores, como o MRK-80
    int  beats = 4;      // duração em tempos (2 = meio compasso)
};

// atalhos da grade: FEEL (ordem do afterbeat) e onde a mão esquerda toca
//   SKA        ki-koo-ki: fraco-FORTE-fraco (o & manda, a esquerda é toda fantasma)
//   ROCKSTEADY koo-ki-koo: FORTE-fraco-FORTE (o e e o a mandam, a direita fica leve)
//   REGGAE     o bubble clássico do Studio One: "e" acentuado, "a" fantasma, & forte
inline Grid makeGrid (int feel, int density)
{
    Grid g;
    for (int b = 0; b < 4; ++b)
    {
        g.rh[b * 4 + 2] = feel == FeelRocksteady ? Ghost : Accent;
        const uint8_t e = feel == FeelSka ? Ghost : Accent;
        const uint8_t a = feel == FeelReggae ? Ghost : (feel == FeelSka ? Ghost : Accent);
        if (density != DenA) g.lh[b * 4 + 1] = e;
        if (density != DenE) g.lh[b * 4 + 3] = a;
    }
    return g;
}

struct Settings
{
    Grid  grid = makeGrid (FeelReggae, DenEA);
    int   beat = BeatOneDrop, noteSize = 0;
    bool  bubbleOn = true, chopOn = true, fillsOn = true;
    float bubbleVel = 0.85f, chopVel = 0.9f, fillVel = 0.8f;   // 0..1 (vai para a velocity do MIDI)
    float ghost = 0.5f;          // força da nota fantasma da mão esquerda em relação ao acento (0..1)
    float swing = 0.15f;         // 0 = reto, 1 = shuffle em quiáltera
    float human = 0.3f;          // 0 = grade perfeita
    int   fillStyle = FillMittoo;
    int   fillEvery = 4;         // compassos (2, 4 ou 8)
    int   seed = 1;              // variação
    int   bubbleLow = 43;        // nota mais grave da tônica da mão esquerda (G2)
    int   chopLow   = 55;        // início da oitava do acorde da mão direita (G3)
    int   fillLow   = 67;        // início da região dos fills (G4)
};

struct Event
{
    double start = 0, length = 0;   // em tempos
    int note = 60, velocity = 100, layer = LayerBubble;
    bool accent = true;             // só no bubble: acento (true) ou nota fantasma (false)
};

// ------------------------------------------------------------------ utilidades
struct Rand
{
    uint32_t s;
    explicit Rand (uint32_t seed) : s (seed ? seed : 0x9e3779b9u) { for (int i = 0; i < 3; ++i) next(); }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() & 0xffffff) / 16777216.0f; }          // 0..1
    float bi()  { return uni() * 2.0f - 1.0f; }                                 // -1..1
    float tri() { return 0.5f * (bi() + bi()); }                                // -1..1, concentrado no centro
    int   below (int n) { return n <= 1 ? 0 : (int) (next() % (uint32_t) n); }
};

inline uint32_t hash (int a, int b, int c, int d)
{
    uint32_t h = 2166136261u;
    for (int v : { a, b, c, d }) { h ^= (uint32_t) v + 0x9e3779b9u + (h << 6) + (h >> 2); h *= 16777619u; }
    return h;
}

inline int noteIn (int pc, int low) { return low + ((pc - low) % 12 + 12) % 12; }

// tríade fechada dentro de uma oitava a partir de "low"
inline std::vector<int> triad (const Chord& c, int low)
{
    std::vector<int> v { noteIn (c.root, low), noteIn (c.root + (c.minor ? 3 : 4), low), noteIn (c.root + 7, low) };
    std::sort (v.begin(), v.end());
    return v;
}

// escala pentatônica do acorde (menor sobre acorde menor, maior sobre maior), em notas MIDI a partir de "low"
inline std::vector<int> pentatonic (const Chord& c, int low, int octaves = 2)
{
    static const int minorP[5] = { 0, 3, 5, 7, 10 }, majorP[5] = { 0, 2, 4, 7, 9 };
    const int* sc = c.minor ? minorP : majorP;
    std::vector<int> v;
    for (int o = -1; o <= octaves; ++o)
        for (int i = 0; i < 5; ++i)
        {
            const int n = c.root + sc[i] + 12 * o + 12 * (low / 12);
            if (n >= low && n < low + 12 * octaves) v.push_back (n);
        }
    std::sort (v.begin(), v.end());
    v.erase (std::unique (v.begin(), v.end()), v.end());
    return v;
}

inline bool isChordTone (const Chord& c, int n)
{
    const int i = ((n - c.root) % 12 + 12) % 12;
    return i == 0 || i == 7 || i == (c.minor ? 3 : 4);
}

// atraso de swing para a semicolcheia "pos16" (0..15 no compasso): só o "e" e o "a" andam
inline double swingOffset (int pos16, float swing)
{
    return (pos16 % 2 == 1) ? (double) std::clamp (swing, 0.0f, 1.0f) * (1.0 / 12.0) : 0.0;
}

// contraste do acento conforme a batida: quanto mais pesada a bateria, mais fraca a nota fantasma
inline float ghostScale (int beat) { return beat == BeatSteppers ? 0.75f : (beat == BeatRockers ? 0.88f : 1.0f); }

// ------------------------------------------------------------------ um compasso
// chords[b] = acorde do tempo b (0..3) do compasso; barIndex = posição absoluta (variação e fills)
inline std::vector<Event> barEvents (int barIndex, const Chord chords[4], const Settings& s)
{
    std::vector<Event> out;
    out.reserve (48);
    const double s16 = 0.25;
    const double L = noteSizeBeats (s.noteSize);
    const float  hum = std::clamp (s.human, 0.0f, 1.0f);

    auto push = [&] (double start, double len, int note, float vel01, int layer, bool accent, Rand& r)
    {
        Event e;
        e.start  = std::clamp (start + (double) (r.tri() * hum * 0.011f), 0.0, 3.999);
        e.length = std::max (0.03, len * (1.0 + (double) (r.bi() * hum * 0.08f)));
        e.note   = std::clamp (note, 0, 127);
        e.velocity = std::clamp ((int) std::lround (127.0f * std::clamp (vel01, 0.0f, 1.0f) * (1.0f + r.bi() * hum * 0.10f)), 1, 127);
        e.layer  = layer;
        e.accent = accent;
        out.push_back (e);
    };

    // ---------------- BUBBLE (mão esquerda) e CHOP (mão direita), lidos da GRADE
    if (s.bubbleOn)
    {
        Rand r (hash (s.seed, barIndex, LayerBubble, 11));
        const float accV = s.bubbleVel * 0.85f;
        const float ghV  = s.bubbleVel * (0.25f + 0.75f * s.ghost) * 0.7f * ghostScale (s.beat);
        for (int p16 = 0; p16 < 16; ++p16)
        {
            const int lv = s.grid.lh[p16];
            if (lv == Off) continue;
            const int root = noteIn (chords[p16 / 4].root, s.bubbleLow);
            const double t = p16 * s16 + swingOffset (p16, s.swing);
            // acento: tônica + oitava (o peso grave que "se sente mais do que se ouve"); fantasma: só a oitava
            if (lv == Accent) push (t, L, root, accV, LayerBubble, true, r);
            push (t, L, root + 12, lv == Accent ? accV : ghV, LayerBubble, lv == Accent, r);
        }
    }
    if (s.chopOn)
    {
        Rand r (hash (s.seed, barIndex, LayerChop, 23));
        for (int p16 = 0; p16 < 16; ++p16)
        {
            const int lv = s.grid.rh[p16];
            if (lv == Off) continue;
            Rand rr (r.next());
            const float v = s.chopVel * (lv == Accent ? 1.0f : 0.65f);
            for (int n : triad (chords[p16 / 4], s.chopLow))
                push (p16 * s16 + swingOffset (p16, s.swing), L, n, v, LayerChop, true, rr);
        }
    }

    // ---------------- FILLS (tempos 3 e 4 do compasso de fill)
    const int every = std::max (1, s.fillEvery);
    const bool fillBar = s.fillsOn && ((barIndex + 1) % every == 0);
    if (fillBar)
    {
        Rand r (hash (s.seed, barIndex, LayerFill, 37));
        const Chord& c3 = chords[2];
        const auto sc = pentatonic (c3, s.fillLow, 2);
        const int nsc = (int) sc.size();
        int k = std::clamp (nsc / 2 + r.below (3) - 1, 0, nsc - 1);
        auto step = [&] (int maxStep)
        {
            const int d = r.below (2 * maxStep + 1) - maxStep;
            k = std::clamp (k + (d == 0 ? 1 : d), 0, nsc - 1);
            return sc[(size_t) k];
        };
        auto landOnChordTone = [&] (const Chord& c, int n)
        {
            for (int d = 0; d < 6; ++d)
            {
                if (isChordTone (c, n - d)) return n - d;
                if (isChordTone (c, n + d)) return n + d;
            }
            return n;
        };
        auto chordAtT = [&] (double t) -> const Chord& { return chords[std::clamp ((int) t, 0, 3)]; };

        if (s.fillStyle == FillMittoo)
        {
            // licks curtos em semicolcheias nos tempos 3 e 4, staccato, terminando em nota do acorde
            static const int rhythms[3][6] = { { 9, 10, 11, 13, 14, -1 }, { 8, 10, 11, 12, 14, 15 }, { 10, 11, 12, 13, 14, -1 } };
            const int* rh = rhythms[r.below (3)];
            std::vector<int> pos;
            for (int i = 0; i < 6; ++i) if (rh[i] >= 0) pos.push_back (rh[i]);
            for (size_t i = 0; i < pos.size(); ++i)
            {
                const double t = pos[i] * s16;
                int n = step (1);
                if (i + 1 == pos.size()) n = landOnChordTone (chordAtT (t), n);
                push (t + swingOffset (pos[i], s.swing), L, n, s.fillVel * (i + 1 == pos.size() ? 1.0f : 0.88f), LayerFill, true, r);
            }
        }
        else if (s.fillStyle == FillWright)
        {
            // melodia "grande e gorda" em oitavas: colcheias e notas longas, do tempo 3 ao fim
            static const double starts[4][5] = { { 2.0, 2.5, 3.0, 3.5, -1 }, { 2.0, 2.75, 3.0, 3.5, -1 }, { 2.5, 3.0, 3.25, 3.5, -1 }, { 2.0, 2.5, 3.0, -1, -1 } };
            const double* st = starts[r.below (4)];
            std::vector<double> t;
            for (int i = 0; i < 5; ++i) if (st[i] >= 0) t.push_back (st[i]);
            for (size_t i = 0; i < t.size(); ++i)
            {
                const bool last = (i + 1 == t.size());
                int n = step (2);
                if (last) n = landOnChordTone (chordAtT (t[i]), n);
                const double len = last ? std::min (0.9, 3.98 - t[i]) : std::max (0.2, (t[i + 1] - t[i]) * 0.85);
                const int p16 = (int) std::lround (t[i] * 4.0);
                const double tt = t[i] + swingOffset (p16, s.swing);
                Rand rr (r.next());
                push (tt, len, n, s.fillVel, LayerFill, true, rr);
                Rand rr2 = rr;
                push (tt, len, n - 12, s.fillVel * 0.9f, LayerFill, true, rr2);
            }
        }
        else if (s.fillStyle == FillGladdy)
        {
            // Gladstone Anderson: fragmentos curtos de melodia na região GRAVE, devagar (semínimas e colcheias)
            static const double starts[3][3] = { { 2.0, 3.0, -1 }, { 2.0, 2.5, 3.0 }, { 2.5, 3.0, 3.5 } };
            const double* st = starts[r.below (3)];
            const auto low = pentatonic (c3, s.chopLow - 12, 1);
            int kk = std::clamp ((int) low.size() / 2 + r.below (3) - 1, 0, (int) low.size() - 1);
            for (int i = 0; i < 3 && st[i] >= 0; ++i)
            {
                const bool last = (i == 2 || st[i + 1] < 0);
                const int d = r.below (3) - 1;
                kk = std::clamp (kk + (d == 0 ? -1 : d), 0, (int) low.size() - 1);
                int n = low[(size_t) kk];
                if (last) n = landOnChordTone (chordAtT (st[i]), n);
                const double len = last ? std::min (0.9, 3.98 - st[i]) : std::max (0.25, (st[i + 1] - st[i]) * 0.9);
                const int p16 = (int) std::lround (st[i] * 4.0);
                push (st[i] + swingOffset (p16, s.swing), len, n, s.fillVel * 0.9f, LayerFill, true, r);
            }
        }
        else
        {
            // Downie / Lindo: acordes sincopados na região aguda, como resposta ao canto
            static const int rhythms[3][3] = { { 10, 13, 14 }, { 9, 11, 14 }, { 11, 12, 15 } };
            const int* rh = rhythms[r.below (3)];
            for (int i = 0; i < 3; ++i)
            {
                Rand rr (r.next());
                const double t = rh[i] * s16;
                const double len = i == 2 ? 0.5 : L;
                for (int n : triad (chordAtT (t), s.fillLow)) { Rand r3 = rr; push (t + swingOffset (rh[i], s.swing), len, n, s.fillVel * (i == 2 ? 1.0f : 0.85f), LayerFill, true, r3); }
            }
        }
    }

    std::sort (out.begin(), out.end(), [] (const Event& a, const Event& b) { return a.start < b.start; });
    return out;
}

// ------------------------------------------------------------------ progressão e loop
inline int totalBeats (const std::vector<Chord>& chords)
{
    int n = 0;
    for (auto& c : chords) n += std::max (1, c.beats);
    return n;
}

inline const Chord& chordForBeat (const std::vector<Chord>& chords, int beat)
{
    static const Chord fallback;
    const int tb = totalBeats (chords);
    if (tb <= 0) return fallback;
    int b = ((beat % tb) + tb) % tb;
    for (auto& c : chords)
    {
        if (b < std::max (1, c.beats)) return c;
        b -= std::max (1, c.beats);
    }
    return chords.back();
}

// compassos do loop: menor número de compassos inteiros que fecha a progressão e o intervalo dos fills (máx. 32)
inline int loopBars (const std::vector<Chord>& chords, const Settings& s)
{
    const int tb = std::max (1, totalBeats (chords));
    const int fe = s.fillsOn ? std::max (1, s.fillEvery) : 1;
    for (int bars = 1; bars <= 32; ++bars)
        if ((bars * 4) % tb == 0 && bars % fe == 0) return bars;
    return 32;
}

inline std::vector<Event> buildLoop (const std::vector<Chord>& chords, const Settings& s, int& bars)
{
    bars = loopBars (chords, s);
    std::vector<Event> all;
    for (int b = 0; b < bars; ++b)
    {
        Chord bc[4];
        for (int i = 0; i < 4; ++i) bc[i] = chordForBeat (chords, b * 4 + i);
        for (auto e : barEvents (b, bc, s))
        {
            e.start += 4.0 * b;
            all.push_back (e);
        }
    }
    return all;
}

// ------------------------------------------------------------------ MÚSICOS (o estudo dos organistas jamaicanos)
// Cada músico é uma receita: a grade de bubble e chop, o balanço (swing, human, ghost), os fills e os dois registros
// de drawbar (mão esquerda e mão direita). BPM na contagem com a caixa no 2 e no 4 (até 100).
struct Musician
{
    const char* name;      // no botão
    const char* who;       // nome completo e época
    const char* rh;        // grade da mão direita
    const char* lh;        // grade da mão esquerda
    int beat;
    float swing, human, ghost;
    int fillStyle, fillEvery;
    float bpm;
    int bubbleDb[9], chopDb[9];   // drawbars 16' 5⅓' 8' 4' 2⅔' 2' 1⅗' 1⅓' 1'
    bool perc;
    int leslie;                   // 0 desligada, 1 lenta, 2 rápida (só na mão direita)
    float drive;
};

enum MusicianId { MusBeckford = 0, MusMittoo, MusCollins, MusWright, MusLindo, MusDownie, MusGladdy, MusSteely, NumMusicians };

inline const Musician& musician (int m)
{
    //            1e&a2e&a3e&a4e&a
    static const Musician t[NumMusicians] = {
        // Theophilus Beckford: o contratempo do ska; a esquerda só pontua, leve, em volta do acorde
        { "BECKFORD", "THEOPHILUS BECKFORD \xc2\xb7 SKA 1959-65",
          "..X...X...X...X.", ".x.x.x.x.x.x.x.x", BeatOneDrop, 0.04f, 0.22f, 0.40f, FillMittoo, 4, 96.0f,
          { 0,0,8,4,0,0,0,0,0 }, { 8,8,8,0,0,0,0,0,0 }, false, 0, 0.15f },
        // Jackie Mittoo: o pioneiro do bubble no Studio One — "e" forte, "a" fantasma; licks curtos no fim da frase
        { "MITTOO", "JACKIE MITTOO \xc2\xb7 STUDIO ONE 1966-72",
          "..X...X...X...X.", ".X.x.X.x.X.x.X.x", BeatOneDrop, 0.12f, 0.30f, 0.50f, FillMittoo, 4, 80.0f,
          { 0,0,8,2,0,0,0,0,0 }, { 8,8,8,0,0,0,0,0,0 }, false, 0, 0.20f },
        // Ansel Collins: o Hammond na frente do skinhead reggae (Double Barrel) — shuffle cheio e melodia a cada 2 compassos
        { "COLLINS", "ANSEL COLLINS \xc2\xb7 EARLY REGGAE 1969-72",
          "..X...X...X...X.", ".X.X.X.X.X.X.X.X", BeatOneDrop, 0.30f, 0.30f, 0.60f, FillWright, 2, 86.0f,
          { 0,0,8,4,0,0,0,0,0 }, { 8,8,8,6,0,0,0,0,0 }, false, 1, 0.35f },
        // Winston Wright: mestre do Hammond (Supersonics, Bunny Lee) — esquerda leve para a melodia "grande e gorda" em oitavas
        { "WRIGHT", "WINSTON WRIGHT \xc2\xb7 SUPERSONICS 1969-76",
          "..X...X...X...X.", ".x.x.x.x.x.x.x.x", BeatOneDrop, 0.35f, 0.35f, 0.45f, FillWright, 4, 76.0f,
          { 0,0,8,4,0,0,0,0,3 }, { 8,8,8,4,0,0,0,0,0 }, false, 1, 0.30f },
        // Wire Lindo: o bubble "difícil de reproduzir" dos Wailers — registro de apito (8' + 4' + 1'), sem Leslie no bubble
        { "LINDO", "EARL \"WIRE\" LINDO \xc2\xb7 WAILERS 1973-80",
          "..X...X...X...X.", ".X.x.X.x.X.x.X.x", BeatOneDrop, 0.25f, 0.28f, 0.35f, FillMittoo, 4, 76.0f,
          { 0,0,8,4,0,0,0,0,6 }, { 8,8,8,0,0,0,0,0,0 }, false, 1, 0.25f },
        // Tyrone Downie: Wailers 1976-80 — esquerda só no "e", acordes sincopados respondendo ao canto
        { "DOWNIE", "TYRONE DOWNIE \xc2\xb7 WAILERS 1976-80",
          "..X...X...X...X.", ".X...X...X...X..", BeatOneDrop, 0.20f, 0.30f, 0.40f, FillDownie, 4, 74.0f,
          { 0,0,8,0,0,0,0,0,6 }, { 8,8,8,0,0,0,0,0,0 }, false, 1, 0.25f },
        // Gladstone "Gladdy" Anderson: Treasure Isle e Roots Radics (rub a dub) — espaço, só o "a", fragmentos graves e lentos
        { "GLADDY", "GLADSTONE ANDERSON \xc2\xb7 ROOTS RADICS 1979-84",
          "..X...X...X...X.", "...x...x...x...x", BeatOneDrop, 0.15f, 0.30f, 0.55f, FillGladdy, 4, 78.0f,
          { 0,0,8,2,0,0,0,0,0 }, { 8,8,8,0,0,0,0,0,0 }, false, 0, 0.15f },
        // Era digital (Steely & Clevie): grade presa, sem swing, "e" e "a" fortes como na programação
        { "STEELY", "WYCLIFFE \"STEELY\" JOHNSON \xc2\xb7 DIGITAL 1985-95",
          "..X...X...X...X.", ".X.X.X.X.X.X.X.X", BeatSteppers, 0.0f, 0.05f, 0.55f, FillDownie, 8, 92.0f,
          { 0,0,8,6,4,0,0,0,6 }, { 8,8,8,8,0,0,0,0,4 }, false, 0, 0.10f },
    };
    return t[std::clamp (m, 0, NumMusicians - 1)];
}

// ------------------------------------------------------------------ ÉPOCAS (os 10 estilos do banco de riddims)
struct Epoch { int musician, beat, feel; float bpm; };   // feel = -1: a grade do músico; senão o atalho de feel
inline constexpr int numEpochs = 10;
inline const Epoch& epoch (int e)
{
    // ROCKSTEADY, EARLY REGGAE, ROOTS, ROCKERS, LOVERS ROCK, RUB A DUB, DIGITAL, RAGGA, DANCEHALL, NEW ROOTS
    static const Epoch t[numEpochs] = {
        { MusMittoo,  BeatOneDrop,  FeelRocksteady, 80.0f },
        { MusCollins, BeatOneDrop,  -1, 84.0f },
        { MusWright,  BeatOneDrop,  -1, 74.0f },
        { MusDownie,  BeatRockers,  -1, 76.0f },
        { MusLindo,   BeatOneDrop,  -1, 72.0f },
        { MusGladdy,  BeatOneDrop,  -1, 78.0f },
        { MusSteely,  BeatRockers,  -1, 92.0f },
        { MusSteely,  BeatSteppers, -1, 94.0f },
        { MusSteely,  BeatSteppers, -1, 96.0f },
        { MusDownie,  BeatOneDrop,  -1, 74.0f },
    };
    return t[std::clamp (e, 0, numEpochs - 1)];
}
inline const char* epochName (int e)
{
    static const char* n[numEpochs] = { "ROCKSTEADY", "EARLY REGGAE", "ROOTS", "ROCKERS", "LOVERS ROCK", "RUB A DUB", "DIGITAL", "RAGGA", "DANCEHALL", "NEW ROOTS" };
    return n[std::clamp (e, 0, numEpochs - 1)];
}
inline const char* epochYears (int e)
{
    static const char* n[numEpochs] = { "1966-68", "1968-72", "1972-79", "1975-80", "1969-94", "1979-84", "1985-89", "1990-95", "1995-2007", "1999-HOJE" };
    return n[std::clamp (e, 0, numEpochs - 1)];
}

// referência da bateria na grade (só desenho): caixa no 2 e no 4 sempre; bumbo conforme a batida
//   ONE DROP: bumbo junto com a caixa no 2 e no 4 · ROCKERS (Style Scott): bumbo no 1 e no 3 · STEPPERS: bumbo em todo tempo
inline bool kickAt (int beat, int p16)
{
    if (p16 % 4 != 0) return false;
    const int b = p16 / 4;
    if (beat == BeatOneDrop) return b == 1 || b == 3;
    if (beat == BeatRockers) return b == 0 || b == 2;
    return true;
}
inline bool snareAt (int p16) { return p16 == 4 || p16 == 12; }

// detecta tônica e maior/menor a partir das notas seguradas (modo "acordes do teclado")
// retorna false se não houver nota
inline bool detectChord (const bool held[128], Chord& out)
{
    int pcs[12] = {}, lowest = -1;
    for (int n = 0; n < 128; ++n)
        if (held[n]) { pcs[n % 12] = 1; if (lowest < 0) lowest = n; }
    if (lowest < 0) return false;
    // procura uma tríade completa (prefere a tônica no baixo), depois quinta, depois terça
    int bestRoot = lowest % 12, bestScore = -1; bool bestMinor = false;
    for (int r = 0; r < 12; ++r)
    {
        if (! pcs[r]) continue;
        for (int m = 0; m < 2; ++m)
        {
            const int third = pcs[(r + (m ? 3 : 4)) % 12], fifth = pcs[(r + 7) % 12];
            int score = third * 4 + fifth * 2 + (r == lowest % 12 ? 1 : 0);
            if (score > bestScore) { bestScore = score; bestRoot = r; bestMinor = (m == 1); }
        }
    }
    out.root = bestRoot;
    out.minor = bestMinor;
    out.beats = 4;
    return true;
}
} // namespace mro
