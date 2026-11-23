// Presets de BPM do MRM-0420 por época e estilo de reggae (mesma tabela do site/riddim).
// Andamentos típicos de cada época, contados como a bateria do MRDM-12 conta (caixa no 2 e no 4).
#pragma once

namespace bpmpresets
{
struct Preset { const char* style; const char* name; int bpm; };

inline constexpr int numPresets = 24;
inline const Preset presets[numPresets] = {
    { "ROCKSTEADY 66-68",     "Rocksteady lento",       74 },
    { "ROCKSTEADY 66-68",     "Rocksteady",             80 },
    { "EARLY REGGAE 68-72",   "Early reggae",           84 },
    { "EARLY REGGAE 68-72",   "Skinhead reggae",        90 },
    { "ROOTS 70s",            "Nyabinghi / roots lento", 66 },
    { "ROOTS 70s",            "Roots one drop",         72 },
    { "ROOTS 70s",            "Roots classico",         76 },
    { "ROCKERS 75-79",        "Rockers",                78 },
    { "ROCKERS 75-79",        "Rockers rapido",         82 },
    { "RUB A DUB 80-84",      "Rub a dub (Volcano)",    84 },
    { "RUB A DUB 80-84",      "Rub a dub rapido",       88 },
    { "DIGITAL 85-89",        "Digital (Sleng Teng)",   82 },
    { "DIGITAL 85-89",        "Digital (King Jammy)",   90 },
    { "DANCEHALL 90s",        "Ragga 90",               96 },
    { "DANCEHALL 90s",        "Dancehall 90s",         100 },
    { "DANCEHALL 2000s",      "Dancehall 2000",         52 },
    { "LOVERS ROCK 75-95",    "Lovers lento",           70 },
    { "LOVERS ROCK 75-95",    "Lovers rock",            76 },
    { "NEW ROOTS 2000s",      "New roots",              72 },
    { "NEW ROOTS 2000s",      "New roots groove",       78 },
    { "DUB",                  "Dub pesado",             64 },
    { "DUB",                  "Dub",                    70 },
    { "STEPPERS UK 80s-90s",  "Steppers UK",            65 },
    { "STEPPERS UK 80s-90s",  "Steppers rapido",        70 },
};
}
