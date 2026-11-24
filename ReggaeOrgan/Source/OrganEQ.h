// OrganEQ.h — o EQUALIZADOR do Mastor Reggae Organ MRO-68 (1.3) e o TIMBRE MIDNITE (header-only, sem JUCE)
//
// Quatro filtros na saída do órgão, depois da soma das três partes e antes do maximizer:
//   CORTE (passa-altas 12 dB/oit., 20 Hz = desligado) · GRAVES (shelf) · MÉDIOS (sino com Q) · AGUDOS (shelf)
// Filtros RBJ (Audio EQ Cookbook), iguais aos BiquadFilterNode da versão para celular (site/web/reggaeorgan/):
// ao mudar a tabela de presets ou o timbre Midnite, mude também EQ_PRESETS e MIDNITE no index.html.
#pragma once

#include <cmath>

namespace mro
{
struct EqParams
{
    bool  on = true;
    float hpf = 20.0f;                     // Hz (20 = sem corte)
    float lowG = 0.0f,  lowF = 120.0f;     // dB, Hz
    float midG = 0.0f,  midF = 1000.0f, midQ = 0.8f;
    float highG = 0.0f, highF = 5000.0f;
    float out = 0.0f;                      // ganho de saída (dB), para compensar o volume
    bool operator== (const EqParams& o) const
    {
        return on == o.on && hpf == o.hpf && lowG == o.lowG && lowF == o.lowF && midG == o.midG && midF == o.midF
            && midQ == o.midQ && highG == o.highG && highF == o.highF && out == o.out;
    }
};

// presets do EQ (só mexem no EQ)
struct EqPreset { const char* name; const char* sub; EqParams p; };
enum { NumEqPresets = 5 };
inline const EqPreset& eqPreset (int i)
{
    static const EqPreset t[NumEqPresets] = {
        { "PLANO",        "sem EQ",                           { true, 20.0f,   0.0f, 120.0f,  0.0f, 1000.0f, 0.8f,  0.0f, 5000.0f } },
        { "ROOTS QUENTE", "graves cheios, agudo macio",       { true, 40.0f,   3.0f, 150.0f,  1.0f,  700.0f, 0.7f, -3.0f, 6000.0f } },
        { "DUB ESCURO",   "sem agudo, para o eco",            { true, 60.0f,   2.0f, 120.0f, -2.0f, 2000.0f, 0.8f, -9.0f, 3500.0f } },
        { "DIGITAL 80",   "fino e brilhante",                 { true, 150.0f, -3.0f, 200.0f,  2.0f, 2500.0f, 0.9f,  4.0f, 7000.0f } },
        { "ABRE O BAIXO", "corta o grave para o baixo",       { true, 180.0f, -4.0f, 250.0f,  1.5f, 1200.0f, 0.8f,  0.0f, 5000.0f } },
    };
    return t[i < 0 ? 0 : (i >= NumEqPresets ? NumEqPresets - 1 : i)];
}

// TIMBRE MIDNITE (Ron Benjamin, Midnite · St. Croix, 1997-2015): órgão cru e seco, como nos discos gravados
// ao vivo direto em fita de dois canais (Unpolished, Ras Mek Peace), sem reverb e sem eco; bubble oco
// (8' + 4' + 2⅔'), chop cheio, sem percussão e sem key click, Leslie lenta e discreta; o EQ tira o grave
// do caminho do baixo pesado, tira um pouco de médio e compensa o volume na SAÍDA. Registros e EQ são uma aproximação de ouvido.
struct OrganTimbre
{
    const char* name;
    int bubbleDb[9], chopDb[9];
    bool perc;
    float drive, click, reverb;
    int lesPreset, lesSpeed;
    float lesDepth;
    EqParams eq;
};
constexpr float kMidniteOutDb = 5.5f;   // compensação de volume (medida no teste: mesmo RMS do timbre do Mittoo)
inline const OrganTimbre& midniteTimbre()
{
    static const OrganTimbre t {
        "MIDNITE",
        { 0, 0, 8, 6, 4, 0, 0, 0, 0 }, { 8, 6, 8, 6, 0, 0, 0, 0, 0 },
        false, 0.12f, 0.0f, 0.0f,        // sem percussão e sem key click: nada de ataque percussivo
        2, 0, 0.45f,
        { true, 110.0f, -2.0f, 220.0f, -2.5f, 1100.0f, 0.9f, -1.5f, 6500.0f, kMidniteOutDb }
    };
    return t;
}

// um biquad estéreo (forma direta transposta II)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] = {}, z2[2] = {};
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    inline float process (float x, int ch)
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }
    void setNorm (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    // tipos: 0 passa-altas, 1 low shelf, 2 sino, 3 high shelf (Q = 0.707 nos shelves: S = 1)
    void design (int type, double sr, double f, double gainDb, double q)
    {
        const double pi = 3.14159265358979323846;
        f = std::fmin (std::fmax (f, 10.0), sr * 0.45);
        const double w = 2.0 * pi * f / sr, cw = std::cos (w), sw = std::sin (w);
        const double A = std::pow (10.0, gainDb / 40.0);
        if (type == 0)
        {
            const double al = sw / (2.0 * 0.7071);
            setNorm ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
        }
        else if (type == 2)
        {
            const double al = sw / (2.0 * q);
            setNorm (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
        }
        else
        {
            const double al = sw / 2.0 * std::sqrt (2.0), sA = 2.0 * std::sqrt (A) * al;
            if (type == 1)
                setNorm (A * ((A + 1) - (A - 1) * cw + sA), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - sA),
                         (A + 1) + (A - 1) * cw + sA, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - sA);
            else
                setNorm (A * ((A + 1) + (A - 1) * cw + sA), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - sA),
                         (A + 1) - (A - 1) * cw + sA, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - sA);
        }
    }
    // resposta em dB numa frequência (para a curva da tela e para o teste)
    double magnitudeDb (double f, double sr) const
    {
        const double pi = 3.14159265358979323846, w = 2.0 * pi * f / sr;
        const double cr = std::cos (w), ci = -std::sin (w), c2r = std::cos (2 * w), c2i = -std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i;
        const double dr = 1 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
        return 10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1e-30);
    }
};

