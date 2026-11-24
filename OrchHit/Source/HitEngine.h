// Mastor Orch Hit MOH-88 — motor sem JUCE (usado pelo plugin e pelo teste offline).
//
// 1) O "sample": o orchestra hit é sintetizado uma vez por timbre, como se uma orquestra tocasse um acorde de
//    LÁ + MI (quintas e oitavas, sem terça; o RAGGA MENOR tem a terça menor) numa sala, e depois passa pelo
//    "sampler dos anos 80" (8 ou 12 bits, taxa reduzida). A mesma receita da versão web (site/web/orchhit/).
// 2) O sampler: cada tecla toca o sample transposto pela velocidade (o agudo fica curto, o grave fica longo),
//    com DUAS MÃOS (a mesma nota uma oitava acima), envelope ADSR e TAMANHO DA NOTA (1/64 = padrão).
// 3) A grade: 4 compassos em ticks de 1/64 (1 compasso = 64 ticks; caixa no 2 e no 4 = ticks 16 e 48),
//    os GROOVES de reggae e os MIDIs prontos.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace moh
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kSampleRate = 44100.0;     // o sample é "gravado" em 44,1 kHz (igual à versão web)
constexpr int kRootNote = 45;               // o sample está em Lá (MIDI 45 = A1 nos nomes do Cubase)
constexpr int kTicksPerBar = 64, kBars = 4, kTotalTicks = kTicksPerBar * kBars;

// ------------------------------------------------------------------ timbres
struct Preset
{
    const char* name; const char* lcd; const char* info;
    std::vector<double> brassNotes, strNotes, windNotes;   // múltiplos de Lá (110 Hz)
    double brass, strings, winds, low, timp, bd, cym, room, bright;
    int bits; double srRec, lpf, cut;
};

inline const std::vector<Preset>& presets()
{
    static const std::vector<double> B { 1, 1.5, 2, 3, 4 }, S { 2, 3, 4, 6, 8 }, W { 6, 8 };
    static const std::vector<Preset> p {
        { "ORCH5",   "ORCH5 FAIRLIGHT",  "8 bits · Lá + Mi · sala",       B, S, W, .8, .85, .25, .45, .6, .5, .1, .55, 0, 8, 24000, 11000, .30 },
        { "M1",      "M1 WORKSTATION",   "16 bits · limpo e brilhante",   B, S, W, .8, .85, .25, .45, .6, .5, .1, .40, 4, 16, 44100, 16000, .30 },
        { "EMU 12",  "EMULATOR 12 BITS", "12 bits · mais corpo",          B, S, W, .8, .95, .25, .45, .6, .5, .1, .55, 0, 12, 27778, 12500, .36 },
        { "BRASS",   "BRASS STAB",       "só metais + tímpano",           B, S, W, 1, .15, 0, .3, .35, .2, 0, .35, 2, 12, 32000, 14000, .26 },
        { "STRINGS", "STRING STAB",      "naipe de cordas",               B, S, W, .1, 1, .2, .5, .25, .15, .05, .5, 0, 16, 44100, 16000, .32 },
        { "RAGGA",   "RAGGA MENOR",      "acorde menor · 12 bits",        { 1, 1.5, 2, 3 }, { 2, 2.4, 3, 4, 4.8, 6 }, { 4.8, 6 },
                                                                           .8, .85, .25, .45, .6, .5, .1, .45, -2, 12, 27778, 12000, .28 },
        { "PESADO",  "HIT PESADO",       "tímpano e bumbo na frente",     B, S, W, .7, .6, .25, .8, 1, .9, .2, .6, 0, 12, 27778, 12500, .40 } };
    return p;
}
inline int numPresets() { return (int) presets().size(); }

