#pragma once
// Mastor Reggae Guitar MRG-68 — motor de som e de groove (C++ puro, sem JUCE: o teste offline compila só isto).
// O mesmo motor da versão para celular (site/web/reggaeguitar/index.html): ao mudar um, mude o outro.
//
// Contagem da Dub Station: caixa no 2 e no 4 (BPM até 100). O skank cai em todos os "e" (passos 3, 7, 11, 15
// da grade de 16 semicolcheias); o bang-bang é downstroke acentuado no "e" + upstroke nas 3 cordas agudas
// na semicolcheia seguinte (1/16 depois), mais fraco, com a mão esquerda abafando entre os dois.
//
// Som: corda em guia de onda (duas polarizações, afinação fracionária com allpass, abafamento contínuo),
// palhetada com posição, captador (comb da posição + ressonância RLC), Fender Twin (tone stack blackface
// calculado do circuito, válvulas, 2x12) ou Roland JC-120 (com chorus), wah fixo, phaser, chorus, mola,
// eco de fita, saturador e limitador.

#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace mrg
{
constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------- dados
enum Guitar { GTele68, GLPSpecial, GStrat, NumGuitars };
enum Pickup { PkPonte, PkMeio, PkBraco, NumPickups };
enum Amp { AmpTwin, AmpJC, NumAmps };
enum Stroke { StSimples, StBangBang, StShuffle, NumStrokes };
enum Mute { MuSeco, MuPalm, MuMaoEsq, MuAberto, NumMutes };
enum Art { ArtSkank, ArtMaoEsq, ArtPalm, ArtTremolo, ArtSustentada, NumArts };
enum VarType { VarLick, VarScratch, VarBangBang, VarFlyer, VarPausa, VarEditada, NumVarTypes };
// passos da grade principal: 0 vazio, 1 skank, 2 chop
// passos do compasso da variação: 0 vazio, 1 skank, 2 bang-bang, 3 skank longo, 4 chop, 5 nota
enum FillStep { FVazio, FSkank, FBang, FLongo, FChop, FNota, NumFillSteps };

inline const char* guitarName (int i) { static const char* n[] = { "Tele 68", "LP Special", "Strat" }; return n[std::clamp (i, 0, 2)]; }
inline const char* pickupName (int i) { static const char* n[] = { "Ponte", "Meio", "Braco" }; return n[std::clamp (i, 0, 2)]; }
inline const char* ampName (int i)    { static const char* n[] = { "Twin Reverb", "JC-120" }; return n[std::clamp (i, 0, 1)]; }
inline const char* strokeName (int i) { static const char* n[] = { "Simples", "Bang-bang", "Shuffle" }; return n[std::clamp (i, 0, 2)]; }
inline const char* muteName (int i)   { static const char* n[] = { "Seco", "Palm mute", "Mao esquerda", "Aberto" }; return n[std::clamp (i, 0, 3)]; }
inline const char* artName (int i)    { static const char* n[] = { "Skank seco", "Mao esquerda", "Palm mute", "Tremolo", "Sustentada" }; return n[std::clamp (i, 0, 4)]; }
inline const char* varName (int i)    { static const char* n[] = { "Lick", "Scratch", "Bang-bang", "Flyer", "Pausa", "Editada" }; return n[std::clamp (i, 0, 5)]; }

struct GuitarModel { double f0, q, out; };
inline const GuitarModel& guitarModel (int g)
{
    static const GuitarModel m[NumGuitars] = { { 4200, 2.4, 1.0 }, { 2900, 2.0, 1.25 }, { 4800, 2.2, 0.9 } };
    return m[std::clamp (g, 0, NumGuitars - 1)];
}
struct PickupModel { double pos, f; };
inline const PickupModel& pickupModel (int p)
{
    static const PickupModel m[NumPickups] = { { 0.075, 1.0 }, { 0.16, 0.95 }, { 0.26, 0.85 } };
    return m[std::clamp (p, 0, NumPickups - 1)];
}

struct Track { const char* name; const char* info; int bpm; };
struct Player
{
    const char* name;
    Track tracks[3]; int numTracks;
    int guitar, pickup, amp, stroke, mute; bool chop;
    bool wah, phaser, chorus; int var;
};
enum { NumPlayers = 12 };
inline const Player& player (int i)
{
    static const Player p[NumPlayers] = {
        { "Ernest Ranglin", { { "My Boy Lollipop", "Millie Small - 1964", 98 }, { "54-46 Was My Number", "Toots - 1968", 84 } }, 2, GStrat, PkMeio, AmpJC, StShuffle, MuSeco, false, false, false, true, VarLick },
        { "Lynn Taitt", { { "Take It Easy", "Hopeton Lewis - 1966", 75 }, { "007 (Shanty Town)", "Desmond Dekker - 1967", 82 }, { "Israelites", "Desmond Dekker - 1968", 90 } }, 3, GTele68, PkPonte, AmpTwin, StSimples, MuPalm, false, false, false, false, VarLick },
        { "Hux Brown", { { "Pressure Drop", "Toots - 1969", 92 }, { "Monkey Man", "Toots - 1969", 94 }, { "Funky Kingston", "Toots - 1972", 96 } }, 3, GTele68, PkPonte, AmpTwin, StBangBang, MuMaoEsq, false, false, false, false, VarScratch },
        { "Peter Tosh", { { "Get Up, Stand Up", "Wailers - 1973", 76 }, { "I Shot the Sheriff", "Wailers - 1973", 92 } }, 2, GLPSpecial, PkPonte, AmpTwin, StSimples, MuSeco, false, true, false, false, VarPausa },
        { "Bob Marley", { { "Is This Love", "Wailers - 1978", 62 } }, 1, GLPSpecial, PkMeio, AmpTwin, StSimples, MuMaoEsq, false, false, false, false, VarPausa },
        { "Chinna Smith", { { "Rat Race", "Wailers - 1976", 76 }, { "Cassandra", "Dennis Brown - 1973", 72 }, { "Marcus Garvey", "Burning Spear - 1975", 76 } }, 3, GTele68, PkPonte, AmpTwin, StSimples, MuSeco, false, false, false, false, VarLick },
        { "Tony Chin", { { "Dreamland", "Bunny Wailer - 1971", 70 }, { "Small Axe", "Wailers - 1971", 72 } }, 2, GTele68, PkMeio, AmpTwin, StSimples, MuSeco, false, false, false, false, VarFlyer },
        { "Bingy Bunny", { { "Bounty Hunter", "Barrington Levy - 1979", 76 }, { "Night Nurse", "Gregory Isaacs - 1982", 74 }, { "Big Ship", "Freddie McGregor - 1982", 76 } }, 3, GTele68, PkPonte, AmpTwin, StBangBang, MuSeco, false, false, false, false, VarBangBang },
        { "Dwight Pinkney", { { "This Is Reggae Music", "Zap Pow - 1974", 80 }, { "Night Nurse (lead)", "Gregory Isaacs - 1982", 74 } }, 2, GStrat, PkBraco, AmpTwin, StSimples, MuPalm, false, false, false, false, VarLick },
        { "Mikey Chung", { { "Sponji Reggae", "Black Uhuru - 1981", 80 }, { "Youth of Eglington", "Black Uhuru - 1981", 74 } }, 2, GStrat, PkMeio, AmpJC, StBangBang, MuSeco, false, false, true, true, VarBangBang },
        { "Al Anderson", { { "No Woman, No Cry (Live!)", "Wailers - 1975", 77 }, { "Crazy Baldhead", "Wailers - 1976", 80 } }, 2, GLPSpecial, PkBraco, AmpTwin, StSimples, MuAberto, false, false, false, false, VarLick },
        { "Junior Marvin", { { "Exodus", "Wailers - 1977", 78 }, { "Jamming", "Wailers - 1977", 62 } }, 2, GStrat, PkBraco, AmpJC, StSimples, MuAberto, false, false, false, true, VarLick },
    };
    return p[std::clamp (i, 0, NumPlayers - 1)];
}

struct VarPreset { int start; int s[16]; };
inline const VarPreset& varPreset (int v)
{
    static const VarPreset p[5] = {
        { 3, { 0,0,1,0, 0,0,1,0, 5,5,5,0, 5,5,5,5 } },   // Lick: frase de nota única nos tempos 3 e 4
        { 4, { 0,0,1,0, 0,0,1,0, 0,0,1,0, 4,4,1,4 } },   // Scratch: chops abafados no tempo 4
        { 1, { 0,0,2,0, 0,0,2,0, 0,0,2,0, 0,0,2,0 } },   // Bang-bang: o compasso inteiro
        { 3, { 0,0,1,0, 0,0,1,0, 0,0,3,0, 0,0,3,0 } },   // Flyer: skank longo no 3 e no 4
        { 3, { 0,0,1,0, 0,0,1,0, 0,0,3,0, 0,0,0,0 } },   // Pausa: tira o do 4, deixa o do 3 soar
    };
    return p[std::clamp (v, 0, 4)];
}
// notas da frase (lick) por passo
inline int lickNote (int step)
{
    static const int n[16] = { 64, 62, 60, 57, 64, 62, 60, 57, 76, 74, 72, 72, 69, 67, 64, 62 };
    return n[step & 15];
}

// voicings reais de skank (4 cordas agudas, 5ª–7ª casa) para tônica e tipo
inline std::array<int, 4> chordVoicing (int root, bool minor)
{
    // forma de pestana tipo "A" nas 4 agudas: 5ª, fundamental, 3ª, 5ª (ou fundamental em cima)
    int r = 57 + ((root - 9) % 12 + 12) % 12;          // tônica entre A3 e G#4
    if (r > 63) r -= 12;
    const int third = minor ? 3 : 4;
    std::array<int, 4> v { r, r + 7, r + 12, r + 12 + third };
    // mantém o acorde na região do braço (A3..C5 no grave)
    while (v[0] < 52) for (auto& n : v) n += 12;
    while (v[0] > 62) for (auto& n : v) n -= 12;
    std::sort (v.begin(), v.end());
    return v;
}

// ---------------------------------------------------------------- utilidades
struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed = 7) : s ((seed * 2654435761u) != 0u ? seed * 2654435761u : 7u) {}
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) ((double) s / 4294967296.0 * 2.0 - 1.0); }
    float uni() { return 0.5f * (next() + 1.0f); }
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] {}, z2[2] {};
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    float process (float x, int ch)
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }
    void set (double nb0, double nb1, double nb2, double a0, double na1, double na2)
    { b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0; }
    void lowpass (double sr, double f, double q)
    { const double w = 2 * kPi * std::min (f, sr * 0.45) / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
      set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
    void highpass (double sr, double f, double q)
    { const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
      set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
    void bandpass (double sr, double f, double q)
    { const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
      set (al, 0, -al, 1 + al, -2 * c, 1 - al); }
    void peaking (double sr, double f, double q, double db)
    { const double A = std::pow (10.0, db / 40), w = 2 * kPi * std::min (f, sr * 0.45) / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
      set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A); }
    void lowshelf (double sr, double f, double db, double S = 0.8)
    { const double A = std::pow (10.0, db / 40), w = 2 * kPi * f / sr, c = std::cos (w);
      const double al = std::sin (w) / 2 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), sa = 2 * std::sqrt (A) * al;
      set (A * ((A + 1) - (A - 1) * c + sa), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sa),
           (A + 1) + (A - 1) * c + sa, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sa); }
    void allpass (double sr, double f, double q)
    { const double w = 2 * kPi * std::min (f, sr * 0.45) / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
      set (1 - al, -2 * c, 1 + al, 1 + al, -2 * c, 1 - al); }
};