class OrganEQ
{
public:
    void prepare (double sampleRate) { sr = sampleRate; for (auto& b : bq) b.reset(); design(); gain = target; }
    void set (const EqParams& p) { if (! (p == cur)) { cur = p; dirty = true; } }
    const EqParams& params() const { return cur; }

    void process (float* L, float* R, int n)
    {
        if (dirty) design();
        if (! cur.on) return;
        for (int s = 0; s < n; ++s)
        {
            float l = L[s], r = R[s];
            for (int k = 0; k < 4; ++k)
                if (active[k]) { l = bq[k].process (l, 0); r = bq[k].process (r, 1); }
            gain += (target - gain) * 0.0015f;   // ganho de saída sem estalo
            L[s] = l * gain; R[s] = r * gain;
        }
    }

    // resposta total em dB (a mesma conta para a tela)
    static double responseDb (const EqParams& p, double f, double sampleRate = 48000.0)
    {
        if (! p.on) return 0.0;
        OrganEQ e; e.sr = sampleRate; e.cur = p; e.design();
        double db = p.out;
        for (int k = 0; k < 4; ++k) if (e.active[k]) db += e.bq[k].magnitudeDb (f, sampleRate);
        return db;
    }

private:
    void design()
    {
        dirty = false;
        target = (float) std::pow (10.0, cur.out / 20.0);
        bq[0].design (0, sr, cur.hpf, 0, 0.7071);
        bq[1].design (1, sr, cur.lowF, cur.lowG, 0.7071);
        bq[2].design (2, sr, cur.midF, cur.midG, std::fmax (0.2f, cur.midQ));
        bq[3].design (3, sr, cur.highF, cur.highG, 0.7071);
        const bool now[4] = { cur.hpf > 20.5f, std::fabs (cur.lowG) > 0.05f, std::fabs (cur.midG) > 0.05f, std::fabs (cur.highG) > 0.05f };
        for (int k = 0; k < 4; ++k)
        {
            if (now[k] && ! active[k]) bq[k].reset();   // filtro que volta a valer começa sem memória velha
            active[k] = now[k];
        }
    }
    double sr = 48000.0;
    EqParams cur;
    Biquad bq[4];
    bool active[4] = {};
    bool dirty = true;
    float gain = 1.0f, target = 1.0f;
};
} // namespace mro
