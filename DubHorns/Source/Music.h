// =============================================================================
//  Mastor Dub Horns — teoria musical, instrumentos do naipe e harmonia diatônica
//  C++17 puro (sem JUCE) para ser testado offline.
// =============================================================================
#pragma once
#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <algorithm>

namespace horns
{
// ------------------------------------------------------------------ escalas
struct Scale { const char* name; int n; int iv[7]; int parent; }; // parent = escala de 7 notas usada pelas sugestões

inline const std::array<Scale, 7>& scales()
{
    static const std::array<Scale, 7> s {{
        { "Menor natural",     7, { 0, 2, 3, 5, 7, 8, 10 }, 0 },
        { "Maior",             7, { 0, 2, 4, 5, 7, 9, 11 }, 1 },
        { "Dorico",            7, { 0, 2, 3, 5, 7, 9, 10 }, 2 },
        { "Mixolidio",         7, { 0, 2, 4, 5, 7, 9, 10 }, 3 },
        { "Pentatonica menor", 5, { 0, 3, 5, 7, 10, 0, 0 }, 0 },
        { "Pentatonica maior", 5, { 0, 2, 4, 7, 9, 0, 0 }, 1 },
        { "Blues",             6, { 0, 3, 5, 6, 7, 10, 0 }, 0 },
    }};
    return s;
}
constexpr int kNumScales = 7;

inline const char* noteName (int pc)
{
    static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n[((pc % 12) + 12) % 12];
}
inline const char* noteNamePt (int pc)
{
    static const char* n[12] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
    return n[((pc % 12) + 12) % 12];
}
inline std::string midiName (int m) { return std::string (noteName (m)) + std::to_string (m / 12 - 1); }

inline int mod12 (int x) { return ((x % 12) + 12) % 12; }

inline bool inScale (int midi, int key, int scale)
{
    const auto& sc = scales()[(size_t) std::clamp (scale, 0, kNumScales - 1)];
    const int rel = mod12 (midi - key);
    for (int i = 0; i < sc.n; ++i) if (sc.iv[i] == rel) return true;
    return false;
}

// Desloca a nota "steps" graus dentro da escala (passos pensados para escala de 7 notas;
// nas pentatônicas/blues o número de passos é proporcional). Nota fora da escala: usa semitons.
inline int diatonic (int midi, int steps, int semiFallback, int key, int scale)
{
    if (steps == 0) return midi;
    const auto& sc = scales()[(size_t) std::clamp (scale, 0, kNumScales - 1)];
    const int rel = mod12 (midi - key);
    int idx = -1;
    for (int i = 0; i < sc.n; ++i) if (sc.iv[i] == rel) { idx = i; break; }
    if (idx < 0) return midi + semiFallback;
    const int st = (int) std::lround ((double) steps * sc.n / 7.0);
    const int base = midi - rel;
    const int tot = idx + st;
    const int o = (int) std::floor ((double) tot / sc.n);
    const int i = tot - o * sc.n;
    return base + o * 12 + sc.iv[i];
}

// Grau da sugestão (0 = tônica, 7 = oitava) -> nota MIDI no tom escolhido (tônica entre C4 e B4)
inline int degreeToMidi (int deg, int key, int scale)
{
    const auto& sk = scales()[(size_t) std::clamp (scale, 0, kNumScales - 1)];
    const auto& par = scales()[(size_t) sk.parent];
    const int o = (int) std::floor (deg / 7.0);
    const int i = deg - o * 7;
    int m = 60 + mod12 (key) + o * 12 + par.iv[i];
    if (sk.n != 7) // encaixa na pentatônica / blues
    {
        int best = m, bd = 99;
        for (int d = -2; d <= 2; ++d)
            if (inScale (m + d, key, scale) && std::abs (d) < bd) { bd = std::abs (d); best = m + d; }
        m = best;
    }
    return m;
}

// ------------------------------------------------------------------ harmonias do naipe
struct Harmony { const char* name; int steps; int semis; };
inline const std::array<Harmony, 7>& harmonies()
{
    static const std::array<Harmony, 7> h {{
        { "Unissono",    0,  0 },
        { "3a acima",    2,  4 },
        { "3a abaixo",  -2, -3 },
        { "5a abaixo",  -4, -5 },
        { "5a acima",    4,  7 },
        { "6a abaixo",  -5, -8 },
        { "4a acima",    3,  5 },
    }};
    return h;
}
constexpr int kNumHarmonies = 7;

// ------------------------------------------------------------------ instrumentos
enum Wave { Saw, Square, Tri };
struct OscDef { Wave wave; float gain; float detuneCents; };
struct InstDef
{
    const char* id; const char* name; const char* desc;
    int lo, hi;                       // extensão (MIDI)
    int numOsc; OscDef osc[3];
    float cut, env, q;                // filtro: corte base, abertura do envelope, Q
    float f1, g1, f2, g2;             // formantes (Hz, dB)
    float atk, breath, breathF, vib, scoop, gain;
};
constexpr int kNumInst = 6;

inline const std::array<InstDef, kNumInst>& instruments()
{
    static const std::array<InstDef, kNumInst> d {{
        { "trumpet",  "TROMPETE",     "Brilhante, ataque rapido", 52, 86,
          3, { { Saw, .50f, 0 }, { Saw, .32f, 7 }, { Square, .10f, -5 } },
          950, 4200, 1.2f, 1250, 6, 2700, 4, .030f, .45f, 2800, 1.0f,  60, 1.00f },
        { "flugel",   "FLUGELHORN",   "Redondo e aveludado", 52, 84,
          2, { { Saw, .40f, 0 }, { Tri, .55f, 4 }, { Saw, 0, 0 } },
          650, 2000, 0.8f,  900, 5, 1800, 2, .050f, .55f, 2000, 1.1f,  50, 1.15f },
        { "alto",     "SAX ALTO",     "Nasal e cortante", 49, 81,
          3, { { Square, .42f, 0 }, { Saw, .34f, 4 }, { Square, .14f, -9 } },
          1100, 3200, 2.2f, 1500, 8, 3000, 3, .035f, .90f, 3500, 1.3f,  90, 0.90f },
        { "tenor",    "SAX TENOR",    "Encorpado e rouco, o das pontadas", 44, 77,
          3, { { Square, .46f, 0 }, { Saw, .40f, 5 }, { Saw, .12f, -8 } },
          800, 2600, 2.4f, 1050, 8, 2400, 3, .040f, 1.0f, 2600, 1.3f, 110, 1.00f },
        { "trombone", "TROMBONE",     "Grave, gordo, ataque lento", 40, 72,
          2, { { Saw, .58f, 0 }, { Saw, .30f, -6 }, { Saw, 0, 0 } },
          450, 2200, 1.3f,  600, 6, 1400, 3, .055f, .40f, 1500, 0.7f,  80, 1.15f },
        { "bari",     "SAX BARITONO", "A base grave do naipe", 36, 65,
          2, { { Square, .50f, 0 }, { Saw, .45f, 5 }, { Saw, 0, 0 } },
          500, 1800, 2.0f,  650, 7, 1600, 3, .050f, .80f, 1800, 1.0f,  90, 1.00f },
    }};
    return d;
}

// Nota que um instrumento toca quando o usuário toca "midi"
inline int sectionNote (int midi, int harm, int octave, int inst, int key, int scale)
{
    const auto& h = harmonies()[(size_t) std::clamp (harm, 0, kNumHarmonies - 1)];
    int m = diatonic (midi, h.steps, h.semis, key, scale) + octave * 12;
    const auto& d = instruments()[(size_t) inst];
    while (m > d.hi) m -= 12;
    while (m < d.lo) m += 12;
    return m;
}

// ------------------------------------------------------------------ sync do eco
struct SyncDiv { const char* name; double beats; };
inline const std::array<SyncDiv, 9>& syncDivs()
{
    static const std::array<SyncDiv, 9> s {{
        { "Livre (ms)", 0 }, { "1/4", 1 }, { "1/4 pontuada", 1.5 }, { "1/4 tercina", 2.0 / 3.0 },
        { "1/8", .5 }, { "1/8 pontuada", .75 }, { "1/8 tercina", 1.0 / 3.0 }, { "1/16", .25 }, { "1/16 pontuada", .375 },
    }};
    return s;
}
} // namespace horns