// ---------------------------------------------------------------- Fender Twin: tone stack blackface (AB763)
struct Iir3 { double b[4] {}, a[4] {}; double z[2][3] {};
    float process (float x, int ch)
    { const double y = b[0] * x + z[ch][0];
      z[ch][0] = b[1] * x - a[1] * y + z[ch][1];
      z[ch][1] = b[2] * x - a[2] * y + z[ch][2];
      z[ch][2] = b[3] * x - a[3] * y;
      return (float) y; }
};
inline void toneStack (Iir3& f, double t, double m, double l, double sr)
{
    const double C1 = 250e-12, C2 = 0.1e-6, C3 = 0.047e-6, R1 = 250e3, R2 = 250e3, R3 = 10e3, R4 = 100e3;
    l = std::pow (std::max (l, 1e-3), 2.0);   // potenciômetro de graves é logarítmico
    const double b1 = t*C1*R1 + m*C3*R3 + l*(C1*R2 + C2*R2) + (C1*R3 + C2*R3);
    const double b2 = t*(C1*C2*R1*R4 + C1*C3*R1*R4) - m*m*(C1*C3*R3*R3 + C2*C3*R3*R3) + m*(C1*C3*R1*R3 + C1*C3*R3*R3 + C2*C3*R3*R3)
                    + l*(C1*C2*R1*R2 + C1*C2*R2*R4 + C1*C3*R2*R4) + l*m*(C1*C3*R2*R3 + C2*C3*R2*R3) + (C1*C2*R1*R3 + C1*C2*R3*R4 + C1*C3*R3*R4);
    const double b3 = l*m*(C1*C2*C3*R1*R2*R3 + C1*C2*C3*R2*R3*R4) - m*m*(C1*C2*C3*R1*R3*R3 + C1*C2*C3*R3*R3*R4)
                    + m*(C1*C2*C3*R1*R3*R3 + C1*C2*C3*R3*R3*R4) + t*C1*C2*C3*R1*R3*R4 - t*m*C1*C2*C3*R1*R3*R4 + t*l*C1*C2*C3*R1*R2*R4;
    const double a1 = (C1*R1 + C1*R3 + C2*R3 + C2*R4 + C3*R4) + m*C3*R3 + l*(C1*R2 + C2*R2);
    const double a2 = m*(C1*C3*R1*R3 - C2*C3*R3*R4 + C1*C3*R3*R3 + C2*C3*R3*R3) + l*m*(C1*C3*R2*R3 + C2*C3*R2*R3) - m*m*(C1*C3*R3*R3 + C2*C3*R3*R3)
                    + l*(C1*C2*R2*R4 + C1*C2*R1*R2 + C1*C3*R2*R4 + C2*C3*R2*R4) + (C1*C2*R1*R4 + C1*C3*R1*R4 + C1*C2*R3*R4 + C1*C2*R1*R3 + C1*C3*R3*R4 + C2*C3*R3*R4);
    const double a3 = l*m*(C1*C2*C3*R1*R2*R3 + C1*C2*C3*R2*R3*R4) - m*m*(C1*C2*C3*R1*R3*R3 + C1*C2*C3*R3*R3*R4)
                    + m*(C1*C2*C3*R3*R3*R4 + C1*C2*C3*R1*R3*R3 - C1*C2*C3*R1*R3*R4) + l*C1*C2*C3*R1*R2*R4 + C1*C2*C3*R1*R3*R4;
    const double K = 2 * sr, K2 = K * K, K3 = K2 * K;
    const double bb[4] = { b3, b2, b1, 0 }, aa[4] = { a3, a2, a1, 1 };
    auto bl = [&] (const double* c, double* o)
    {
        o[0] =  c[0]*K3 + c[1]*K2 + c[2]*K + c[3];
        o[1] = -3*c[0]*K3 - c[1]*K2 + c[2]*K + 3*c[3];
        o[2] =  3*c[0]*K3 - c[1]*K2 - c[2]*K + 3*c[3];
        o[3] = -c[0]*K3 + c[1]*K2 - c[2]*K + c[3];
    };
    double B[4], A[4];
    bl (bb, B); bl (aa, A);
    for (int i = 0; i < 4; ++i) { f.b[i] = B[i] / A[0]; f.a[i] = A[i] / A[0]; }
}

