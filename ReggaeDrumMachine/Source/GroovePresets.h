// Gerado por tools/make_grooves.py a partir de tools/grooves.json — não editar à mão.
// Presets de groove do MRDM-12: músicas de cada baterista (visor azul do módulo BATERISTA).
#pragma once

namespace dubdrums
{
struct GroovePreset
{
    int drummer; const char* song; const char* artist; int year; int bpm; bool approx;
    int style, kick, snare, backbeat, hat, swing; bool swingAll; int dyn;
    int rndKick, rndSnare, rndHat, rndPerc; bool fill, intro; int kit, bars, seed;
    const char* note;   // como é o groove (aparece gravado embaixo do visor)
};

inline constexpr int numGroovePresets = 21;
inline const GroovePreset groovePresets[numGroovePresets] = {
    { 0, "Night Nurse", "Gregory Isaacs", 1982, 74, false, 0, 17, 68, 1, 0, 1, true, 1, 10, 15, 20, 15, true, false, 0, 4, 1982, "ARO NO CONTRATEMPO, BUMBO 1 E 3" },
    { 0, "Rock 'n' Groove", "Bunny Wailer", 1981, 72, true, 0, 17, 68, 0, 1, 0, true, 3, 10, 15, 25, 20, true, false, 0, 4, 1981, "RUB A DUB: CAIXA SECA, HI-HAT 1/16" },
    { 0, "Looking My Love", "Barrington Levy", 1979, 76, true, 0, 17, 68, 2, 0, 1, true, 1, 10, 15, 20, 15, false, false, 0, 4, 1979, "CAIXA E ARO JUNTOS NO 2 E NO 4" },
    { 0, "Big Ship", "Freddie McGregor", 1982, 78, true, 0, 17, 68, 1, 2, 1, true, 5, 10, 15, 20, 15, true, false, 0, 4, 1182, "HI-HAT NO CONTRATEMPO, ARO" },
    { 0, "Man of Mystery", "Dub Syndicate", 1984, 70, true, 0, 17, 68, 0, 0, 0, true, 5, 5, 10, 20, 25, true, false, 0, 4, 1984, "BUMBO 1 E 3, CAIXA 2 E 4 (DUB)" },
    { 1, "None Shall Escape the Judgement", "Johnny Clarke", 1974, 100, false, 0, 85, 68, 0, 2, 1, false, 2, 15, 20, 25, 25, true, false, 0, 4, 1974, "FLYING CYMBAL: HI-HAT ABERTO NO E" },
    { 1, "Every Knee Shall Bow", "Johnny Clarke", 1975, 96, true, 0, 85, 68, 0, 2, 2, false, 3, 15, 20, 25, 25, false, false, 0, 4, 1975, "FLYING CYMBAL COM BUMBO CORRIDO" },
    { 1, "Stalag 17", "Ansell Collins", 1973, 78, false, 1, 68, 68, 0, 0, 2, true, 1, 15, 25, 30, 25, true, false, 0, 4, 1973, "ONE DROP CHEIO DE ENFEITES" },
    { 1, "Money in My Pocket", "Dennis Brown", 1972, 72, true, 1, 68, 68, 1, 0, 1, true, 5, 10, 20, 25, 20, true, false, 0, 4, 1972, "ARO NA BATIDA, VIRADAS DE FUSA" },
    { 2, "One Drop", "Bob Marley & The Wailers", 1979, 66, false, 1, 68, 68, 1, 0, 2, true, 5, 10, 15, 20, 15, true, false, 0, 4, 1979, "O 1 VAZIO, ARO NA BATIDA" },
    { 2, "Exodus", "Bob Marley & The Wailers", 1977, 67, false, 0, 85, 68, 0, 1, 1, true, 4, 10, 15, 20, 20, true, false, 0, 4, 1977, "BUMBO NOS QUATRO TEMPOS" },
    { 2, "Stir It Up", "Bob Marley & The Wailers", 1973, 74, false, 1, 68, 68, 1, 3, 4, true, 1, 10, 15, 20, 15, false, false, 0, 4, 1973, "TERCINA NO HI-HAT, ARO" },
    { 2, "Three Little Birds", "Bob Marley & The Wailers", 1977, 74, false, 1, 68, 68, 0, 0, 2, true, 0, 10, 15, 15, 10, true, false, 0, 4, 1377, "ONE DROP LEVE, CAIXA NA BATIDA" },
    { 2, "Is This Love", "Bob Marley & The Wailers", 1978, 61, false, 1, 68, 68, 1, 2, 1, true, 5, 10, 15, 20, 15, true, false, 0, 4, 1978, "HI-HAT NO CONTRATEMPO, ARO" },
    { 3, "Right Time", "The Mighty Diamonds", 1976, 72, true, 0, 17, 68, 1, 1, 0, true, 3, 10, 20, 25, 20, true, false, 0, 4, 1976, "ROCKERS: TOQUE DUPLO NO ARO" },
    { 3, "I Need a Roof", "The Mighty Diamonds", 1976, 74, true, 0, 25, 68, 0, 1, 0, true, 1, 10, 15, 25, 20, true, false, 0, 4, 1676, "BUMBO DOBRADO PUXANDO O 3" },
    { 3, "Shine Eye Gal", "Black Uhuru", 1979, 69, false, 0, 85, 68, 2, 1, 1, true, 3, 10, 15, 25, 20, true, false, 0, 4, 1979, "STEPPERS COM CAIXA E ARO" },
    { 3, "Sinsemilla", "Black Uhuru", 1980, 80, false, 0, 85, 68, 0, 1, 0, true, 2, 10, 15, 25, 20, true, false, 0, 4, 1980, "STEPPERS PESADO, SYNDRUM" },
    { 3, "Sponji Reggae", "Black Uhuru", 1981, 75, false, 0, 17, 68, 1, 1, 0, true, 3, 10, 15, 25, 20, true, false, 0, 4, 1981, "RUB A DUB COM SWISH NO HI-HAT" },
    { 4, "Under Mi Sleng Teng", "Wayne Smith", 1985, 82, false, 0, 17, 68, 0, 1, 0, true, 2, 0, 0, 0, 0, true, false, 1, 4, 1985, "CASIO MT-40: PRESET ROCK" },
    { 4, "Tempo", "Anthony Red Rose", 1985, 88, false, 0, 17, 68, 2, 1, 0, true, 2, 0, 0, 0, 0, true, false, 1, 4, 1986, "OBERHEIM DX: CAIXA COM ARO" },
};
} // namespace dubdrums
