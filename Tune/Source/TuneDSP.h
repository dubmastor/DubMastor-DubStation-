#pragma once
// Mastor Tune MT-0420 — DSP da afinação. Independente do JUCE (testado em tests/render_test.cpp).
//
//  PitchTracker  : detecção de afinação YIN numa cópia reduzida (~11 kHz) da voz
//  Psola         : troca de afinação TD-PSOLA. Os grãos são recortados da própria voz,
//                  então o timbre (formantes) não muda: só a nota.
//  Corrector     : decide a nota certa (escala, acordes, notas corrigidas no editor) e a
//                  velocidade da correção (knob CLEAN <-> ROBÔ)
//  RiddimEar     : escuta o riddim (entrada lateral) e acha o tom e o acorde do momento
//  VoiceEQ, Saturator : processamento da aba PROCESSAMENTO

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <complex>
#include <cstdint>
#include <cstdlib>
#include <climits>

namespace mt
{
static constexpr float kPi = 3.14159265358979f;

//==============================================================================
// Escalas (máscara de 12 bits a partir da tônica)
struct ScaleDef { const char* name; std::array<int, 12> steps; int n; };

inline const std::array<ScaleDef, 12>& scales()
{
    static const std::array<ScaleDef, 12> s {{
        { "Maior",        { 0, 2, 4, 5, 7, 9, 11 }, 7 },
        { "Menor",        { 0, 2, 3, 5, 7, 8, 10 }, 7 },
        { "Dorico",       { 0, 2, 3, 5, 7, 9, 10 }, 7 },
        { "Mixolidio",    { 0, 2, 4, 5, 7, 9, 10 }, 7 },
        { "Frigio",       { 0, 1, 3, 5, 7, 8, 10 }, 7 },
        { "Lidio",        { 0, 2, 4, 6, 7, 9, 11 }, 7 },
        { "Locrio",       { 0, 1, 3, 5, 6, 8, 10 }, 7 },
        { "Harm. menor",  { 0, 2, 3, 5, 7, 8, 11 }, 7 },
        { "Penta maior",  { 0, 2, 4, 7, 9 }, 5 },
        { "Penta menor",  { 0, 3, 5, 7, 10 }, 5 },
        { "Blues",        { 0, 3, 5, 6, 7, 10 }, 6 },
        { "Cromatica",    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, 12 },
    }};
    return s;
}
enum ScaleId { Maior = 0, Menor = 1, Cromatica = 11, NumScales = 12 };

inline uint16_t scaleMask (int key, int scale)
{
    const auto& d = scales()[(size_t) std::clamp (scale, 0, NumScales - 1)];
    uint16_t m = 0;
    for (int i = 0; i < d.n; ++i) m |= (uint16_t) (1u << ((key + d.steps[(size_t) i]) % 12));
    return m;
}

inline uint16_t triadMask (int root, bool minor)
{
    root = ((root % 12) + 12) % 12;
    return (uint16_t) ((1u << root) | (1u << ((root + (minor ? 3 : 4)) % 12)) | (1u << ((root + 7) % 12)));
}

inline float hzToMidi (float hz) { return 69.0f + 12.0f * std::log2 (hz / 440.0f); }

//==============================================================================
// Biquad simples (RBJ)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
    }
    void lowPass (double fs, double f, double q)
    {
        const double w = 2.0 * 3.141592653589793 * std::min (f, fs * 0.49) / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass (double fs, double f, double q)
    {
        const double w = 2.0 * 3.141592653589793 * std::min (f, fs * 0.49) / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peak (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * 3.141592653589793 * std::min (f, fs * 0.49) / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
};

//==============================================================================
// Detecção de afinação (YIN) a ~11 kHz
struct PitchResult { float hz = 0.0f; float confidence = 0.0f; float rmsDb = -120.0f; };

class PitchTracker
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        D = std::max (1, (int) std::lround (sampleRate / 11025.0));
        dfs = sampleRate / (double) D;
        for (int i = 0; i < 2; ++i) { aa[i].reset(); aa[i].lowPass (sampleRate, dfs * 0.42, i == 0 ? 0.541 : 1.307); }
        std::fill (std::begin (ring), std::end (ring), 0.0f);
        w = 0; phase = 0; hopCount = 0; last = {};
        hist[0] = hist[1] = hist[2] = 0.0f;
    }

    // faixa da voz: 0 = grave, 1 = média, 2 = aguda
    void setVoice (int v)
    {
        static const float lo[3] = { 60.0f, 80.0f, 120.0f }, hi[3] = { 420.0f, 750.0f, 1200.0f };
        v = std::clamp (v, 0, 2);
        fMin = lo[v]; fMax = hi[v];
    }

    int  hopFullRate() const { return kHop * D; }
    // atraso entre o centro da janela analisada e a amostra mais nova (amostras na taxa cheia)
    int  analysisDelay() const { return (kN / 2) * D; }

    // devolve true quando sai uma análise nova (a cada kHop amostras reduzidas)
    inline bool push (float x, PitchResult& out)
    {
        float y = aa[1].process (aa[0].process (x));
        if (++phase < D) return false;
        phase = 0;
        ring[w] = y;
        w = (w + 1) & (kRing - 1);
        if (++hopCount < kHop) return false;
        hopCount = 0;
        analyse();
        out = last;
        return true;
    }

private:
    static constexpr int kRing = 1024, kN = 512, kHop = 64;

    void analyse()
    {
        float buf[kN];
        double e = 0;
        for (int i = 0; i < kN; ++i)
        {
            buf[i] = ring[(w - kN + i) & (kRing - 1)];
            e += (double) buf[i] * buf[i];
        }
        const float rms = (float) std::sqrt (e / kN);
        last.rmsDb = 20.0f * std::log10 (rms + 1e-9f);

        const int tauMin = std::max (2, (int) std::floor (dfs / fMax));
        const int tauMax = std::min (kN / 2 - 2, (int) std::ceil (dfs / fMin));
        const int W = kN - tauMax - 1;
        float d[kN / 2 + 2];
        d[0] = 1.0f;
        double run = 0.0;
        for (int tau = 1; tau <= tauMax + 1; ++tau)
        {
            double s = 0.0;
            for (int j = 0; j < W; ++j) { const float df = buf[j] - buf[j + tau]; s += (double) df * df; }
            run += s;
            d[tau] = run > 0.0 ? (float) (s * tau / run) : 1.0f;
        }

        int best = -1;
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            if (d[tau] < 0.15f)
            {
                while (tau + 1 <= tauMax && d[tau + 1] < d[tau]) ++tau;
                best = tau; break;
            }
        }
        if (best < 0)
        {
            int m = tauMin;
            for (int tau = tauMin; tau <= tauMax; ++tau) if (d[tau] < d[m]) m = tau;
            if (d[m] < 0.3f) best = m;
        }

        float hz = 0.0f, conf = 0.0f;
        if (best > 0 && last.rmsDb > -55.0f)
        {
            float t = (float) best;
            if (best > 1 && best < tauMax)
            {
                const float a = d[best - 1], b = d[best], c = d[best + 1], den = a - 2 * b + c;
                if (std::abs (den) > 1e-9f) t += 0.5f * (a - c) / den;
            }
            hz = (float) (dfs / t);
            conf = 1.0f - d[best];
        }
        // mediana de 3 tira os erros de oitava isolados
        hist[0] = hist[1]; hist[1] = hist[2]; hist[2] = hz;
        float a = hist[0], b = hist[1], c = hist[2];
        float med = std::max (std::min (a, b), std::min (std::max (a, b), c));
        if (hz > 0.0f && med > 0.0f && std::abs (std::log2 (med / hz)) < 0.08f) med = hz;   // segue rápido quando concorda
        last.hz = (hz > 0.0f) ? (med > 0.0f ? med : hz) : 0.0f;
        last.confidence = conf;
    }

    double fs = 44100.0, dfs = 11025.0;
    int D = 4, phase = 0, hopCount = 0, w = 0;
    float fMin = 80.0f, fMax = 750.0f;
    float ring[kRing] {};
    float hist[3] {};
    Biquad aa[2];
    PitchResult last;
};