inline float tube (float x, float drive, float asym)
{
    const float o = std::tanh (drive * asym);
    return (std::tanh (drive * (x + asym)) - o) / drive;
}

// ---------------------------------------------------------------- uma polarização da corda
struct Waveguide
{
    static constexpr int kMax = 4096;
    float dl[kMax] {}; float exc[kMax] {};
    int N = 100, wi = 0, excLen = 0, pos = 0, muteAt = -1;
    double c = 0, wPrev = 0, lPrev = 0, aPrev = 0, ring = 0.9965, muteG = 0.8, bright = 0.5;

    void start (double sr, double hz, float vel, double pickPos, float dark, uint32_t seed)
    {
        const double P = sr / hz, s0 = 0.5;
        N = std::clamp ((int) std::floor (P - s0 - 0.1), 4, kMax - 4);
        const double d = P - s0 - N;
        c = (1 - d) / (1 + d);
        std::fill (dl, dl + kMax, 0.0f);
        wi = 0; wPrev = lPrev = aPrev = 0; pos = 0;
        excLen = N;
        Rng r (seed);
        const int k = std::max (2, (int) std::floor (pickPos * N));
        const double a = 0.55 - 0.35 * vel + dark * 0.3;
        double lp = 0, mean = 0;
        for (int i = 0; i < N; ++i)
        {
            const double tri = i < k ? (double) i / k : 1.0 - (double) (i - k) / (N - k);
            lp = (1 - a) * r.next() + a * lp;
            exc[i] = (float) (0.55 * tri + 0.45 * lp);
            mean += exc[i];
        }
        mean /= N;
        for (int i = 0; i < N; ++i) exc[i] -= (float) mean;
        for (int i = N - 1; i >= k; --i) exc[i] -= 0.9f * exc[i - k];   // posição da palhetada (de trás para frente: usa o original)
        const double fc = (1800.0 + 2600.0 * vel) * (1.0 - dark * 0.5), ap = std::exp (-2 * kPi * fc / sr);
        for (int pass = 0; pass < 2; ++pass)
        { double y = 0; for (int i = 0; i < N; ++i) { y = (1 - ap) * exc[i] + ap * y; exc[i] = (float) y; } }
        for (int i = 0; i < N; ++i) exc[i] *= vel;
    }
    void retune (double sr, double hz)   // bend e vibrato nas notas de lead (mantém a corda vibrando)
    {
        const double P = sr / hz, s0 = 0.5;
        N = std::clamp ((int) std::floor (P - s0 - 0.1), 4, kMax - 4);
        const double d = P - s0 - N;
        c = (1 - d) / (1 + d);
    }
    inline float tick()
    {
        const bool muted = muteAt >= 0 && pos >= muteAt;
        const double g = muted ? muteG : ring, s = muted ? 0.5 : bright;
        const double w = dl[(wi + kMax - N) % kMax];
        const double l = (1 - s) * w + s * wPrev; wPrev = w;
        const double ap = c * l + lPrev - c * aPrev; lPrev = l; aPrev = ap;
        const double y = (pos < excLen ? exc[pos] : 0.0f) + g * ap;
        dl[wi] = (float) y; wi = (wi + 1) % kMax; ++pos;
        return (float) y;
    }
};

