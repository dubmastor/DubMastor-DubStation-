// Mastor Reggae Clavinet MRCL-71 — gerador das linhas da CLAVINET MACHINE.
// Mesmo algoritmo da versão para celular (site/web/reggaeclavinet/index.html, função generate): ao mudar um, mude o outro.
// Contagem do reggae da Dub Station: caixa no 2 e no 4. Passos de semicolcheia no compasso (0..15):
// o "&" de cada tempo = 2, 6, 10, 14 e o "a" = 3, 7, 11, 15. Sem JUCE: compila sozinho no teste.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mrcl
{
inline double clampd (double x, double a, double b) { return std::min (b, std::max (a, x)); }

// ------------------------------------------------------------------ CLAVINETISTAS
// a = SKANK DUPLO, b = CONTRAPONTO, c = RIFF (2 compassos, 0..31). acc = força do "a".
// Inspirados no estilo de cada um (créditos de clavinet em discos de reggae), não são transcrições.
struct Musician
{
    const char* name; const char* who; const char* tag; const char* desc;
    std::vector<int> a; double acc; std::vector<int> b; std::vector<int> c; int timbre; double swing;
};

inline const std::vector<Musician>& musicians()
{
    static const std::vector<Musician> m = {
        { "WIRE", "Earl \"Wire\" Lindo", "WAILERS 1973 \xc2\xb7 1978-81", "Skank duplo abafado, bem curto, colado no piano: o \"&\" forte e o \"a\" leve. Roots.",
          { 2, 3, 6, 7, 10, 11, 14, 15 }, 0.55, { 3, 7, 11, 13, 15 }, { 2, 6, 9, 10, 14, 18, 22, 25, 26, 30 }, 1, 0.15 },
        { "TOUTER", "Bernard \"Touter\" Harvey", "WAILERS 1974 \xc2\xb7 INNER CIRCLE", "Mais funk: semicolcheias com notas fantasma e o wah abrindo, sem perder o contratempo.",
          { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15 }, 0.6, { 3, 5, 8, 11, 12, 15 }, { 2, 3, 6, 10, 11, 14, 18, 19, 22, 26, 29, 30 }, 2, 0.1 },
        { "DOWNIE", "Tyrone Downie", "WAILERS 1976-81", "Respostas sincopadas entre os acordes do piano: entra no \"a\" e no \"e\", deixa espa\xc3\xa7o.",
          { 2, 3, 6, 10, 11, 14 }, 0.65, { 3, 7, 9, 11, 15 }, { 3, 6, 10, 13, 14, 19, 22, 26, 29, 30 }, 0, 0.2 },
        { "LYN", "Robbie Lyn", "SESS\xc3\x95" "ES \xc2\xb7 SLY & ROBBIE", "Riff mel\xc3\xb3" "dico em oitavas que vira gancho da m\xc3\xbasica, com phaser.",
          { 2, 6, 7, 10, 14, 15 }, 0.6, { 2, 5, 8, 11, 14 }, { 2, 4, 6, 9, 10, 14, 18, 20, 22, 25, 26, 28 }, 3, 0.1 },
        { "ANSEL", "Ansel Collins", "EARLY REGGAE 1969-73", "Early reggae: chops saltados nas colcheias, com shuffle forte.",
          { 2, 3, 6, 7, 10, 11, 14, 15 }, 0.75, { 2, 5, 8, 10, 13 }, { 2, 6, 8, 10, 14, 18, 22, 24, 26, 30 }, 0, 0.45 },
        { "WRIGHT", "Winston Wright", "SUPERSONICS \xc2\xb7 TOOTS", "Stabs secos no contratempo e uma figurinha em oitava no fim do compasso.",
          { 2, 6, 10, 14, 15 }, 0.7, { 6, 11, 14, 15 }, { 2, 6, 10, 13, 14, 15, 18, 22, 26, 29, 30, 31 }, 1, 0.2 },
        { "BOVELL", "Dennis Bovell", "UK DUB \xc2\xb7 LOVERS ROCK", "Pouca nota e mais longa, com phaser: deixa o dub respirar.",
          { 2, 6, 10, 14 }, 0.5, { 6, 11, 14 }, { 2, 10, 14, 18, 26, 30 }, 4, 0.15 } };
    return m;
}

inline const char* styleName (int s) { static const char* n[3] = { "SKANK DUPLO", "CONTRAPONTO", "RIFF EM OITAVAS" }; return n[std::clamp (s, 0, 2)]; }
inline const char* styleDesc (int s)
{
    static const char* n[3] = { "\"&\" e \"a\" \xc2\xb7 acorde curto, abafado", "responde entre os acordes do piano", "frase de 2 compassos, m\xc3\xa3o direita" };
    return n[std::clamp (s, 0, 2)];
}
inline const char* fillName (int f) { static const char* n[4] = { "SEM", "PUXADA", "RAJADA", "SUBIDA" }; return n[std::clamp (f, 0, 3)]; }
inline const char* fillDesc (int f)
{
    static const char* n[4] = { "s\xc3\xb3 a linha", "o pr\xc3\xb3ximo acorde adiantado no \"a\" do 4", "semicolcheias no 4, crescendo", "pentat\xc3\xb4nica subindo no 3 e no 4" };
    return n[std::clamp (f, 0, 3)];
}
inline const double durBeats[6] = { 2, 4, 8, 12, 16, 32 };
inline const char* durName (int d) { static const char* n[6] = { "1/2 COMP.", "1 COMP.", "2 COMP.", "3 COMP.", "4 COMP.", "8 COMP." }; return n[std::clamp (d, 0, 5)]; }
inline const char* rootName (int r) { static const char* n[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }; return n[((r % 12) + 12) % 12]; }

struct Chord { int slot = 0, root = 9; bool minor = true; double beats = 4; };
struct Event { double start = 0, length = 0.0625; int note = 48, vel = 100, slot = 0; };
struct Line { std::vector<Event> events; int bars = 2; };

struct Settings
{
    int musician = 0, fill = 0, fillEvery = 4, nota = 0;   // nota: 0 = 1/64, 1 = 1/32
    double center = 29, density = 0.8, swing = 0.15, velocity = 0.85;
    uint32_t seed = 1;
};

// gerador aleatório igual ao da versão web (xorshift de 32 bits)
struct Rng
{
    uint32_t s;
    explicit Rng (double seed) { s = (uint32_t) (int64_t) seed; if (s == 0) s = 0x9e3779b9u; }
    double operator()() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (double) s / 4294967296.0; }
};