// ------------------------------------------------------------------ peças de DSP
struct Rng   // mulberry32, o mesmo gerador da versão web
{
    uint32_t s;
    explicit Rng (uint32_t seed) : s (seed) {}
    double next()
    {
        s += 0x6D2B79F5u; uint32_t t = s;
        t = (t ^ (t >> 15)) * (1u | t);
        t ^= t + (t ^ (t >> 7)) * (61u | t);
        return (double) (t ^ (t >> 14)) / 4294967296.0;
    }
    double gauss() { return std::sqrt (-2.0 * std::log (next() + 1e-9)) * std::cos (2.0 * kPi * next()); }
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    enum Type { LP, HP, BP, Peak, HighShelf };
    void set (Type t, double f, double q, double gainDb = 0, double sr = kSampleRate)
    {
        f = std::min (f, sr * 0.49);
        const double w = 2 * kPi * f / sr, c = std::cos (w), s = std::sin (w), al = s / (2 * q), A = std::pow (10.0, gainDb / 40);
        double B0, B1, B2, A0, A1, A2;
        switch (t)
        {
            case LP: B0 = (1 - c) / 2; B1 = 1 - c; B2 = B0; A0 = 1 + al; A1 = -2 * c; A2 = 1 - al; break;
            case HP: B0 = (1 + c) / 2; B1 = -(1 + c); B2 = B0; A0 = 1 + al; A1 = -2 * c; A2 = 1 - al; break;
            case BP: B0 = al; B1 = 0; B2 = -al; A0 = 1 + al; A1 = -2 * c; A2 = 1 - al; break;
            case Peak: B0 = 1 + al * A; B1 = -2 * c; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * c; A2 = 1 - al / A; break;
            case HighShelf:
            default:
            {   // high shelf, inclinação 1 (como o BiquadFilter "highshelf" da Web Audio)
                const double sq = 2 * std::sqrt (A) * (s / 2 * std::sqrt (2.0));
                B0 = A * ((A + 1) + (A - 1) * c + sq); B1 = -2 * A * ((A - 1) + (A + 1) * c); B2 = A * ((A + 1) + (A - 1) * c - sq);
                A0 = (A + 1) - (A - 1) * c + sq; A1 = 2 * ((A - 1) - (A + 1) * c); A2 = (A + 1) - (A - 1) * c - sq;
            }
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    double process (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void process (std::vector<double>& v) { for (auto& x : v) x = process (x); }
};

// envelope "à moda Web Audio": rampa linear até o pico, segura e cai com setTarget (constante de tempo tau)
inline double envAt (double t, double start, double att, double hold, double tau, double peak)
{
    if (t < start) return 0;
    if (t < start + att) return peak * (t - start) / att;
    if (t < start + att + hold) return peak;
    return peak * std::exp (-(t - start - att - hold) / tau);
}

inline double polyBlep (double ph, double dt)
{
    if (ph < dt) { ph /= dt; return ph + ph - ph * ph - 1.0; }
    if (ph > 1.0 - dt) { ph = (ph - 1.0) / dt; return ph * ph + ph + ph + 1.0; }
    return 0.0;
}

inline void fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = 2 * kPi / (double) len * (inverse ? 1 : -1);
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1);
            for (size_t j = 0; j < len / 2; ++j)
            {
                const auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl;
            }
        }
    }
    if (inverse) for (auto& x : a) x /= (double) n;
}

inline void normalise (std::vector<double>& d)
{
    double m = 0; for (double v : d) m = std::max (m, std::abs (v));
    if (m > 0) for (auto& v : d) v /= m;
}

