// Monstro Machine MM-2500 — motor de som e dados dos kits (sem JUCE).
// A versão web (site/academiadebeats/monstromachine/index.html) tem a mesma síntese, os mesmos kits
// e os mesmos grooves em JavaScript (funções charOf, play e PATTERNS): ao mudar um, mude o outro.
// 16 passos = 1 compasso 4/4 (1 passo = 1 semicolcheia). Reggae na contagem com a caixa no 2 e no 4.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace mm {

constexpr int kPads = 16, kSteps = 16, kSeqs = 4, kFirstNote = 36; // pads nas notas 36–51 (canal 10)
constexpr double kPi = 3.14159265358979323846;

inline const char* padName (int i)
{
    static const char* n[kPads] = { "KICK", "KICK 2", "SNARE", "CLAP", "RIM", "HH FECH", "HH ABER", "CRASH",
                                    "PERC 1", "PERC 2", "PERC 3", "SHAKER", "808", "808 SLIDE", "FX", "VOX" };
    return n[i];
}

enum Style { reggae, trap, boombap, funk, drill, kStyles };
inline const char* styleName (int s)
{
    static const char* n[kStyles] = { "Reggae Steppers", "Trap", "Boom Bap", "Funk", "Drill" };
    return n[s];
}

struct Kit
{
    int vol; const char* name; Style style; int bpm;
    const char* credits;   // separado por ", " (vazio = não divulgado)
    const char* bio;
    const char* contents;  // separado por "; " (vazio = não publicado)
};