// ---------------------------------------------------------------- uma corda tocada (duas polarizações + captador)
struct StringVoice
{
    Waveguide p0, p1;
    float hist[Waveguide::kMax] {}; int hi = 0, q = 10;
    bool active = false, held = false;
    int delay = 0, age = 0, silent = 0, note = -1, tag = 0;
    float gain = 1, panL = 0.7f, panR = 0.7f;
    float noiseEnv = 0, relEnv = 0; int relAt = -1; Rng nz { 3 };
    double baseHz = 110, bendRatio = 1;

    void start (double sr, int midi, float vel, int holdSamples, int delaySamples, double pickPos, double pickupPos,
                float dark, double ring, double muteG, float pan, uint32_t seed, int noteTag, bool sustain)
    {
        baseHz = 440.0 * std::pow (2.0, (midi - 69) / 12.0);
        const double bright = std::min (0.62, midi < 60 ? 0.5 : 0.5 + 0.004 * (midi - 60));
        p0.start (sr, baseHz * std::pow (2.0, -0.7 / 1200.0), vel, pickPos, dark, seed * 31 + 1);
        p1.start (sr, baseHz * std::pow (2.0, 0.7 / 1200.0), vel, pickPos, dark, seed * 31 + 8);
        for (auto* w : { &p0, &p1 }) { w->ring = ring; w->muteG = muteG; w->bright = bright; w->muteAt = sustain ? -1 : holdSamples; }
        q = std::max (1, (int) std::floor (pickupPos * sr / baseHz));
        std::fill (hist, hist + Waveguide::kMax, 0.0f); hi = 0;
        delay = delaySamples; age = 0; silent = 0; note = midi; tag = noteTag; held = sustain;
        gain = 1; panL = (float) std::cos ((pan + 1) * kPi / 4); panR = (float) std::sin ((pan + 1) * kPi / 4);
        noiseEnv = 0.05f * vel; relAt = sustain ? -1 : holdSamples; relEnv = 0; nz = Rng (seed + 99);
        bendRatio = 1; active = true;
    }
    void setPitch (double sr, double ratio)
    {
        p0.retune (sr, baseHz * ratio * std::pow (2.0, -0.7 / 1200.0));
        p1.retune (sr, baseHz * ratio * std::pow (2.0, 0.7 / 1200.0));
    }
    void release() { if (! active) return; held = false; p0.muteAt = p1.muteAt = p0.pos; relAt = age; }
    inline void tick (float& L, float& R)
    {
        if (! active) return;
        if (delay > 0) { --delay; return; }
        float y = 0.62f * p0.tick() + 0.38f * p1.tick();
        const float out = y - 0.7f * hist[(hi + Waveguide::kMax - q) % Waveguide::kMax];
        hist[hi] = y; hi = (hi + 1) % Waveguide::kMax;
        float s = out * gain;
        if (noiseEnv > 1.0e-4f) { s += noiseEnv * nz.next() * 0.6f; noiseEnv *= 0.993f; }   // ruído da palheta
        if (relAt >= 0 && age == relAt) relEnv = 0.02f;
        if (relEnv > 1.0e-4f) { s += relEnv * nz.next(); relEnv *= 0.996f; }                // ruído da mão soltando
        L += s * panL; R += s * panR;
        ++age;
        silent = std::fabs (out) < 1.0e-5f ? silent + 1 : 0;
        if (silent > 2000 && ! held && age > 4000) active = false;
    }
};