// ------------------------------------------------------------------ síntese do sample
inline std::vector<float> renderHit (const Preset& p)
{
    const double SR = kSampleRate, t0 = 0.01, A = 110.0;
    const int N = (int) SR;
    Rng r (11);
    std::vector<double> noise ((size_t) N);
    for (auto& v : noise) v = r.next() * 2 - 1;

    std::vector<double> dry ((size_t) N, 0.0);
    enum Wave { Saw, Square, Sine };
    // uma voz: oscilador limitado em banda, entrada levemente atrasada e, nos metais, o "scoop" (entra 1,8% abaixo e sobe)
    auto voice = [&] (std::vector<double>& bus, Wave w, double f, double cents, double att, double hold, double tau, double peak, bool scoop)
    {
        const double d = std::abs (r.gauss() * 0.007), start = t0 + d, det = std::pow (2.0, cents / 1200);
        double ph = r.next();
        for (int i = (int) (start * SR); i < N; ++i)
        {
            const double t = i / SR;
            double ff = f * det;
            if (scoop) ff *= t < start + 0.06 ? 0.982 * std::pow (1 / 0.982, (t - start) / 0.06) : 1.0;
            const double dt = ff / SR;
            double v;
            if (w == Saw) v = 2 * ph - 1 - polyBlep (ph, dt);
            else { v = (ph < 0.5 ? 1.0 : -1.0) + polyBlep (ph, dt) - polyBlep (std::fmod (ph + 0.5, 1.0), dt); }
            bus[(size_t) i] += v * envAt (t, start, att, hold, tau, peak);
            ph += dt; if (ph >= 1) ph -= 1;
        }
    };
    auto mixInto = [&] (const std::vector<double>& bus, double g) { for (int i = 0; i < N; ++i) dry[(size_t) i] += bus[(size_t) i] * g; };

    if (p.brass > 0)
    {   // METAIS: filtro que abre em 7 kHz e fecha até 900 Hz, formantes de metal
        std::vector<double> bus ((size_t) N, 0.0);
        const double n = (double) p.brassNotes.size() * 3;
        for (double m : p.brassNotes) for (int k = 0; k < 3; ++k) voice (bus, Saw, A * m, r.gauss() * 7, .012, .07, .16, 1 / std::sqrt (n), true);
        Biquad lp, k1, k2; k1.set (Biquad::Peak, 1200, 1.2, 6); k2.set (Biquad::Peak, 2600, 2, 3);
        for (int i = 0; i < N; ++i)
        {
            const double t = i / SR;
            if (i % 16 == 0) lp.set (Biquad::LP, t < t0 + .02 ? 7000 : 900 + 6100 * std::exp (-(t - t0 - .02) / .1), 0.707);
            bus[(size_t) i] = k2.process (k1.process (lp.process (bus[(size_t) i])));
        }
        mixInto (bus, p.brass);
    }
    if (p.strings > 0)
    {   // CORDAS: naipe de 8 por nota, corpo do violino, "bridge hill" e ruído de arco
        std::vector<double> bus ((size_t) N, 0.0);
        const double n = (double) p.strNotes.size() * 8;
        for (double m : p.strNotes) for (int k = 0; k < 8; ++k) voice (bus, Saw, A * m, r.gauss() * 10, .008, .05, .22, 1 / std::sqrt (n), false);
        Biquad bp; bp.set (Biquad::BP, 3200, 0.6);
        for (int i = 0; i < N; ++i) bus[(size_t) i] += bp.process (noise[(size_t) i]) * envAt (i / SR, t0, .002, .01, .025, .35);
        Biquad f[6];
        f[0].set (Biquad::HP, 180, .707); f[1].set (Biquad::Peak, 280, 2, 5); f[2].set (Biquad::Peak, 480, 2.5, 3);
        f[3].set (Biquad::Peak, 2400, 1, 6); f[4].set (Biquad::Peak, 3200, 1.5, 3); f[5].set (Biquad::LP, 10000, .707);
        for (auto& b : f) b.process (bus);
        mixInto (bus, p.strings);
    }
    if (p.winds > 0)
    {   // MADEIRAS reforçando o agudo
        std::vector<double> bus ((size_t) N, 0.0);
        for (double m : p.windNotes) for (int k = 0; k < 3; ++k) voice (bus, Square, A * m, r.gauss() * 6, .015, .06, .15, .25, false);
        Biquad a, b; a.set (Biquad::LP, 2800, .707); b.set (Biquad::Peak, 1500, 1.5, 4); a.process (bus); b.process (bus);
        mixInto (bus, p.winds);
    }
    if (p.low > 0)
    {   // CONTRABAIXOS e CELLOS
        std::vector<double> bus ((size_t) N, 0.0);
        for (double f : { A / 2, A }) for (int k = 0; k < 4; ++k) voice (bus, Saw, f, r.gauss() * 8, .01, .06, .25, .3, false);
        Biquad a; a.set (Biquad::LP, 2000, .707); a.process (bus);
        mixInto (bus, p.low);
    }
    if (p.timp > 0)
    {   // TÍMPANO: modos inarmônicos com a afinação caindo
        const double modes[5][3] = { { 1, 1, .9 }, { 1.5, .55, .6 }, { 1.98, .35, .45 }, { 2.44, .2, .35 }, { 2.9, .12, .25 } };
        for (auto& m : modes)
        {
            double ph = 0;
            for (int i = (int) (t0 * SR); i < N; ++i)
            {
                const double t = i / SR, f = 55 * m[0] * (1 + 0.02 * std::exp (-(t - t0) / .05));
                dry[(size_t) i] += p.timp * std::sin (ph) * envAt (t, t0, .002, 0, m[2], m[1]);
                ph += 2 * kPi * f / SR;
            }
        }
    }
    if (p.bd > 0)
    {   // BUMBO SINFÔNICO
        double ph = 0;
        for (int i = (int) (t0 * SR); i < N; ++i)
        {
            const double t = i / SR, f = 45 + 25 * std::exp (-(t - t0) / .05);
            dry[(size_t) i] += std::sin (ph) * envAt (t, t0, .003, 0, .45, p.bd);
            ph += 2 * kPi * f / SR;
        }
    }
    if (p.cym > 0)
    {   // PRATO
        Biquad hp; hp.set (Biquad::HP, 3500, .707);
        for (int i = 0; i < N; ++i) dry[(size_t) i] += hp.process (noise[(size_t) i]) * envAt (i / SR, t0, .003, 0, .5, p.cym);
    }

    // SALA DE CONCERTO: resposta sintética de ~2 s que escurece com o tempo, com primeiras reflexões
    const int irLen = (int) (SR * 2.2);
    std::vector<double> ir ((size_t) irLen);
    double lp = 0;
    for (int i = 0; i < irLen; ++i)
    {
        const double t = i / SR, c = std::min (.95, .2 + t * .5);
        lp = lp * c + (r.next() * 2 - 1) * (1 - c);
        ir[(size_t) i] = t < .018 ? 0 : lp * std::exp (-t / .4);
    }
    for (auto e : { std::pair<double, double> { .011, .5 }, { .019, .4 }, { .027, .35 }, { .034, .3 } }) ir[(size_t) (e.first * SR)] += e.second * .2;
    size_t L = 1; while (L < (size_t) (N + irLen)) L <<= 1;
    std::vector<std::complex<double>> X (L), H (L);
    for (int i = 0; i < N; ++i) X[(size_t) i] = dry[(size_t) i];
    for (int i = 0; i < irLen; ++i) H[(size_t) i] = ir[(size_t) i];
    fft (X, false); fft (H, false);
    for (size_t i = 0; i < L; ++i) X[i] *= H[i];
    fft (X, true);
    std::vector<double> wet ((size_t) N);
    double pd = 0, pw = 0;
    for (int i = 0; i < N; ++i) { wet[(size_t) i] = X[(size_t) i].real(); pd = std::max (pd, std::abs (dry[(size_t) i])); pw = std::max (pw, std::abs (wet[(size_t) i])); }
    const double wg = pw > 0 ? p.room * 1.6 * 0.5 * pd / pw : 0;
    Biquad tone; tone.set (Biquad::HighShelf, 3500, .707, p.bright);
    std::vector<double> d ((size_t) N);
    for (int i = 0; i < N; ++i) d[(size_t) i] = tone.process (dry[(size_t) i] + wg * wet[(size_t) i]);
    normalise (d);

    // corte do sample (como o ORCH5): p.cut segundos e fade rápido
    const int cut = (int) (p.cut * SR), fade = (int) (0.08 * SR);
    d.resize ((size_t) (cut + fade));
    for (int i = 0; i < fade; ++i) d[(size_t) (cut + i)] *= std::pow (1.0 - (double) i / fade, 2.0);

    // SAMPLER DOS ANOS 80: anti-alias, taxa reduzida (sample-and-hold) e poucos bits sem dither
    if (p.bits < 16 || p.srRec < SR)
    {
        const double aa = std::exp (-2 * kPi * p.srRec * .42 / SR);
        double s1 = 0, s2 = 0;
        for (auto& v : d) { s1 = s1 * aa + v * (1 - aa); s2 = s2 * aa + s1 * (1 - aa); v = s2; }
        normalise (d);
        const double step = SR / p.srRec, q = std::pow (2.0, p.bits - 1);
        double hold = 0, next = 0;
        for (size_t i = 0; i < d.size(); ++i)
        {
            if ((double) i >= next) { hold = std::round (d[i] * q) / q; next += step; }
            d[i] = hold;
        }
    }
    const double c = std::exp (-2 * kPi * p.lpf / SR);
    double y = 0;
    for (auto& v : d) { y = y * c + v * (1 - c); v = y; }
    normalise (d);
    return std::vector<float> (d.begin(), d.end());
}