// Pesquisa: páginas da Academia de Beats / Beatmaker Store (Monstro.zip Vol. 1–4)
inline const std::vector<Kit>& kits()
{
    static const std::vector<Kit> k = {
        { 1, "DubMastor", reggae, 80, "Adonai, Cidade Verde Sounds, Costa Gold, DJ Cia, Cacife Clandestino",
          "Apresentado pela Academia como o monstro do reggae e mestre do DJ Coala. O kit mistura timbres modernos com os sons das próprias produções de reggae dele.",
          "57 drum fills; 31 tons; 5 cymbals; 78 hihats; 81 kicks; 45 percs; 3 shakers; 338 snares; 40 efeitos" },
        { 1, "Nox", trap, 140, "Matuê (DJ), Predella, Tati Zaqui, Welisson, Kant, Bivolt, Costa Gold",
          "Produtor e DJ do Matuê. O kit vem com bônus de loops de bateria e melodias.",
          "34 808s; 6 claps; 19 cymbals; 11 hihats; 10 kicks; 12 percs; 11 vocal chops; 8 snares; 19 efeitos" },
        { 1, "Neo Beats", trap, 138, "MC Poze do Rodo, Xamã, Haikaiss, Borges, Agnes Nunes, Sidoka, Krawk",
          "Kit com timbres usados nas próprias produções dele.",
          "8 808s; 10 claps; 11 cymbals; 10 hihats; 10 kicks; 10 snares; 20 efeitos" },
        { 1, "Ecologyk", trap, 140, "Matuê, Haikaiss, Costa Gold, Konai, Diego Thug, MC Guime, Negra Li, Jade Baraldo",
          "Paulo Henrique Castro, DJ e produtor na cena brasileira desde 2009, com milhões de streams no Spotify.", "" },
        { 1, "Pedro Lotto", trap, 142, "Veigh, Vulgo FK, MC PH, Orochi",
          "Produtor de trap com créditos em faixas como \"Ballena\" (Vulgo FK, MC PH, Veigh) e \"Mesma História\" (Orochi).", "" },
        { 1, "Skeeter", trap, 140, "", "Kit listado na coleção Monstro da Beatmaker Store.", "" },
        { 2, "André Nine", funk, 130, "MC Hariel, Kawe, Costa Gold, MC Ryan SP",
          "Seleção de timbres usados nas produções dele, com bônus misterioso.",
          "13 808s; 12 claps; 7 cymbals; 26 hihats; 12 kicks; 16 percs; 15 snares" },
        { 2, "DogDu Beats", trap, 140, "Sidoka, Zemaru, Chris MC",
          "Timbres das produções dele, com efeitos de vox exclusivos de bônus.",
          "7 808s; 7 claps; 4 cymbals; 7 hihats; 8 kicks; 8 percs; 7 snares; FX de vox" },
        { 2, "DJ Coala", boombap, 90, "Sidoka, RZO, Qualy, Tribo da Periferia",
          "Produtor da escola rap/boom bap, aluno do DubMastor. Kit com bônus de efeitos de voz e FX.",
          "12 808s + 5 basses; 11 claps; 7 cymbals; 10 hihats; 10 kicks; 15 percs; 12 snares" },
        { 2, "Caio Passos", funk, 130, "MC Hariel, Yunk Vino, Hyperanhas, MC Daniel, MC Pedrinho, Felp 22",
          "Professor da Academia de Beats. O kit tem baterias de funk, vox e loops exclusivos.",
          "9 808s; 4 claps; 6 cymbals; 6 hihats; 6 kicks; 21 percs; 2 rimshots; 19 efeitos de voz; 4 snares" },
        { 2, "Chiocki", trap, 145, "Kant, NOG, Spinard",
          "Professor da Academia de Beats. São 145 timbres selecionados, incluindo chants, drum fills, open hats, shakers e snaps.",
          "11 808s; 11 chants; 10 claps; 10 cymbals; 10 drum fills; 10 FX; 10 hihats; 10 kicks; 10 open hats" },
        { 2, "Robert Beats", trap, 140, "", "Kit do Vol. 2. Há pouca informação pública sobre os créditos dele.", "" },
        { 2, "DJ Caíque", funk, 130, "", "Kit do Vol. 2. Há pouca informação pública sobre os créditos dele.", "" },
        { 3, "Chris Beats", trap, 140, "Filipe Ret, L7nnon, MC Cabelinho, MC Maneirinho, Hyperanhas, Tz da Coronel",
          "Bônus com os timbres usados em \"Corte Americano\" (Filipe Ret e L7nnon).",
          "12 808s; 3 basses; 11 claps; 2 cymbals; 7 hihats; 4 kicks; 6 one shots; 6 percs" },
        { 3, "Portugal No Beat", trap, 142, "Filipe Ret, Jovem Dex, Greg Ferreira, Gaab",
          "Pack de timbres das produções dele.",
          "8 808s; 8 claps; 8 cymbals; 8 hihats; 6 kicks; 8 percs; 4 rimshots; 10 snares" },
        { 3, "Kouth", trap, 140, "Derek, UCLÃ, Lil Ivy, Denov, MC Igu, VK Mac, Kweller",
          "Beatmaker com kit de 8 timbres por categoria.",
          "8 808s; 8 claps; 8 cymbals; 8 hihats; 8 kicks; 8 percs; 8 rimshots; 8 snares; 8 efeitos" },
        { 3, "Jess", trap, 140, "Orochi, Bin, PL Quest, Borges", "Pack compacto com basses de ambiência.",
          "4 808s; 4 basses; 5 claps; 2 cymbals; 5 hihats; 5 kicks; 5 rimshots; 5 snares" },
        { 3, "Deekapz", funk, 130, "festivais: Primavera Sound, Lollapalooza, The Town",
          "Dupla de produtores (Matheus Henrique e Paulo Vitor) que mistura funk, trap e música brasileira. Criaram a festa gratuita Deekapz 0800 e dão aula na Academia.", "" },
        { 3, "Blakbone", drill, 142, "", "Professor da Academia de Beats. Há pouca informação pública sobre os créditos dele.", "" },
        { 3, "DJ Coala Vol.3", boombap, 92, "Sidoka, RZO, Qualy, Tribo da Periferia", "Segundo kit do DJ Coala na série.", "" },
        { 3, "Dallass", trap, 140, "", "Kit do Vol. 3. Há pouca informação pública sobre os créditos dele.", "" },
        { 3, "Celo", trap, 140, "Leozin", "Kit do Vol. 3. Aparece como Celo1st em faixas do Leozin.", "" },
        { 3, "Peunubeat", trap, 140, "", "Kit do Vol. 3. Há pouca informação pública sobre os créditos dele.", "" },
        { 3, "Tibery", trap, 140, "", "Kit do Vol. 3. Há pouca informação pública sobre os créditos dele.", "" },
        { 4, "Nagalli", trap, 140, "L7nnon, Veigh, MC Cabelinho, Kayblack, Vulgo FK",
          "André Nagalli de Oliveira, de Piracicaba (SP). Sócio-fundador e diretor musical da Supernova Entertainment. Lançou o álbum \"Magic Show\" em 2024 e é apontado como o produtor de rap mais ouvido do Brasil. Também dá aula na Academia.", "" },
        { 4, "Neckklace", trap, 140, "Nagalli (Magic Show), G.A, MC Luanna, Niink",
          "Coprodutor de \"Serenata\", do álbum \"Magic Show\" do Nagalli.", "" },
        { 4, "DJ Oreia", funk, 130, "", "DJ e produtor, professor da Academia de Beats.", "" },
        { 4, "Kotim", funk, 130, "Mc Don Juan",
          "Artista e produtor de \"Quem vai patrocinar?\", com participação em \"Sereia me perdoa\" (Mc Don Juan).", "" },
        { 4, "DJ Win", funk, 130, "", "Kit do Vol. 4. Há pouca informação pública sobre os créditos dele.", "" },
        { 4, "Cheek", drill, 142, "", "Kit do Vol. 4. Há pouca informação pública sobre os créditos dele.", "" },
        { 4, "DJ Coala Vol.4", boombap, 90, "Sidoka, RZO, Qualy", "Terceiro kit do DJ Coala na série.", "" },
        { 4, "Attlanta", trap, 140, "",
          "Produtora e professora da Academia de Beats. Foi destaque na série \"O Beat Delas\" da Rolling Stone Brasil, sobre mulheres produtoras.", "" },
        { 4, "Fepache", trap, 140, "Vulgo FK, MC PH, Veigh", "Produtor com crédito em \"Ballena\".", "" },
    };
    return k;
}

