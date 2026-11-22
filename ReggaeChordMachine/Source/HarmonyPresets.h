// Gerado por tools/harmony_presets.py — não editar à mão.
// Banco de PRESETS de harmonia da DubStation (MRCM-1240 e MRM-0420), por estilo e década.
#pragma once

namespace dubpresets
{
struct Chord { int root; int minor; int dur; };   // tônica 0..11 (C = 0), 1 = menor, duração 0..5 (1/2, 1, 2, 3, 4, 8 comp.)
struct Preset { const char* folder; const char* era; const char* name; int numChords; Chord chords[4]; };

inline constexpr int numFolders = 6;
inline const char* const folders[numFolders] = { "ROOTS", "ROCKERS", "RUB A DUB", "LOVERS ROCK", "DANCEHALL DIGITAL", "NEW ROOTS 2000" };

inline constexpr int numPresets = 39;
inline const Preset presets[numPresets] = {
    { "ROOTS", "70s", "Satta (Abyssinians)", 4, { { 9, 1, 1 }, { 2, 1, 1 }, { 9, 1, 0 }, { 4, 1, 0 } } },
    { "ROOTS", "70s", "Marcus Garvey (Burning Spear)", 2, { { 9, 1, 1 }, { 7, 0, 1 } } },
    { "ROOTS", "70s", "Garvey coro", 3, { { 2, 1, 0 }, { 0, 0, 0 }, { 9, 1, 1 } } },
    { "ROOTS", "70s", "Nyabinghi (um acorde)", 1, { { 4, 1, 2 } } },
    { "ROOTS", "70s", "Menor i-iv", 2, { { 7, 1, 1 }, { 0, 1, 1 } } },
    { "ROOTS", "70s", "Studio One I-IV", 2, { { 0, 0, 1 }, { 5, 0, 1 } } },
    { "ROOTS", "70s", "Wailers i-VII-VI", 3, { { 9, 1, 1 }, { 7, 0, 0 }, { 5, 0, 0 } } },
    { "ROOTS", "70s", "Real Rock (Bm-F#m)", 2, { { 11, 1, 1 }, { 6, 1, 1 } } },
    { "ROCKERS", "70s", "Stalag (Cm-Bb)", 2, { { 0, 1, 1 }, { 10, 0, 1 } } },
    { "ROCKERS", "70s", "Channel One i-VII", 2, { { 7, 1, 0 }, { 5, 0, 0 } } },
    { "ROCKERS", "70s", "Rockers menor (Em-Am)", 2, { { 4, 1, 1 }, { 9, 1, 1 } } },
    { "ROCKERS", "70s", "Revolutionaries I-IV-V", 3, { { 7, 0, 1 }, { 0, 0, 0 }, { 2, 0, 0 } } },
    { "ROCKERS", "70s", "Dorico (Dm-G)", 2, { { 2, 1, 1 }, { 7, 0, 1 } } },
    { "ROCKERS", "70s", "Melodica (Am-Em)", 2, { { 9, 1, 1 }, { 4, 1, 1 } } },
    { "RUB A DUB", "80s", "Night Nurse (Am-G)", 2, { { 9, 1, 1 }, { 7, 0, 1 } } },
    { "RUB A DUB", "80s", "Night Nurse ponte", 3, { { 2, 1, 0 }, { 9, 1, 0 }, { 7, 0, 1 } } },
    { "RUB A DUB", "80s", "Cuss Cuss (Dm-F-G)", 3, { { 2, 1, 1 }, { 5, 0, 0 }, { 7, 0, 0 } } },
    { "RUB A DUB", "80s", "Roots Radics (Fm-Eb)", 2, { { 5, 1, 1 }, { 3, 0, 1 } } },
    { "RUB A DUB", "80s", "Rub a dub i-iv", 2, { { 2, 1, 1 }, { 7, 1, 1 } } },
    { "RUB A DUB", "80s", "Dancehall 82 (Em-D)", 2, { { 4, 1, 1 }, { 2, 0, 1 } } },
    { "RUB A DUB", "80s", "Volcano (Cm-Fm)", 3, { { 0, 1, 1 }, { 5, 1, 0 }, { 10, 0, 0 } } },
    { "LOVERS ROCK", "70s", "I-vi-IV-V (G-Em-C-D)", 4, { { 7, 0, 1 }, { 4, 1, 1 }, { 0, 0, 1 }, { 2, 0, 1 } } },
    { "LOVERS ROCK", "80s", "Lovers I-IV-V (A-D-E)", 3, { { 9, 0, 1 }, { 2, 0, 0 }, { 4, 0, 0 } } },
    { "LOVERS ROCK", "80s", "Soul ii-V-I (Dm-G-C)", 3, { { 2, 1, 1 }, { 7, 0, 1 }, { 0, 0, 2 } } },
    { "LOVERS ROCK", "80s", "vi-IV-I-V (Am-F-C-G)", 4, { { 9, 1, 1 }, { 5, 0, 1 }, { 0, 0, 1 }, { 7, 0, 1 } } },
    { "LOVERS ROCK", "80s", "IV-V-iii-vi (C-D-Bm-Em)", 4, { { 0, 0, 0 }, { 2, 0, 0 }, { 11, 1, 0 }, { 4, 1, 0 } } },
    { "LOVERS ROCK", "90s", "Crossover I-V-vi-IV (Bb)", 4, { { 10, 0, 1 }, { 5, 0, 1 }, { 7, 1, 1 }, { 3, 0, 1 } } },
    { "DANCEHALL DIGITAL", "80s", "Digital 85 (um acorde)", 1, { { 7, 1, 2 } } },
    { "DANCEHALL DIGITAL", "80s", "Ring the Alarm (Stalag 85)", 2, { { 0, 1, 1 }, { 10, 0, 1 } } },
    { "DANCEHALL DIGITAL", "90s", "Ragga i-VII (Dm-C)", 2, { { 2, 1, 1 }, { 0, 0, 1 } } },
    { "DANCEHALL DIGITAL", "90s", "Dancehall 92 (Dm-Gm-C)", 3, { { 2, 1, 1 }, { 7, 1, 0 }, { 0, 0, 0 } } },
    { "DANCEHALL DIGITAL", "90s", "Real Rock 94 (Bm-F#m)", 2, { { 11, 1, 1 }, { 6, 1, 1 } } },
    { "DANCEHALL DIGITAL", "90s", "Ragga menor (Am-Em-F-G)", 4, { { 9, 1, 0 }, { 4, 1, 0 }, { 5, 0, 0 }, { 7, 0, 0 } } },
    { "NEW ROOTS 2000", "00s", "Full Up 2000 (Am-Dm)", 2, { { 9, 1, 1 }, { 2, 1, 1 } } },
    { "NEW ROOTS 2000", "00s", "One drop i-VI-VII (Em)", 3, { { 4, 1, 1 }, { 0, 0, 0 }, { 2, 0, 0 } } },
    { "NEW ROOTS 2000", "00s", "Juggling (Gm-Eb-F)", 3, { { 7, 1, 1 }, { 3, 0, 0 }, { 5, 0, 0 } } },
    { "NEW ROOTS 2000", "00s", "Conscious (Bm-G-A)", 3, { { 11, 1, 1 }, { 7, 0, 0 }, { 9, 0, 0 } } },
    { "NEW ROOTS 2000", "00s", "Pop reggae I-V-vi-IV (C)", 4, { { 0, 0, 1 }, { 7, 0, 1 }, { 9, 1, 1 }, { 5, 0, 1 } } },
    { "NEW ROOTS 2000", "00s", "Neo roots i-iv-v (Am)", 3, { { 9, 1, 1 }, { 2, 1, 0 }, { 4, 1, 0 } } },
};
}