//==============================================================================
// TD-PSOLA em tempo real. Entrada e saída na taxa cheia; atraso fixo = latency().
// Marcas de análise acompanham os ciclos da voz; marcas de síntese ficam a cada período/razão.
class Psola
{
public:
    void prepare (double sampleRate, int voiceType = 1)
    {
        fs = sampleRate;
        lat = (int) std::ceil (0.032 * sampleRate);
        maxPeriod = (int) (lat / 2.6);
        size = 1;
        while (size < lat * 4 + 4096) size <<= 1;
        in.assign ((size_t) size, 0.0f);
        lp.assign ((size_t) size, 0.0f);
        out.assign ((size_t) size, 0.0f);
        win.assign (4097, 0.0f);
        for (int i = 0; i <= 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * kPi * (float) i / 4096.0f);
        marks.assign (256, 0);
        tl.assign (128, {});
        markLp.reset(); markLp.lowPass (sampleRate, 900.0, 0.707);
        (void) voiceType;
        reset();
    }

    void reset()
    {
        std::fill (in.begin(), in.end(), 0.0f);
        std::fill (lp.begin(), lp.end(), 0.0f);
        std::fill (out.begin(), out.end(), 0.0f);
        n = 0; nextSynth = (int64_t) 0; markCount = 0; markHead = 0; lastMark = 0;
        for (auto& t : tl) t = {};
        tlHead = 0;
        defaultPeriod = (float) (fs / 200.0);
        markLp.reset();
    }