// ------------------------------------------------------------------ tamanho da nota
constexpr int kNumLengths = 7;
inline const char* lengthName (int i) { static const char* n[kNumLengths] = { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "LIVRE" }; return n[std::clamp (i, 0, kNumLengths - 1)]; }
inline int lengthTicks (int i) { static const int t[kNumLengths] = { 1, 2, 4, 8, 16, 32, 0 }; return t[std::clamp (i, 0, kNumLengths - 1)]; }   // 0 = LIVRE

// ------------------------------------------------------------------ sampler
struct Envelope { double a = .002, d = .25, s = .6, r = .06; };

class Sampler
{
public:
    static constexpr int kVoices = 24;
    void prepare (double sampleRate) { sr = sampleRate; allOff(); }
    void setSample (const std::vector<float>* s) { sample = s; }
    Envelope env; bool twoHands = true; double hand = .75;

    // offAfter > 0: a nota se solta sozinha depois desse número de amostras (TAMANHO DA NOTA)
    void noteOn (int note, float vel, int64_t offAfter)
    {
        if (sample == nullptr || sample->empty()) return;
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
        *v = Voice {};
        v->active = true; v->note = note; v->vel = vel; v->age = ++counter;
        v->rate = std::pow (2.0, (note - kRootNote) / 12.0) * kSampleRate / sr;
        v->offAt = offAfter > 0 ? offAfter : -1;
    }
    void noteOff (int note)
    {
        for (auto& v : voices) if (v.active && v.note == note && v.stage < 3 && v.offAt < 0) v.stage = 3, v.relFrom = v.level;
    }
    void allOff() { for (auto& v : voices) v.active = false; }
    int activeVoices() const { int n = 0; for (auto& v : voices) n += v.active; return n; }

