// Presets de BPM do MRM-0420 por estilo de reggae (mesma tabela do site/riddim).
// Andamentos típicos de cada estilo, contados como a bateria do MRDM-12 conta (caixa no 2 e no 4).
#pragma once

namespace bpmpresets
{
struct Preset { const char* style; const char* name; int bpm; };

inline constexpr int numPresets = 20;
inline const Preset presets[numPresets] = {
    { "ROOTS",          "Roots lento",           66 },
    { "ROOTS",          "Roots one drop",        72 },
    { "ROOTS",          "Roots classico",        76 },
    { "ROCKSTEADY",     "Rocksteady",            80 },
    { "ROCKERS",        "Rockers",               78 },
    { "ROCKERS",        "Rockers rapido",        84 },
    { "RUB A DUB",      "Rub a dub",             84 },
    { "RUB A DUB",      "Rub a dub rapido",      90 },
    { "LOVERS ROCK",    "Lovers lento",          70 },
    { "LOVERS ROCK",    "Lovers rock",           76 },
    { "LOVERS ROCK",    "Lovers dancante",       82 },
    { "DANCEHALL",      "Dancehall digital 85",  92 },
    { "DANCEHALL",      "Ragga 90",              98 },
    { "DANCEHALL",      "Dancehall 2000",       104 },
    { "NEW ROOTS",      "New roots",             72 },
    { "NEW ROOTS",      "New roots groove",      78 },
    { "DUB",            "Dub pesado",            64 },
    { "DUB",            "Dub",                   70 },
    { "STEPPERS",       "Steppers UK",          130 },
    { "STEPPERS",       "Steppers rapido",      140 },
};
}