    int latency() const { return lat; }
    int maxAllowedPeriod() const { return maxPeriod; }

    // informa (em tempo absoluto de entrada) o período e a razão de afinação de um trecho
    void setControl (int64_t time, float periodSamples, float ratio, bool voiced)
    {
        Ctl c; c.t = time; c.period = voiced ? std::clamp (periodSamples, 24.0f, (float) maxPeriod) : 0.0f; c.ratio = ratio; c.voiced = voiced;
        tl[(size_t) tlHead] = c;
        tlHead = (tlHead + 1) % (int) tl.size();
    }

    // processa uma amostra: entra x (agora), sai a amostra de latency() atrás, afinada
    inline float process (float x)
    {
        in[(size_t) (n & (size - 1))] = x;
        lp[(size_t) (n & (size - 1))] = markLp.process (x);
        const int64_t outT = n - lat;

        placeAnalysisMarks();

        // marcas de síntese: cada grão começa no ponto de leitura atual (nunca no passado)
        int guard = 0;
        while (outT >= 0 && guard++ < 8)
        {
            const Ctl c = ctlAt (nextSynth);
            const float pa = c.voiced ? c.period : defaultPeriod;
            if ((float) nextSynth - pa > (float) outT) break;
            const float ratio = c.voiced ? std::clamp (c.ratio, 0.5f, 2.0f) : 1.0f;
            addGrain (nextSynth, pa, ratio, outT);
            nextSynth += std::max<int64_t> (8, (int64_t) std::lround (pa / ratio));
        }

        float y = 0.0f;
        if (outT >= 0)
        {
            auto& o = out[(size_t) (outT & (size - 1))];
            y = o; o = 0.0f;
        }
        ++n;
        return y;
    }

    // amostra seca com o mesmo atraso (para misturar sem mudar o tempo)
    inline float dryDelayed() const
    {
        const int64_t t = n - 1 - lat;
        return t >= 0 ? in[(size_t) (t & (size - 1))] : 0.0f;
    }

private:
    struct Ctl { int64_t t = -1; float period = 0; float ratio = 1; bool voiced = false; };

    Ctl ctlAt (int64_t t) const
    {
        // controle mais recente com tempo <= t (ou o mais antigo disponível)
        Ctl best; int64_t bestT = INT64_MIN;
        Ctl oldest; int64_t oldestT = INT64_MAX;
        for (const auto& c : tl)
        {
            if (c.t < 0) continue;
            if (c.t <= t && c.t > bestT) { best = c; bestT = c.t; }
            if (c.t < oldestT) { oldest = c; oldestT = c.t; }
        }
        return bestT != INT64_MIN ? best : oldest;
    }

