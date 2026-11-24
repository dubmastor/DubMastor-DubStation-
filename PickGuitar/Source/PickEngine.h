#pragma once
// Mastor Pick Guitar MPG-11 — motor de som e sequenciador (header-only, sem JUCE).
// O mesmo código roda no plugin e no teste offline (tests/render_test.cpp).
//
// Contagem da Dub Station: caixa no 2 e no 4. A grade tem 16 semicolcheias = 1 compasso (1 e & a).
// Cadeia: palheta (corda Karplus-Strong abafada) -> amp (captador, corpo, drive, tom) -> EQ (7 pontos)
//         -> Mastor Big Knob (LPF à esquerda, HPF à direita) -> saturador -> seco + Mastor Tape Echo + mola -> master.
// A linha nasce do baixo: FOLLOW dobra nota a nota, GROOVE toca a grade com a nota do baixo,
// COUNTER faz uma contra-linha sobre o baixo. PICA-PAU: palhetadas rápidas repetidas dentro da semicolcheia.
#include <cmath>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include "TapeEcho.h"   // dub::TapeEcho (HalfDub/Source) — o mesmo eco de fita do Mastor Big Knob

namespace mpg
{
static constexpr float kPi  = 3.14159265358979f;
static constexpr float kTau = 6.28318530717959f;
static constexpr int kSteps = 16;          // semicolcheias da grade (1 compasso)
static constexpr int kCycle = 128;         // ciclo do pica-pau: 8 compassos
static constexpr double kStepBeats = 0.25; // semicolcheia em tempos

//==============================================================================
// teoria
enum Scale { MenorNatural, Maior, Dorico, Mixolidio, MenorHarmonica, kNumScales };
inline const char* scaleName (int s)
{
    static const char* n[kNumScales] = { "Menor natural", "Maior", "Dórico", "Mixolídio", "Menor harmônica" };
    return n[std::clamp (s, 0, kNumScales - 1)];
}
inline const int* scaleSteps (int s)
{
    static const int S[kNumScales][7] = { { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 9, 10 },
                                          { 0, 2, 4, 5, 7, 9, 10 }, { 0, 2, 3, 5, 7, 8, 11 } };
    return S[std::clamp (s, 0, kNumScales - 1)];
}
inline const char* noteName (int m)
{
    static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n[((m % 12) + 12) % 12];
}
inline int bassRoot (int key) { int r = 36 + key; if (r > 42) r -= 12; return r; }
inline int floorDiv (int a, int b) { return (int) std::floor ((double) a / (double) b); }
inline int deg2m (int d, int base, int scale)
{
    const int* s = scaleSteps (scale);
    return base + 12 * floorDiv (d, 7) + s[((d % 7) + 7) % 7];
}
inline int m2deg (int m, int key, int scale)
{
    const int* s = scaleSteps (scale);
    const int rel = m - bassRoot (key);
    const int o = floorDiv (rel, 12), pc = ((rel % 12) + 12) % 12;
    int bi = 0, bd = 99;
    for (int i = 0; i < 7; ++i) { const int d = std::abs (s[i] - pc); if (d < bd) { bd = d; bi = i; } }
    return o * 7 + bi;
}
inline float mtof (float m) { return 440.0f * std::pow (2.0f, (m - 69.0f) / 12.0f); }

//==============================================================================
// linhas guia (demo): {semicolcheia, grau na escala, duração em semicolcheias} · 2 compassos
struct LineEv { int step, deg, len; };
struct Line { const char* name; std::vector<LineEv> ev; };
inline const std::vector<Line>& lines()
{
    static const std::vector<Line> L {
        { "Roots menor", { {0,0,2},{2,0,1},{3,2,1},{4,4,2},{6,3,1},{7,2,1},{8,0,2},{10,0,1},{11,-1,1},{12,-3,3},{15,-2,1},
                           {16,3,2},{18,3,1},{19,5,1},{20,7,2},{22,5,1},{23,3,1},{24,4,2},{26,4,1},{27,6,1},{28,4,2},{30,2,1},{31,1,1} } },
        { "Rocksteady andando", { {0,0,1},{1,2,1},{2,4,1},{3,5,1},{4,4,2},{6,2,1},{7,1,1},{8,0,1},{9,2,1},{10,4,1},{11,2,1},{12,0,2},{14,-1,1},{15,-2,1},
                                  {16,3,1},{17,5,1},{18,7,1},{19,5,1},{20,3,2},{22,2,1},{23,1,1},{24,4,1},{25,6,1},{26,8,1},{27,6,1},{28,4,2},{30,6,1},{31,7,1} } },
        { "Pesado anos 80", { {0,0,3},{3,0,1},{4,4,1},{5,3,1},{6,2,2},{8,0,3},{11,0,1},{12,2,1},{13,3,1},{14,4,2},
                              {16,3,3},{19,3,1},{20,5,1},{21,4,1},{22,3,2},{24,4,3},{27,4,1},{28,1,1},{29,2,1},{30,3,2} } }
    };
    return L;
}
static constexpr int kLineSteps = 32;

// desenhos do COUNTER (graus acima da nota do baixo)
struct Shape { const char* name; std::vector<int> d; };
inline const std::vector<Shape>& shapes()
{
    static const std::vector<Shape> S {
        { "Arpejo 1-5-3", { 0, 4, 2, 4 } },
        { "Subida 1-3-5-6", { 0, 2, 4, 5, 4, 2 } },
        { "Oitava e quinta", { 0, 4, 7, 4 } },
        { "Vizinha 1-2-3", { 0, 1, 2, 1 } },
        { "Larga 1-5-8-5-3", { 0, 4, 7, 4, 2 } }
    };
    return S;
}

//==============================================================================
enum Mode { Follow, Groove, Counter };
enum Source { SrcMidi, SrcGuide };
enum Art { Mantem, PicaPau };
enum Pickup { Braco, Meio, Ponte };
enum SatType { Fita, Valvula, Transistor };
enum EchoSync { Slap, S16, S8, S8d, S4, kNumSync };
inline const char* echoSyncName (int s)
{
    static const char* n[kNumSync] = { "Slapback 110 ms", "1/16", "1/8", "1/8 pontuada", "1/4" };
    return n[std::clamp (s, 0, kNumSync - 1)];
}
inline double echoSeconds (int sync, double bpm)
{
    const double q = 60.0 / std::max (20.0, bpm);
    switch (sync) { case S16: return q / 4; case S8: return q / 2; case S8d: return q * 0.75; case S4: return q; default: return 0.11; }
}

// EQ: HPF, grave (shelf), 3 sinos, agudo (shelf), LPF · {freq, ganho dB (no HPF/LPF = ressonância), Q}
static constexpr int kEqBands = 7;
enum EqType { EqHP, EqLS, EqPK, EqHS, EqLP };
inline EqType eqType (int i) { static const EqType t[kEqBands] = { EqHP, EqLS, EqPK, EqPK, EqPK, EqHS, EqLP }; return t[i]; }
inline const char* eqName (int i) { static const char* n[kEqBands] = { "HPF", "Grave", "Sino 1", "Sino 2", "Sino 3", "Agudo", "LPF" }; return n[i]; }
struct EqBand { float f, g, q; };
using EqSet = std::array<EqBand, kEqBands>;
static const EqSet kEqFlat { { {20,0,1},{120,0,0.7f},{400,0,1},{1000,0,1},{3000,0,1},{6000,0,0.7f},{20000,0,1} } };
struct EqPreset { const char* name; EqSet set; };
inline const std::vector<EqPreset>& eqPresets()
{
    static const std::vector<EqPreset> P {
        { "Flat", kEqFlat },
        { "LPF", { { {20,0,1},{120,0,0.7f},{400,0,1},{1000,0,1},{3000,0,1},{6000,0,0.7f},{1800,6,1} } } },
        { "HPF", { { {350,6,1},{120,0,0.7f},{400,0,1},{1000,0,1},{3000,0,1},{6000,0,0.7f},{20000,0,1} } } },
        { "Pick clássico", { { {140,3,1},{120,0,0.7f},{320,-2,1.2f},{1000,3,1},{2600,2,1.4f},{6000,-3,0.7f},{7500,3,1} } } },
        { "Escuro", { { {90,3,1},{160,2,0.7f},{500,1,1},{1400,-2,1},{3000,-4,1},{5000,-6,0.7f},{4200,3,1} } } },
        { "Brilho de ponte", { { {180,3,1},{120,-2,0.7f},{450,-2,1.2f},{1200,2,1},{3200,4,1.2f},{7000,2,0.7f},{12000,3,1} } } }
    };
    return P;
}
static constexpr int kEqDefaultPreset = 3; // Pick clássico

//==============================================================================
// guitarristas: padrões originais no espírito de cada um (não são transcrições)
struct Preset
{
    const char* name; const char* role; const char* card; int era;
    int mode; const char* grid; int line, oct, shape, pickup;
    float mute, pos, atk, drive, tone, body;
    float human, acc;
    int echoSync; float echoFb, echoMix, spring;
    int art;
};
static constexpr int kNumEras = 3;
inline const char* eraName (int e) { static const char* n[kNumEras] = { "Ska e rocksteady", "Roots anos 70", "Roots anos 80" }; return n[std::clamp (e, 0, 2)]; }
inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P {
        { "Ernest Ranglin", "Ska · tracking do baixo, jazzy e solto",
          "Origem da história: no ska a guitarra seguia o baixo nota a nota. Aqui: Follow, abafado leve, captador do meio e swing.",
          0, Follow, "XxXxXxXoXxXxXxXo", 1, 12, 1, Meio, 45, 30, 70, 22, 72, 55, 22, 40, Slap, 18, 16, 30, Mantem },
        { "Lynn Taitt", "Rocksteady · palhetada que imita steel drum",
          "Rocksteady: dobra do baixo bem curta e brilhante, do captador da ponte, com o pica-pau em tercinas.",
          0, Follow, "X.xxX.xxX.xxX.xo", 1, 12, 0, Ponte, 72, 18, 78, 20, 66, 45, 14, 45, Slap, 12, 12, 22, PicaPau },
        { "Hux Brown", "Reggae inicial · pick firme colado na bateria",
          "Passagem do rocksteady para o reggae: o ritmo sai da grade e a nota vem do baixo, com acento no começo de cada tempo.",
          0, Groove, "X.x.Xxx.X.x.Xxxo", 0, 12, 0, Ponte, 70, 20, 72, 30, 62, 50, 12, 55, Slap, 15, 14, 25, PicaPau },
        { "Eric Frater", "Studio One · pick com eco de fita",
          "Som de Studio One: pick quente com eco de fita bem presente. Groove em pares e a última nota aberta.",
          1, Groove, "XxX.XxX.XxX.Xx.o", 0, 12, 2, Meio, 64, 28, 60, 26, 58, 58, 15, 45, S8d, 38, 30, 28, Mantem },
        { "Earl \"Chinna\" Smith", "Roots · contra-linha que conversa com o baixo",
          "Anos 70: a linha abafada deixa de dobrar e vira um segundo baixo. Counter com desenho largo, ponte brilhante e síncopas.",
          1, Counter, "X.xx.xX.X.xx.xo.", 0, 12, 4, Ponte, 60, 16, 80, 34, 78, 42, 16, 50, Slap, 20, 18, 30, Mantem },
        { "Dwight Pinkney", "Roots · melódico, mistura abafada e aberta",
          "Contra-linha cantada, com notas abertas no meio das abafadas para a linha aparecer acima do baixo.",
          1, Counter, "X.xo.xX.X.xo.xXo", 0, 12, 1, Meio, 58, 24, 66, 24, 68, 55, 14, 40, Slap, 16, 15, 35, Mantem },
        { "Bingy Bunny & Tony Chin", "Roots Radics / Revolutionaries · pesado",
          "Anos 80: notas retas, muito abafadas e escuras, do captador do braço, segurando o riddim junto com o baixo.",
          2, Groove, "XxxxXxxxXxxxXxxx", 2, 12, 2, Braco, 88, 40, 45, 30, 42, 68, 10, 50, Slap, 10, 10, 18, PicaPau },
        { "Mikey Chung", "Black Uhuru · nota única brilhante e espaçada",
          "Linhas de nota única que completam guitarra base, baixo e teclados. Counter com oitava e quinta, ponte e eco.",
          2, Counter, "X.X.xoX.X.X.xoX.", 2, 12, 2, Ponte, 55, 14, 82, 36, 80, 40, 10, 55, S8, 30, 26, 20, Mantem }
    };
    return P;
}
static constexpr int kDefaultPreset = 2; // Hux Brown

