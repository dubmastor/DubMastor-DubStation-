// Mastor Reggae Percussion MRP-77 — motor de som (sem JUCE): os 37 sons sintetizados (acústicos, digitais 80-90 e Dub FX), os grooves do estudo,
// a cadeia de efeitos (EQ, saturador, Tape Echo, Big Knob) e o MIDI. A versão para celular
// (site/web/reggaepercussion/index.html) tem a mesma síntese em Web Audio: ao mudar um, mude o outro.
// Contagem da Dub Station: caixa no 2 e no 4; 1 passo = 1 semicolcheia; 16 passos = 1 compasso.
#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mrp
{
constexpr float kPi = 3.14159265358979f;
inline float clampf (float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

struct Rng
{
    uint32_t s = 0x9e3779b9u;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 16777216.0f; }   // 0..1
    float bi() { return next() * 2.0f - 1.0f; }
};

// ------------------------------------------------------------------ sons
// cat: 0 = acústica, 1 = digital 80-90, 2 = Dub FX
struct Sound { const char* key; const char* name; const char* sub; int note; int note2; uint32_t argb; int cat = 0; };
constexpr int numSounds = 37;
constexpr int numCats = 3;
inline const char* catName (int c) { static const char* n[numCats] = { "Ac\xc3\xbastica", "Digital 80-90", "Dub FX" }; return n[c % numCats]; }
inline const std::array<Sound, numSounds>& sounds()
{
    static const std::array<Sound, numSounds> s { {
        { "egg", "Shaker", "Egg", 82, -1, 0xffe6b83a },        { "tub", "Shaker", "Tubular", 88, -1, 0xffe6b83a },
        { "mar", "Maracas", "Par", 70, -1, 0xffe6b83a },       { "gan", "Ganz\xc3\xa1", "Metal", 91, -1, 0xffe6b83a },
        { "cab", "Cabasa", "Raspado", 69, -1, 0xffd9c7a0 },    { "skr", "Shekere", "Caba\xc3\xa7" "a", 90, -1, 0xffd9c7a0 },
        { "tam", "Pandeiro", "Tapa", 54, -1, 0xffe5604a },     { "pla", "Platinela", "S\xc3\xb3 platinelas", 89, -1, 0xffe5604a },
        { "wbh", "Woodblock", "Agudo", 76, -1, 0xffc48a52 },   { "wbl", "Woodblock", "Grave", 77, -1, 0xffc48a52 },
        { "cla", "Clave", "Madeira", 75, -1, 0xffc48a52 },     { "cow", "Cowbell", "Abafado", 56, -1, 0xfff2a541 },
        { "agh", "Agog\xc3\xb4", "Agudo", 67, -1, 0xfff2a541 },       { "agl", "Agog\xc3\xb4", "Grave", 68, -1, 0xfff2a541 },
        { "trm", "Tri\xc3\xa2ngulo", "Abafado", 80, -1, 0xfff2a541 }, { "tro", "Tri\xc3\xa2ngulo", "Aberto", 81, -1, 0xfff2a541 },
        { "gui", "Guiro", "Curto / longo", 73, 74, 0xff6fbf78 }, { "vib", "Vibraslap", "Chocalho", 58, -1, 0xff6fbf78 },
        { "cgh", "Conga", "Aberta aguda", 63, -1, 0xff8fb6d9 }, { "cgl", "Conga", "Grave", 64, -1, 0xff8fb6d9 },
        { "cgs", "Conga", "Slap", 62, -1, 0xff8fb6d9 },        { "bgh", "Bong\xc3\xb4", "Agudo", 60, -1, 0xff8fb6d9 },
        { "bgl", "Bong\xc3\xb4", "Grave", 61, -1, 0xff8fb6d9 },
        // digitais 80-90
        { "tsh", "Tom Simmons", "Agudo", 83, -1, 0xff5fd0c8, 1 },  { "tsm", "Tom Simmons", "M\xc3\xa9" "dio", 84, -1, 0xff5fd0c8, 1 },
        { "tsg", "Tom Simmons", "Grave", 85, -1, 0xff5fd0c8, 1 },  { "cxd", "Caixa digital", "Sleng Teng", 86, -1, 0xffb98be0, 1 },
        { "c808", "Caixa 808", "Anal\xc3\xb3gica", 87, -1, 0xffb98be0, 1 }, { "clp", "Clap 808", "Palmas", 92, -1, 0xffb98be0, 1 },
        { "rim", "Rim digital", "Aro MT-40", 93, -1, 0xffb98be0, 1 }, { "cb8", "Cowbell 808", "Anal\xc3\xb3gico", 94, -1, 0xff5fd0c8, 1 },
        { "hhe", "Hat eletr\xc3\xb4nico", "Fechado", 95, -1, 0xff5fd0c8, 1 }, { "blp", "Blip Casio", "Synth perc", 96, -1, 0xff5fd0c8, 1 },
        // Dub FX
        { "lz", "Laser dub", "Zap", 97, -1, 0xffff5c8a, 2 },    { "sir", "Sirene dub", "Siren", 98, -1, 0xffff5c8a, 2 },
        { "tir", "Tiro dub", "Gun shot", 99, -1, 0xffff5c8a, 2 }, { "drp", "Drop dub", "Queda", 100, -1, 0xffff5c8a, 2 } } };
    return s;
}
inline int soundIndex (const char* key)
{
    for (int i = 0; i < numSounds; ++i) if (std::strcmp (sounds()[(size_t) i].key, key) == 0) return i;
    return 0;
}

// pads: um por som (o guiro tem dois: curto e longo) = 38 pads, cada um com a sua nota MIDI
struct Pad { int snd; int lg; int note; };   // lg: -1 = qualquer, 0 = curto, 1 = longo
constexpr int numPads = 38;
inline const std::array<Pad, numPads>& pads()
{
    static const std::array<Pad, numPads> p = [] {
        std::array<Pad, numPads> a {}; int k = 0;
        for (int i = 0; i < numSounds; ++i)
        {
            const auto& s = sounds()[(size_t) i];
            if (s.note2 >= 0) { a[(size_t) k++] = { i, 0, s.note }; a[(size_t) k++] = { i, 1, s.note2 }; }
            else a[(size_t) k++] = { i, -1, s.note };
        }
        return a;
    }();
    return p;
}
inline int padOf (int snd, bool lg)
{
    for (int i = 0; i < numPads; ++i)
        if (pads()[(size_t) i].snd == snd && (pads()[(size_t) i].lg < 0 || pads()[(size_t) i].lg == (lg ? 1 : 0))) return i;
    return 0;
}
// notas do MRD-12 (bateria): 36 a 51. Nenhum pad do MRP-77 usa essa faixa.
inline bool isDrumNote (int n) { return n >= 36 && n <= 51; }

// ------------------------------------------------------------------ peças (as 6 linhas do groove)
constexpr int numTracks = 6;
struct TrackDefault { const char* id; const char* soundKey; float dec, vol, pan; };
inline const std::array<TrackDefault, numTracks>& trackDefaults()
{
    static const std::array<TrackDefault, numTracks> t { {
        { "shk", "egg", 0.15f, 0.80f, 0.30f }, { "cab", "cab", 0.12f, 0.70f, -0.25f }, { "tam", "tam", 0.30f, 0.60f, -0.40f },
        { "wbl", "wbh", 0.20f, 0.60f, 0.45f }, { "cow", "cow", 0.25f, 0.55f, 0.20f },  { "gui", "gui", 0.40f, 0.60f, -0.15f } } };
    return t;
}

// ------------------------------------------------------------------ grooves do estudo (X = acento, x = fraca, - = pausa)
// p: shk cab tam wbl cow gui. snd: som de cada linha (nullptr = som padrão da peça; os kits digitais e Dub FX trocam as peças)
struct Groove { const char* group; const char* name; int swing; const char* p[numTracks]; const char* snd[numTracks]; };
inline const char* defaultSound (int t) { return trackDefaults()[(size_t) t].soundKey; }
inline const std::vector<Groove>& grooves()
{
    static const std::vector<Groove> g {
        { "Kits completos", "Kit roots (Seeco)", 58, { "xxXxxxXxxxXxxxXx", "", "--x---x---x---x-", "", "", "" }, {} },
        { "Kits completos", "Kit rockers (Sticky)", 52, { "", "xxXxxxXxxxXxxxXx", "", "", "X--X--X---X-X---", "" }, {} },
        { "Kits completos", "Kit steppers", 50, { "xXXxxXXxxXXxxXXx", "", "", "", "--X---X---X---X-", "" }, {} },
        { "Kits completos", "Kit ska/rocksteady", 60, { "", "", "----X-------X---", "--X---X---X---X-", "", "X-x-X-x-X-x-X-x-" }, {} },
        { "Shaker", "Shaker roots (A)", 54, { "xxXxxxXxxxXxxxXx", "", "", "", "", "" }, {} },
        { "Shaker", "Shaker Seeco (B)", 62, { "x-Xxx-Xxx-Xxx-Xx", "", "", "", "", "" }, {} },
        { "Shaker", "Shaker steppers (C)", 50, { "xXXxxXXxxXXxxXXx", "", "", "", "", "" }, {} },
        { "Shaker", "Shaker 3+3+2 (D)", 50, { "X--X--X-X--X--X-", "", "", "", "", "" }, {} },
        { "Pandeiro", "Pandeiro roots", 56, { "", "", "--X---X---X---X-", "", "", "" }, {} },
        { "Pandeiro", "Pandeiro shuffle", 62, { "", "", "--Xx--Xx--Xx--Xx", "", "", "" }, {} },
        { "Pandeiro", "Pandeiro ska/rocksteady", 58, { "", "", "--x-X-x---x-X-x-", "", "", "" }, {} },
        { "Cabasa", "Cabasa roots/dub", 54, { "", "--X---X---X---X-", "", "", "", "" }, {} },
        { "Cabasa", "Cabasa rockers", 52, { "", "xxXxxxXxxxXxxxXx", "", "", "", "" }, {} },
        { "Cowbell", "Cowbell steppers", 50, { "", "", "", "", "--X---X---X---X-", "" }, {} },
        { "Cowbell", "Cowbell sincopado (Sticky)", 52, { "", "", "", "", "X--X--X---X-X---", "" }, {} },
        { "Woodblock", "Woodblock ska", 60, { "", "", "", "--X---X---X---X-", "", "" }, {} },
        { "Woodblock", "Woodblock clave 3+3+2", 54, { "", "", "", "X--X--X---X-X---", "", "" }, {} },
        { "Guiro", "Guiro mento/ska", 58, { "", "", "", "", "", "X-x-X-x-X-x-X-x-" }, {} },
        { "Guiro", "Guiro roots", 56, { "", "", "", "", "", "--X---X---X---X-" }, {} },
        { "Kits digitais 80-90", "Kit Sleng Teng (85)", 50, { "xxXxxxXxxxXxxxXx", "--x---x---x---x-", "----X-------X---", "--X--X----X--X--", "X-------X-------", "-------------XxX" }, { "hhe", "rim", "cxd", "blp", "cb8", "tsm" } },
        { "Kits digitais 80-90", "Kit 808 dancehall (90)", 52, { "x-Xxx-Xxx-Xxx-Xx", "----X-------X---", "---x---x---x---x", "X--X--X---X-X---", "------X-------X-", "--------------xX" }, { "hhe", "clp", "c808", "cb8", "blp", "tsh" } },
        { "Kits digitais 80-90", "Kit Simmons roots (80)", 56, { "xxXxxxXxxxXxxxXx", "--X---X---X---X-", "---------X--X---", "-------------X--", "X-----------x--X", "--x-------x-----" }, { "egg", "rim", "tsh", "tsm", "tsg", "cb8" } },
        { "Kits digitais 80-90", "Kit steppers digital (90)", 50, { "xXXxxXXxxXXxxXXx", "--X---X---X---X-", "----X-------X---", "X---X---X---X---", "--x---x---x---x-", "X-------X-------" }, { "hhe", "rim", "c808", "cb8", "blp", "tsg" } },
        { "Simmons 80", "Simmons rockers (82)", 56, { "x-x-x-x-x-x-x-x-", "----X-------X---", "--x-------x-----", "------x-------x-", "X-------X-------", "--x---x---x---x-" }, { "hhe", "rim", "tsh", "tsm", "tsg", "cb8" } },
        { "Simmons 80", "Simmons steppers (84)", 52, { "xxXxxxXxxxXxxxXx", "----X-------X---", "X---X---X---X---", "--x---x---x---x-", "-------------xX-", "X-------X-------" }, { "hhe", "rim", "tsg", "tsm", "tsh", "cb8" } },
        { "Simmons 80", "Simmons rub-a-dub (83)", 58, { "xxXxxxXxxxXxxxXx", "----X-------X---", "---x-----x-x----", "X-------X-------", "--------------Xx", "------x-------x-" }, { "egg", "rim", "tsm", "tsg", "tsh", "cb8" } },
        { "Simmons 80", "Simmons virada no 4", 56, { "x-X-x-X-x-X-x-X-", "----X-------X---", "------------xX--", "--------------Xx", "X-------------xX", "--x-------x-----" }, { "hhe", "cxd", "tsh", "tsm", "tsg", "rim" } },
        { "Simmons 80", "Simmons Sleng Teng toms (85)", 50, { "xxXxxxXxxxXxxxXx", "----X-------X---", "--X-----------X-", "------X----X----", "X-------X--x----", "---------x------" }, { "hhe", "cxd", "tsh", "tsm", "tsg", "blp" } },
        { "Simmons 80", "Simmons dub toms + eco", 54, { "xxXxxxXxxxXxxxXx", "----X-------X---", "X---------------", "------X---------", "----------X-X---", "---------------X" }, { "egg", "rim", "tsg", "tsm", "tsh", "lz" } },
        { "Simmons 80", "Simmons 808 dancehall (88)", 52, { "x-Xxx-Xxx-Xxx-Xx", "----X-------X---", "---x---x---x---x", "----X-------X---", "X--X--X---X-----", "------X-------X-" }, { "hhe", "clp", "tsh", "c808", "tsg", "cb8" } },
        { "Dub FX", "Dub FX: laser + drop", 54, { "xxXxxxXxxxXxxxXx", "-------------X--", "X---------------", "", "", "--X---X---X---X-" }, { "egg", "lz", "drp", "sir", "tir", "rim" } },
        { "Dub FX", "Dub FX: sirene + tiro", 54, { "xxXxxxXxxxXxxxXx", "X---------------", "------------X---", "---------------X", "", "--X---X---X---X-" }, { "hhe", "sir", "tir", "lz", "drp", "rim" } },
        { "Outros", "Vazio", 54, { "", "", "", "", "", "" }, {} } };
    return g;
}
inline int grooveSound (const Groove& g, int t) { return soundIndex (g.snd[t] != nullptr ? g.snd[t] : defaultSound (t)); }
inline int stepValue (const char* s, int i) { if (s == nullptr || (int) std::char_traits<char>::length (s) <= i) return 0; return s[i] == 'X' ? 2 : (s[i] == 'x' ? 1 : 0); }

// contagem Mastor: caixa no 2 e no 4 (passos 4 e 12), palheta nos "&" (2, 6, 10, 14)
inline bool isSnareStep (int st) { return st == 4 || st == 12; }
inline bool isSkankStep (int st) { return st == 2 || st == 6 || st == 10 || st == 14; }
inline const char* countName (int st) { static const char* n[16] = { "1","e","&","a","2","e","&","a","3","e","&","a","4","e","&","a" }; return n[st & 15]; }

// ------------------------------------------------------------------ biquad (RBJ)
struct Biquad
{
    enum Type { LP, HP, BP, PEAK };
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (Type t, float f, float q, float sr, float gainDb = 0.0f)
    {
        f = clampf (f, 10.0f, sr * 0.45f);
        const float w = 2.0f * kPi * f / sr, cw = std::cos (w), sw = std::sin (w), alpha = sw / (2.0f * std::max (0.05f, q));
        float B0, B1, B2, A0, A1, A2;
        if (t == LP)      { B0 = (1 - cw) * 0.5f; B1 = 1 - cw; B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; }
        else if (t == HP) { B0 = (1 + cw) * 0.5f; B1 = -(1 + cw); B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; }
        else if (t == BP) { B0 = alpha; B1 = 0; B2 = -alpha; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; }
        else
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A; A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    float proc (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    // ganho (linear) na frequência f, para desenhar a curva do EQ
    double mag (double f, double sr) const
    {
        const double w = 2.0 * 3.141592653589793 * f / sr, c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2), dr = 1 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
        return std::sqrt ((nr * nr + ni * ni) / std::max (1e-12, dr * dr + di * di));
    }
};

// ------------------------------------------------------------------ amostra carregada pelo usuário
struct SampleData { std::vector<float> l, r; double rate = 48000.0; };

// ------------------------------------------------------------------ voz: soma de componentes (ruído filtrado / oscilador)
struct Comp
{
    bool noise = true; int wave = 0;          // 0 seno, 1 triângulo, 2 quadrada
    float f0 = 440, f1 = 440, sweep = 0.06f;  // varredura de altura (tambores)
    float gain = 0, attack = 0.001f, decay = 0.1f;
    int delay = 0;                            // amostras até começar
    float lfoF = 0, lfoD = 0, lfoPh = 0;      // vibrato (sirene)
    bool useF = false, useHp = false; Biquad f, hp;
    float phase = 0; int t = 0; bool done = true;
};

struct Voice
{
    static constexpr int maxComps = 18;
    std::array<Comp, maxComps> c; int n = 0;
    float gl = 1, gr = 1;
    bool active = false;
    // amostra
    std::shared_ptr<const SampleData> smp; double pos = 0, inc = 1; int len = 0; float sGain = 0; int sFadeAt = 0, sFadeLen = 1;
};

struct HitInfo { int snd; float vel; bool lg; float dec; float vol; float pan; int voiceTag; };

class Synth
{
public:
    void prepare (float sampleRate) { sr = sampleRate; for (auto& v : voices) v.active = false; }

    // toca um som. dec 0..1 (duração), vol 0..1, pan -1..1; lg = guiro longo; stepDur em segundos
    void hit (int snd, float vel, bool lg, float dec, float vol, float pan, float stepDur, std::shared_ptr<const SampleData> sample = nullptr)
    {
        Voice& v = take();
        v.n = 0; v.active = true; v.smp.reset();
        const float x = (clampf (pan, -1, 1) + 1.0f) * 0.5f, g = std::pow (clampf (vol, 0, 1), 1.5f);
        v.gl = std::cos (x * kPi * 0.5f) * g; v.gr = std::sin (x * kPi * 0.5f) * g;
        const float dmul = 0.25f + clampf (dec, 0, 1) * 1.6f;
        const bool acc = vel > 0.8f;
        if (sample != nullptr && ! sample->l.empty())
        {
            v.smp = sample; v.pos = 0; v.inc = sample->rate / sr * (1.0 + (rng.next() - 0.5) * 0.04);
            const int full = (int) ((double) sample->l.size() / v.inc);
            v.len = std::min (full, (int) (stepDur * 4.0f * dmul * sr));
            v.sGain = vel; v.sFadeAt = (int) (v.len * 0.7f); v.sFadeLen = std::max (1, v.len - v.sFadeAt);
            return;
        }
        auto D = [&] (float s) { return s * dmul; };
        // sem alocar memória (roda na linha de áudio)
        struct Key { const char* v; bool operator== (const char* o) const { return std::strcmp (v, o) == 0; } };
        const Key k { sounds()[(size_t) std::clamp (snd, 0, numSounds - 1)].key };
        if (k == "egg") nz (v, 0, vel * 0.9f, Biquad::BP, 5200, 1.2f, acc ? 0.012f : 0.006f, D (0.07f), 4800);
        else if (k == "tub") nz (v, 0, vel * 0.9f, Biquad::BP, 8200, 2.5f, acc ? 0.012f : 0.006f, D (0.07f), 6000);
        else if (k == "mar") nz (v, 0, vel * 0.9f, Biquad::BP, 3800, 1.8f, acc ? 0.012f : 0.006f, D (0.07f), 2500);
        else if (k == "gan") nz (v, 0, vel * 0.8f, Biquad::BP, 9500, 2.0f, acc ? 0.02f : 0.01f, D (0.09f), 7000);
        else if (k == "cab") { nz (v, 0, vel * 0.7f, Biquad::HP, 6500, 0.7f, 0.003f, D (0.05f)); nz (v, 0.012f, vel * 0.4f, Biquad::BP, 9000, 2, 0.002f, D (0.04f)); }
        else if (k == "skr") { nz (v, 0, vel * 0.8f, Biquad::BP, 4200, 1.4f, 0.004f, D (0.09f), 2000); nz (v, 0.02f, vel * 0.4f, Biquad::BP, 6000, 2, 0.002f, D (0.05f)); }
        else if (k == "tam" || k == "pla")
        {
            const bool p = k == "pla";
            if (! p) osc (v, 0, vel * 0.25f, 0, 190, D (0.05f));
            nz (v, 0, vel * 0.5f, Biquad::BP, p ? 11000.0f : 8500.0f, 3, 0.001f, D (0.16f));
            for (int j = 0; j < 3; ++j) osc (v, j * 0.004f, vel * 0.04f, 2, 6000.0f + j * 1370.0f + rng.next() * 300.0f, D (0.12f), 9000);
        }
        else if (k == "wbh" || k == "wbl") { const float f = k == "wbl" ? 720.0f : 1180.0f; osc (v, 0, vel * 0.6f, 1, f, D (0.06f)); osc (v, 0, vel * 0.25f, 0, f * 2.7f, D (0.025f)); }
        else if (k == "cla") { osc (v, 0, vel * 0.7f, 0, 2500, D (0.035f)); osc (v, 0, vel * 0.15f, 1, 5100, D (0.015f)); }
        else if (k == "cow" || k == "agh" || k == "agl")
        {
            const float f0 = k == "cow" ? 545.0f : (k == "agh" ? 880.0f : 660.0f), f1 = k == "cow" ? 815.0f : (k == "agh" ? 1320.0f : 990.0f);
            osc (v, 0, vel * 0.25f, 2, f0, D (0.18f), f0 * 1.4f); osc (v, 0, vel * 0.25f, 2, f1, D (0.18f), f0 * 1.4f);
        }
        else if (k == "trm" || k == "tro") { const float d = k == "tro" ? 0.9f : 0.08f; const float fs[3] = { 4200, 6100, 7900 }; for (int j = 0; j < 3; ++j) osc (v, 0, vel * (0.12f - j * 0.03f), 0, fs[j], D (d)); }
        else if (k == "gui")
        {
            const int cnt = (int) std::lround ((lg ? 8 : 4) + dmul * 6);
            const float span = stepDur * (lg ? 1.7f : 0.6f) * (0.6f + dmul * 0.8f);
            for (int j = 0; j < cnt && v.n < Voice::maxComps; ++j) nz (v, j * span / cnt, vel * 0.55f * (1.0f - (float) j / cnt * 0.4f), Biquad::BP, 3200, 4, 0.001f, 0.012f);
        }
        else if (k == "vib") { for (int j = 0; j < 14; ++j) nz (v, j * j * 0.003f * dmul * 2.0f, vel * 0.5f * (1.0f - j / 14.0f), Biquad::BP, 2600, 6, 0.001f, 0.02f); }
        else if (k == "cgh") { drum (v, 360, 320, D (0.22f), vel * 0.8f); nz (v, 0, vel * 0.15f, Biquad::BP, 1500, 1, 0.001f, 0.03f); }
        else if (k == "cgl") { drum (v, 250, 215, D (0.28f), vel * 0.85f); nz (v, 0, vel * 0.12f, Biquad::BP, 1200, 1, 0.001f, 0.03f); }
        else if (k == "cgs") { drum (v, 420, 380, D (0.06f), vel * 0.5f); nz (v, 0, vel * 0.7f, Biquad::BP, 1800, 1, 0.001f, D (0.05f)); }
        else if (k == "bgh") { drum (v, 640, 580, D (0.12f), vel * 0.7f); nz (v, 0, vel * 0.1f, Biquad::BP, 2500, 1, 0.001f, 0.02f); }
        else if (k == "bgl") { drum (v, 460, 410, D (0.15f), vel * 0.75f); nz (v, 0, vel * 0.1f, Biquad::BP, 2000, 1, 0.001f, 0.02f); }
        // ---- digitais 80-90
        else if (k == "tsh" || k == "tsm" || k == "tsg")
        {
            const float f0 = k == "tsh" ? 420.0f : (k == "tsm" ? 310.0f : 220.0f), f1 = k == "tsh" ? 190.0f : (k == "tsm" ? 135.0f : 88.0f);
            sweep (v, 0, f0, f1, 0.28f, D (0.5f), vel * 0.8f); sweep (v, 1, f0 * 1.5f, f1 * 1.5f, 0.2f, D (0.18f), vel * 0.18f);
            nz (v, 0, vel * 0.3f, Biquad::BP, 1400, 1, 0.001f, 0.05f);
        }
        else if (k == "cxd") { osc (v, 0, vel * 0.35f, 2, 230, D (0.06f), 900); nz (v, 0, vel * 0.55f, Biquad::HP, 2200, 0.7f, 0.001f, D (0.13f)); }
        else if (k == "c808") { osc (v, 0, vel * 0.45f, 0, 185, D (0.09f)); osc (v, 0, vel * 0.25f, 0, 330, D (0.06f)); nz (v, 0, vel * 0.5f, Biquad::HP, 1800, 0.7f, 0.001f, D (0.18f)); }
        else if (k == "clp") { for (int j = 0; j < 3; ++j) nz (v, j * 0.011f, vel * 0.55f, Biquad::BP, 1150, 1.6f, 0.001f, 0.012f); nz (v, 0.033f, vel * 0.45f, Biquad::BP, 1150, 1.4f, 0.002f, D (0.16f)); }
        else if (k == "rim") { osc (v, 0, vel * 0.5f, 0, 1700, D (0.02f)); osc (v, 0, vel * 0.3f, 2, 520, D (0.018f), 1600); }
        else if (k == "cb8") { osc (v, 0, vel * 0.3f, 2, 587, D (0.22f), 1500); osc (v, 0, vel * 0.3f, 2, 845, D (0.22f), 1500); }
        else if (k == "hhe")
        {
            const float fs[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
            for (float f : fs) osc (v, 0, vel * 0.07f, 2, f * 2.1f, D (acc ? 0.09f : 0.045f), 9000);
            nz (v, 0, vel * 0.25f, Biquad::HP, 8000, 0.7f, 0.001f, D (acc ? 0.08f : 0.04f));
        }
        else if (k == "blp") sweep (v, 2, 1046, 880, 0.03f, D (0.07f), vel * 0.22f, 3500);
        // ---- Dub FX
        else if (k == "lz") sweep (v, 3, 3200, 180, 0.26f * dmul, 0.32f * dmul, vel * 0.25f, 4000);
        else if (k == "sir")
        {
            sweep (v, 2, 900, 900, 0.001f, 1.1f * dmul, vel * 0.16f, 2800);
            Comp& c = v.c[(size_t) (v.n - 1)]; c.attack = 0.03f; c.lfoF = 4.5f; c.lfoD = 320; c.lfoPh = 0;
        }
        else if (k == "tir") { nz (v, 0, vel * 0.8f, Biquad::LP, 7000, 0.7f, 0.001f, D (0.3f)); sweep (v, 0, 140, 45, 0.12f, D (0.25f), vel * 0.7f); }
        else if (k == "drp") sweep (v, 0, 950, 55, 0.55f * dmul, 0.7f * dmul, vel * 0.6f);
    }

    // guia: bumbo + aro no 2 e no 4, palheta (acorde curto) nos "&"
    void guide (int st, float stepDur)
    {
        if (isSnareStep (st))
        {
            Voice& v = take(); v.n = 0; v.active = true; v.smp.reset(); v.gl = v.gr = 0.5f;
            nz (v, 0, 0.6f, Biquad::BP, 2000, 4, 0.001f, 0.05f); osc (v, 0, 0.5f, 0, 420, 0.04f); drum (v, 110, 45, 0.25f, 1.0f);
            v.c[(size_t) (v.n - 1)].sweep = 0.15f;
        }
        if (isSkankStep (st))
        {
            Voice& v = take(); v.n = 0; v.active = true; v.smp.reset(); v.gl = v.gr = 0.5f;
            const float fs[3] = { 261.6f, 329.6f, 392.0f };
            for (float f : fs) osc (v, 0, 0.06f, 3, f, stepDur * 0.5f, 1400);
        }
    }

    void render (float* L, float* R, int n)
    {
        for (auto& v : voices)
        {
            if (! v.active) continue;
            bool any = false;
            if (v.smp != nullptr)
            {
                const auto& s = *v.smp; const int sl = (int) s.l.size();
                for (int i = 0; i < n; ++i)
                {
                    const int done = (int) (v.pos / std::max (1e-9, v.inc));
                    if (done >= v.len) break;
                    const int i0 = (int) v.pos; if (i0 + 1 >= sl) { v.len = 0; break; }
                    const float fr = (float) (v.pos - i0);
                    const float a = s.l[(size_t) i0] + (s.l[(size_t) i0 + 1] - s.l[(size_t) i0]) * fr;
                    const float b = s.r.empty() ? a : s.r[(size_t) i0] + (s.r[(size_t) i0 + 1] - s.r[(size_t) i0]) * fr;
                    float g = v.sGain; if (done > v.sFadeAt) g *= 1.0f - (float) (done - v.sFadeAt) / v.sFadeLen;
                    L[i] += a * g * v.gl; R[i] += b * g * v.gr;
                    v.pos += v.inc;
                }
                if ((int) (v.pos / std::max (1e-9, v.inc)) < v.len) any = true;
                if (! any) { v.active = false; v.smp.reset(); }
                continue;
            }
            for (int ci = 0; ci < v.n; ++ci)
            {
                Comp& c = v.c[(size_t) ci];
                if (c.done) continue;
                any = true;
                const float total = c.attack + c.decay;
                const float kDec = std::log (0.0008f / std::max (0.001f, c.gain)) / std::max (0.001f, c.decay);
                for (int i = 0; i < n; ++i)
                {
                    if (c.delay > 0) { --c.delay; continue; }
                    const float t = c.t / sr;
                    if (t >= total) { c.done = true; break; }
                    const float env = t < c.attack ? c.gain * t / std::max (1e-5f, c.attack) : c.gain * std::exp (kDec * (t - c.attack));
                    float x;
                    if (c.noise) x = rng.bi();
                    else
                    {
                        float fr = t < c.sweep ? c.f0 * std::pow (c.f1 / c.f0, t / c.sweep) : c.f1;
                        if (c.lfoD > 0) { c.lfoPh += c.lfoF / sr; if (c.lfoPh >= 1.0f) c.lfoPh -= 1.0f; fr = std::max (20.0f, fr + c.lfoD * std::sin (2.0f * kPi * c.lfoPh)); }
                        c.phase += fr / sr; if (c.phase >= 1.0f) c.phase -= 1.0f;
                        x = c.wave == 0 ? std::sin (2.0f * kPi * c.phase)
                          : c.wave == 1 ? 4.0f * std::abs (c.phase - 0.5f) - 1.0f
                          : c.wave == 2 ? (c.phase < 0.5f ? 1.0f : -1.0f)
                          : 2.0f * c.phase - 1.0f;   // 3 = dente de serra
                    }
                    if (c.useF) x = c.f.proc (x);
                    if (c.useHp) x = c.hp.proc (x);
                    x *= env;
                    L[i] += x * v.gl; R[i] += x * v.gr;
                    ++c.t;
                }
            }
            if (! any) v.active = false;
        }
    }

    int activeVoices() const { int a = 0; for (auto& v : voices) a += v.active ? 1 : 0; return a; }
    void allOff() { for (auto& v : voices) { v.active = false; v.smp.reset(); } }

private:
    Voice& take()
    {
        for (auto& v : voices) if (! v.active) return v;
        Voice& v = voices[(size_t) next]; next = (next + 1) % (int) voices.size(); v.smp.reset(); return v;   // a mais antiga
    }
    void nz (Voice& v, float delaySec, float g, Biquad::Type type, float f, float q, float a, float d, float hp = 0)
    {
        if (v.n >= Voice::maxComps) return;
        Comp& c = v.c[(size_t) v.n++]; c = Comp();
        c.noise = true; c.gain = g; c.attack = a; c.decay = d; c.delay = (int) (delaySec * sr); c.done = false; c.t = 0;
        c.useF = true; c.f.set (type, f, q, sr);
        if (hp > 0) { c.useHp = true; c.hp.set (Biquad::HP, hp, 0.707f, sr); }
    }
    void osc (Voice& v, float delaySec, float g, int wave, float f, float d, float bp = 0)
    {
        if (v.n >= Voice::maxComps) return;
        Comp& c = v.c[(size_t) v.n++]; c = Comp();
        c.noise = false; c.wave = wave; c.f0 = c.f1 = f; c.gain = g; c.attack = 0.001f; c.decay = d; c.delay = (int) (delaySec * sr); c.done = false; c.t = 0;
        c.phase = rng.next();
        if (bp > 0) { c.useF = true; c.f.set (Biquad::BP, bp, 3, sr); }
    }
    // oscilador com queda de altura exponencial (toms eletrônicos, efeitos); lp > 0 = passa-baixa
    void sweep (Voice& v, int wave, float f0, float f1, float sw, float d, float g, float lp = 0)
    {
        if (v.n >= Voice::maxComps) return;
        Comp& c = v.c[(size_t) v.n++]; c = Comp();
        c.noise = false; c.wave = wave; c.f0 = f0; c.f1 = std::max (20.0f, f1); c.sweep = std::max (0.001f, sw); c.gain = g; c.attack = 0.002f; c.decay = d; c.done = false; c.t = 0; c.phase = 0;
        if (lp > 0) { c.useF = true; c.f.set (Biquad::LP, lp, 0.707f, sr); }
    }
    void drum (Voice& v, float f0, float f1, float d, float g)
    {
        if (v.n >= Voice::maxComps) return;
        Comp& c = v.c[(size_t) v.n++]; c = Comp();
        c.noise = false; c.wave = 0; c.f0 = f0; c.f1 = f1; c.sweep = 0.06f; c.gain = g; c.attack = 0.002f; c.decay = d; c.done = false; c.t = 0; c.phase = 0;
    }

    float sr = 48000.0f;
    std::array<Voice, 64> voices {};
    int next = 0;
    Rng rng;
};

// ------------------------------------------------------------------ AUTO MIX: nível de cada timbre (medido renderizando o próprio som)
// Igual à versão para celular: RMS máximo em janelas de 50 ms, som tocado com vol 0,7 e duração 0,3, normalizado para vol 1.
namespace loud
{
inline float rmsShort (const float* L, const float* R, int n, int w)
{
    float best = 0;
    for (int a = 0; a < n; a += w)
    {
        double e = 0; int c = 0;
        for (int i = a; i < std::min (n, a + w); ++i) { e += (double) L[i] * L[i]; ++c; if (R != nullptr) { e += (double) R[i] * R[i]; ++c; } }
        best = std::max (best, (float) std::sqrt (e / std::max (1, c)));
    }
    return best;
}
inline const std::array<float, numSounds>& soundLevels()
{
    static const std::array<float, numSounds> lv = [] {
        std::array<float, numSounds> a {};
        const float sr = 44100.0f; const int n = (int) (sr * 1.3f);
        std::vector<float> L ((size_t) n), R ((size_t) n);
        for (int si = 0; si < numSounds; ++si)
        {
            auto syn = std::make_unique<Synth>(); syn->prepare (sr);
            std::fill (L.begin(), L.end(), 0.0f); std::fill (R.begin(), R.end(), 0.0f);
            syn->hit (si, 1.0f, true, 0.30f, 0.70f, 0.0f, 0.2f);
            syn->render (L.data(), R.data(), n);
            a[(size_t) si] = rmsShort (L.data(), R.data(), n, 2205) / std::pow (0.7f, 1.5f);
        }
        return a;
    }();
    return lv;
}
inline float sampleLevel (const SampleData& s)
{
    if (s.l.empty()) return 0;
    const int w = std::max (1, (int) (s.rate * 0.05));
    return rmsShort (s.l.data(), s.r.empty() ? nullptr : s.r.data(), (int) s.l.size(), w) * 1.41f;
}
// alvo: mediana dos timbres a 75% de volume
inline float target()
{
    auto v = soundLevels(); std::sort (v.begin(), v.end());
    return v[(size_t) (numSounds / 2)] * std::pow (0.75f, 1.5f);
}
// sons brilhantes ficam um pouco abaixo; efeitos quase no nível
inline float weight (int snd)
{
    static const char* br[] = { "egg", "tub", "mar", "gan", "cab", "skr", "pla", "hhe", "trm", "tro" };
    static const char* fx[] = { "lz", "sir", "tir", "drp" };
    const char* k = sounds()[(size_t) std::clamp (snd, 0, numSounds - 1)].key;
    for (auto* b : br) if (std::strcmp (b, k) == 0) return 0.8f;
    for (auto* f : fx) if (std::strcmp (f, k) == 0) return 0.9f;
    return 1.0f;
}
constexpr float loudRatio = 1.8f;   // acima disso (≈ +5 dB) a peça fica marcada ALTO
// razão entre o nível da peça (com o volume 0..1) e o alvo; base = nível do som (ou da amostra)
inline float ratio (float base, int snd, float vol) { return base * std::pow (clampf (vol, 0, 1), 1.5f) / std::max (1e-6f, target() * weight (snd)); }
// volume que o AUTO MIX põe (0,10 a 1,00)
inline float autoVolume (float base, int snd)
{
    if (base <= 0) return -1;
    return std::round (clampf (100.0f * std::pow (target() * weight (snd) / base, 2.0f / 3.0f), 10, 100)) / 100.0f;
}
} // namespace loud

// ------------------------------------------------------------------ efeitos: EQ → saturador → Tape Echo → Big Knob
struct FxSettings
{
    bool eqOn = true; float hpf = 30, bellF = 2500, bellG = 0, lpf = 18000;
    bool satOn = false; float drive = 0.3f;          // padrão de fábrica: saturador em bypass
    bool echoOn = false; int echoSteps = 3; float echoFb = 0.45f, echoMix = 0.3f;   // Tape Echo em bypass
    float big = 0.8f; float bpm = 75;
};

class FxChain
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c) { buf[(size_t) c].assign ((size_t) (sr * 3.2f), 0.0f); hp[c].reset(); bell[c].reset(); lp[c].reset(); tape[c].reset(); }
        w = 0; dSmooth = -1; curEq = {};
    }
    void process (float* L, float* R, int n, const FxSettings& s)
    {
        auto diff = [] (float a, float b) { return std::abs (a - b) > 1e-6f; };
        if (diff (s.hpf, curEq[0]) || diff (s.bellF, curEq[1]) || diff (s.bellG, curEq[2]) || diff (s.lpf, curEq[3]))
        {
            curEq = { s.hpf, s.bellF, s.bellG, s.lpf };
            for (int c = 0; c < 2; ++c)
            {
                hp[c].set (Biquad::HP, s.hpf, 0.707f, sr); bell[c].set (Biquad::PEAK, s.bellF, 1.0f, sr, s.bellG); lp[c].set (Biquad::LP, s.lpf, 0.707f, sr);
                tape[c].set (Biquad::LP, 3200, 0.707f, sr);
            }
        }
        const float k = 1.0f + s.drive * 8.0f, norm = 1.0f / std::tanh (k);
        const float target = std::min ((float) buf[0].size() - 2.0f, 60.0f / std::max (30.0f, s.bpm) / 4.0f * s.echoSteps * sr);
        if (dSmooth < 0) dSmooth = target;
        const int size = (int) buf[0].size();
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { L[i], R[i] };
            for (int c = 0; c < 2; ++c)
            {
                float y = x[c];
                if (s.eqOn) y = lp[c].proc (bell[c].proc (hp[c].proc (y)));
                if (s.satOn) y = std::tanh (y * k) * norm * 0.9f;
                x[c] = y;
            }
            dSmooth += (target - dSmooth) * 0.0005f;
            float rpos = (float) w - dSmooth; while (rpos < 0) rpos += size;
            const int i0 = (int) rpos, i1 = (i0 + 1) % size; const float fr = rpos - i0;
            for (int c = 0; c < 2; ++c)
            {
                auto& b = buf[(size_t) c];
                const float d = tape[c].proc (b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr);
                b[(size_t) w] = x[c] + d * s.echoFb;
                const float out = x[c] + (s.echoOn ? d * s.echoMix : 0.0f);
                (c == 0 ? L[i] : R[i]) = out * s.big * 1.1f;
            }
            w = (w + 1) % size;
        }
    }
    static float levelOf (const float* x, int n) { float m = 0; for (int i = 0; i < n; ++i) m = std::max (m, std::abs (x[i])); return m; }