    void placeAnalysisMarks()
    {
        // marca nova quando já temos amostras para procurar o pico do ciclo
        const int64_t avail = n - 8;
        for (int guard = 0; guard < 4; ++guard)
        {
            const Ctl c = ctlAt (lastMark);
            const float p = c.voiced ? c.period : defaultPeriod;
            const int64_t pred = lastMark + (int64_t) std::lround (p);
            const int r = c.voiced ? (int) (p * 0.3f) : 0;
            if (pred + r > avail) break;
            int64_t m = pred;
            if (r > 0)
            {
                float bestV = -1.0f;
                for (int64_t k = pred - r; k <= pred + r; ++k)
                {
                    const float v = lp[(size_t) (k & (size - 1))];
                    if (v > bestV) { bestV = v; m = k; }
                }
                // pico positivo do ciclo (marca estável no mesmo ponto da onda)
            }
            if (m <= lastMark) m = lastMark + 1;
            lastMark = m;
            marks[(size_t) markHead] = m;
            markHead = (markHead + 1) % (int) marks.size();
            markCount = std::min (markCount + 1, (int) marks.size());
        }
    }

    int64_t nearestMark (int64_t t) const
    {
        int64_t best = t; int64_t bd = INT64_MAX;
        for (int i = 0; i < markCount; ++i)
        {
            const int64_t m = marks[(size_t) ((markHead - 1 - i + (int) marks.size()) % (int) marks.size())];
            const int64_t dd = std::llabs (m - t);
            if (dd < bd) { bd = dd; best = m; }
            if (m < t - 2 * maxPeriod) break;
        }
        return best;
    }

    void addGrain (int64_t s, float pa, float ratio, int64_t outT)
    {
        int64_t a = nearestMark (s);
        const int P = std::max (8, (int) std::lround (pa));
        // o grão precisa de amostras até a + P (já recebidas)
        if (a + P > n) a = n - P;
        const float gain = 1.0f / ratio;   // normaliza a soma dos grãos sobrepostos
        const float step = 4096.0f / (float) (2 * P);
        for (int k = -P; k < P; ++k)
        {
            const int64_t ot = s + k;
            if (ot < outT) continue;   // nunca escreve no que já saiu
            const float wv = win[(size_t) std::min (4096, (int) ((float) (k + P) * step))];
            out[(size_t) (ot & (size - 1))] += in[(size_t) ((a + k) & (size - 1))] * wv * gain;
        }
    }

    double fs = 44100.0;
    int lat = 1411, maxPeriod = 540, size = 8192;
    int64_t n = 0, nextSynth = 0, lastMark = 0;
    float defaultPeriod = 220.0f;
    std::vector<float> in, lp, out, win;
    std::vector<int64_t> marks;
    int markCount = 0, markHead = 0;
    std::vector<Ctl> tl;
    int tlHead = 0;
    Biquad markLp;
};

//==============================================================================
// Escolha da nota e da correção. amount: 0 = clean (transparente) .. 1 = robô (hard-tune)
struct NoteChoice { int target = -1; bool doubt = false; };

class Corrector
{
public:
    void prepare (double hopSeconds) { hopSec = hopSeconds; reset(); }
    void reset() { curTarget = -1; corrCents = 0.0f; center = 0.0f; haveCenter = false; }