// ---------------------------------------------------------------- bateria e baixo de referência (acompanhamento)
struct Band
{
    double sr = 48000;
    struct Hit { int kind = 0; int t = -1; float v = 0; int delay = 0; Rng r { 5 }; };
    std::array<Hit, 32> hits;
    struct Bass { double ph = 0, hz = 55; int t = -1, len = 0, delay = 0; } bass[4];
    void hit (int kind, float v, int delay)
    {
        for (auto& h : hits) if (h.t < 0) { h.kind = kind; h.t = 0; h.v = v; h.delay = delay; h.r = Rng ((uint32_t) (kind * 77 + delay + 3)); return; }
    }
    void note (int midi, double beats, double secPerBeat, int delay)
    {
        for (auto& b : bass) if (b.t < 0) { b.hz = 440.0 * std::pow (2.0, (midi - 69) / 12.0); b.ph = 0; b.t = 0; b.len = (int) (beats * secPerBeat * sr); b.delay = delay; return; }
    }
    float tick()
    {
        float s = 0;
        for (auto& h : hits)
        {
            if (h.t < 0) continue;
            if (h.delay > 0) { --h.delay; continue; }
            const double t = h.t / sr;
            if (h.kind == 0)        // bumbo
            { const double f = 46 + 70 * std::exp (-t * 30); s += (float) (0.9 * h.v * std::sin (2 * kPi * (46 * t + 70.0 / 30.0 * (1 - std::exp (-t * 30)))) * std::exp (-t * 8)); (void) f; if (t > 0.45) h.t = -2; }
            else if (h.kind == 1)   // aro (caixa no 2 e no 4)
            { s += (float) (h.v * (0.4 * std::sin (2 * kPi * 480 * t) + 0.25 * std::sin (2 * kPi * 1700 * t) + 0.3 * h.r.next()) * std::exp (-t * 55)); if (t > 0.12) h.t = -2; }
            else                    // chimbal
            { s += (float) (h.v * 0.1 * h.r.next() * std::exp (-t * 110)); if (t > 0.05) h.t = -2; }
            if (h.t >= 0) ++h.t; else h.t = -1;
        }
        for (auto& b : bass)
        {
            if (b.t < 0) continue;
            if (b.delay > 0) { --b.delay; continue; }
            const double t = b.t / sr;
            double env = std::min (1.0, t / 0.005) * (0.35 + 0.4 * std::exp (-t * 4));
            if (b.t > b.len - (int) (0.02 * sr)) env *= std::max (0.0, (double) (b.len - b.t) / (0.02 * sr));
            b.ph += b.hz / sr;
            s += (float) (0.8 * env * (std::sin (2 * kPi * b.ph) + 0.22 * std::sin (4 * kPi * b.ph)));
            if (++b.t >= b.len) b.t = -1;
        }
        return s;
    }
};