// ---------- caráter do kit: mesmo gerador da versão web (FNV-1a + mulberry) ----------
struct Char
{
    double kStart, kEnd, kDec, kClick, bDec, bDrive, sTone, sNoise, sDec, sBand, hp, ohDec, perc, vox, fx;
    int bass;
};

inline uint32_t fnv (const std::string& s)
{
    uint32_t h = 2166136261u;
    // a versão web percorre a string em UTF-16; nomes com acento usam o mesmo código (Latin-1)
    for (size_t i = 0; i < s.size(); ++i)
    {
        uint32_t c = (unsigned char) s[i];
        if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) { c = ((c & 0x1F) << 6) | ((unsigned char) s[i + 1] & 0x3F); ++i; }
        h = (h ^ c) * 16777619u;
    }
    return h;
}

struct Rng
{
    uint32_t seed;
    double next()
    {
        uint32_t a = seed ^ (seed >> 15);
        seed = a * (seed | 1u) + 0x6D2B79F5u;
        return (double) ((seed ^ (seed >> 7)) % 10000u) / 10000.0;
    }
};

inline Char charOf (const Kit& k)
{
    Rng r { fnv (k.name) };
    const auto s = k.style;
    Char c {};
    c.kStart = s == reggae ? 120 + r.next() * 40 : 150 + r.next() * 90;
    c.kEnd   = s == boombap ? 48 + r.next() * 10 : 38 + r.next() * 14;
    c.kDec   = s == reggae ? .35 + r.next() * .2 : s == boombap ? .28 + r.next() * .15 : .45 + r.next() * .3;
    c.kClick = .2 + r.next() * .6;
    static const int bassNotes[6] = { 31, 33, 34, 36, 29, 28 };
    c.bass   = bassNotes[(int) std::floor (r.next() * 6)];
    c.bDec   = s == drill ? 1.6 : .7 + r.next() * 1.1;
    c.bDrive = s == reggae ? .1 : .2 + r.next() * .8;
    c.sTone  = 160 + r.next() * 120;
    c.sNoise = .6 + r.next() * .5;
    c.sDec   = s == reggae ? .12 + r.next() * .08 : .16 + r.next() * .12;
    c.sBand  = 1500 + r.next() * 3000;
    c.hp     = 6000 + r.next() * 3500;
    c.ohDec  = .25 + r.next() * .35;
    static const double percs[5] = { 220, 280, 330, 400, 180 };
    c.perc   = percs[(int) std::floor (r.next() * 5)];
    static const double voxs[5] = { 330, 392, 440, 494, 262 };
    c.vox    = voxs[(int) std::floor (r.next() * 5)];
    c.fx     = r.next();
    return c;
}

// ---------- grooves de fábrica (mesmos da web). valor: 1 = nota, 2 = acento ----------
using Grid = std::array<std::array<uint8_t, kSteps>, kPads>;
struct Hit { int pad; int step; int v; };

inline Grid makeGrid (std::initializer_list<Hit> hits)
{
    Grid g {};
    for (auto& h : hits) g[(size_t) h.pad][(size_t) h.step] = (uint8_t) h.v;
    return g;
}