struct ChordAt { Chord c; int k = 0; };
inline ChordAt chordAt (const std::vector<Chord>& cs, double beat)
{
    double tb = 0; for (auto& c : cs) tb += c.beats;
    if (cs.empty() || tb <= 0) return { Chord(), 0 };
    double b = std::fmod (beat, tb); if (b < 0) b += tb;
    for (int k = 0; k < (int) cs.size(); ++k) { if (b < cs[(size_t) k].beats - 1e-9) return { cs[(size_t) k], k }; b -= cs[(size_t) k].beats; }
    return { cs.back(), (int) cs.size() - 1 };
}

inline int loopBars (const std::vector<Chord>& cs, const Settings& s)
{
    double tb = 0; for (auto& c : cs) tb += c.beats;
    const int bars = std::max (1, (int) std::ceil (tb / 4.0));
    const int phrase = s.fillEvery == 8 ? 8 : 4;
    if (s.fill > 0 && std::fmod (tb, 4.0) < 1e-9 && bars < phrase && phrase % bars == 0) return phrase;
    return std::max (bars, 2);   // pelo menos 2 compassos (o riff tem 2)
}

inline std::vector<int> pcs (const Chord& c) { return { c.root, (c.root + (c.minor ? 3 : 4)) % 12, (c.root + 7) % 12 }; }
inline std::vector<int> penta (const Chord& c)
{
    static const int mi[5] = { 0, 3, 5, 7, 10 }, ma[5] = { 0, 2, 4, 7, 9 };
    std::vector<int> v; for (int i = 0; i < 5; ++i) v.push_back ((c.root + (c.minor ? mi[i] : ma[i])) % 12); return v;
}
inline bool has (const std::vector<int>& v, int x) { return std::find (v.begin(), v.end(), x) != v.end(); }
inline int nearUp (int pc, int lo) { int n = lo; while (((n % 12) + 12) % 12 != pc) ++n; return n; }
inline std::vector<int> voicing (const Chord& c, double center)
{
    std::vector<int> v; for (int pc : pcs (c)) v.push_back (nearUp (pc, (int) center - 5));
    std::sort (v.begin(), v.end()); return v;
}
inline std::vector<int> poolNotes (const std::vector<int>& set, int lo, int hi)
{
    std::vector<int> out; for (int n = lo; n <= hi; ++n) if (has (set, ((n % 12) + 12) % 12)) out.push_back (n); return out;
}
inline int jsRound (double x) { return (int) std::floor (x + 0.5); }

