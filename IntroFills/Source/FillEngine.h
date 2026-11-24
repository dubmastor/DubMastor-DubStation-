// FillEngine.h — Mastor Intro Fills MIF: as viradas de introdução, o groove de chegada e o "feel" (humanização)
//
// Mesmo conteúdo e mesmas regras da versão para celular (site/web/introfills/, funções buildSequence e perform):
// ao mudar um, mude o outro. Tudo na contagem da Dub Station: 16 semicolcheias por compasso, a caixa/aro no 2 e no 4
// (passos 4 e 12). Na virada, o passo 16 é o "1" do groove (a chegada).
// Header-only, sem JUCE (testável em tests/render_test.cpp).
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace mif
{
// ---------------- peças (notas do Mastor Reggae Drums MRD-12) ----------------
enum Piece { Bumbo = 0, Aro, Caixa, Timbal, HHF, Surdo, HHP, HHA, Tom2, Prato, Tom1, NumPieces };

struct PieceInfo { int note; const char* file; const char* name; float gain; bool hat, open; };

inline const PieceInfo& piece (int p)
{
    static const PieceInfo info[NumPieces] = {
        { 36, "DM-KICKS1",   "BUMBO",      1.00f, false, false },
        { 37, "DM-SNARES28", "ARO",        0.90f, false, false },
        { 38, "DM-SNARES6",  "CAIXA",      0.95f, false, false },
        { 40, "SYN-SNARE3",  "TIMBAL",     0.80f, false, false },
        { 42, "DM-HATs6",    "HH FECHADO", 0.55f, true,  false },
        { 43, "SYN-SURDO",   "SURDO",      0.90f, false, false },
        { 44, "DM-HATs39",   "HH PEDAL",   0.45f, true,  false },
        { 46, "DM-HATs17",   "HH ABERTO",  0.55f, true,  true  },
        { 47, "SYN-TOM2",    "TOM 2",      0.85f, false, false },
        { 49, "DM-PRATOS1",  "PRATO",      0.45f, false, false },
        { 50, "SYN-TOM1",    "TOM 1",      0.85f, false, false },
    };
    return info[std::clamp (p, 0, NumPieces - 1)];
}

// linhas da grade, de cima para baixo
inline const std::array<int, NumPieces>& rows()
{
    static const std::array<int, NumPieces> r { Prato, HHA, HHF, HHP, Timbal, Tom1, Tom2, Surdo, Caixa, Aro, Bumbo };
    return r;
}

inline int pieceForNote (int note)
{
    for (int p = 0; p < NumPieces; ++p) if (piece (p).note == note) return p;
    return -1;
}

// ---------------- viradas ----------------
// s = posição em semicolcheias (16 = o 1 do groove), v = força 1 (fraca), 2 (média), 3 (forte)
struct Ev { double s; int p; int v; bool drag = false, flam = false, groove = false, ghost = false; };
using Fill = std::vector<Ev>;

struct FillDef { const char* name; Fill ev; };
struct Drummer { const char* name; const char* label; const char* tag; std::vector<FillDef> fills; };

inline Ev e (double s, int p, int v) { return { s, p, v }; }
inline Ev eDrag (double s, int p, int v) { Ev x { s, p, v }; x.drag = true; return x; }
inline Ev eFlam (double s, int p, int v) { Ev x { s, p, v }; x.flam = true; return x; }

inline const std::vector<Drummer>& drummers()
{
    static const std::vector<Drummer> d = [] {
        std::vector<Drummer> v;
        v.push_back ({ "Carlton Barrett", "CARLTON\nBARRETT", "WAILERS", {
            { "Drag e estalo", { e (0, HHP, 1), e (4, HHP, 1), e (8, HHP, 1), e (12, HHP, 1), eDrag (16, Caixa, 3) } },
            { "Tercinas caixa e tom", { e (0, HHP, 1), e (4, HHP, 1),
                e (8, Caixa, 1), e (8 + 2.0 / 3, Caixa, 1), e (8 + 4.0 / 3, Caixa, 1), e (10, Caixa, 2), e (10 + 2.0 / 3, Caixa, 2), e (10 + 4.0 / 3, Caixa, 2),
                e (12, Tom2, 2), e (14, Surdo, 3), e (15, Surdo, 2), e (16, Caixa, 3) } } } });
        v.push_back ({ "Sly Dunbar", "SLY\nDUNBAR", "REVOLUTIONARIES", {
            { "Timbales picados", { e (0, HHP, 1), e (4, HHP, 1), e (8, Timbal, 3), e (9, Timbal, 1), e (10, Tom1, 3), e (11, Timbal, 1),
                                    e (12, Tom1, 3), e (14, Surdo, 3), e (15, Surdo, 2), e (16, Bumbo, 3) } },
            { "Syndrum seco", { e (8, Caixa, 3), e (11, Tom1, 2), e (12, Tom2, 3), e (13, Surdo, 2), e (14, Bumbo, 3), e (16, Bumbo, 3) } } } });
        v.push_back ({ "Style Scott", "STYLE\nSCOTT", "ROOTS RADICS", {
            { "Aro seco e flam", { e (0, HHP, 1), e (4, HHP, 1), e (8, HHP, 1), e (12, HHP, 1), e (9, Aro, 3), e (11, Aro, 3), eFlam (12, Caixa, 3), e (15, Aro, 2) } },
            { "Dub com espaço", { e (12, Aro, 3), e (14, Caixa, 2), e (15, HHA, 2) } } } });
        v.push_back ({ "Horsemouth", "HORSE\nMOUTH", "FLYING CYMBAL", {
            { "Pickup e flying", { e (0, HHP, 1), e (4, HHP, 1), e (8, HHP, 1), e (13, Caixa, 2), e (14, Caixa, 1), e (15, Caixa, 3) } } } });
        {
            Fill rufoAro; const int vv[8] = { 3, 1, 1, 3, 1, 1, 3, 3 };
            for (int i = 0; i < 8; ++i) rufoAro.push_back (e (8 + i, Aro, vv[i]));
            rufoAro.push_back (e (16, Caixa, 3));
            Fill rufo { e (0, HHP, 1), e (4, HHP, 1), e (8, HHP, 1) };
            for (int i = 0; i < 8; ++i) rufo.push_back (e (12 + i / 2.0, Caixa, i < 3 ? 1 : (i < 6 ? 2 : 3)));
            rufo.push_back (e (16, Caixa, 3));
            v.push_back ({ "Santa Davis", "SANTA\nDAVIS", "SOUL SYNDICATE", { { "Rufo de aro", rufoAro }, { "Rufo fechado", rufo } } });
        }
        v.push_back ({ "Mikey Boo", "MIKEY\nBOO", "BLACK UHURU", {
            { "Caixa e tons", { e (8, Caixa, 2), e (10, Caixa, 2), e (12, Tom1, 3), e (13, Tom2, 2), e (14, Surdo, 3), e (15, Bumbo, 2), e (16, Caixa, 3) } } } });
        return v;
    }();
    return d;
}

inline const char* styleName (int i) { static const char* n[5] = { "One drop", "Rockers", "Steppers", "Dub", "Rocksteady" }; return n[std::clamp (i, 0, 4)]; }
inline const char* sizeName (int i)  { static const char* n[4] = { "Anacruse", "1/2 compasso", "1 compasso", "2 compassos" }; return n[std::clamp (i, 0, 3)]; }
constexpr int NumStyles = 5, NumSizes = 4;

// groove de um compasso (caixa/aro no 2 e no 4: passos 4 e 12)
inline Fill grooveBar (int style)
{
    Fill g;
    auto add = [&] (std::initializer_list<int> steps, int p, int v) { for (int s : steps) g.push_back (e (s, p, v)); };
    switch (style)
    {
        case 0: add ({ 0, 2, 4, 6, 8, 10, 12, 14 }, HHF, 2); add ({ 15 }, HHF, 1); add ({ 4, 12 }, Bumbo, 3); add ({ 4, 12 }, Aro, 3); break;   // one drop
        case 1: add ({ 0, 2, 4, 6, 8, 10, 12, 14 }, HHF, 2); add ({ 0, 4, 8, 12 }, Bumbo, 3); add ({ 4, 12 }, Aro, 3); break;                   // rockers
        case 2: add ({ 0, 2, 4, 6, 8, 10, 12, 14 }, HHF, 2); add ({ 0, 2, 4, 6, 8, 10, 12, 14 }, Bumbo, 3); add ({ 4, 12 }, Aro, 3); break;     // steppers
        case 3: add ({ 2, 6, 10, 14 }, HHA, 2); add ({ 0, 4, 8, 12 }, HHP, 1); add ({ 4, 12 }, Bumbo, 3); add ({ 4, 12 }, Aro, 3); break;      // dub
        default: { for (int i = 0; i < 16; ++i) g.push_back (e (i, HHF, 1)); add ({ 4, 12 }, Aro, 3); add ({ 4, 12 }, Bumbo, 3); }   // rocksteady
    }
    return g;
}

inline int regionStart (int size) { static const int r[4] = { 14, 8, 0, 0 }; return r[std::clamp (size, 0, 3)]; }

// ---------------- gerador aleatório igual ao da versão web (Park-Miller) ----------------
struct Rng
{
    double seed = 1;
    double rnd() { seed = std::fmod (seed * 16807.0, 2147483647.0); return seed / 2147483647.0; }
    double gauss()
    {
        double u = 0, v = 0;
        while (u == 0) u = rnd();
        while (v == 0) v = rnd();
        return std::sqrt (-2.0 * std::log (u)) * std::cos (2.0 * 3.14159265358979 * v);
    }
};

struct Settings
{
    int drummer = 0, fill = 0, style = 0, size = 2, land = 0;    // land 0 = chega com o estalo, 1 = sem o estalo
    double intensity = 0.72, human = 1.0, swing = 0.64, ghost = 0.2;
    double bpm = 74;
};

struct Sequence { std::vector<Ev> evs; double total = 0, g0 = 0; };

// a virada (com as edições do usuário) + ghost notes + 2 compassos do groove
inline Sequence buildSequence (const Settings& st, const Fill& fill, Rng& rng)
{
    rng.seed = 7 + st.drummer * 31 + st.fill * 7 + st.style;
    Sequence q;
    double barOff = 0;
    if (st.size == 3) { for (int s : { 0, 4, 8, 12 }) q.evs.push_back (e (s, HHP, 1)); barOff = 16; }
    const int rs = regionStart (st.size);
    for (auto x : fill)
    {
        if (x.s >= 16) { if (st.land == 0) { x.s += barOff; q.evs.push_back (x); } continue; }
        if (x.s < rs) continue;
        x.s += barOff;
        q.evs.push_back (x);
    }
    for (int s = std::max (rs, 8); s < 16; ++s)     // ghost notes na região da virada
    {
        const bool used = std::any_of (fill.begin(), fill.end(), [&] (const Ev& x) { return (int) std::floor (x.s) == s; });
        if (! used && rng.rnd() < st.ghost * 0.5) { Ev g = e (s + barOff, Caixa, 0); g.ghost = true; q.evs.push_back (g); }
    }
    q.g0 = barOff + 16;
    const auto gb = grooveBar (st.style);
    for (int b = 0; b < 2; ++b)
        for (auto x : gb)
        {
            const double s = q.g0 + b * 16 + x.s;
            if (b == 0 && x.s == 0 && st.land == 0
                && std::any_of (q.evs.begin(), q.evs.end(), [&] (const Ev& y) { return y.s == q.g0; }))
                if (x.p == HHF || x.p == HHP) continue;
            x.s = s; x.groove = true;
            q.evs.push_back (x);
        }
    q.total = q.g0 + 32;
    return q;
}

// passo -> tempo em semínimas, com o swing nas semicolcheias (tercinas ficam retas)
inline double stepToBeats (double s, double swing)
{
    const double beat = std::floor (s / 4), frac = s - beat * 4;
    const bool isInt = std::abs (frac - std::round (frac)) < 1e-6;
    double off;
    if (isInt) { const int sub = (int) std::lround (frac); const double o[4] = { 0, swing / 2, 0.5, 0.5 + swing / 2 }; off = o[sub & 3]; }
    else off = frac / 4;
    return beat + off;
}

struct Note { int p; double beat; int vel; };   // beat = posição em semínimas desde o começo da sequência

// o "feel": chimbal com acento no tempo, caixa do 2 e do 4 atrás do tempo, virada crescendo até a chegada,
// mão esquerda mais fraca nas notas rápidas, drift lento de tempo; drag e flam viram notas de enfeite
inline std::vector<Note> perform (const Settings& st, const Sequence& seq, Rng& rng)
{
    const double h = st.human, I = 0.6 + 0.55 * st.intensity, msToBeats = st.bpm / 60.0 / 1000.0;
    auto evs = seq.evs;
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.s < b.s; });
    std::vector<const Ev*> fillNotes;
    for (auto& x : evs) if (! x.groove && ! x.ghost && x.p != HHP && x.s < seq.g0) fillNotes.push_back (&x);
    std::vector<Note> out;
    double drift = 0, lastS = -1, prevFillS = -99;
    int hand = 0;
    for (auto& x : evs)
    {
        if (x.s != lastS) { drift = drift * 0.85 + rng.gauss() * 1.4 * h; drift = std::clamp (drift, -9 * h, 9 * h); lastS = x.s; }
        const double posInBeat = std::fmod (std::fmod (x.s, 4.0) + 4.0, 4.0);
        const bool onBeat = std::abs (posInBeat) < 1e-6;
        const int beat = ((int) std::floor (x.s / 4)) % 4;
        double v, dt = 0, jit = 2.5;
        if (x.ghost) { v = 24 + rng.rnd() * 12; jit = 4; }
        else if (x.groove)
        {
            if (x.p == HHF) { v = onBeat ? (beat == 1 || beat == 3 ? 86 : 76) : (std::abs (posInBeat - 2) < 1e-6 ? 58 : 44); jit = 2; }
            else if (x.p == HHA) { v = 72; dt = 3; }
            else if (x.p == HHP) { v = 50; dt = 4; }
            else if (x.p == Bumbo) { v = (st.style == 1 || st.style == 2) && ! (beat == 1 || beat == 3) ? 96 : 110; dt = 2; jit = 1.6; }
            else if (x.p == Aro || x.p == Caixa) { v = 106; dt = 11; jit = 1.8; }
            else { const double t[4] = { 0, 58, 86, 112 }; v = t[std::clamp (x.v, 0, 3)]; }
            v *= I; v += rng.gauss() * (2 + 5 * h);
        }
        else if (x.p == HHP) { v = 46 + rng.gauss() * 4 * h; dt = 3; }
        else
        {
            const double t[4] = { 0, 52, 82, 108 };
            v = t[std::clamp (x.v, 0, 3)];
            if (x.s >= seq.g0) { v = std::max (v, 118.0); dt = 7; }        // chegada: o estalo mais forte, levemente atrás
            else
            {
                const auto it = std::find (fillNotes.begin(), fillNotes.end(), &x);
                const int i = (int) (it - fillNotes.begin()), n = (int) fillNotes.size();
                const double prog = n > 1 ? (double) i / (n - 1) : 1;
                v *= 0.8 + 0.28 * prog;                                     // crescendo da virada
                const double gap = x.s - prevFillS;
                if (gap <= 1.01) { hand ^= 1; if (hand) v *= 0.9; }
                else { hand = 0; if (i + 1 < n && fillNotes[(size_t) i + 1]->s - x.s <= 1.01) v *= 0.88; }
                prevFillS = x.s;
                dt = -3 * prog;                                             // viradas empurram um pouco no fim
                jit = 3.2;
            }
            v *= I; v += rng.gauss() * (3 + 6 * h);
        }
        const int vel = (int) std::clamp (std::round (v), 1.0, 127.0);
        const double tMs = dt * (0.35 + 0.65 * h) + drift + rng.gauss() * jit * h;
        const double t = stepToBeats (x.s, st.swing) + tMs * msToBeats;
        if (x.drag)
        {
            const double g1 = -74 + rng.gauss() * 4 * h, v1 = std::round (vel * (0.26 + rng.rnd() * 0.06));
            const double g2 = -41 + rng.gauss() * 3 * h, v2 = std::round (vel * (0.32 + rng.rnd() * 0.06));
            out.push_back ({ x.p, t + g1 * msToBeats, (int) std::clamp (v1, 1.0, 127.0) });
            out.push_back ({ x.p, t + g2 * msToBeats, (int) std::clamp (v2, 1.0, 127.0) });
        }
        if (x.flam)
        {
            const double g1 = -26 + rng.gauss() * 4 * h, v1 = std::round (vel * (0.3 + rng.rnd() * 0.08));
            out.push_back ({ x.p, t + g1 * msToBeats, (int) std::clamp (v1, 1.0, 127.0) });
        }
        out.push_back ({ x.p, t, vel });
    }
    std::stable_sort (out.begin(), out.end(), [] (const Note& a, const Note& b) { return a.beat < b.beat; });
    return out;
}