inline std::array<Grid, kSeqs> stylePatterns (int style)
{
    auto L = [] (int pad, std::initializer_list<int> steps, int v, std::vector<Hit>& out) { for (int s : steps) out.push_back ({ pad, s, v }); };
    std::array<std::vector<Hit>, kSeqs> h;
    switch (style)
    {
        case reggae: // steppers / rub a dub: caixa ou aro no 2 e no 4 (passos 5 e 13), nunca o drop no 3
            L (0, {0,4,8,12}, 1, h[0]); L (4, {4,12}, 2, h[0]); L (5, {2,6,10,14}, 1, h[0]); L (11, {1,3,5,7,9,11,13,15}, 1, h[0]); L (12, {0,7,8,14}, 1, h[0]);
            L (0, {0,8}, 1, h[1]); L (2, {4}, 1, h[1]); L (2, {12}, 2, h[1]); L (5, {2,6,10,14}, 1, h[1]); L (6, {14}, 1, h[1]); L (11, {1,3,5,7,9,11,13,15}, 1, h[1]); L (12, {0,3,8,11}, 1, h[1]);
            L (0, {0,4,8,12}, 1, h[2]); L (2, {12}, 2, h[2]); L (4, {4}, 1, h[2]); L (5, {0,2,4,6,8,10,12,14}, 1, h[2]); L (8, {7,15}, 1, h[2]); L (12, {0,6,8}, 1, h[2]);
            L (0, {0,8}, 1, h[3]); L (4, {4,12}, 1, h[3]); L (5, {2,6,10,14}, 1, h[3]); L (7, {0}, 1, h[3]); L (14, {15}, 1, h[3]); L (12, {0,10}, 1, h[3]);
            break;
        case trap:
            L (0, {0,10}, 1, h[0]); L (3, {8}, 2, h[0]); L (5, {0,2,4,6,8,10,11,12,14,15}, 1, h[0]); L (6, {6}, 1, h[0]); L (12, {0,10}, 1, h[0]); L (15, {15}, 1, h[0]);
            L (0, {0,3,10}, 1, h[1]); L (3, {8}, 2, h[1]); L (5, {0,2,4,5,6,8,10,12,13,14,15}, 1, h[1]); L (8, {7}, 1, h[1]); L (13, {0}, 1, h[1]); L (12, {10}, 1, h[1]);
            L (0, {0,6,10}, 1, h[2]); L (2, {8}, 1, h[2]); L (3, {8}, 2, h[2]); L (5, {0,1,2,4,6,8,9,10,12,14}, 1, h[2]); L (12, {0,6,10}, 1, h[2]); L (14, {15}, 1, h[2]);
            L (0, {0}, 1, h[3]); L (3, {8}, 2, h[3]); L (7, {0}, 1, h[3]); L (5, {0,4,8,12}, 1, h[3]); L (12, {0}, 1, h[3]); L (14, {0}, 1, h[3]);
            break;
        case boombap:
            L (0, {0,7,10}, 1, h[0]); L (2, {4,12}, 2, h[0]); L (5, {0,2,4,6,8,10,12,14}, 1, h[0]); L (6, {14}, 1, h[0]); L (12, {0,10}, 1, h[0]);
            L (0, {0,3,10}, 1, h[1]); L (2, {4,12}, 1, h[1]); L (5, {0,2,4,6,8,10,12,14}, 1, h[1]); L (4, {15}, 1, h[1]); L (12, {0,3,10}, 1, h[1]);
            L (0, {0,7,9}, 1, h[2]); L (2, {4,12}, 2, h[2]); L (5, {0,2,4,6,8,10,12,14}, 1, h[2]); L (15, {6}, 1, h[2]); L (12, {0}, 1, h[2]);
            L (0, {0,10}, 1, h[3]); L (2, {4,12}, 1, h[3]); L (5, {2,6,10,14}, 1, h[3]); L (7, {0}, 1, h[3]); L (12, {0}, 1, h[3]);
            break;
        case funk:
            L (0, {0,3,8,11}, 1, h[0]); L (3, {4,12}, 2, h[0]); L (8, {0,3,6,8,11,14}, 1, h[0]); L (5, {2,6,10,14}, 1, h[0]); L (12, {0,8}, 1, h[0]); L (15, {6}, 1, h[0]);
            L (0, {0,3,6,8,11,14}, 1, h[1]); L (3, {4,12}, 1, h[1]); L (9, {2,10}, 1, h[1]); L (5, {0,2,4,6,8,10,12,14}, 1, h[1]); L (12, {0,6}, 1, h[1]);
            L (0, {0,6,8,14}, 1, h[2]); L (3, {4,12}, 2, h[2]); L (8, {3,11}, 1, h[2]); L (10, {2,5,10,13}, 1, h[2]); L (15, {12}, 1, h[2]); L (12, {0,8}, 1, h[2]);
            L (0, {0,8}, 1, h[3]); L (3, {4,12}, 1, h[3]); L (8, {3,6,11,14}, 1, h[3]); L (14, {15}, 1, h[3]);
            break;
        default: // drill
            L (0, {0,11}, 1, h[0]); L (2, {6}, 2, h[0]); L (3, {14}, 1, h[0]); L (5, {0,3,4,7,8,10,12,15}, 1, h[0]); L (13, {0,11}, 1, h[0]); L (4, {3}, 1, h[0]);
            L (0, {0,7,11}, 1, h[1]); L (2, {6,14}, 1, h[1]); L (5, {0,2,3,4,6,8,10,11,12,14}, 1, h[1]); L (13, {0}, 1, h[1]); L (12, {7,11}, 1, h[1]);
            L (0, {0,11}, 1, h[2]); L (2, {6}, 1, h[2]); L (3, {14}, 1, h[2]); L (5, {0,3,6,8,11,14}, 1, h[2]); L (9, {9}, 1, h[2]); L (13, {0,11}, 1, h[2]);
            L (0, {0}, 1, h[3]); L (2, {6,14}, 1, h[3]); L (7, {0}, 1, h[3]); L (13, {0}, 1, h[3]);
            break;
    }
    std::array<Grid, kSeqs> out {};
    for (int s = 0; s < kSeqs; ++s)
        for (auto& x : h[(size_t) s]) out[(size_t) s][(size_t) x.pad][(size_t) x.step] = (uint8_t) x.v;
    return out;
}