inline Line generate (int style, const std::vector<Chord>& cs, const Settings& s)
{
    Line line;
    const auto& mus = musicians();
    const auto& m = mus[(size_t) std::clamp (s.musician, 0, (int) mus.size() - 1)];
    const int bars = loopBars (cs, s);
    line.bars = bars;
    if (cs.empty()) return line;
    Rng r ((double) s.seed * 7919.0 + style * 104729.0 + std::clamp (s.musician, 0, 6) * 31.0 + 1.0);
    const double L = s.nota == 1 ? 0.125 : 0.0625, sw = s.swing * 0.125, vel = s.velocity, dens = s.density;
    const int C = (int) s.center;
    auto push = [&] (double beat, double len, int note, double v, int slot)
    { line.events.push_back ({ beat, len, note, std::clamp (jsRound (v * vel * 127.0), 1, 127), slot }); };
    auto off = [&] (int step) { return step % 2 == 1 ? sw : 0.0; };
    const int phrase = std::min (s.fillEvery == 8 ? 8 : 4, bars);
    int walk = C + 2;
    std::vector<int> riffMel;
    for (size_t i = 0; i < m.c.size(); ++i) riffMel.push_back ((int) std::floor (r() * 5));

    for (int bar = 0; bar < bars; ++bar)
    {
        const bool fillBar = s.fill > 0 && (bar % phrase == phrase - 1);
        for (int st = 0; st < 16; ++st)
        {
            const double beat = bar * 4 + st * 0.25;
            const auto ca = chordAt (cs, beat);
            const auto& c = ca.c;
            const auto v3 = voicing (c, s.center);
            if (fillBar && st >= 8)
            {
                const double t = beat + (st % 2 ? sw : 0.0);
                if (s.fill == 1)        // PUXADA
                {
                    if (st == 10 || st == 14) for (int n : v3) push (t, L, n, 1, c.slot);
                    if (st == 15) { const auto nx = chordAt (cs, bar * 4 + 4).c; for (int n : voicing (nx, s.center)) push (t, 0.2, n, 0.95, nx.slot); }
                }
                else if (s.fill == 2)   // RAJADA
                {
                    if (st == 10) for (int n : v3) push (t, L, n, 1, c.slot);
                    if (st >= 12) for (int n : v3) push (beat, 0.1, n, 0.55 + 0.15 * (st - 12), c.slot);
                }
                else if (s.fill == 3)   // SUBIDA
                {
                    const auto p = poolNotes (penta (c), C - 5, C + 9);
                    const int k = st - 8;
                    if (! p.empty()) push (t, 0.18, p[(size_t) std::min ((int) p.size() - 1, k + 1)], 0.7 + 0.04 * k, c.slot);
                }
                continue;
            }
            if (style == 0)   // SKANK DUPLO: acorde curto no "&" e no "a"
            {
                if (! has (m.a, st)) continue;
                const bool isAnd = st % 4 == 2, isA = st % 4 == 3, ghost = ! isAnd && ! isA;
                if (! isAnd && r() > dens) continue;
                const double v = isAnd ? 1.0 : (isA ? m.acc : 0.32);
                for (size_t i = ghost ? 1 : 0; i < v3.size(); ++i) push (beat + off (st), L, v3[i], v, c.slot);
            }
            else if (style == 1)   // CONTRAPONTO: nota ou dupla nas brechas do skank
            {
                if (! has (m.b, st) || r() > 0.35 + 0.65 * dens) continue;
                const auto p = poolNotes (r() < 0.6 ? pcs (c) : penta (c), C - 5, C + 7);
                if (p.empty()) continue;
                int best = p[0]; for (int n : p) if (std::abs (n - walk) < std::abs (best - walk)) best = n;
                const int bi = (int) (std::find (p.begin(), p.end(), best) - p.begin());
                const int idx = std::clamp (bi + (int) std::floor (r() * 3) - 1, 0, (int) p.size() - 1);
                walk = p[(size_t) idx];
                push (beat + off (st), std::max (L, 0.18), walk, st % 4 == 2 ? 0.85 : 0.7, c.slot);
                if (r() < 0.3) { const auto t3 = poolNotes (pcs (c), walk + 3, walk + 5); if (! t3.empty()) push (beat + off (st), std::max (L, 0.18), t3[0], 0.6, c.slot); }
            }
            else   // RIFF EM OITAVAS: a mesma frase rítmica de 2 compassos, nas notas do acorde da vez
            {
                const int s32 = (bar % 2) * 16 + st;
                const auto it = std::find (m.c.begin(), m.c.end(), s32);
                if (it == m.c.end()) continue;
                const size_t i = (size_t) (it - m.c.begin());
                const auto p = poolNotes (penta (c), C - 5, C + 1);
                if (p.empty()) continue;
                const int n = p[(size_t) std::clamp (riffMel[i], 0, (int) p.size() - 1)];
                int nxt = 32; for (int x : m.c) if (x > s32) { nxt = x; break; }
                const double len = std::min (0.45, (nxt - s32) * 0.25 - 0.04);
                push (beat + off (st), std::max (L, len), n, 0.9, c.slot);
                push (beat + off (st), std::max (L, len), n + 12, 0.75, c.slot);
            }
        }
    }
    const double end = bars * 4.0;
    for (auto& e : line.events) e.length = std::max (0.03, std::min (e.length, end - e.start - 0.01));
    std::stable_sort (line.events.begin(), line.events.end(), [] (const Event& a, const Event& b) { return a.start < b.start; });
    return line;
}

// acorde a partir das notas seguradas (MIDI que chega na trilha: o skank do MRP-80, do MRM-0420 ou de outra trilha)
inline bool detectChord (const bool held[128], Chord& out)
{
    int pc[12] = {}, lowest = -1;
    for (int n = 0; n < 128; ++n) if (held[n]) { pc[n % 12] = 1; if (lowest < 0) lowest = n; }
    if (lowest < 0) return false;
    int bestRoot = lowest % 12, bestScore = -1; bool bestMinor = false;
    for (int rr = 0; rr < 12; ++rr)
    {
        if (! pc[rr]) continue;
        for (int mm = 0; mm < 2; ++mm)
        {
            const int score = pc[(rr + (mm ? 3 : 4)) % 12] * 4 + pc[(rr + 7) % 12] * 2 + (rr == lowest % 12 ? 1 : 0);
            if (score > bestScore) { bestScore = score; bestRoot = rr; bestMinor = mm == 1; }
        }
    }
    out.root = bestRoot; out.minor = bestMinor; out.beats = 4; out.slot = 0;
    return true;
}
} // namespace mrcl