private:
    float sr = 48000.0f;
    Biquad hp[2], bell[2], lp[2], tape[2];
    std::array<std::vector<float>, 2> buf;
    int w = 0; float dSmooth = -1;
    std::array<float, 4> curEq {};
};

// ------------------------------------------------------------------ MIDI (arquivo .mid, sem JUCE: o teste usa o mesmo código)
struct MidiNote { int tick; int len; int ch; int note; int vel; };
// pat[faixa][passo 0..127]; bars = compassos; bar = -1 todos ou só um; track = -1 todas
inline std::vector<MidiNote> buildNotes (const std::array<std::array<uint8_t, 128>, numTracks>& pat, const std::array<int, numTracks>& snd,
                                        const std::array<bool, numTracks>& audible, const std::array<int, numPads>& padNotes,
                                        int bars, int bar, int track, bool colored, int channel, bool swingOn, float swing)
{
    std::vector<MidiNote> out;
    const int ST = 120;   // 480 ticks por semínima → 120 por semicolcheia
    const int sw = swingOn ? (int) std::lround ((swing * 2.0f - 1.0f) * ST) : 0;
    int b = 0;
    for (int bk = 0; bk < bars; ++bk)
    {
        if (bar >= 0 && bk != bar) continue;
        for (int t = 0; t < numTracks; ++t)
        {
            if (track >= 0 ? t != track : ! audible[(size_t) t]) continue;
            for (int i = 0; i < 16; ++i)
            {
                const int v = pat[(size_t) t][(size_t) (bk * 16 + i)];
                if (! v) continue;
                const bool isGui = std::strcmp (sounds()[(size_t) snd[(size_t) t]].key, "gui") == 0;
                const int at = b * 16 * ST + i * ST + (i % 2 ? std::max (0, sw) : 0);
                int len = (isGui && v == 2 ? ST * 2 - 10 : ST - 10) - (i % 2 ? std::max (0, sw) : 0);
                const int note = padNotes[(size_t) padOf (snd[(size_t) t], v == 2)];
                out.push_back ({ at, std::max (10, len), colored ? (bk % 16) + 1 : channel, note, v == 2 ? 110 : 62 });
            }
        }
        ++b;
    }
    std::sort (out.begin(), out.end(), [] (const MidiNote& a, const MidiNote& c) { return a.tick < c.tick; });
    return out;
}
} // namespace mrp