// ---------- sample do usuário ----------
struct Sample
{
    std::vector<float> l, r;
    double rate = 44100;
    std::string name;
};

// ---------- síntese ----------
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    enum Type { none, lp, hp, bp };
    void set (Type t, double f, double q, double sr)
    {
        if (t == none) { b0 = 1; b1 = b2 = a1 = a2 = 0; return; }
        f = std::fmin (f, sr * .45);
        const double w = 2 * kPi * f / sr, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        if (t == lp) { b0 = (1 - cs) / 2; b1 = 1 - cs; b2 = b0; }
        else if (t == hp) { b0 = (1 + cs) / 2; b1 = -(1 + cs); b2 = b0; }
        else { b0 = al; b1 = 0; b2 = -al; }
        a1 = -2 * cs; a2 = 1 - al;
        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }
    double tick (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};

enum Wave { sine, tri, square, saw, noise };

struct Partial
{
    Wave wave = sine;
    double f0 = 0, f1 = 0, sweep = 0, peak = 0, att = .001, dec = .1, delay = 0, len = 0; // segundos
    bool linAtt = false, linGlide = false;
    int nFilt = 0; Biquad::Type ft = Biquad::none; double ff[3] {}, fq = .7;
    Biquad flt[3];
    double phase = 0;
};

struct Voice
{
    bool active = false;
    int pad = 0;
    double t = 0, gain = 1, drive = 0;
    int nParts = 0;
    Partial parts[6];
    std::shared_ptr<const Sample> smp; double pos = 0, rate = 1, endPos = 0, fadeStart = 0;
};