// timbre de fábrica (definido pelo Mastor): ponte, abafado 100, palheta na ponte, ataque 100, drive 0, tom 62, corpo 96
struct Factory { static constexpr float mute = 100, pos = 7, atk = 100, drive = 0, tone = 62, body = 96; };

//==============================================================================
// parâmetros (o processor preenche a cada bloco)
using Grid = std::array<char, kSteps>;          // '.' pausa, 'x' abafada, 'X' acento, 'o' aberta, 'r' pica-pau só nela
using Edits = std::array<int8_t, kCycle>;       // 0 nada, 1 pica-pau posto à mão, 2 tirado à mão
struct Params
{
    int mode = Groove, source = SrcMidi, line = 0, oct = 12, shape = 0, key = 9, scale = MenorNatural;
    float gate = 8, human = 12, swing = 10, acc = 55;
    int art = PicaPau, rep = 3; float ppSwing = 10, dose = 5;
    int pickup = Ponte; float mute = Factory::mute, pos = Factory::pos, atk = Factory::atk, drive = Factory::drive, tone = Factory::tone, body = Factory::body;
    bool eqOn = true; EqSet eq = eqPresets()[kEqDefaultPreset].set;
    float bk = 0;                                 // -100 (LPF) .. 0 .. +100 (HPF)
    bool satOn = false; int satType = Fita; float satDrive = 40, satMix = 100, satOut = 0;
    bool echoOn = false; int echoSync = Slap; float echoFb = 15, echoMix = 14, spring = 25; bool throwOn = false;
    float master = 80; bool power = true;
    bool guideBass = true;
    Grid grid {};
    Edits edits {};
    Params() { setGrid (presets()[kDefaultPreset].grid); }
    void setGrid (const char* g) { for (int i = 0; i < kSteps; ++i) grid[(size_t) i] = g[i] ? g[i] : '.'; }
};