// ---------------------------------------------------------------- mola do Twin (3 molas com dispersão)
struct Spring
{
    struct Line { std::vector<float> buf; int w = 0, len = 1; double ap[6] {}, apY[6] {}; double lp = 0; };
    Line lines[2][3];
    Biquad hp, lp;
    void prepare (double sr)
    {
        const double D[3] = { 0.0331, 0.0389, 0.0437 };
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 3; ++i)
            {
                auto& l = lines[ch][i];
                l.len = (int) (D[i] * sr * (1.0 + 0.013 * ch * (i + 1)));
                l.buf.assign ((size_t) l.len + 1, 0.0f); l.w = 0;
                for (auto& x : l.ap) x = 0;
                for (auto& x : l.apY) x = 0;
                l.lp = 0;
            }
        hp.highpass (sr, 350, 0.7); lp.lowpass (sr, 4200, 0.7); hp.reset(); lp.reset();
    }
    float process (float x, int ch)
    {
        float out = 0;
        const double g[3] = { 0.80, 0.78, 0.76 }, apc[3] = { 0.68, 0.64, 0.71 };
        for (int i = 0; i < 3; ++i)
        {
            auto& l = lines[ch][i];
            double v = l.buf[(size_t) l.w];
            for (int k = 0; k < 6; ++k)      // dispersão: o "boing" da mola
            { const double y = apc[i] * v + l.ap[k] - apc[i] * l.apY[k]; l.ap[k] = v; l.apY[k] = y; v = y; }
            l.lp = 0.7 * v + 0.3 * l.lp;
            out += (float) l.lp;
            l.buf[(size_t) l.w] = (float) (x - g[i] * l.lp);
            l.w = (l.w + 1) % l.len;
        }
        return lp.process (hp.process (out * 0.6f, ch), ch);
    }
};

// ---------------------------------------------------------------- cadeia: captador → wah → phaser → ampli → chorus → mola → eco → saturador
struct SoundParams
{
    int guitar = GTele68, pickup = PkPonte, amp = AmpTwin;
    float gvol = 0.8f, tone = 0.6f, treble = 0.4f, middle = 1.0f, bass = 0.6f, reverb = 0.3f;
    bool wahOn = false, phOn = false, chOn = false, molaOn = true, echoOn = false, satOn = false;   // Tape Echo e saturador em bypass
    float wah = 0.45f, ph = 0.4f, ch = 0.5f, mola = 0.35f, echo = 0.35f, sat = 0.4f, out = 0.7f;
    double bpm = 76;
};

struct Chain
{
    double sr = 48000;
    Biquad pickup, wah, ph[4], twinHp, shelf, cab[6], jc[4];
    Iir3 ts;
    Spring spring;
    std::vector<float> chBuf[2], echoBuf[2];
    int chW = 0, echoW = 0;
    double phLfo = 0, chLfo = 0, echoLp[2] {}, lim = 1.0;
    float tsKey[3] { -1, -1, -1 };
    SoundParams p;