    // soma o hit (mono) em out
    void render (float* out, int n)
    {
        if (sample == nullptr) return;
        const auto& s = *sample;
        const double len = (double) s.size() - 1;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (int i = 0; i < n; ++i)
            {
                if (v.offAt >= 0 && v.clock >= v.offAt && v.stage < 3) { v.stage = 3; v.relFrom = v.level; }
                switch (v.stage)
                {
                    case 0: v.level += 1.0 / std::max (1.0, env.a * sr); if (v.level >= 1) { v.level = 1; v.stage = 1; } break;
                    case 1: v.level -= (1.0 - env.s) / std::max (1.0, env.d * sr); if (v.level <= env.s) { v.level = env.s; v.stage = 2; } break;
                    case 2: v.level = env.s; break;
                    default: v.level -= v.relFrom / std::max (1.0, env.r * sr); break;
                }
                if (v.level <= 0 && v.stage == 3) { v.active = false; break; }
                double x = 0; bool any = false;
                if (v.pos < len) { const int k = (int) v.pos; const double f = v.pos - k; x += 0.85 * (s[(size_t) k] + f * (s[(size_t) k + 1] - s[(size_t) k])); any = true; }
                const double p2 = v.pos * 2;
                if (twoHands && p2 < len) { const int k = (int) p2; const double f = p2 - k; x += 0.85 * hand * (s[(size_t) k] + f * (s[(size_t) k + 1] - s[(size_t) k])); any = true; }
                if (! any) { v.active = false; break; }
                out[i] += (float) (x * v.level * v.vel);
                v.pos += v.rate; ++v.clock;
            }
        }
    }