// carrega um guitarrista (modo, grade, timbre, eco); duração, swing e pica-pau ficam no padrão de fábrica
inline void applyPreset (Params& p, int i)
{
    const auto& P = presets()[(size_t) std::clamp (i, 0, (int) presets().size() - 1)];
    p.mode = P.mode; p.setGrid (P.grid); p.line = P.line; p.oct = P.oct; p.shape = P.shape; p.pickup = P.pickup;
    p.mute = P.mute; p.pos = P.pos; p.atk = P.atk; p.drive = P.drive; p.tone = P.tone; p.body = P.body;
    p.human = P.human; p.acc = P.acc; p.gate = 8; p.swing = 10;
    p.echoSync = P.echoSync; p.echoFb = P.echoFb; p.echoMix = P.echoMix; p.spring = P.spring;  // o eco continua ligado/desligado como estava
    p.art = P.art; p.rep = 3; p.ppSwing = 10;
    p.edits.fill (0);
}

//==============================================================================
// pica-pau: a DOSE (0–10%) espalha por igual no ciclo de 8 compassos uma fração das palhetadas abafadas
// e gira a posição dentro do compasso a cada uma, para não cair sempre na mesma nota.
inline void autoPicaPau (const Grid& grid, float dose, std::array<bool, kCycle>& out)
{
    out.fill (false);
    std::array<int, kCycle> pos {};   // sem alocar memória (roda na thread de áudio)
    int T = 0;
    for (int L = 0; L < kCycle / kSteps; ++L)
        for (int i = 0; i < kSteps; ++i)
            if (grid[(size_t) i] == 'x' || grid[(size_t) i] == 'X') pos[(size_t) T++] = L * kSteps + i;
    if (T == 0 || dose <= 0.0f) return;
    const int n = std::max (1, (int) std::lround (dose / 100.0f * (float) T));
    const int hpl = T / (kCycle / kSteps);
    for (int m = 0; m < n; ++m)
    {
        int j = (int) std::floor ((m + 0.5) * T / n);
        if (hpl > 0) j = (j / hpl) * hpl + ((j % hpl) + m * 3) % hpl;
        int guard = 0;
        while (out[(size_t) pos[(size_t) (j % T)]] && guard++ < T) ++j;
        out[(size_t) pos[(size_t) (j % T)]] = true;
    }
}
inline bool isPicaPau (const Params& p, const std::array<bool, kCycle>& autoSet, int cyc)
{
    cyc = ((cyc % kCycle) + kCycle) % kCycle;
    const char v = p.grid[(size_t) (cyc % kSteps)];
    if (v == '.' || v == 'o') return false;
    if (p.edits[(size_t) cyc] == 2) return false;
    if (v == 'r' || p.edits[(size_t) cyc] == 1) return true;
    return p.art == PicaPau && autoSet[(size_t) cyc];
}