class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        echoBuf.assign ((size_t) (sr * 3.0) + 2, 0.0f);
        echoBufR.assign (echoBuf.size(), 0.0f);
        echoW = 0; echoLp = echoLpR = 0;
        for (auto& v : voices) v.active = false;
    }

    void setKit (int i)
    {
        const int n = (int) kits().size();
        kit = ((i % n) + n) % n;
        ch = charOf (kits()[(size_t) kit]);
    }
    int kitIndex() const { return kit; }
    const Char& character() const { return ch; }

    // parâmetros por pad (semitons, % do decay)
    float tune[kPads] {}, decay[kPads] { 100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100 };
    std::shared_ptr<const Sample> samples[kPads];
    std::atomic<uint32_t> hits[kPads] {};   // contador para piscar o pad na interface

    // efeitos (saturador e Tape Echo começam em bypass: padrão de fábrica)
    bool satOn = false, echoOn = false;
    double satDrive = .35, echoSteps = 3, echoFb = .45, echoMix = .3, bpm = 140, master = .8;

    void trigger (int pad, float vel, int offsetSamples = 0)
    {
        if (pad < 0 || pad >= kPads) return;
        hits[pad].fetch_add (1, std::memory_order_relaxed);
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) { v = &voices[(size_t) stealIdx]; stealIdx = (stealIdx + 1) % (int) voices.size(); }
        build (*v, pad, vel);
        v->t = -offsetSamples / sr;
    }

    // mistura estéreo, soma em L/R (já zerados pelo chamador)
    void render (float* L, float* R, int n)
    {
        for (auto& v : voices)
            if (v.active) renderVoice (v, L, R, n);

        const double stepDur = 60.0 / std::fmax (20.0, bpm) / 4.0;
        const size_t dl = (size_t) std::fmin ((double) echoBuf.size() - 2, std::fmax (1.0, stepDur * echoSteps * sr));
        const double lpA = 1 - std::exp (-2 * kPi * 3200 / sr);
        const double k = satDrive * 40 + 1, tk = std::tanh (k);
        for (int i = 0; i < n; ++i)
        {
            double l = L[i] * master, r = R[i] * master;
            if (satOn) { l = std::tanh (k * l) / tk; r = std::tanh (k * r) / tk; }
            if (echoOn)
            {
                const size_t rp = (echoW + echoBuf.size() - dl) % echoBuf.size();
                echoLp += lpA * (echoBuf[rp] - echoLp);
                echoLpR += lpA * (echoBufR[rp] - echoLpR);
                echoBuf[echoW] = (float) (l + echoLp * echoFb);
                echoBufR[echoW] = (float) (r + echoLpR * echoFb);
                l += echoLp * echoMix; r += echoLpR * echoMix;
            }
            else { echoBuf[echoW] = echoBufR[echoW] = 0; echoLp *= .999; echoLpR *= .999; }
            echoW = (echoW + 1) % echoBuf.size();
            // limitador suave (no lugar do compressor da versão web)
            L[i] = (float) std::tanh (l * 1.1);
            R[i] = (float) std::tanh (r * 1.1);
        }
    }

