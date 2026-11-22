// Gerado por tools/harmony_presets.py — não editar à mão.
// Banco de PRESETS de harmonia da DubStation (MRCM-1240 e MRM-0420), por estilo e década.
#pragma once

namespace dubpresets
{
struct Chord { int root; int minor; int dur; };   // tônica 0..11 (C = 0), 1 = menor, duração 0..5 (1/2, 1, 2, 3, 4, 8 comp.)
struct Preset { const char* folder; const char* era; const char* name; int numChords; Chord chords[4]; };

inline constexpr int numFolders = 6;
inline const char* const folders[numFolders] = { "ROOTS - ANOS 70", "ROCKERS - FIM DOS 70", "RUB A DUB - ANOS 80", "LOVERS ROCK - 70 A 90", "DANCEHALL - 80 E 90", "NEW ROOTS - ANOS 2000" };

inline constexpr int numPresets = 60;
inline const Preset presets[numPresets] = {
    { "ROOTS - ANOS 70", "70s", "Satta Massagana (Abyssinians 1969)", 4, { { 9, 1, 1 }, { 2, 1, 1 }, { 9, 1, 1 }, { 4, 1, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Pressure Drop (Toots 1969)", 4, { { 8, 0, 1 }, { 3, 0, 1 }, { 1, 0, 1 }, { 8, 0, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Marcus Garvey (Burning Spear 1975)", 2, { { 9, 1, 1 }, { 7, 0, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Natural Mystic (Bob Marley 1977)", 3, { { 2, 1, 1 }, { 4, 1, 1 }, { 9, 1, 2 } } },
    { "ROOTS - ANOS 70", "70s", "Exodus (Bob Marley 1977)", 4, { { 9, 1, 4 }, { 0, 0, 0 }, { 2, 1, 0 }, { 9, 1, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Jamming (Bob Marley 1977)", 4, { { 11, 1, 1 }, { 4, 0, 1 }, { 7, 0, 1 }, { 6, 1, 1 } } },
    { "ROOTS - ANOS 70", "70s", "One Love (Bob Marley 1977)", 4, { { 10, 0, 1 }, { 5, 0, 1 }, { 3, 0, 1 }, { 10, 0, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Smile Jamaica (Bob Marley 1976)", 3, { { 11, 1, 1 }, { 7, 0, 1 }, { 9, 0, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Police and Thieves (Junior Murvin 1976)", 4, { { 7, 0, 1 }, { 9, 1, 1 }, { 2, 0, 1 }, { 9, 1, 1 } } },
    { "ROOTS - ANOS 70", "70s", "Roots (Bob Marley 1977)", 4, { { 6, 1, 1 }, { 11, 1, 1 }, { 2, 0, 1 }, { 1, 1, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Legalize It (Peter Tosh 1976)", 2, { { 7, 0, 1 }, { 5, 0, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Burial (Peter Tosh 1976)", 4, { { 10, 0, 1 }, { 7, 1, 1 }, { 3, 0, 1 }, { 5, 0, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Uptown Top Ranking (Althea & Donna 1977)", 2, { { 9, 1, 1 }, { 7, 0, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Money in My Pocket (Dennis Brown 1979)", 4, { { 5, 0, 1 }, { 7, 1, 1 }, { 2, 1, 0 }, { 0, 0, 0 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Now That We Found Love (Third World 1978)", 3, { { 0, 1, 1 }, { 5, 1, 1 }, { 7, 1, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Ku Klux Klan (Steel Pulse 1978)", 2, { { 9, 1, 1 }, { 4, 1, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "Soldiers (Steel Pulse 1978)", 4, { { 11, 1, 1 }, { 6, 0, 1 }, { 7, 0, 1 }, { 9, 0, 1 } } },
    { "ROCKERS - FIM DOS 70", "70s", "One Drop (Bob Marley 1979)", 4, { { 0, 0, 1 }, { 9, 1, 1 }, { 2, 1, 1 }, { 7, 0, 1 } } },
    { "ROCKERS - FIM DOS 70", "80s", "Could You Be Loved (Bob Marley 1980)", 4, { { 11, 1, 1 }, { 4, 1, 1 }, { 2, 0, 1 }, { 11, 1, 1 } } },
    { "ROCKERS - FIM DOS 70", "80s", "Buffalo Soldier (Bob Marley 1983)", 4, { { 9, 0, 1 }, { 6, 1, 1 }, { 2, 0, 1 }, { 9, 0, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Night Nurse (Gregory Isaacs 1982)", 3, { { 9, 1, 1 }, { 7, 0, 1 }, { 2, 1, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Here I Come (Barrington Levy 1984)", 2, { { 4, 1, 1 }, { 9, 1, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Under Mi Sensi (Barrington Levy 1985)", 3, { { 0, 0, 1 }, { 5, 0, 1 }, { 7, 0, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Sensimilla (Barrington Levy 1980)", 2, { { 5, 1, 1 }, { 0, 1, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Pass the Kouchie (Mighty Diamonds 1981)", 2, { { 0, 0, 1 }, { 2, 1, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Bam Bam (Sister Nancy 1982)", 2, { { 0, 0, 1 }, { 10, 0, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Cool Down the Pace (Gregory Isaacs 1982)", 2, { { 10, 0, 1 }, { 0, 1, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Truths and Rights (Johnny Osbourne 1980)", 3, { { 1, 1, 1 }, { 4, 0, 1 }, { 8, 0, 1 } } },
    { "RUB A DUB - ANOS 80", "80s", "Sinsemilla (Black Uhuru 1980)", 4, { { 9, 1, 1 }, { 4, 1, 1 }, { 9, 1, 1 }, { 7, 0, 1 } } },
    { "RUB A DUB - ANOS 80", "70s", "Shine Eye Gal (Black Uhuru 1979)", 2, { { 0, 1, 2 }, { 7, 1, 2 } } },
    { "LOVERS ROCK - 70 A 90", "70s", "Help Me Make It Through the Night (John Holt 1974)", 4, { { 4, 0, 1 }, { 9, 0, 1 }, { 6, 1, 1 }, { 11, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "70s", "Is This Love (Bob Marley 1978)", 3, { { 6, 1, 2 }, { 2, 0, 1 }, { 9, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "70s", "Waiting in Vain (Bob Marley 1977)", 2, { { 8, 0, 1 }, { 1, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "80s", "Red Red Wine (UB40 1983)", 4, { { 1, 0, 1 }, { 6, 0, 1 }, { 8, 0, 1 }, { 6, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "80s", "Kingston Town (UB40 1989)", 4, { { 8, 0, 1 }, { 0, 1, 1 }, { 1, 0, 1 }, { 3, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "90s", "Close to You (Maxi Priest 1990)", 3, { { 3, 1, 2 }, { 8, 1, 1 }, { 10, 1, 1 } } },
    { "LOVERS ROCK - 70 A 90", "90s", "Rockaway (Beres Hammond 1994)", 4, { { 8, 0, 1 }, { 3, 0, 1 }, { 5, 1, 1 }, { 1, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "90s", "Tempted to Touch (Beres Hammond 1990)", 4, { { 0, 0, 1 }, { 9, 1, 1 }, { 2, 1, 1 }, { 7, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "90s", "My Number One (Gregory Isaacs 1990)", 3, { { 7, 0, 1 }, { 9, 1, 1 }, { 2, 0, 1 } } },
    { "LOVERS ROCK - 70 A 90", "80s", "Wild World (Maxi Priest 1988)", 4, { { 9, 1, 1 }, { 2, 0, 1 }, { 7, 0, 1 }, { 0, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "80s", "Ring the Alarm (Tenor Saw 1985)", 2, { { 0, 0, 1 }, { 10, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "90s", "Mr. Loverman (Shabba Ranks 1992)", 4, { { 9, 1, 1 }, { 2, 1, 1 }, { 7, 0, 1 }, { 9, 1, 1 } } },
    { "DANCEHALL - 80 E 90", "90s", "Informer (Snow 1992)", 2, { { 6, 0, 1 }, { 4, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "90s", "Sweat (Inner Circle 1992)", 4, { { 10, 0, 1 }, { 7, 1, 1 }, { 0, 1, 1 }, { 5, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "90s", "Here Comes the Hotstepper (Ini Kamoze 1994)", 2, { { 9, 1, 1 }, { 7, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "90s", "You Don't Love Me (Dawn Penn 1994)", 2, { { 9, 1, 1 }, { 2, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "90s", "Boombastic (Shaggy 1995)", 3, { { 2, 0, 1 }, { 7, 0, 1 }, { 9, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "00s", "It Wasn't Me (Shaggy 2000)", 3, { { 0, 0, 1 }, { 7, 0, 1 }, { 5, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "00s", "Get Busy (Sean Paul 2003)", 2, { { 5, 1, 1 }, { 6, 0, 1 } } },
    { "DANCEHALL - 80 E 90", "00s", "Temperature (Sean Paul 2005)", 4, { { 10, 1, 1 }, { 8, 0, 1 }, { 10, 1, 1 }, { 0, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "00s", "She's Royal (Tarrus Riley 2006)", 4, { { 3, 0, 1 }, { 8, 0, 1 }, { 3, 0, 1 }, { 10, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "00s", "Welcome to Jamrock (Damian Marley 2005)", 2, { { 7, 1, 1 }, { 2, 1, 1 } } },
    { "NEW ROOTS - ANOS 2000", "00s", "Herbalist (Alborosie 2008)", 2, { { 5, 1, 1 }, { 10, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "00s", "Kingston Town (Alborosie 2008)", 2, { { 4, 1, 1 }, { 9, 1, 1 } } },
    { "NEW ROOTS - ANOS 2000", "00s", "Mystical Reggae (Alborosie 2009)", 3, { { 11, 1, 1 }, { 7, 0, 1 }, { 9, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "10s", "Callin (Alborosie 2011)", 2, { { 9, 1, 1 }, { 2, 1, 1 } } },
    { "NEW ROOTS - ANOS 2000", "00s", "Ganja Farmer (Marlon Asher 2005)", 4, { { 0, 0, 1 }, { 4, 1, 1 }, { 9, 1, 1 }, { 5, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "10s", "Dem Gone (Kabaka Pyramid 2018)", 3, { { 9, 1, 1 }, { 7, 0, 1 }, { 5, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "10s", "Access Granted (Protoje 2013)", 4, { { 7, 1, 1 }, { 0, 1, 1 }, { 7, 1, 1 }, { 5, 0, 1 } } },
    { "NEW ROOTS - ANOS 2000", "10s", "Smile Jamaica (Chronixx 2014)", 4, { { 7, 0, 1 }, { 0, 0, 1 }, { 4, 1, 1 }, { 2, 0, 1 } } },
};
}