//==============================================================================
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    { b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0); a1 = (float) (na1 / na0); a2 = (float) (na2 / na0); }
    void identity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    // RBJ cookbook; HP/LP com Q em dB como no Web Audio (o mesmo som da versão para navegador)
    void design (EqType t, float f, float g, float q, float fs)
    {
        const double w = 2.0 * 3.14159265358979 * std::clamp ((double) f, 10.0, 0.49 * fs) / fs, c = std::cos (w), s = std::sin (w);
        if (t == EqHP || t == EqLP)
        {
            const double Q = std::pow (10.0, (g - 3.0) / 20.0), al = s / (2.0 * Q);
            if (t == EqLP) set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
            else           set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
            return;
        }
        const double A = std::pow (10.0, g / 40.0);
        if (t == EqPK)
        {
            const double al = s / (2.0 * std::max (0.05f, q));
            set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
            return;
        }
        const double al = s / 2.0 * std::sqrt (2.0), sA = 2.0 * std::sqrt (A) * al;
        if (t == EqLS) set (A * ((A + 1) - (A - 1) * c + sA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sA),
                            (A + 1) + (A - 1) * c + sA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sA);
        else           set (A * ((A + 1) + (A - 1) * c + sA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sA),
                            (A + 1) - (A - 1) * c + sA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sA);
    }
    float magnitudeDb (float f, float fs) const
    {
        const double w = 2.0 * 3.14159265358979 * f / fs;
        const double cr = std::cos (w), sr = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2, ni = -(b1 * sr + b2 * s2);
        const double dr = 1.0 + a1 * cr + a2 * c2, di = -(a1 * sr + a2 * s2);
        return (float) (10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di + 1e-30) + 1e-30));
    }
};

// Big Knob bipolar: frequências de corte
inline float bigKnobLP (float v) { return v < 0 ? 20000.0f * std::pow (300.0f / 20000.0f, -v / 100.0f) : 20000.0f; }
inline float bigKnobHP (float v) { return v > 0 ? 20.0f * std::pow (3000.0f / 20.0f, v / 100.0f) : 10.0f; }

// curva de resposta (EQ ligado + Big Knob) para a interface
inline float responseDb (const Params& p, float f, float fs = 48000.0f)
{
    float d = 0;
    Biquad b;
    if (p.eqOn)
        for (int i = 0; i < kEqBands; ++i) { b.design (eqType (i), p.eq[(size_t) i].f, p.eq[(size_t) i].g, p.eq[(size_t) i].q, fs); d += b.magnitudeDb (f, fs); }
    if (std::abs (p.bk) > 0.5f)
    {
        b.design (EqLP, bigKnobLP (p.bk), 7, 1, fs); d += b.magnitudeDb (f, fs);
        b.design (EqHP, bigKnobHP (p.bk), 7, 1, fs); d += b.magnitudeDb (f, fs);
    }
    return d;
}

//==============================================================================
// ruído determinístico (o teste offline sai sempre igual)
struct Rng
{
    uint32_t s = 0x9e3779b9u;
    inline float uni() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    inline float bi() { return uni() * 2.0f - 1.0f; }
};