private:
    struct Voice { bool active = false; int note = 0; float vel = 1; uint64_t age = 0; double rate = 1, pos = 0, level = 0, relFrom = 0; int stage = 0; int64_t clock = 0, offAt = -1; };
    std::array<Voice, kVoices> voices {};
    const std::vector<float>* sample = nullptr;
    double sr = 48000; uint64_t counter = 0;
};

// ------------------------------------------------------------------ grade, grooves e MIDIs
struct GridNote { int tick, len, note; };
using Grid = std::vector<GridNote>;

constexpr int kRootDefault = 38;   // Ré (D1 nos nomes do Cubase): a nota base dos grooves e dos MIDIs
inline int progOffset (int bar) { static const int p[4] = { 0, 0, -4, -2 }; return p[bar & 3]; }   // Ré – Ré – Si♭ – Dó

struct Groove { const char* name; const char* info; std::array<std::vector<int>, 4> bars; };
inline const std::vector<Groove>& grooves()
{
    static const std::vector<Groove> g {
        { "PONTUAÇÃO NO 2 E 4", "hit junto da caixa em todo compasso",   {{ { 16, 48 }, { 16, 48 }, { 16, 48 }, { 16, 48 } }} },
        { "INTRO ANUNCIADA",    "1 · 2 · 3e · 4, depois só no 2",        {{ { 0, 16, 40, 48 }, { 16 }, { 16 }, { 16, 48 } }} },
        { "RESPOSTA 3a → 4",    "2 e 4 com puxada a cada 2 compassos",   {{ { 16, 48 }, { 16, 44, 48 }, { 16, 48 }, { 16, 44, 48 } }} },
        { "FIM DE FRASE",       "só no 4º compasso: 2 · 4 · 4e · 4a",    {{ {}, {}, {}, { 16, 48, 56, 60 } }} },
        { "CORTE DE DUB",       "um hit no 1 e espaço",                  {{ { 0 }, {}, { 0 }, {} }} } };
    return g;
}
inline Grid grooveGrid (int i, int len = 1)
{
    Grid g;
    const auto& G = grooves()[(size_t) std::clamp (i, 0, (int) grooves().size() - 1)];
    for (int b = 0; b < 4; ++b) for (int t : G.bars[(size_t) b]) g.push_back ({ b * kTicksPerBar + t, std::max (1, len), kRootDefault + progOffset (b) });
    return g;
}

struct Clip { const char* name; const char* info; std::vector<GridNote> notes; };   // nota relativa à base (0 = a linha onde soltar)
inline const std::vector<Clip>& clips()
{
    static const std::vector<Clip> c {
        { "STAB NO 2 E 4",        "1 compasso · junto da caixa",       { { 16, 1, 0 }, { 48, 1, 0 } } },
        { "RESPOSTA 3a → 4",      "2 compassos · puxada sincopada",    { { 16, 1, 0 }, { 48, 1, 0 }, { 80, 1, 0 }, { 108, 1, 0 }, { 112, 1, 0 } } },
        { "INTRO 1 · 2 · 3e · 4", "1 compasso · anúncio do riddim",    { { 0, 1, 0 }, { 16, 1, 0 }, { 40, 1, 0 }, { 48, 1, 0 } } },
        { "RÉ · RÉ · SI♭ · DÓ",   "4 compassos · stabs no 2 e 4",      { { 16, 1, 0 }, { 48, 1, 0 }, { 80, 1, 0 }, { 112, 1, 0 }, { 144, 1, -4 }, { 176, 1, -4 }, { 208, 1, -2 }, { 240, 1, -2 } } },
        { "VIRADA",               "1 compasso · no 4, subindo",        { { 48, 1, 0 }, { 52, 1, 2 }, { 56, 1, 3 }, { 60, 1, 5 } } },
        { "DOBRA EM 1/64",        "1 compasso · eco curto no 2 e 4",   { { 16, 1, 0 }, { 17, 1, 0 }, { 48, 1, 0 }, { 49, 1, 0 } } } };
    return c;
}