private:
    static double mtof (double m) { return 440.0 * std::pow (2.0, (m - 69) / 12.0); }

    Partial& add (Voice& v)
    {
        auto& p = v.parts[v.nParts++];
        p = Partial {};
        return p;
    }
    void tone (Voice& v, Wave w, double f0, double f1, double sweep, double peak, double dec, double delay = 0)
    {
        auto& p = add (v); p.wave = w; p.f0 = f0; p.f1 = f1; p.sweep = sweep; p.peak = peak; p.att = .002; p.dec = dec; p.delay = delay;
        p.len = dec + .05;
    }
    void noiseP (Voice& v, double len, Biquad::Type t, double freq, double q, double peak, double dec, double delay = 0)
    {
        auto& p = add (v); p.wave = noise; p.ft = t; p.nFilt = 1; p.ff[0] = freq; p.fq = q; p.peak = peak; p.att = .001; p.dec = dec; p.delay = delay; p.len = len;
    }

    void build (Voice& v, int i, float vel)
    {
        v = Voice {};
        v.active = true; v.pad = i; v.gain = vel;
        const double tu = std::pow (2.0, tune[i] / 12.0), dk = decay[i] / 100.0;
        const Char& p = ch;

        if (auto s = std::atomic_load (&samples[i]))
        {
            v.smp = s; v.rate = tu * s->rate / sr; v.endPos = (double) s->l.size();
            v.fadeStart = v.endPos * dk * .9;
            return;
        }

        switch (i)
        {
            case 0: tone (v, sine, p.kStart * tu, p.kEnd * tu, .07, 1, p.kDec * dk); noiseP (v, .02, Biquad::hp, 3000, .7, p.kClick * .5, .015); break;
            case 1: tone (v, sine, p.kStart * 1.2 * tu, p.kEnd * 1.3 * tu, .04, .9, p.kDec * .6 * dk); tone (v, tri, 900, 120, .02, .4, .03); break;
            case 2: tone (v, tri, p.sTone * tu, p.sTone * .8 * tu, .05, .6, .1 * dk); noiseP (v, .5, Biquad::bp, p.sBand, .8, p.sNoise, p.sDec * dk); break;
            case 3: for (double d : { 0.0, .011, .022 }) noiseP (v, .05, Biquad::bp, 1200 * tu, 1.2, .9, .03, d);
                    noiseP (v, .4, Biquad::bp, 1100 * tu, 1, .7, .16 * dk, .03); break;
            case 4: tone (v, square, 1650 * tu, 0, 0, .3, .035 * dk); tone (v, tri, 470 * tu, 0, 0, .4, .05 * dk); break;
            case 5: noiseP (v, .1, Biquad::hp, p.hp * tu, .7, .45, .035 * dk); break;
            case 6: noiseP (v, 1, Biquad::hp, p.hp * .9 * tu, .7, .4, p.ohDec * dk); break;
            case 7: noiseP (v, 2, Biquad::hp, 4500 * tu, .5, .45, 1.2 * dk); for (double m : { 1.0, 1.48, 1.93 }) tone (v, square, 540 * m * tu, 0, 0, .04, 1 * dk); break;
            case 8: tone (v, sine, p.perc * tu, p.perc * .85 * tu, .05, .7, .18 * dk); break;
            case 9: tone (v, sine, p.perc * 1.5 * tu, p.perc * 1.3 * tu, .04, .6, .12 * dk); noiseP (v, .02, Biquad::bp, 2500, 1, .2, .01); break;
            case 10: tone (v, tri, p.perc * 3 * tu, 0, 0, .5, .05 * dk); break;
            case 11: { auto& q = add (v); q.wave = noise; q.ft = Biquad::bp; q.nFilt = 1; q.ff[0] = 7000 * tu; q.fq = 1.5; q.peak = .35; q.att = .02; q.linAtt = true; q.dec = .07 * dk; q.len = .2; break; }
            case 12: case 13:
            {
                const double f = mtof (p.bass) * tu;
                auto& q = add (v); q.wave = sine; q.att = .005; q.peak = .9; q.dec = p.bDec * dk; q.len = p.bDec * dk + .05;
                if (i == 13) { q.f0 = f * 2; q.f1 = f; q.sweep = .12; } else { q.f0 = f * 1.5; q.f1 = f; q.sweep = .02; }
                v.drive = p.bDrive;
                break;
            }
            case 14: if (p.fx < .5) tone (v, saw, 2400 * tu, 80, .6, .25, .6 * dk); else noiseP (v, 1, Biquad::bp, 800 * tu, 4, .5, .7 * dk); break;
            default:
            {
                auto& q = add (v); q.wave = saw; q.f0 = p.vox * tu; q.f1 = p.vox * .94 * tu; q.sweep = .2; q.linGlide = true;
                q.att = .01; q.peak = .5; q.dec = .22 * dk; q.len = .4 * dk;
                q.ft = Biquad::bp; q.nFilt = 3; q.ff[0] = 700; q.ff[1] = 1150; q.ff[2] = 2600; q.fq = 8;
                break;
            }
        }
        for (int k = 0; k < v.nParts; ++k)
        {
            auto& q = v.parts[k];
            for (int f = 0; f < q.nFilt; ++f) q.flt[f].set (q.ft, q.ff[f], q.fq, sr);
        }
    }

    double env (const Partial& p, double t) const
    {
        if (t < p.att)
            return p.linAtt ? p.peak * t / p.att : std::exp (std::log (1e-4) + (std::log (p.peak) - std::log (1e-4)) * t / p.att);
        const double x = (t - p.att) / p.dec;
        return x >= 1 ? 0 : std::exp (std::log (p.peak) + (std::log (1e-4) - std::log (p.peak)) * x);
    }

    void renderVoice (Voice& v, float* L, float* R, int n)
    {
        const double dt = 1.0 / sr;
        if (v.smp)
        {
            const auto& s = *v.smp;
            for (int i = 0; i < n; ++i, v.t += dt)
            {
                if (v.t < 0) continue;
                if (v.pos >= v.endPos - 1) { v.active = false; return; }
                const size_t a = (size_t) v.pos; const double fr = v.pos - (double) a;
                double g = v.gain;
                if (v.pos > v.fadeStart) g *= std::exp (-(v.pos - v.fadeStart) / (.03 * s.rate));
                if (g < 1e-5 && v.pos > v.fadeStart) { v.active = false; return; }
                const double l = s.l[a] + (s.l[a + 1] - s.l[a]) * fr;
                const double r = s.r.empty() ? l : s.r[a] + (s.r[a + 1] - s.r[a]) * fr;
                L[i] += (float) (l * g); R[i] += (float) (r * g);
                v.pos += v.rate;
            }
            return;
        }
        double maxLen = 0;
        for (int k = 0; k < v.nParts; ++k) maxLen = std::fmax (maxLen, v.parts[k].delay + v.parts[k].len);
        const double kd = v.drive * 40 + 1, tkd = std::tanh (kd);
        for (int i = 0; i < n; ++i, v.t += dt)
        {
            if (v.t < 0) continue;
            if (v.t > maxLen) { v.active = false; return; }
            double sum = 0;
            for (int k = 0; k < v.nParts; ++k)
            {
                auto& p = v.parts[k];
                const double tt = v.t - p.delay;
                if (tt < 0 || tt > p.len) continue;
                double x;
                if (p.wave == noise) x = nextNoise();
                else
                {
                    double f = p.f0;
                    if (p.f1 > 0 && p.sweep > 0)
                        f = tt >= p.sweep ? p.f1 : (p.linGlide ? p.f0 + (p.f1 - p.f0) * tt / p.sweep : p.f0 * std::pow (p.f1 / p.f0, tt / p.sweep));
                    p.phase += f * dt; p.phase -= std::floor (p.phase);
                    const double ph = p.phase;
                    switch (p.wave)
                    {
                        case sine: x = std::sin (2 * kPi * ph); break;
                        case tri: x = 1 - 4 * std::fabs (ph - .5); break;
                        case square: x = ph < .5 ? 1 : -1; break;
                        default: x = 2 * ph - 1; break;
                    }
                }
                if (p.nFilt == 1) x = p.flt[0].tick (x);
                else if (p.nFilt > 1) { double y = 0; for (int f = 0; f < p.nFilt; ++f) y += p.flt[f].tick (x); x = y; }
                sum += x * env (p, tt);
            }
            if (v.drive > 0) sum = std::tanh (kd * sum) / tkd * .9;
            const float o = (float) (sum * v.gain);
            L[i] += o; R[i] += o;
        }
    }

    double nextNoise() { noiseSeed = noiseSeed * 1664525u + 1013904223u; return (double) (noiseSeed >> 8) / 8388608.0 - 1.0; }

    double sr = 44100;
    int kit = 0, stealIdx = 0;
    Char ch = charOf (kits()[0]);
    std::array<Voice, 48> voices {};
    std::vector<float> echoBuf, echoBufR;
    size_t echoW = 0;
    double echoLp = 0, echoLpR = 0;
    uint32_t noiseSeed = 22222;
};