// corda abafada: Karplus-Strong com passa-baixas de um polo no loop (igual à versão para navegador)
class PickVoice
{
public:
    static constexpr int kMax = 8192;
    bool active = false;
    int midi = 0;
    void start (int m, float velocity, bool open, int gateSamples, const Params& p, float fs, Rng& rng)
    {
        midi = m; vel = velocity; isOpen = open;
        const float rate = 48000.0f / fs;
        const float mute = open ? p.mute * 0.25f : p.mute;
        const float g48 = open ? 0.8f : 0.62f - mute / 100.0f * 0.42f;
        const float loss48 = open ? 0.9985f : 0.9965f - mute / 100.0f * 0.016f;
        g = 1.0f - std::pow (1.0f - g48, rate);
        loss = std::pow (loss48, rate);
        const float P = fs / mtof ((float) m) - (1.0f - g) / g;
        N = std::clamp ((int) std::floor (P), 2, kMax - 2);
        fr = std::clamp (P - (float) N, 0.0f, 1.0f);
        const float a = 0.15f + p.atk / 100.0f * 0.8f;
        float lp = 0, pk = 0;
        for (int i = 0; i <= N; ++i) { lp += a * (rng.bi() - lp); buf[(size_t) i] = lp; }
        const int pp = std::max (1, (int) std::lround ((float) N * (0.06f + (0.45f - 0.06f) * p.pos / 100.0f)));
        for (int i = N; i >= pp; --i) buf[(size_t) i] -= buf[(size_t) (i - pp)];
        for (int i = 0; i <= N; ++i) pk = std::max (pk, std::abs (buf[(size_t) i]));
        const float nrm = pk > 1e-6f ? 1.0f / pk : 1.0f;
        for (int i = 0; i <= N; ++i) buf[(size_t) i] *= nrm;
        j = 0; prev = buf[0]; y = 0; n = 0;
        clickLen = (int) (0.005f * fs); clickAmt = 0.5f * p.atk / 100.0f; clickPrev = 0;
        gateEnd = (int) ((float) gateSamples * (open ? 1.6f : 1.0f));
        relLen = std::max (1, (int) (0.03f * fs));
        maxLen = (int) ((open ? 1.8f : 0.7f) * fs);
        active = true;
    }
    inline float tick (Rng& rng)
    {
        const float v = buf[(size_t) j];
        const float vv = v * (1.0f - fr) + prev * fr;
        prev = v;
        y += g * (vv - y);
        buf[(size_t) j] = y * loss;
        if (++j > N) j = 0;
        float s = v;
        if (n < clickLen) { const float w = rng.bi(); s += (w - clickPrev) * clickAmt * (1.0f - (float) n / (float) clickLen); clickPrev = w; }
        float env = 1.0f;
        if (n >= gateEnd) env = std::max (0.0f, 1.0f - (float) (n - gateEnd) / (float) relLen);
        ++n;
        if (n >= gateEnd + relLen || n >= maxLen) active = false;
        return s * vel * 0.7f * env;
    }
private:
    std::array<float, kMax> buf {};
    int N = 2, j = 0, n = 0, gateEnd = 0, relLen = 1, maxLen = 1, clickLen = 0;
    float fr = 0, prev = 0, y = 0, g = 0.5f, loss = 0.99f, vel = 1, clickAmt = 0, clickPrev = 0;
    bool isOpen = false;
};

// baixo guia (só com a fonte LINHA GUIA, para ouvir a palheta sozinho no standalone)
class BassVoice
{
public:
    bool active = false;
    void start (int m, float durSec, float sampleRate)
    {
        fs = sampleRate;
        f = mtof ((float) m); ph = 0; n = 0; dur = (int) (durSec * fs); rel = (int) (0.04f * fs); att = (int) (0.008f * fs);
        dec = std::exp (-1.0f / (0.15f * fs)); level = 0; lp.design (EqLP, 420, 0, 1, fs); active = true;
    }
    inline float tick()
    {
        ph += f / fs; if (ph >= 1) ph -= 1;
        const float tri = 4.0f * std::abs (ph - 0.5f) - 1.0f;
        const float x = std::sin (kTau * ph) + 0.35f * tri;
        if (n < att) level = 0.55f * (float) n / (float) att;
        else level = 0.32f + (level - 0.32f) * dec;
        float e = level;
        if (n > dur) e *= std::max (0.0f, 1.0f - (float) (n - dur) / (float) rel);
        ++n; if (n > dur + rel) active = false;
        return lp.process (x * e);
    }
private:
    float f = 110, ph = 0, fs = 48000, dec = 0.99f, level = 0;
    int n = 0, dur = 0, rel = 1, att = 1;
    Biquad lp;
};