// coloca as notas de um MIDI na grade a partir do tick t0 com a base na nota root
inline void placeClip (Grid& g, const std::vector<GridNote>& notes, int t0, int root, int lo = 0, int hi = 127)
{
    for (auto n : notes)
    {
        const int t = t0 + n.tick, p = root + n.note;
        if (t >= 0 && t < kTotalTicks && p >= lo && p <= hi) g.push_back ({ t, std::min (n.len, kTotalTicks - t), p });
    }
}

// ------------------------------------------------------------------ bateria do PREVIEW (caixa no 2 e no 4)
class PreviewDrums
{
public:
    void prepare (double sampleRate) { sr = sampleRate; for (auto& h : hits) h.on = false; }
    // tick de 1/64 dentro do compasso
    void trigger (int tickInBar)
    {
        if (tickInBar % 4 == 0) add (Hat);
        if (tickInBar == 56) add (OpenHat);
        if (tickInBar == 0 || tickInBar == 40) add (Kick);
        if (tickInBar == 16 || tickInBar == 48) { add (Kick); add (Snare); }
    }
    void render (float* out, int n)
    {
        for (auto& h : hits)
        {
            if (! h.on) continue;
            for (int i = 0; i < n; ++i)
            {
                const double t = h.t / sr; double v = 0;
                if (h.type == Kick)
                {
                    const double f = 50 + 110 * std::exp (-t / .03);
                    h.ph += 2 * kPi * f / sr; v = std::sin (h.ph) * .9 * std::exp (-t / .07);
                    if (t > .45) h.on = false;
                }
                else
                {
                    const double nz = rng.next() * 2 - 1;
                    if (h.type == Snare)
                    {
                        h.ph += 2 * kPi * 200 / sr;
                        v = h.f.process (nz) * .55 * std::exp (-t / .035) + std::sin (h.ph) * .4 * std::exp (-t / .02);
                        if (t > .25) h.on = false;
                    }
                    else
                    {
                        const double d = h.type == OpenHat ? .2 : .04;
                        v = h.f.process (nz) * (h.type == OpenHat ? .18 : .14) * std::exp (-t / (d / 6.9));
                        if (t > d) h.on = false;
                    }
                }
                out[i] += (float) v; ++h.t;
                if (! h.on) break;
            }
        }
    }
private:
    enum Type { Kick, Snare, Hat, OpenHat };
    struct Hit { bool on = false; Type type = Kick; double t = 0, ph = 0; Biquad f; };
    void add (Type t)
    {
        for (auto& h : hits)
            if (! h.on)
            {
                h = Hit {}; h.on = true; h.type = t;
                if (t == Snare) h.f.set (Biquad::BP, 3500, .5, 0, sr); else if (t != Kick) h.f.set (Biquad::HP, 8000, .707, 0, sr);
                return;
            }
    }
    std::array<Hit, 16> hits {};
    double sr = 48000; Rng rng { 7 };
};

// nome da nota como no Cubase (C3 = dó central, MIDI 60)
inline const char* rootName (int n) { static const char* r[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return r[((n % 12) + 12) % 12]; }
inline int cubaseOctave (int n) { return n / 12 - 2; }
} // namespace moh