// ---------- MIDI (arquivo .mid do padrão, canal 10, pads nas notas 36–51) ----------
inline std::vector<uint8_t> patternToMidi (const Grid& g, double bpm)
{
    const int tpq = 96, st = tpq / 4;
    struct Ev { int t, st, note, vel; };
    std::vector<Ev> ev;
    for (int i = 0; i < kPads; ++i)
        for (int s = 0; s < kSteps; ++s)
        {
            const int v = g[(size_t) i][(size_t) s];
            if (! v) continue;
            int len = st;
            if (i == 12 || i == 13) { int n = s + 1; while (n < kSteps && ! g[12][(size_t) n] && ! g[13][(size_t) n]) ++n; len = (n - s) * st; }
            ev.push_back ({ s * st, 0x99, kFirstNote + i, v == 2 ? 120 : 96 });
            ev.push_back ({ s * st + len - 1, 0x89, kFirstNote + i, 0 });
        }
    std::stable_sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.t != b.t ? a.t < b.t : (a.st == 0x89 && b.st != 0x89); });
    auto vl = [] (std::vector<uint8_t>& o, uint32_t n) {
        uint8_t b[5]; int c = 0; b[c++] = n & 127; while (n >>= 7) b[c++] = (uint8_t) ((n & 127) | 128);
        while (c) o.push_back (b[--c]);
    };
    const uint32_t us = (uint32_t) std::lround (60e6 / bpm);
    std::vector<uint8_t> tr { 0, 0xFF, 0x51, 3, (uint8_t) (us >> 16), (uint8_t) (us >> 8), (uint8_t) us };
    int last = 0;
    for (auto& e : ev) { vl (tr, (uint32_t) (e.t - last)); tr.push_back ((uint8_t) e.st); tr.push_back ((uint8_t) e.note); tr.push_back ((uint8_t) e.vel); last = e.t; }
    vl (tr, (uint32_t) (kSteps * st - last)); tr.insert (tr.end(), { 0xFF, 0x2F, 0 });
    std::vector<uint8_t> out { 'M','T','h','d', 0,0,0,6, 0,0, 0,1, 0,(uint8_t) tpq, 'M','T','r','k' };
    const uint32_t L = (uint32_t) tr.size();
    out.insert (out.end(), { (uint8_t) (L >> 24), (uint8_t) (L >> 16), (uint8_t) (L >> 8), (uint8_t) L });
    out.insert (out.end(), tr.begin(), tr.end());
    return out;
}

} // namespace mm
