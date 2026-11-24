// Monstro Machine MM-2500 — motor sem JUCE: kits do Monstro.zip, caráter de cada kit, variações dos timbres
// (KICK 01, KICK 02… do mesmo produtor), grooves por estilo, síntese dos 16 pads, saturador e tape echo.
// A versão para celular (site/monstro/app/) tem a mesma síntese em JavaScript (play, charOf, vpar, PATTERNS):
// ao mudar um, mude o outro (mesmo gerador aleatório: o mesmo kit e a mesma variação soam iguais).
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace mm
{
constexpr int numPads = 16, numSteps = 16, numSeqs = 4;
constexpr double PI = 3.14159265358979323846;

struct Kit { int vol; const char* name; const char* style; int bpm; const char* credits; const char* bio; const char* contents; };
inline const std::vector<Kit>& kits()
{
    static const std::vector<Kit> k {
    { 1, "DubMastor", "reggae", 80, "Adonai, Cidade Verde Sounds, Costa Gold, DJ Cia, Cacife Clandestino", "Apresentado pela Academia como o monstro do reggae e mestre do DJ Coala. O kit mistura timbres modernos com os sons das próprias produções de reggae dele.", "57 drum fills|31 tons|5 cymbals|78 hihats|81 kicks|45 percs|3 shakers|338 snares|40 efeitos" },
    { 1, "Nox", "trap", 140, "Matuê (DJ), Predella, Tati Zaqui, Welisson, Kant, Bivolt, Costa Gold", "Produtor e DJ do Matuê. O kit vem com bônus de loops de bateria e melodias.", "34 808s|6 claps|19 cymbals|11 hihats|10 kicks|12 percs|11 vocal chops|8 snares|19 efeitos" },
    { 1, "Neo Beats", "trap", 138, "MC Poze do Rodo, Xamã, Haikaiss, Borges, Agnes Nunes, Sidoka, Krawk", "Kit com timbres usados nas próprias produções dele.", "8 808s|10 claps|11 cymbals|10 hihats|10 kicks|10 snares|20 efeitos" },
    { 1, "Ecologyk", "trap", 140, "Matuê, Haikaiss, Costa Gold, Konai, Diego Thug, MC Guime, Negra Li, Jade Baraldo", "Paulo Henrique Castro, DJ e produtor na cena brasileira desde 2009, com milhões de streams no Spotify.", "" },
    { 1, "Pedro Lotto", "trap", 142, "Veigh, Vulgo FK, MC PH, Orochi", "Produtor de trap com créditos em faixas como \"Ballena\" (Vulgo FK, MC PH, Veigh) e \"Mesma História\" (Orochi).", "" },
    { 1, "Skeeter", "trap", 140, "", "Kit listado na coleção Monstro da Beatmaker Store.", "" },
    { 2, "André Nine", "funk", 130, "MC Hariel, Kawe, Costa Gold, MC Ryan SP", "Seleção de timbres usados nas produções dele, com bônus misterioso.", "13 808s|12 claps|7 cymbals|26 hihats|12 kicks|16 percs|15 snares" },
    { 2, "DogDu Beats", "trap", 140, "Sidoka, Zemaru, Chris MC", "Timbres das produções dele, com efeitos de vox exclusivos de bônus.", "7 808s|7 claps|4 cymbals|7 hihats|8 kicks|8 percs|7 snares|FX de vox" },
    { 2, "DJ Coala", "boombap", 90, "Sidoka, RZO, Qualy, Tribo da Periferia", "Produtor da escola rap/boom bap, aluno do DubMastor. Kit com bônus de efeitos de voz e FX.", "12 808s + 5 basses|11 claps|7 cymbals|10 hihats|10 kicks|15 percs|12 snares" },
    { 2, "Caio Passos", "funk", 130, "MC Hariel, Yunk Vino, Hyperanhas, MC Daniel, MC Pedrinho, Felp 22", "Professor da Academia de Beats. O kit tem baterias de funk, vox e loops exclusivos.", "9 808s|4 claps|6 cymbals|6 hihats|6 kicks|21 percs|2 rimshots|19 efeitos de voz|4 snares" },
    { 2, "Chiocki", "trap", 145, "Kant, NOG, Spinard", "Professor da Academia de Beats. São 145 timbres selecionados, incluindo chants, drum fills, open hats, shakers e snaps.", "11 808s|11 chants|10 claps|10 cymbals|10 drum fills|10 FX|10 hihats|10 kicks|10 open hats" },
    { 2, "Robert Beats", "trap", 140, "", "Kit do Vol. 2. Há pouca informação pública sobre os créditos dele.", "" },
    { 2, "DJ Caíque", "funk", 130, "", "Kit do Vol. 2. Há pouca informação pública sobre os créditos dele.", "" },
    { 3, "Chris Beats", "trap", 140, "Filipe Ret, L7nnon, MC Cabelinho, MC Maneirinho, Hyperanhas, Tz da Coronel", "Bônus com os timbres usados em \"Corte Americano\" (Filipe Ret e L7nnon).", "12 808s|3 basses|11 claps|2 cymbals|7 hihats|4 kicks|6 one shots|6 percs" },
    { 3, "Portugal No Beat", "trap", 142, "Filipe Ret, Jovem Dex, Greg Ferreira, Gaab", "Pack de timbres das produções dele.", "8 808s|8 claps|8 cymbals|8 hihats|6 kicks|8 percs|4 rimshots|10 snares" },
    { 3, "Kouth", "trap", 140, "Derek, UCLÃ, Lil Ivy, Denov, MC Igu, VK Mac, Kweller", "Beatmaker com kit de 8 timbres por categoria.", "8 808s|8 claps|8 cymbals|8 hihats|8 kicks|8 percs|8 rimshots|8 snares|8 efeitos" },
    { 3, "Jess", "trap", 140, "Orochi, Bin, PL Quest, Borges", "Pack compacto com basses de ambiência.", "4 808s|4 basses|5 claps|2 cymbals|5 hihats|5 kicks|5 rimshots|5 snares" },
    { 3, "Deekapz", "funk", 130, "festivais: Primavera Sound, Lollapalooza, The Town", "Dupla de produtores (Matheus Henrique e Paulo Vitor) que mistura funk, trap e música brasileira. Criaram a festa gratuita Deekapz 0800 e dão aula na Academia.", "" },
    { 3, "Blakbone", "drill", 142, "", "Professor da Academia de Beats. Há pouca informação pública sobre os créditos dele.", "" },
    { 3, "DJ Coala Vol.3", "boombap", 92, "Sidoka, RZO, Qualy, Tribo da Periferia", "Segundo kit do DJ Coala na série.", "" },
    { 3, "Dallass", "trap", 140, "", "Kit do Vol. 3. Há pouca informação pública sobre os créditos dele.", "" },
    { 3, "Celo", "trap", 140, "Leozin", "Kit do Vol. 3. Aparece como Celo1st em faixas do Leozin.", "" },
    { 3, "Peunubeat", "trap", 140, "", "Kit do Vol. 3. Há pouca informação pública sobre os créditos dele.", "" },
    { 3, "Tibery", "trap", 140, "", "Kit do Vol. 3. Há pouca informação pública sobre os créditos dele.", "" },
    { 4, "Nagalli", "trap", 140, "L7nnon, Veigh, MC Cabelinho, Kayblack, Vulgo FK", "André Nagalli de Oliveira, de Piracicaba (SP). Sócio-fundador e diretor musical da Supernova Entertainment. Lançou o álbum \"Magic Show\" em 2024 e é apontado como o produtor de rap mais ouvido do Brasil. Também dá aula na Academia.", "" },
    { 4, "Neckklace", "trap", 140, "Nagalli (Magic Show), G.A, MC Luanna, Niink", "Coprodutor de \"Serenata\", do álbum \"Magic Show\" do Nagalli.", "" },
    { 4, "DJ Oreia", "funk", 130, "", "DJ e produtor, professor da Academia de Beats.", "" },
    { 4, "Kotim", "funk", 130, "Mc Don Juan", "Artista e produtor de \"Quem vai patrocinar?\", com participação em \"Sereia me perdoa\" (Mc Don Juan).", "" },
    { 4, "DJ Win", "funk", 130, "", "Kit do Vol. 4. Há pouca informação pública sobre os créditos dele.", "" },
    { 4, "Cheek", "drill", 142, "", "Kit do Vol. 4. Há pouca informação pública sobre os créditos dele.", "" },
    { 4, "DJ Coala Vol.4", "boombap", 90, "Sidoka, RZO, Qualy", "Terceiro kit do DJ Coala na série.", "" },
    { 4, "Attlanta", "trap", 140, "", "Produtora e professora da Academia de Beats. Foi destaque na série \"O Beat Delas\" da Rolling Stone Brasil, sobre mulheres produtoras.", "" },
    { 4, "Fepache", "trap", 140, "Vulgo FK, MC PH, Veigh", "Produtor com crédito em \"Ballena\".", "" },
    };
    return k;
}
inline int numKits() { return (int) kits().size(); }

inline const char* styleName (const std::string& s)
{
    if (s == "reggae") return "Reggae Steppers";
    if (s == "trap") return "Trap";
    if (s == "boombap") return "Boom Bap";
    if (s == "funk") return "Funk";
    return "Drill";
}
inline const std::array<const char*, 5>& styleKeys() { static const std::array<const char*, 5> s { "reggae", "trap", "boombap", "funk", "drill" }; return s; }

inline const std::array<const char*, numPads>& padNames()
{ static const std::array<const char*, numPads> p { "KICK", "KICK 2", "SNARE", "CLAP", "RIM", "HH FECH", "HH ABER", "CRASH", "PERC 1", "PERC 2", "PERC 3", "SHAKER", "808", "808 SLIDE", "FX", "VOX" }; return p; }
inline const std::array<int, numPads>& midiNotes()
{ static const std::array<int, numPads> n { 36, 35, 38, 39, 37, 42, 46, 49, 63, 64, 62, 70, 34, 33, 55, 54 }; return n; }

// família de cada pad: o ◀ ▶ do TIMBRE troca só dentro da família, no kit do mesmo produtor
enum Fam { KICK, SNARE, CLAP, RIM, HIHAT, OPENHAT, CYMBAL, PERC, SHAKER, B808, FX, VOX, numFams };
inline const std::array<const char*, numFams>& famNames()
{ static const std::array<const char*, numFams> f { "KICK", "SNARE", "CLAP", "RIM", "HI-HAT", "OPEN HAT", "CYMBAL", "PERC", "SHAKER", "808", "FX", "VOX" }; return f; }
inline const std::array<int, numPads>& padFam()
{ static const std::array<int, numPads> f { KICK, KICK, SNARE, CLAP, RIM, HIHAT, OPENHAT, CYMBAL, PERC, PERC, PERC, SHAKER, B808, B808, FX, VOX }; return f; }

inline std::string lower (std::string s) { for (auto& c : s) c = (char) std::tolower ((unsigned char) c); return s; }
inline bool has (const std::string& s, const char* w) { return s.find (w) != std::string::npos; }
// mesma ordem de regras da versão web (FAMRE)
inline int famOfContent (const std::string& raw)
{
    const auto s = lower (raw);
    if (has (s, "vox") || has (s, "vocal") || has (s, "chant") || has (s, "voz")) return VOX;
    if (has (s, "open hat")) return OPENHAT;
    if (has (s, "kick")) return KICK;
    if (has (s, "snare")) return SNARE;
    if (has (s, "clap") || has (s, "snap")) return CLAP;
    if (has (s, "rim")) return RIM;
    if (has (s, "hihat") || has (s, "hi-hat") || has (s, " hat")) return HIHAT;
    if (has (s, "cymbal") || has (s, "crash") || has (s, "ride")) return CYMBAL;
    if (has (s, "shaker")) return SHAKER;
    if (has (s, "808") || has (s, "bass")) return B808;
    if (has (s, "perc") || has (s, "tons") || has (s, "one shot") || has (s, "drum fill")) return PERC;
    if (has (s, "efeito") || has (s, "fx")) return FX;
    return -1;
}
// quantos timbres de cada família o kit traz (8 quando o kit não publica a lista)
inline int famCount (int kit, int fam)
{
    const std::string c = kits()[(size_t) kit].contents;
    int n = 0; size_t p = 0;
    while (p <= c.size())
    {
        const size_t q = c.find ('|', p);
        const std::string item = c.substr (p, q == std::string::npos ? std::string::npos : q - p);
        if (! item.empty() && famOfContent (item) == fam) n += std::atoi (item.c_str());
        if (q == std::string::npos) break;
        p = q + 1;
    }
    return n > 0 ? n : 8;
}

// ---------------------------------------------------------------- aleatório igual ao da web
// FNV-1a sobre as unidades UTF-16 (como o hash da web); continua de h para juntar pedaços sem alocar
inline uint32_t hashCont (uint32_t h, const char* utf8)
{
    const size_t len = std::strlen (utf8);
    for (size_t i = 0; i < len;)
    {
        uint32_t cp = (unsigned char) utf8[i]; int n = 1;
        if (cp >= 0xF0) { cp &= 0x07; n = 4; } else if (cp >= 0xE0) { cp &= 0x0F; n = 3; } else if (cp >= 0xC0) { cp &= 0x1F; n = 2; }
        for (int k = 1; k < n && i + (size_t) k < len; ++k) cp = (cp << 6) | ((unsigned char) utf8[i + (size_t) k] & 0x3F);
        i += (size_t) n;
        const uint32_t unit = cp > 0xFFFF ? (0xD800 + ((cp - 0x10000) >> 10)) : cp;   // charCodeAt(0)
        h = (h ^ unit) * 16777619u;
    }
    return h;
}
inline uint32_t hashStr (const std::string& utf8) { return hashCont (2166136261u, utf8.c_str()); }
struct Rng
{
    int32_t seed;
    explicit Rng (uint32_t s) : seed ((int32_t) s) {}
    double operator()()
    {
        const uint32_t s = (uint32_t) seed;
        const uint32_t m = (s ^ (s >> 15)) * (s | 1u);
        seed = (int32_t) (m + 0x6D2B79F5u);
        const uint32_t t = (uint32_t) seed;
        return (double) ((t ^ (t >> 7)) % 10000u) / 10000.0;
    }
};

struct Char { double kStart, kEnd, kDec, kClick, bDec, bDrive, sTone, sNoise, sDec, sBand, hp, ohDec, perc, vox, fx; int bass; };
inline Char charOf (int kit)
{
    const auto& k = kits()[(size_t) kit];
    Rng r (hashStr (k.name));
    const std::string s = k.style;   // nomes curtos: sem alocação
    Char c {};
    c.kStart = s == "reggae" ? 120 + r() * 40 : 150 + r() * 90;
    c.kEnd = s == "boombap" ? 48 + r() * 10 : 38 + r() * 14;
    c.kDec = s == "reggae" ? .35 + r() * .2 : s == "boombap" ? .28 + r() * .15 : .45 + r() * .3;
    c.kClick = .2 + r() * .6;
    static const int bass[6] { 31, 33, 34, 36, 29, 28 };
    c.bass = bass[(int) std::floor (r() * 6)];
    c.bDec = s == "drill" ? 1.6 : .7 + r() * 1.1;
    c.bDrive = s == "reggae" ? .1 : .2 + r() * .8;
    c.sTone = 160 + r() * 120; c.sNoise = .6 + r() * .5;
    c.sDec = s == "reggae" ? .12 + r() * .08 : .16 + r() * .12;
    c.sBand = 1500 + r() * 3000; c.hp = 6000 + r() * 3500; c.ohDec = .25 + r() * .35;
    static const double perc[5] { 220, 280, 330, 400, 180 }, vox[5] { 330, 392, 440, 494, 262 };
    c.perc = perc[(int) std::floor (r() * 5)]; c.vox = vox[(int) std::floor (r() * 5)]; c.fx = r();
    return c;
}
struct Var { double t = 0, d = 1, b = 1; };
inline Var varOf (int kit, int pad, int v)
{
    if (v <= 0) return {};
    char num[16]; std::snprintf (num, sizeof num, "|%d", v);
    uint32_t h = hashCont (2166136261u, kits()[(size_t) kit].name);
    h = hashCont (hashCont (hashCont (h, "|"), famNames()[(size_t) padFam()[(size_t) pad]]), num);
    Rng r (h);
    Var x; x.t = (r() * 2 - 1) * 3; x.d = .65 + r() * .8; x.b = .55 + r() * 1; return x;
}

// ---------------------------------------------------------------- grooves por estilo (4 SEQs; 16 passos = 1 compasso 4/4)
using Seq = std::array<std::array<uint8_t, numSteps>, numPads>;
inline Seq P (std::initializer_list<std::pair<int, std::initializer_list<int>>> o)
{
    Seq g {};
    for (auto& [pad, steps] : o) for (int s : steps) g[(size_t) pad][(size_t) std::abs (s == -100 ? 0 : s)] = (uint8_t) (s < 0 ? 2 : 1);
    return g;
}
// (-100 = acento no passo 0; a web usa -0)
inline const std::array<Seq, numSeqs>& patterns (const std::string& style)
{
    static const std::array<Seq, numSeqs> reggae {
        P ({ { 0, { 0, 4, 8, 12 } }, { 4, { -4, -12 } }, { 5, { 2, 6, 10, 14 } }, { 11, { 1, 3, 5, 7, 9, 11, 13, 15 } }, { 12, { 0, 7, 8, 14 } } }),
        P ({ { 0, { 0, 8 } }, { 2, { 4, -12 } }, { 5, { 2, 6, 10, 14 } }, { 6, { 14 } }, { 11, { 1, 3, 5, 7, 9, 11, 13, 15 } }, { 12, { 0, 3, 8, 11 } } }),
        P ({ { 0, { 0, 4, 8, 12 } }, { 2, { -12 } }, { 4, { 4 } }, { 5, { 0, 2, 4, 6, 8, 10, 12, 14 } }, { 8, { 7, 15 } }, { 12, { 0, 6, 8 } } }),
        P ({ { 0, { 0, 8 } }, { 4, { 4, 12 } }, { 5, { 2, 6, 10, 14 } }, { 7, { 0 } }, { 14, { 15 } }, { 12, { 0, 10 } } }) };
    static const std::array<Seq, numSeqs> trap {
        P ({ { 0, { 0, 10 } }, { 3, { -8 } }, { 5, { 0, 2, 4, 6, 8, 10, 11, 12, 14, 15 } }, { 6, { 6 } }, { 12, { 0, 10 } }, { 15, { 15 } } }),
        P ({ { 0, { 0, 3, 10 } }, { 3, { -8 } }, { 5, { 0, 2, 4, 5, 6, 8, 10, 12, 13, 14, 15 } }, { 8, { 7 } }, { 13, { 0 } }, { 12, { 10 } } }),
        P ({ { 0, { 0, 6, 10 } }, { 2, { 8 } }, { 3, { -8 } }, { 5, { 0, 1, 2, 4, 6, 8, 9, 10, 12, 14 } }, { 12, { 0, 6, 10 } }, { 14, { 15 } } }),
        P ({ { 0, { 0 } }, { 3, { -8 } }, { 7, { 0 } }, { 5, { 0, 4, 8, 12 } }, { 12, { 0 } }, { 14, { 0 } } }) };
    static const std::array<Seq, numSeqs> boombap {
        P ({ { 0, { 0, 7, 10 } }, { 2, { -4, -12 } }, { 5, { 0, 2, 4, 6, 8, 10, 12, 14 } }, { 6, { 14 } }, { 12, { 0, 10 } } }),
        P ({ { 0, { 0, 3, 10 } }, { 2, { 4, 12 } }, { 5, { 0, 2, 4, 6, 8, 10, 12, 14 } }, { 4, { 15 } }, { 12, { 0, 3, 10 } } }),
        P ({ { 0, { 0, 7, 9 } }, { 2, { -4, -12 } }, { 5, { 0, 2, 4, 6, 8, 10, 12, 14 } }, { 15, { 6 } }, { 12, { 0 } } }),
        P ({ { 0, { 0, 10 } }, { 2, { 4, 12 } }, { 5, { 2, 6, 10, 14 } }, { 7, { 0 } }, { 12, { 0 } } }) };
    static const std::array<Seq, numSeqs> funk {
        P ({ { 0, { 0, 3, 8, 11 } }, { 3, { -4, -12 } }, { 8, { 0, 3, 6, 8, 11, 14 } }, { 5, { 2, 6, 10, 14 } }, { 12, { 0, 8 } }, { 15, { 6 } } }),
        P ({ { 0, { 0, 3, 6, 8, 11, 14 } }, { 3, { 4, 12 } }, { 9, { 2, 10 } }, { 5, { 0, 2, 4, 6, 8, 10, 12, 14 } }, { 12, { 0, 6 } } }),
        P ({ { 0, { 0, 6, 8, 14 } }, { 3, { -4, -12 } }, { 8, { 3, 11 } }, { 10, { 2, 5, 10, 13 } }, { 15, { 12 } }, { 12, { 0, 8 } } }),
        P ({ { 0, { 0, 8 } }, { 3, { 4, 12 } }, { 8, { 3, 6, 11, 14 } }, { 14, { 15 } } }) };
    static const std::array<Seq, numSeqs> drill {
        P ({ { 0, { 0, 11 } }, { 2, { -6 } }, { 3, { 14 } }, { 5, { 0, 3, 4, 7, 8, 10, 12, 15 } }, { 13, { 0, 11 } }, { 4, { 3 } } }),
        P ({ { 0, { 0, 7, 11 } }, { 2, { 6, 14 } }, { 5, { 0, 2, 3, 4, 6, 8, 10, 11, 12, 14 } }, { 13, { 0 } }, { 12, { 7, 11 } } }),
        P ({ { 0, { 0, 11 } }, { 2, { 6 } }, { 3, { 14 } }, { 5, { 0, 3, 6, 8, 11, 14 } }, { 9, { 9 } }, { 13, { 0, 11 } } }),
        P ({ { 0, { 0 } }, { 2, { 6, 14 } }, { 7, { 0 } }, { 13, { 0 } } }) };
    if (style == "reggae") return reggae;
    if (style == "trap") return trap;
    if (style == "boombap") return boombap;
    if (style == "funk") return funk;
    return drill;
}

// ---------------------------------------------------------------- DSP
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    enum Type { LP, HP, BP, HS };
    void set (Type t, double f, double q, double sr, double gainDb = 0)
    {
        f = std::min (f, sr * 0.45); f = std::max (f, 10.0);
        const double w = 2 * PI * f / sr, cw = std::cos (w), sw = std::sin (w), al = sw / (2 * std::max (q, 0.0001));
        double B0, B1, B2, A0, A1, A2;
        if (t == LP) { B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }
        else if (t == HP) { B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = B0; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }
        else if (t == BP) { B0 = al; B1 = 0; B2 = -al; A0 = 1 + al; A1 = -2 * cw; A2 = 1 - al; }
        else
        {   // high shelf (como o da Web Audio, inclinação 1)
            const double A = std::pow (10.0, gainDb / 40.0), s2 = 2 * std::sqrt (A) * (sw / 2 * std::sqrt ((A + 1 / A) * (1 / 1.0 - 1) + 2));
            B0 = A * ((A + 1) + (A - 1) * cw + s2); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - s2);
            A0 = (A + 1) - (A - 1) * cw + s2; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - s2;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    double tick (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};

// um gerador de um pad: oscilador ou ruído, com varredura, filtro(s), envelope e atraso
struct Gen
{
    enum Src { SINE, TRI, SQUARE, SAW, NOISE } src = SINE;
    double f0 = 440, f1 = 0, sweep = 0; bool linSweep = false;
    double peak = 1, att = .002, dec = .1, delay = 0, stopAt = 1e9; bool linAtt = false;
    int nFilt = 0; Biquad flt[3]; bool parallel = false;
    double drive = 0;   // 808: waveshaper tanh
    double phase = 0;
};

struct Voice
{
    bool active = false; int pad = -1; double t = 0, len = 0; float gainL = 1, gainR = 1;
    std::array<Gen, 6> g; int n = 0; Biquad shelf; bool useShelf = false;
    const std::vector<float>* smpL = nullptr; const std::vector<float>* smpR = nullptr; std::shared_ptr<const void> hold;
    double smpPos = 0, smpRate = 1, smpRel = 1e9;
};

struct SampleData { std::vector<float> l, r; double rate = 48000; };

class Synth
{
public:
    void prepare (double sampleRate) { sr = sampleRate; for (auto& v : voices) v.active = false; }

    // dispara o pad (som do kit ou a amostra do usuário)
    void hit (int pad, int kit, const Char& c, const Var& vp, double vel, double tuneSemi, double decPct,
              std::shared_ptr<const SampleData> smp)
    {
        Voice& v = freeVoice();
        v = Voice(); v.active = true; v.pad = pad; v.t = 0;
        v.gainL = v.gainR = (float) vel;
        const double tune = std::pow (2.0, (tuneSemi + (smp ? 0 : vp.t)) / 12.0), dk = decPct / 100.0 * (smp ? 1.0 : vp.d);
        if (! smp && std::abs (vp.b - 1) > 1e-6) { v.useShelf = true; v.shelf.set (Biquad::HS, 3000, .707, sr, (vp.b - 1) * 12); }
        if (smp)
        {
            v.hold = smp; v.smpL = &smp->l; v.smpR = smp->r.empty() ? &smp->l : &smp->r;
            v.smpRate = tune * smp->rate / sr;
            const double dur = (double) smp->l.size() / smp->rate;
            v.smpRel = dur * dk * .9; v.len = std::min (dur / tune, v.smpRel + .3);
            return;
        }
        (void) kit;
        auto tone = [&] (Gen::Src s, double f0, double f1, double sweep, double peak, double dec) -> Gen&
        { Gen& x = v.g[(size_t) v.n++]; x = Gen(); x.src = s; x.f0 = f0; x.f1 = f1; x.sweep = sweep; x.peak = peak; x.att = .002; x.dec = dec; x.stopAt = dec + .05; return x; };
        auto noise = [&] (double dur, Biquad::Type ty, double f, double q, double peak, double dec, double delay = 0) -> Gen&
        { Gen& x = v.g[(size_t) v.n++]; x = Gen(); x.src = Gen::NOISE; x.peak = peak; x.att = .001; x.dec = dec; x.delay = delay; x.stopAt = dur;
          x.nFilt = 1; x.flt[0].set (ty, f, q, sr); return x; };
        switch (pad)
        {
            case 0: tone (Gen::SINE, c.kStart * tune, c.kEnd * tune, .07, 1, c.kDec * dk); noise (.02, Biquad::HP, 3000, .7, c.kClick * .5, .015); break;
            case 1: tone (Gen::SINE, c.kStart * 1.2 * tune, c.kEnd * 1.3 * tune, .04, .9, c.kDec * .6 * dk); tone (Gen::TRI, 900, 120, .02, .4, .03); break;
            case 2: tone (Gen::TRI, c.sTone * tune, c.sTone * .8 * tune, .05, .6, .1 * dk); noise (.5, Biquad::BP, c.sBand, .8, c.sNoise, c.sDec * dk); break;
            case 3: for (double d : { 0.0, .011, .022 }) noise (.05 + d, Biquad::BP, 1200 * tune, 1.2, .9, .03, d);
                    noise (.43, Biquad::BP, 1100 * tune, 1, .7, .16 * dk, .03); break;
            case 4: tone (Gen::SQUARE, 1650 * tune, 0, 0, .3, .035 * dk); tone (Gen::TRI, 470 * tune, 0, 0, .4, .05 * dk); break;
            case 5: noise (.1, Biquad::HP, c.hp * tune, .7, .45, .035 * dk); break;
            case 6: noise (1, Biquad::HP, c.hp * .9 * tune, .7, .4, c.ohDec * dk); break;
            case 7: noise (2, Biquad::HP, 4500 * tune, .5, .45, 1.2 * dk); for (double m : { 1.0, 1.48, 1.93 }) tone (Gen::SQUARE, 540 * m * tune, 0, 0, .04, 1 * dk); break;
            case 8: tone (Gen::SINE, c.perc * tune, c.perc * .85 * tune, .05, .7, .18 * dk); break;
            case 9: tone (Gen::SINE, c.perc * 1.5 * tune, c.perc * 1.3 * tune, .04, .6, .12 * dk); noise (.02, Biquad::BP, 2500, 1, .2, .01); break;
            case 10: tone (Gen::TRI, c.perc * 3 * tune, 0, 0, .5, .05 * dk); break;
            case 11: { auto& x = noise (.2, Biquad::BP, 7000 * tune, 1.5, .35, .07 * dk); x.att = .02; x.linAtt = true; break; }
            case 12: case 13:
            {
                const double f = 440.0 * std::pow (2.0, (c.bass - 69) / 12.0) * tune;
                auto& x = tone (Gen::SINE, pad == 13 ? f * 2 : f * 1.5, f, pad == 13 ? .12 : .02, .9, c.bDec * dk);
                x.att = .005; x.dec = c.bDec * dk - .005; x.drive = c.bDrive; x.stopAt = c.bDec * dk + .05;
                break;
            }
            case 14:
                if (c.fx < .5) tone (Gen::SAW, 2400 * tune, 80, .6, .25, .6 * dk);
                else noise (1, Biquad::BP, 800 * tune, 4, .5, .7 * dk);
                break;
            default:
            {
                auto& x = tone (Gen::SAW, c.vox * tune, c.vox * .94 * tune, .2, .5, .22 * dk);
                x.linSweep = true; x.att = .01; x.stopAt = .4 * dk; x.nFilt = 3; x.parallel = true;
                const double ff[3] { 700, 1150, 2600 };
                for (int k = 0; k < 3; ++k) x.flt[k].set (Biquad::BP, ff[k], 8, sr);
                break;
            }
        }
        v.len = 0;
        for (int i = 0; i < v.n; ++i) v.len = std::max (v.len, v.g[(size_t) i].delay + std::min (v.g[(size_t) i].stopAt, v.g[(size_t) i].att + v.g[(size_t) i].dec + .02));
    }

    void render (float* L, float* R, int n)
    {
        const double dt = 1.0 / sr;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (int i = 0; i < n; ++i)
            {
                double x = 0;
                if (v.smpL)
                {
                    const size_t p = (size_t) v.smpPos;
                    if (p + 1 >= v.smpL->size()) { v.active = false; break; }
                    const double fr = v.smpPos - (double) p;
                    double env = v.t < v.smpRel ? 1.0 : std::exp (-(v.t - v.smpRel) / .03);
                    const double l = ((*v.smpL)[p] * (1 - fr) + (*v.smpL)[p + 1] * fr) * env;
                    const double r = ((*v.smpR)[p] * (1 - fr) + (*v.smpR)[p + 1] * fr) * env;
                    v.smpPos += v.smpRate;
                    L[i] += (float) (l * v.gainL); R[i] += (float) (r * v.gainR);
                    v.t += dt; if (v.t > v.len) { v.active = false; break; }
                    continue;
                }
                for (int k = 0; k < v.n; ++k) x += genTick (v.g[(size_t) k], v.t);
                if (v.useShelf) x = v.shelf.tick (x);
                L[i] += (float) (x * v.gainL); R[i] += (float) (x * v.gainR);
                v.t += dt;
                if (v.t > v.len) { v.active = false; break; }
            }
        }
    }

private:
    double genTick (Gen& x, double tAll)
    {
        const double t = tAll - x.delay;
        if (t < 0 || t > x.stopAt) return 0;
        // envelope (rampas exponenciais como na Web Audio)
        double e;
        const double lo = .0001;
        if (t < x.att) e = x.linAtt ? lo + (x.peak - lo) * t / x.att : x.peak * std::pow (lo / x.peak, 1 - t / x.att);
        else { const double u = (t - x.att) / std::max (1e-4, x.dec); if (u >= 1) return 0; e = x.peak * std::pow (lo / x.peak, u); }
        double s;
        if (x.src == Gen::NOISE) s = noise();
        else
        {
            double f = x.f0;
            if (x.f1 > 0 && x.sweep > 0)
            {
                const double u = std::min (1.0, t / x.sweep);
                f = x.linSweep ? x.f0 + (x.f1 - x.f0) * u : x.f0 * std::pow (x.f1 / x.f0, u);
            }
            x.phase += f / sr; x.phase -= std::floor (x.phase);
            const double p = x.phase;
            switch (x.src)
            {
                case Gen::SINE: s = std::sin (2 * PI * p); break;
                case Gen::TRI: s = 1 - 4 * std::abs (p - .5); break;
                case Gen::SQUARE: s = p < .5 ? 1 : -1; break;
                case Gen::SAW: case Gen::NOISE: default: s = 2 * p - 1; break;
            }
        }
        if (x.nFilt > 0)
        {
            if (x.parallel) { double a = 0; for (int k = 0; k < x.nFilt; ++k) a += x.flt[k].tick (s); s = a; }
            else for (int k = 0; k < x.nFilt; ++k) s = x.flt[k].tick (s);
        }
        if (x.drive > 0) { const double k = x.drive * 40 + 1; s = std::tanh (k * s) / std::tanh (k); }
        return s * e;
    }
    double noise() { ns = ns * 1664525u + 1013904223u; return (double) (ns >> 8) / 8388608.0 - 1.0; }
    Voice& freeVoice()
    {
        for (auto& v : voices) if (! v.active) return v;
        Voice* o = &voices[0];
        for (auto& v : voices) if (v.t > o->t) o = &v;
        return *o;
    }
    double sr = 48000; uint32_t ns = 22222u;
    std::array<Voice, 48> voices;
};

// ---------------------------------------------------------------- saturador + tape echo + saída (padrão de fábrica: bypass)
struct FxSettings { bool satOn = false; double drive = .35; bool echoOn = false; int echoSteps = 3; double echoFb = .45, echoMix = .3, master = .8, stepDur = .1; };
class Fx
{
public:
    void prepare (double s) { sr = s; buf.assign ((size_t) (sr * 3.2) * 2, 0.f); w = 0; lpL.set (Biquad::LP, 3200, .707, sr); lpR.set (Biquad::LP, 3200, .707, sr); env = 0; }
    void process (float* L, float* R, int n, const FxSettings& f)
    {
        const double k = f.drive * 40 + 1, nk = std::tanh (k);
        const size_t size = buf.size() / 2;
        const size_t d = (size_t) std::clamp (f.stepDur * f.echoSteps * sr, 1.0, (double) size - 1);
        for (int i = 0; i < n; ++i)
        {
            double l = L[i] * f.master, r = R[i] * f.master;
            if (f.satOn) { l = std::tanh (k * l) / nk; r = std::tanh (k * r) / nk; }
            const size_t rd = (w + size - d) % size;
            const double el = lpL.tick (buf[rd * 2]), er = lpR.tick (buf[rd * 2 + 1]);
            buf[w * 2] = (float) ((f.echoOn ? l : 0) + el * f.echoFb);
            buf[w * 2 + 1] = (float) ((f.echoOn ? r : 0) + er * f.echoFb);
            w = (w + 1) % size;
            if (f.echoOn) { l += el * f.echoMix; r += er * f.echoMix; }
            // compressor de saída (−10 dB, 4:1) como o da versão web
            const double pk = std::max (std::abs (l), std::abs (r));
            env = pk > env ? pk : env * 0.9995;
            const double thr = 0.316, g = env > thr ? std::pow (env / thr, 1.0 / 4.0 - 1.0) : 1.0;
            L[i] = (float) (l * g); R[i] = (float) (r * g);
        }
    }
private:
    double sr = 48000, env = 0; std::vector<float> buf; size_t w = 0; Biquad lpL, lpR;
};
} // namespace mm