//==============================================================================
// mola: pentes paralelos + passa-tudo (Schroeder) com cadeia de dispersão para o "drip" de mola
class Spring
{
public:
    void prepare (float fs)
    {
        const float cm[4] = { 29.7f, 37.1f, 41.1f, 43.7f }, ap[2] = { 5.0f, 1.7f };
        for (size_t ch = 0; ch < 2; ++ch)
        {
            for (size_t i = 0; i < 4; ++i) comb[ch][i].init ((int) ((cm[i] + (float) ch * 0.9f) * 0.001f * fs));
            for (size_t i = 0; i < 2; ++i) all[ch][i].init ((int) ((ap[i] + (float) ch * 0.13f) * 0.001f * fs));
            for (size_t i = 0; i < 6; ++i) disp[ch][i].init ((int) (2 + i + ch));
        }
    }
    inline void process (float in, float& l, float& r)
    {
        float o[2];
        for (size_t ch = 0; ch < 2; ++ch)
        {
            float x = in;
            for (auto& d : disp[ch]) x = d.ap (x, 0.62f);
            float s = 0;
            for (auto& c : comb[ch]) s += c.comb (x, 0.78f, 0.35f);
            s *= 0.25f;
            for (auto& a : all[ch]) s = a.ap (s, 0.5f);
            o[ch] = s;
        }
        l = o[0]; r = o[1];
    }
private:
    struct Line
    {
        std::vector<float> b; int i = 0; float lp = 0;
        void init (int n) { b.assign ((size_t) std::max (1, n), 0.0f); i = 0; lp = 0; }
        inline float comb (float x, float fb, float damp)
        { const float y = b[(size_t) i]; lp = y * (1 - damp) + lp * damp; b[(size_t) i] = x + lp * fb; if (++i >= (int) b.size()) i = 0; return y; }
        inline float ap (float x, float g)
        { const float d = b[(size_t) i]; const float v = x + g * d; b[(size_t) i] = v; if (++i >= (int) b.size()) i = 0; return d - g * v; }
    };
    std::array<std::array<Line, 4>, 2> comb;
    std::array<std::array<Line, 2>, 2> all;
    std::array<std::array<Line, 6>, 2> disp;
};

//==============================================================================
struct Transport { bool playing = false; double ppq = 0, bpm = 76; };
struct MidiIn { int offset = 0, note = 60; float vel = 1; bool on = true; };
struct NoteLog { double beat; int midi; float vel; bool open; bool bass; };

class Engine
{
public:
    // para a interface (lidos pelo editor sem trava)
    std::atomic<int> lastPick { -1 }, lastPickOpen { 0 }, pickCount { 0 }, curStep { -1 }, curCycle { -1 }, latchNote { -1 };
    std::vector<NoteLog>* log = nullptr; // só no teste offline

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        echo.prepare (sampleRate);
        spring.prepare (fs);
        pending.clear(); pending.reserve (1024); due.reserve (1024);
        for (auto& v : voices) v.active = false;
        for (auto& b : bass) b.active = false;
        for (auto& bq : chain) bq.reset();
        for (auto& bq : eq) bq.reset();
        bkL.reset(); bkH.reset(); satTone.reset();
        expected = -1; nextStep = 0;
        dirty = true;
    }

    void setParams (const Params& np)
    {
        const bool gridChanged = np.grid != p.grid || std::abs (np.dose - p.dose) > 1e-4f;
        p = np;
        if (gridChanged || dirty) autoPicaPau (p.grid, p.dose, autoSet);
        updateFilters();
        dirty = false;
    }
    const Params& params() const { return p; }
    const std::array<bool, kCycle>& autoPP() const { return autoSet; }

    // processa um bloco: L/R limpos na entrada (instrumento)
    void process (float* L, float* R, int numSamples, const Transport& t, const std::vector<MidiIn>& midi)
    {
        if (numSamples <= 0) return;
        const double bpm = std::clamp (t.bpm, 20.0, 300.0);
        const double beats = (double) numSamples / fs * bpm / 60.0;
        due.clear();

        if (t.playing)
        {
            if (std::abs (t.ppq - expected) > 1.0e-3)   // começou ou pulou: recomeça na semicolcheia seguinte
            {
                pending.clear();
                nextStep = (int64_t) std::ceil (t.ppq / kStepBeats - 1.0e-9);
            }
            const double end = t.ppq + beats;
            while ((double) nextStep * kStepBeats < end) scheduleStep (nextStep++, bpm);
            for (size_t i = 0; i < pending.size();)
            {
                if (pending[i].beat < end)
                {
                    const int off = std::clamp ((int) ((pending[i].beat - t.ppq) / beats * numSamples), 0, numSamples - 1);
                    due.push_back ({ off, pending[i] });
                    pending.erase (pending.begin() + (long) i);
                }
                else ++i;
            }
            expected = end;
            const int st = (int) (((int64_t) std::floor (t.ppq / kStepBeats)) % kSteps);
            curStep.store (st);
            curCycle.store ((int) (((int64_t) std::floor (t.ppq / kStepBeats)) % kCycle));
        }
        else
        {
            if (expected >= 0) pending.clear();
            expected = -1;
            curStep.store (-1); curCycle.store (-1);
        }

        // MIDI: a nota do baixo (do MRB-69, do MRM-0420 ou do teclado)
        for (auto& m : midi)
        {
            if (! m.on) continue;
            latch = m.note; latchNote.store (m.note);
            if (p.mode == Follow)
            {
                Ev e { 0, m.note + p.oct, m.vel, false, gateSec (bpm) * bpm / 60.0, false, 0 };
                due.push_back ({ std::clamp (m.offset, 0, numSamples - 1), e });
            }
        }
        std::sort (due.begin(), due.end(), [] (const Due& a, const Due& b) { return a.off < b.off; });

        // eco: tempo e envio
        dub::EchoSettings es;
        es.delaySec = (float) echoSeconds (p.echoSync, bpm);
        es.feedback = p.throwOn ? 0.78f : p.echoFb / 100.0f * 0.85f;
        es.toneHz = 2800.0f; es.wow = 0.25f; es.heads = 1; es.pingPong = false;
        es.send = p.throwOn ? 1.0f : (p.echoOn ? p.echoMix / 100.0f : 0.0f);
        echo.setSettings (es);

        const float springSend = p.spring / 100.0f * 0.5f * 0.5f;
        const float targetMaster = p.power ? p.master / 100.0f : 0.0f;
        const float mCoef = std::exp (-1.0f / (0.03f * fs));
        size_t di = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            while (di < due.size() && due[di].off <= i) trigger (due[di++].ev, bpm);
            float g = 0, b = 0;
            for (auto& v : voices) if (v.active) g += v.tick (rng);
            for (auto& v : bass) if (v.active) b += v.tick();
            // amp
            g = chain[0].process (g);           // HP 110
            g = chain[1].process (g);           // corpo
            g = chain[2].process (g);           // captador
            g = std::clamp (g * 1.4f, -1.0f, 1.0f);
            g = std::tanh (ampK * g) / std::tanh (ampK) * ampPost;
            g = chain[3].process (g);           // tom
            // EQ + Big Knob
            if (p.eqOn) for (auto& e : eq) g = e.process (g);
            g = bkH.process (bkL.process (g));
            // saturador
            if (p.satOn)
            {
                const float w = satTone.process (sat (g) * satPost);
                g = g * (1.0f - p.satMix / 100.0f) + w * (p.satMix / 100.0f);
            }
            float el = 0, er = 0;
            echo.processSample (g, g, el, er);
            float sl = 0, sr = 0;
            spring.process ((g + 0.5f * (el + er)) * springSend, sl, sr);
            masterSm = targetMaster + mCoef * (masterSm - targetMaster);
            L[i] += (g + el + sl * 0.5f + b * 0.8f) * masterSm;
            R[i] += (g + er + sr * 0.5f + b * 0.8f) * masterSm;
        }
        // reserva de -6 dB e limitador suave no fim (nunca passa de 0 dBFS)
        for (int i = 0; i < numSamples; ++i) { L[i] = softLimit (L[i] * kOutTrim); R[i] = softLimit (R[i] * kOutTrim); peakIn = std::max (peakIn, std::max (std::abs (L[i]), std::abs (R[i]))); }
    }

    static constexpr float kOutTrim = 0.5f;
    static inline float softLimit (float x) { const float a = std::abs (x); return a < 0.8f ? x : std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x); }
    float peakIn = 0;   // pico da saída (teste)

    // segundos do gate da palheta (a DURAÇÃO é % da semicolcheia, como na versão para navegador)
    double gateSec (double bpm) const { return 60.0 / bpm * kStepBeats * p.gate / 100.0 * 1.6; }