    // escolhe a nota: escala (allowed) e, se houver, as notas do acorde (chord)
    static NoteChoice choose (float midi, uint16_t allowed, uint16_t chord, int current)
    {
        NoteChoice r;
        if (allowed == 0) allowed = 0x0fff;
        const int base = (int) std::floor (midi);
        int below = -1, above = -1;
        for (int k = base; k >= base - 12; --k) if (allowed & (1u << (((k % 12) + 12) % 12))) { below = k; break; }
        for (int k = base + 1; k <= base + 13; ++k) if (allowed & (1u << (((k % 12) + 12) % 12))) { above = k; break; }
        if (below < 0 && above < 0) { r.target = (int) std::lround (midi); return r; }
        if (below < 0) below = above;
        if (above < 0) above = below;

        const float db = midi - (float) below, da = (float) above - midi;
        int pick = db <= da ? below : above;
        // histerese: continua na nota atual enquanto não passar 60 cents dela
        if (current >= 0 && (current == below || current == above) && std::abs (midi - (float) current) < 0.6f)
            pick = current;

        const float gap = (float) (above - below);
        const float frac = gap > 0 ? db / gap : 0.0f;
        r.doubt = gap > 0 && frac > 0.38f && frac < 0.62f;

        if (chord != 0)
        {
            auto inChord = [&] (int k) { return (chord & (1u << (((k % 12) + 12) % 12))) != 0; };
            const bool cb = inChord (below), ca = inChord (above);
            if (cb != ca && gap > 0 && frac > 0.3f && frac < 0.7f)
            {
                const int chordNote = cb ? below : above;
                if (chordNote != pick) r.doubt = true;
                pick = chordNote;   // na dúvida, a nota do acorde ganha
            }
        }
        r.target = pick;
        return r;
    }

    // devolve a razão de afinação (saída / entrada) para esta análise
    float update (float midi, int target, float amount, bool voiced)
    {
        amount = std::clamp (amount, 0.0f, 1.0f);
        if (! voiced || target < 0)
        {
            haveCenter = false;
            corrCents *= 0.6f;                      // solta a correção devagar nos trechos sem nota
            if (std::abs (corrCents) < 0.5f) corrCents = 0.0f;
            curTarget = -1;
            return std::pow (2.0f, corrCents / 1200.0f);
        }

        // centro da nota (média lenta) — no clean a correção segue o centro e o vibrato fica intacto
        const float centerTau = 0.16f * (1.0f - amount) + 0.001f;
        const float ca = 1.0f - std::exp (-(float) hopSec / centerTau);
        if (! haveCenter || target != curTarget) { center = midi; haveCenter = true; }
        else center += ca * (midi - center);

        const float follow = std::clamp ((amount - 0.35f) / 0.5f, 0.0f, 1.0f);   // robô: corrige o instantâneo
        const float ref = follow * midi + (1.0f - follow) * center;

        float desired = ((float) target - ref) * 100.0f;
        // clean: força da correção um pouco menor que 100 % (respira), robô: 100 %
        desired *= 0.82f + 0.18f * std::clamp (amount * 1.6f, 0.0f, 1.0f);
        // clean: o que já está afinado (até 3 cents) fica intocado
        if (amount < 0.7f && std::abs (desired) < 3.0f) desired = 0.0f;

        // velocidade (retune speed): 180 ms no clean .. 0 ms no robô
        const float speedSec = amount >= 0.92f ? 0.0f : 0.18f * std::pow (1.0f - amount / 0.92f, 1.6f);
        if (speedSec <= 0.0005f) corrCents = desired;
        else corrCents += (1.0f - std::exp (-(float) hopSec / speedSec)) * (desired - corrCents);

        curTarget = target;
        return std::pow (2.0f, corrCents / 1200.0f);
    }

    int currentTarget() const { return curTarget; }

private:
    double hopSec = 0.0058;
    int curTarget = -1;
    float corrCents = 0.0f, center = 0.0f;
    bool haveCenter = false;
};