// ---------------- arquivo .mid (tipo 0, 480 ticks por semínima, canal 10) ----------------
inline std::vector<uint8_t> makeMidiFile (const std::vector<Note>& notes, double bpm, const std::string& name)
{
    const int PPQ = 480;
    struct M { long t; int order; uint8_t a, b, c; };
    std::vector<M> ev;
    for (auto& n : notes)
    {
        const long t = std::max (0L, std::lround (n.beat * PPQ));
        const uint8_t note = (uint8_t) piece (n.p).note;
        ev.push_back ({ t, 1, 0x99, note, (uint8_t) std::clamp (n.vel, 1, 127) });
        ev.push_back ({ t + 60, 0, 0x89, note, 0 });
    }
    std::stable_sort (ev.begin(), ev.end(), [] (const M& a, const M& b) { return a.t != b.t ? a.t < b.t : a.order < b.order; });
    std::vector<uint8_t> trk;
    auto vlq = [&] (unsigned long n)
    {
        uint8_t buf[5]; int k = 0; buf[k++] = (uint8_t) (n & 127); n >>= 7;
        while (n) { buf[k++] = (uint8_t) ((n & 127) | 128); n >>= 7; }
        while (k) trk.push_back (buf[--k]);
    };
    vlq (0); trk.insert (trk.end(), { 0xff, 0x03 }); vlq (name.size()); trk.insert (trk.end(), name.begin(), name.end());
    const long tempo = std::lround (60000000.0 / std::max (20.0, bpm));
    vlq (0); trk.insert (trk.end(), { 0xff, 0x51, 3, (uint8_t) ((tempo >> 16) & 255), (uint8_t) ((tempo >> 8) & 255), (uint8_t) (tempo & 255) });
    vlq (0); trk.insert (trk.end(), { 0xff, 0x58, 4, 4, 2, 24, 8 });
    long last = 0;
    for (auto& m : ev) { vlq ((unsigned long) (m.t - last)); trk.insert (trk.end(), { m.a, m.b, m.c }); last = m.t; }
    vlq (0); trk.insert (trk.end(), { 0xff, 0x2f, 0 });
    std::vector<uint8_t> f { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, (uint8_t) (PPQ >> 8), (uint8_t) (PPQ & 255), 'M', 'T', 'r', 'k' };
    const uint32_t L = (uint32_t) trk.size();
    f.insert (f.end(), { (uint8_t) (L >> 24), (uint8_t) (L >> 16), (uint8_t) (L >> 8), (uint8_t) L });
    f.insert (f.end(), trk.begin(), trk.end());
    return f;
}
} // namespace mif