    void prepare (double rate)
    {
        sr = rate;
        for (int i = 0; i < 2; ++i) { chBuf[i].assign ((size_t) (0.05 * sr) + 4, 0.0f); echoBuf[i].assign ((size_t) (2.1 * sr), 0.0f); echoLp[i] = 0; }
        chW = echoW = 0; phLfo = chLfo = 0; lim = 1.0;
        twinHp.highpass (sr, 40, 0.7);
        shelf.lowshelf (sr, 1100, 11);
        cab[0].highpass (sr, 85, 0.7); cab[1].peaking (sr, 140, 1.2, 6); cab[2].peaking (sr, 2600, 1.3, 2);
        cab[3].peaking (sr, 5200, 2.5, 1); cab[4].lowpass (sr, 5000, 0.54); cab[5].lowpass (sr, 5000, 1.3);
        jc[0].highpass (sr, 60, 0.7); jc[1].peaking (sr, 900, 0.7, 2.5); jc[2].lowpass (sr, 7000, 0.54); jc[3].lowpass (sr, 7000, 1.3);
        spring.prepare (sr);
        tsKey[0] = -1;
        update (p);
    }
    void update (const SoundParams& np)
    {
        p = np;
        const auto& g = guitarModel (p.guitar);
        const auto& pk = pickupModel (p.pickup);
        pickup.lowpass (sr, g.f0 * pk.f * (0.55 + 0.45 * p.tone), g.q * (0.6 + 0.4 * p.tone));
        wah.peaking (sr, 450 + p.wah * 1800, 3.0, p.wahOn ? 15.0 : 0.0);
        if (std::fabs (tsKey[0] - p.treble) > 1e-6f || std::fabs (tsKey[1] - p.middle) > 1e-6f || std::fabs (tsKey[2] - p.bass) > 1e-6f)
        {
            toneStack (ts, p.treble, p.middle, p.bass, sr);
            tsKey[0] = p.treble; tsKey[1] = p.middle; tsKey[2] = p.bass;
        }
    }
    void process (float* L, float* R, int n)
    {
        const auto& g = guitarModel (p.guitar);
        const float inGain = (float) (0.22 * g.out * (0.3 + 0.9 * p.gvol));
        const double beat = 60.0 / std::clamp (p.bpm, 40.0, 200.0);
        const int echoDelay = std::clamp ((int) (beat * 0.75 * sr), 1, (int) echoBuf[0].size() - 2);
        const float echoFb = 0.2f + p.echo * 0.5f, satDrive = 1.0f + p.sat * 6.0f;
        const float molaWet = p.molaOn ? p.mola * 0.7f * (0.5f + p.reverb) : 0.0f;
        const double phRate = 0.15 + p.ph * 1.2, chRate = 0.3 + p.ch * 1.5;
        for (int i = 0; i < n; ++i)
        {
            if ((i & 31) == 0 && p.phOn)
            {
                const double f = 800 + 600 * std::sin (2 * kPi * phLfo);
                for (auto& a : ph) a.allpass (sr, std::max (120.0, f), 0.7);
            }
            phLfo += phRate / sr; if (phLfo >= 1) phLfo -= 1;
            chLfo += chRate / sr; if (chLfo >= 1) chLfo -= 1;
            float x[2] = { L[i] * inGain, R[i] * inGain };
            for (int ch = 0; ch < 2; ++ch)
            {
                float v = pickup.process (x[ch], ch);
                v = wah.process (v, ch);
                if (p.phOn) { float a = v; for (auto& f : ph) a = f.process (a, ch); v = 0.7f * v + 0.7f * a; }
                if (p.amp == AmpTwin)
                {
                    float t = twinHp.process (v, ch);
                    t = tube (t * 1.4f, 1.3f, 0.08f);
                    t = ts.process (t, ch) * 6.0f;
                    t = shelf.process (t, ch);
                    t = tube (t, 1.1f, 0.03f);
                    for (auto& f : cab) t = f.process (t, ch);
                    v = t;
                }
                else
                {
                    float t = v;
                    for (auto& f : jc) t = f.process (t, ch);
                    v = t * 1.6f;
                }
                x[ch] = v;
            }
            // chorus estéreo (o do JC-120)
            {
                const float mono = 0.5f * (x[0] + x[1]);
                chBuf[0][(size_t) chW] = mono; chBuf[1][(size_t) chW] = mono;
                if (p.chOn)
                {
                    const float wet = 0.55f + p.ch * 0.4f;
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        const double d = (0.012 + (ch ? -1 : 1) * 0.0011 * std::sin (2 * kPi * chLfo)) * sr;
                        double rp = chW - d; while (rp < 0) rp += (double) chBuf[ch].size();
                        const int i0 = (int) rp; const double fr = rp - i0;
                        const float a = chBuf[ch][(size_t) i0 % chBuf[ch].size()], b = chBuf[ch][(size_t) (i0 + 1) % chBuf[ch].size()];
                        x[ch] = 0.75f * x[ch] + wet * (float) (a + (b - a) * fr);
                    }
                }
                chW = (chW + 1) % (int) chBuf[0].size();
            }
            for (int ch = 0; ch < 2; ++ch)
            {
                float v = x[ch];
                if (molaWet > 0) v += molaWet * spring.process (v, ch);
                // eco de fita (colcheia pontuada)
                const size_t sz = echoBuf[ch].size();
                const float e = echoBuf[ch][((size_t) echoW + sz - (size_t) echoDelay) % sz];
                echoLp[ch] = 0.35 * e + 0.65 * echoLp[ch];
                echoBuf[ch][(size_t) echoW] = (float) (v + echoFb * echoLp[ch]);
                if (p.echoOn) v += 0.45f * (float) echoLp[ch];
                if (p.satOn) v = tube (v, satDrive, 0.05f) * std::min (1.0f, satDrive / 2.0f);
                v *= p.out * 1.2f;
                x[ch] = v;
            }
            echoW = (echoW + 1) % (int) echoBuf[0].size();
            // limitador de pico simples
            const double pk = std::max (std::fabs (x[0]), std::fabs (x[1]));
            const double target = pk > 0.9 ? 0.9 / pk : 1.0;
            lim = target < lim ? target : lim + (1.0 - lim) * 0.0005;
            L[i] = (float) (x[0] * lim); R[i] = (float) (x[1] * lim);
        }
    }
};

// ---------------------------------------------------------------- o groove: o que tocar em cada semicolcheia
struct GrooveSettings
{
    int steps[16] { 0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0 };   // 0 vazio, 1 skank, 2 chop
    int stroke = StSimples, mute = MuSeco;
    float lag = 0.45f, strum = 0.35f, dur = 0.3f;
    bool vary = true; int every = 4, start = 3; float intensity = 0.8f;
    int fill[16] { 0,0,1,0, 0,0,1,0, 5,5,5,0, 5,5,5,5 };
    bool band = false;
};