//==============================================================================
// Escuta do riddim: cromagrama -> tom (perfis de Krumhansl) e acorde (tríades)
class RiddimEar
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        D = std::max (1, (int) std::lround (sampleRate / 11025.0));
        dfs = sampleRate / (double) D;
        for (int i = 0; i < 2; ++i) { aa[i].reset(); aa[i].lowPass (sampleRate, dfs * 0.42, i == 0 ? 0.541 : 1.307); }
        buf.assign (kN, 0.0f);
        winF.assign (kN, 0.0f);
        for (int i = 0; i < kN; ++i) winF[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * kPi * (float) i / (float) kN);
        bin2pc.assign (kN / 2, -1);
        binW.assign (kN / 2, 0.0f);
        for (int b = 1; b < kN / 2; ++b)
        {
            const double f = (double) b * dfs / kN;
            if (f < 60.0 || f > 2000.0) continue;
            const double m = 69.0 + 12.0 * std::log2 (f / 440.0);
            const double off = std::abs (m - std::round (m));       // só o miolo de cada semitom (sem vazamento)
            if (off > 0.35) continue;
            bin2pc[(size_t) b] = (((int) std::lround (m)) % 12 + 12) % 12;
            binW[(size_t) b] = (float) ((1.0 - off / 0.35) / std::sqrt (f / 60.0));   // grave pesa mais (baixo e acordes)
        }
        reset();
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        w = 0; phase = 0; fill = 0;
        shortC.fill (0.0f); longC.fill (0.0f); chordHist.fill (0.0f);
        keyRoot = -1; keyMinor = false; chordRoot = -1; chordMinor = false; activeFrames = 0; silentFrames = 0;
    }

    // devolve true quando sai uma análise nova (~93 ms)
    bool push (float x)
    {
        const float y = aa[1].process (aa[0].process (x));
        if (++phase < D) return false;
        phase = 0;
        buf[(size_t) w] = y;
        w = (w + 1) % kN;
        if (++fill < kHop) return false;
        fill = 0;
        analyse();
        return true;
    }

    bool hasSignal() const { return activeFrames > 6 && silentFrames < 20; }
    bool hasKey() const { return activeFrames > 6 && keyRoot >= 0; }
    // aprende o tom de novo (o cromagrama longo começa do zero)
    void relearn() { longC.fill (0.0f); chordHist.fill (0.0f); keyRoot = -1; activeFrames = 0; }
    const std::array<float, 12>& getShortChroma() const { return shortC; }
    const std::array<float, 12>& getLongChroma() const { return longC; }

    // tom a partir de um cromagrama qualquer (ex.: a harmonia do MRM) — devolve a tônica, minor = modo
    static int keyFromChroma (const std::array<float, 12>& c, bool& minor)
    {
        int root = 0; float best = -2.0f; minor = false;
        for (int r = 0; r < 12; ++r)
            for (int m = 0; m < 2; ++m)
            {
                const float cc = corr (c, m ? kMin : kMaj, r);
                if (cc > best) { best = cc; root = r; minor = m == 1; }
            }
        return root;
    }
    int  getKeyRoot() const { return keyRoot; }
    bool getKeyMinor() const { return keyMinor; }
    int  getChordRoot() const { return chordRoot; }
    bool getChordMinor() const { return chordMinor; }