private:
    struct Ev { double beat; int midi; float vel; bool open; double gateBeats; bool bass; double bassBeats; };
    struct Due { int off; Ev ev; };

    void push (const Ev& e) { if (pending.size() < 1000) pending.push_back (e); }

    void scheduleStep (int64_t k, double bpm)
    {
        const int g16 = (int) (k % kSteps), st = (int) (k % kLineSteps), cyc = (int) (k % kCycle);
        const char c = p.grid[(size_t) g16];
        const double sd = kStepBeats;
        double t = (double) k * sd + ((k % 2) ? p.swing / 100.0 * sd * 0.33 : 0.0);
        const double gateB = sd * p.gate / 100.0 * 1.6;
        const float acc = 1.0f - p.acc / 100.0f * 0.45f;
        const auto& line = lines()[(size_t) std::clamp (p.line, 0, (int) lines().size() - 1)];
        const int root = bassRoot (p.key);

        int bm = -1, bdeg = 0;
        const LineEv* hit = nullptr;
        if (p.source == SrcGuide)
        {
            const LineEv* cur = nullptr;
            for (auto& e : line.ev) { if (e.step <= st) cur = &e; if (e.step == st) hit = &e; }
            if (cur) { bdeg = cur->deg; bm = deg2m (bdeg, root, p.scale); }
            if (hit && p.guideBass) push ({ t, deg2m (hit->deg, root, p.scale), 1, false, 0, true, hit->len * sd * 0.92 });
        }
        else if (latch >= 0) { bm = latch; bdeg = m2deg (latch, p.key, p.scale); }
        if (bm < 0) return;

        const double h = p.human / 100.0;
        auto human = [&] (double beat) { return beat + std::max (0.0, (double) rng.bi() * h * 0.012) * bpm / 60.0; };
        auto hv = [&] (float v) { return v * (1.0f - rng.uni() * (float) h * 0.25f); };

        if (p.mode == Follow)
        {
            if (p.source == SrcGuide && hit)
                push ({ human (t), deg2m (hit->deg, root, p.scale) + p.oct, hv ((g16 % 2) ? acc : 1.0f), c == 'o',
                        std::min (gateB, hit->len * sd * 0.9), false, 0 });
            return;
        }
        if (c == '.') return;
        int m = bm + p.oct;
        if (p.mode == Counter)
        {
            const auto& sh = shapes()[(size_t) std::clamp (p.shape, 0, (int) shapes().size() - 1)].d;
            int idx = 0;
            for (int i = 0; i < g16; ++i) if (p.grid[(size_t) i] != '.') ++idx;
            m = deg2m (bdeg + sh[(size_t) (idx % (int) sh.size())], root, p.scale) + p.oct;
        }
        if (isPicaPau (p, autoSet, cyc))
        {
            const int n = std::clamp (p.rep, 2, 4);
            const double sub = sd / n;
            for (int r = 0; r < n; ++r)
            {
                const double tk = t + r * sub + ((r % 2) ? p.ppSwing / 100.0 * sub * 0.5 : 0.0);
                const float v = r == 0 ? (c == 'X' ? 1.0f : acc) : acc * ((r % 2) ? 0.72f : 0.86f);
                push ({ human (tk), m, hv (v), false, std::min (gateB, sub * 0.8), false, 0 });
            }
        }
        else push ({ human (t), m, hv (c == 'X' ? 1.0f : acc), c == 'o', gateB, false, 0 });
    }

    void trigger (const Ev& e, double bpm)
    {
        if (log) log->push_back ({ e.beat, e.midi, e.vel, e.open, e.bass });
        if (e.bass)
        {
            for (auto& b : bass) if (! b.active) { b.start (e.midi, (float) (e.bassBeats * 60.0 / bpm), fs); return; }
            bass[0].start (e.midi, (float) (e.bassBeats * 60.0 / bpm), fs);
            return;
        }
        const int gs = std::max (1, (int) (e.gateBeats * 60.0 / bpm * fs));
        PickVoice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (! v) { v = &voices[(size_t) steal]; steal = (steal + 1) % (int) voices.size(); }
        v->start (std::clamp (e.midi, 24, 100), e.vel, e.open, gs, p, fs, rng);
        lastPick.store (e.midi); lastPickOpen.store (e.open ? 1 : 0); pickCount.fetch_add (1);
    }

    void updateFilters()
    {
        static const float puF[3] = { 480, 750, 1100 }, puG[3] = { 4, 4, 7 };
        chain[0].design (EqHP, 110, 0, 1, fs);                          // Q ≈ 0,707
        chain[1].design (EqPK, 220, -6.0f + 12.0f * p.body / 100.0f, 1.0f, fs);
        chain[2].design (EqPK, puF[std::clamp (p.pickup, 0, 2)], puG[std::clamp (p.pickup, 0, 2)], 0.9f, fs);
        chain[3].design (EqLP, 1400.0f + (7500.0f - 1400.0f) * p.tone / 100.0f, 0.0f, 1, fs);
        ampK = 1.0f + 7.0f * p.drive / 100.0f;
        ampPost = 0.9f / std::sqrt (ampK * 0.6f + 0.4f);
        for (int i = 0; i < kEqBands; ++i) eq[(size_t) i].design (eqType (i), p.eq[(size_t) i].f, p.eq[(size_t) i].g, p.eq[(size_t) i].q, fs);
        bkL.design (EqLP, bigKnobLP (p.bk), 7, 1, fs);
        bkH.design (EqHP, bigKnobHP (p.bk), 7, 1, fs);
        // saturador
        satK = 1.2f + (14.0f - 1.2f) * p.satDrive / 100.0f;
        satNorm = 1.0f;
        const float m = std::max (std::abs (satRaw (1.0f)), std::abs (satRaw (-1.0f)));
        satNorm = m > 1e-6f ? 1.0f / m : 1.0f;
        satPost = std::pow (10.0f, p.satOut / 20.0f) / std::sqrt (satK * 0.5f + 0.5f);
        static const float tf[3] = { 7500, 11000, 15000 };
        satTone.design (EqLP, tf[std::clamp (p.satType, 0, 2)], 0, 1, fs);
    }
    inline float satRaw (float x) const
    {
        x = std::clamp (x, -1.0f, 1.0f);
        const float k = satK;
        if (p.satType == Fita) return std::tanh (k * x * 0.8f) / std::tanh (k * 0.8f) * 0.92f + 0.08f * x;
        if (p.satType == Valvula) return std::tanh (k * (x + 0.18f)) - std::tanh (k * 0.18f);
        float y = std::clamp (x * k * 0.5f, -1.0f, 1.0f);
        return y - y * y * y / 3.0f;
    }
    inline float sat (float x) const { return satRaw (x) * satNorm; }

    float fs = 48000.0f;
    Params p;
    bool dirty = true;
    std::array<bool, kCycle> autoSet {};
    std::array<PickVoice, 16> voices;
    std::array<BassVoice, 4> bass;
    int steal = 0, latch = -1;
    Rng rng;
    std::vector<Ev> pending;
    std::vector<Due> due;
    double expected = -1;
    int64_t nextStep = 0;
    std::array<Biquad, 4> chain;
    std::array<Biquad, kEqBands> eq;
    Biquad bkL, bkH, satTone;
    float ampK = 1, ampPost = 0.9f, satK = 1, satNorm = 1, satPost = 1, masterSm = 0;
    dub::TapeEcho echo;
    Spring spring;
};

} // namespace mpg