// uma ação do groove (o processador transforma em cordas tocadas)
struct Action
{
    enum Kind { Strum, Lead, Kick, Rim, Hat, BassNote } kind = Strum;
    double offsetBeats = 0;     // a partir do início da semicolcheia
    float vel = 0.9f;
    double holdSec = 0.05;
    int mute = MuSeco;          // abafamento usado na corda
    bool up = false;            // upstroke (aguda primeiro)
    bool top3 = false;          // só as 3 cordas agudas
    double spread = -1;         // atraso entre cordas (s); -1 = do botão STRUM
    double dbl = 0.012;         // atraso da segunda guitarra (dobra estéreo)
    int note = 60; double beats = 0;
};

inline double holdFor (int mute, float dur)
{
    const double base = 0.025 + dur * 0.09;
    return mute == MuMaoEsq ? 0.012 : mute == MuPalm ? base * 0.7 : mute == MuAberto ? 0.5 : base;
}

// bang-bang / double chop na contagem com a caixa no 2 e no 4
inline void bangBang (std::vector<Action>& out, float vel, double lagBeats, double secPerBeat)
{
    const double gapBeats = 0.25;   // a semicolcheia seguinte
    Action a; a.kind = Action::Strum; a.offsetBeats = lagBeats; a.vel = vel; a.up = false;
    a.holdSec = std::min (0.06, gapBeats * secPerBeat * 0.32); a.spread = 0.0024; a.dbl = 0.006; a.mute = MuSeco;
    out.push_back (a);
    Action b = a; b.offsetBeats = lagBeats + gapBeats; b.vel = vel * 0.62f; b.up = true; b.top3 = true;
    b.holdSec = std::min (0.045, gapBeats * secPerBeat * 0.24); b.spread = 0.0016;
    out.push_back (b);
}

// absStep = semicolcheia desde o início (bar = absStep / 16)
inline std::vector<Action> grooveStep (const GrooveSettings& s, long long absStep, double bpm)
{
    std::vector<Action> out;
    const int i = (int) (absStep % 16);
    const long long bar = absStep / 16;
    const double spb = 60.0 / std::clamp (bpm, 40.0, 200.0);
    const double lagBeats = s.lag * 0.025 / spb;
    const int every = std::max (1, s.every);
    const bool fill = s.vary && (bar % every) == every - 1 && i >= (s.start - 1) * 4;
    int v = s.steps[i];
    bool bb = s.stroke == StBangBang;
    if (fill)
    {
        const int f = s.fill[i];
        const float k = 0.45f + 0.7f * s.intensity;
        v = 0;
        if (f == FSkank) v = 1;
        if (f == FBang) { v = 1; bb = true; }
        if (f == FLongo)
        { Action a; a.offsetBeats = lagBeats; a.vel = std::min (1.0f, 0.9f * k); a.mute = MuAberto; a.holdSec = i >= 12 ? 0.35 : 0.16; out.push_back (a); }
        if (f == FChop)
        { Action a; a.offsetBeats = lagBeats; a.vel = std::min (1.0f, (0.45f + 0.03f * (i % 4)) * k); a.mute = MuMaoEsq; a.holdSec = 0.012; out.push_back (a); }
        if (f == FNota)
        { Action a; a.kind = Action::Lead; a.offsetBeats = lagBeats; a.note = lickNote (i); a.vel = std::min (1.0f, (i % 4 == 0 ? 0.9f : 0.75f) * k); a.holdSec = 0.09 + (i == 15 ? 0.2 : 0.0); out.push_back (a); }
    }
    if (v == 1)
    {
        const float vel = (i == 6 || i == 14) ? 0.92f : 0.86f;
        if (bb) bangBang (out, vel, lagBeats, spb);
        else
        {
            Action a; a.offsetBeats = lagBeats; a.vel = vel; a.mute = s.mute; a.holdSec = holdFor (s.mute, s.dur); out.push_back (a);
            if (s.stroke == StShuffle)
            { Action b; b.offsetBeats = lagBeats + 0.25 * 0.67; b.vel = 0.58f; b.up = true; b.top3 = true; b.holdSec = 0.03; out.push_back (b); }
        }
    }
    if (v == 2)
    { Action a; a.offsetBeats = lagBeats; a.vel = 0.55f; a.mute = MuMaoEsq; a.holdSec = 0.014; out.push_back (a); }
    if (s.band)
    {
        if (i == 4 || i == 12) { Action k; k.kind = Action::Kick; out.push_back (k); Action r; r.kind = Action::Rim; out.push_back (r); }
        if (i % 2 == 0) { Action h; h.kind = Action::Hat; h.vel = i % 4 == 2 ? 1.0f : 0.6f; out.push_back (h); }
        static const int bl[16][2] = { {0,9}, {-1,0}, {-1,0}, {-1,0}, {-1,0}, {-1,0}, {7,4}, {-1,0}, {12,9}, {-1,0}, {-1,0}, {-1,0}, {10,4}, {-1,0}, {7,4}, {-1,0} };
        if (bl[i][0] >= 0) { Action b; b.kind = Action::BassNote; b.note = bl[i][0]; b.beats = bl[i][1] / 10.0; out.push_back (b); }
    }
    return out;
}

} // namespace mrg