private:
    static constexpr int kN = 4096, kHop = 1024;

    static void fft (std::vector<std::complex<float>>& a)
    {
        const size_t N = a.size();
        for (size_t i = 1, j = 0; i < N; ++i)
        {
            size_t bit = N >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= N; len <<= 1)
        {
            const float ang = -2.0f * kPi / (float) len;
            const std::complex<float> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < N; i += len)
            {
                std::complex<float> wv (1.0f, 0.0f);
                for (size_t j = 0; j < len / 2; ++j)
                {
                    const auto u = a[i + j], v = a[i + j + len / 2] * wv;
                    a[i + j] = u + v; a[i + j + len / 2] = u - v;
                    wv *= wl;
                }
            }
        }
    }

    void analyse()
    {
        std::vector<std::complex<float>>& a = work;
        a.resize (kN);
        double e = 0;
        for (int i = 0; i < kN; ++i)
        {
            const float v = buf[(size_t) ((w + i) % kN)];
            e += (double) v * v;
            a[(size_t) i] = { v * winF[(size_t) i], 0.0f };
        }
        const float rmsDb = 10.0f * std::log10 ((float) (e / kN) + 1e-12f);
        if (rmsDb < -50.0f) { ++silentFrames; return; }
        silentFrames = 0;
        ++activeFrames;
        fft (a);
        std::array<float, 12> c {};
        for (int b = 1; b < kN / 2; ++b)
        {
            const int pc = bin2pc[(size_t) b];
            if (pc < 0) continue;
            c[(size_t) pc] += std::abs (a[(size_t) b]) * binW[(size_t) b];
        }
        float mx = 1e-9f;
        for (float v : c) mx = std::max (mx, v);
        for (auto& v : c) v /= mx;

        for (int i = 0; i < 12; ++i)
        {
            shortC[(size_t) i] += 0.45f * (c[(size_t) i] - shortC[(size_t) i]);
            longC[(size_t) i]  += 0.012f * (c[(size_t) i] - longC[(size_t) i]);
        }
        // acorde do momento: tríade que mais bate com o cromagrama curto
        float bc = -1e9f;
        for (int r = 0; r < 12; ++r)
            for (int m = 0; m < 2; ++m)
            {
                const uint16_t mask = triadMask (r, m == 1);
                float s = 0.0f;
                for (int i = 0; i < 12; ++i) s += (mask & (1u << i)) ? shortC[(size_t) i] : -0.35f * shortC[(size_t) i];
                if (s > bc) { bc = s; chordRoot = r; chordMinor = m == 1; }
            }
        // histórico dos acordes (a terça de cada acorde conta inteira, mesmo com o baixo forte)
        {
            const uint16_t mask = triadMask (chordRoot, chordMinor);
            for (int i = 0; i < 12; ++i)
            {
                const float v = (mask & (1u << i)) ? (i == chordRoot ? 1.4f : 1.0f) : 0.0f;
                chordHist[(size_t) i] += 0.012f * (v - chordHist[(size_t) i]);
            }
        }
        // tom: perfis de Krumhansl–Kessler sobre os acordes tocados + o cromagrama longo
        std::array<float, 12> mix {};
        for (int i = 0; i < 12; ++i) mix[(size_t) i] = chordHist[(size_t) i] + 0.35f * longC[(size_t) i];
        keyRoot = keyFromChroma (mix, keyMinor);
    }

    static constexpr float kMaj[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static constexpr float kMin[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

    static float corr (const std::array<float, 12>& c, const float* prof, int root)
    {
        float mc = 0, mp = 0;
        for (int i = 0; i < 12; ++i) { mc += c[(size_t) i]; mp += prof[i]; }
        mc /= 12; mp /= 12;
        float num = 0, dc = 0, dp = 0;
        for (int i = 0; i < 12; ++i)
        {
            const float x = c[(size_t) ((i + root) % 12)] - mc, y = prof[i] - mp;
            num += x * y; dc += x * x; dp += y * y;
        }
        return num / std::sqrt (dc * dp + 1e-12f);
    }

    double fs = 44100.0, dfs = 11025.0;
    int D = 4, phase = 0, w = 0, fill = 0, activeFrames = 0, silentFrames = 0;
    std::vector<float> buf, winF, binW;
    std::vector<int> bin2pc;
    std::vector<std::complex<float>> work;
    std::array<float, 12> shortC {}, longC {}, chordHist {};
    Biquad aa[2];
    int keyRoot = -1, chordRoot = -1;
    bool keyMinor = false, chordMinor = false;
};

//==============================================================================
// EQ da voz: HP (24 dB/oit) + 3 sinos + LP (24 dB/oit) — o mesmo desenho da curva da interface
struct EqSettings { float hp = 20, lp = 20000; float f[3] { 250, 1000, 4000 }; float g[3] { 0, 0, 0 }; };

class VoiceEQ
{
public:
    void prepare (double sampleRate) { fs = sampleRate; last = {}; last.hp = -1; for (auto& c : ch) for (auto& b : c) b.reset(); }
    void setSettings (const EqSettings& s)
    {
        if (same (s, last)) return;
        last = s;
        for (auto& c : ch)
        {
            c[0].highPass (fs, std::max (10.0f, s.hp), 0.541);
            c[1].highPass (fs, std::max (10.0f, s.hp), 1.307);
            for (int i = 0; i < 3; ++i) c[(size_t) (2 + i)].peak (fs, s.f[i], 0.9, s.g[i]);
            c[5].lowPass (fs, std::min (s.lp, (float) fs * 0.45f), 0.541);
            c[6].lowPass (fs, std::min (s.lp, (float) fs * 0.45f), 1.307);
        }
        hpOn = s.hp > 21.0f; lpOn = s.lp < 19500.0f;
    }
    inline float process (int c, float x)
    {
        auto& b = ch[(size_t) (c & 1)];
        if (hpOn) x = b[1].process (b[0].process (x));
        for (int i = 2; i < 5; ++i) x = b[(size_t) i].process (x);
        if (lpOn) x = b[6].process (b[5].process (x));
        return x;
    }
    // resposta em dB (para a curva) — mesma fórmula usada na interface
    static float responseDb (const EqSettings& s, float f)
    {
        float g = 0;
        if (s.hp > 21.0f) g -= 10.0f * std::log10 (1.0f + std::pow (s.hp / f, 8.0f));
        if (s.lp < 19500.0f) g -= 10.0f * std::log10 (1.0f + std::pow (f / s.lp, 8.0f));
        for (int i = 0; i < 3; ++i)
        {
            const float d = (std::log2 (f) - std::log2 (s.f[i])) / 0.9f;
            g += s.g[i] * std::exp (-d * d);
        }
        return g;
    }
private:
    static bool same (const EqSettings& a, const EqSettings& b)
    {
        if (a.hp != b.hp || a.lp != b.lp) return false;
        for (int i = 0; i < 3; ++i) if (a.f[i] != b.f[i] || a.g[i] != b.g[i]) return false;
        return true;
    }
    double fs = 44100.0;
    EqSettings last;
    bool hpOn = false, lpOn = false;
    std::array<std::array<Biquad, 7>, 2> ch;
};

//==============================================================================
// Saturador: 0 = Fita, 1 = Válvula, 2 = Sound System
class Saturator
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int c = 0; c < 2; ++c)
        {
            tapeLp[c].reset(); tapeLp[c].lowPass (sampleRate, 9500.0, 0.6);
            ssHp[c].reset();   ssHp[c].highPass (sampleRate, 110.0, 0.7);
            ssLp[c].reset();   ssLp[c].lowPass (sampleRate, 6500.0, 0.8);
            dc[c] = 0.0f; dcIn[c] = 0.0f;
        }
        dcA = std::exp (-2.0f * kPi * 12.0f / (float) sampleRate);
    }
    void set (int m, float drive01) { mode = std::clamp (m, 0, 2); drive = std::clamp (drive01, 0.0f, 1.0f); }

    inline float process (int c, float x)
    {
        c &= 1;
        const float gDb = 2.0f + 22.0f * drive;
        const float g = std::pow (10.0f, gDb / 20.0f);
        float y;
        if (mode == 0)       // fita: compressão macia, grave cheio, agudo arredondado
        {
            y = std::tanh (x * g) / std::sqrt (g);
            y = tapeLp[c].process (y);
        }
        else if (mode == 1)  // válvula: assimétrica (harmônicos pares), brilho
        {
            const float v = x * g;
            y = v >= 0.0f ? 1.0f - std::exp (-v) : (-1.0f + std::exp (v * 1.4f)) / 1.4f;
            y = (0.85f * y + 0.15f * v / (1.0f + std::abs (v))) / std::sqrt (g);
        }
        else                 // sound system: amplificador puxado no talo, banda estreita
        {
            float v = ssHp[c].process (x) * g * 1.5f;
            v = std::clamp (v, -1.5f, 1.5f);
            v = v - (v * v * v) / 6.75f;
            y = ssLp[c].process (v) * 1.3f / std::sqrt (g);
        }
        // bloqueia o DC que a assimetria cria
        const float o = y - dcIn[c] + dcA * dc[c];
        dcIn[c] = y; dc[c] = o;
        return o;
    }
private:
    double fs = 44100.0;
    int mode = 0;
    float drive = 0.35f, dcA = 0.999f;
    Biquad tapeLp[2], ssHp[2], ssLp[2];
    float dc[2] {}, dcIn[2] {};
};

} // namespace mt
