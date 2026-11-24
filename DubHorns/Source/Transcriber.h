// =============================================================================
//  Mastor Dub Horns — transcrição de áudio (MP3/WAV) para uma frase MIDI
//  C++ puro (sem JUCE) para poder ser testado offline. A versão web usa o mesmo algoritmo.
//
//  1. mono, filtro de banda na região dos metais e redução para ~11 kHz
//  2. altura por quadro com YIN (de Cheveigné & Kawahara), 5,8 ms por quadro
//  3. separa as notas por mudança de altura e por ataque (energia que sobe de repente)
//  4. quantiza em semicolcheias no BPM do projeto e reconhece a articulação
//     (scoop, fall, doit, staccato) pela curva de afinação de cada nota
//
//  Funciona melhor com o metal sozinho ou bem na frente. Numa mixagem cheia
//  ele pega a linha mais forte na região dos metais (modo "música inteira").
// =============================================================================
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include "Phrases.h"

namespace horns
{
struct TranscribedNote { double t0 = 0, t1 = 0; int midi = 60; float vel = 0.8f; int artic = Ten; double level = 0; };

struct TranscribeResult
{
    std::vector<TranscribedNote> notes;   // notas em segundos (antes da quantização)
    Phrase phrase;                        // frase pronta para a aba Frases
    int dropped = 0;                      // notas que não couberam (mais de 4 compassos / 32 notas)
};

namespace detail
{
    struct Bq
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void set (int type, double f, double q, double sr)   // 0 passa-baixas, 1 passa-altas
        {
            const double w = 2.0 * 3.14159265358979 * f / sr, c = std::cos (w), al = std::sin (w) / (2.0 * q), a0 = 1 + al;
            if (type == 0) { b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; }
            else           { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; }
            a1 = -2 * c / a0; a2 = (1 - al) / a0; z1 = z2 = 0;
        }
        double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    };
    inline double median (std::vector<double> v)
    {
        if (v.empty()) return 0;
        std::nth_element (v.begin(), v.begin() + (long) v.size() / 2, v.end());
        return v[v.size() / 2];
    }
}

// mono: amostras mono; fullMix: true = música inteira (foco mais estreito e voz mais exigente)
inline TranscribeResult transcribe (const std::vector<float>& mono, double sr, double bpm, bool fullMix = false)
{
    using namespace detail;
    TranscribeResult res;
    const double stepSec = 60.0 / std::clamp (bpm, 40.0, 200.0) / 4.0;
    if (mono.size() < 2048 || sr < 8000) return res;

    // ---- 1. filtro + redução de taxa
    const int dec = std::max (1, (int) std::floor (sr / 11025.0));
    const double fs = sr / dec;
    Bq hp1, hp2, lp1, lp2;
    hp1.set (1, fullMix ? 170.0 : 80.0, 0.7, sr); hp2.set (1, fullMix ? 170.0 : 80.0, 0.7, sr);
    lp1.set (0, std::min (fullMix ? 2200.0 : 4000.0, fs * 0.45), 0.7, sr); lp2.set (0, std::min (fullMix ? 2200.0 : 4000.0, fs * 0.45), 0.7, sr);
    std::vector<double> x; x.reserve (mono.size() / (size_t) dec + 1);
    for (size_t i = 0; i < mono.size(); ++i)
    {
        const double y = lp2.run (lp1.run (hp2.run (hp1.run ((double) mono[i]))));
        if (i % (size_t) dec == 0) x.push_back (y);
    }

    // ---- 2. YIN por quadro
    const int W = 512, hop = 64;
    const int tauMin = (int) std::floor (fs / 1100.0), tauMax = std::min (W - 2, (int) std::ceil (fs / 70.0));
    const int nFrames = (int) ((x.size() > (size_t) (W + tauMax)) ? (x.size() - (size_t) (W + tauMax)) / (size_t) hop : 0);
    if (nFrames < 8) return res;
    std::vector<double> f0 ((size_t) nFrames, 0.0), ap ((size_t) nFrames, 1.0), rms ((size_t) nFrames, 0.0), rmsS ((size_t) nFrames, 0.0);
    std::vector<double> d ((size_t) tauMax + 1);
    double maxRms = 1e-9;
    for (int fr = 0; fr < nFrames; ++fr)
    {
        const double* s = x.data() + (size_t) fr * (size_t) hop;
        double e = 0; for (int i = 0; i < W; ++i) e += s[i] * s[i];
        rms[(size_t) fr] = std::sqrt (e / W); maxRms = std::max (maxRms, rms[(size_t) fr]);
        { double es = 0; for (int i = W / 2 - 64; i < W / 2 + 64; ++i) es += s[i] * s[i]; rmsS[(size_t) fr] = std::sqrt (es / 128.0); }   // janela curta, no centro
        d[0] = 0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double acc = 0;
            for (int i = 0; i < W; ++i) { const double v = s[i] - s[i + tau]; acc += v * v; }
            d[(size_t) tau] = acc;
        }
        double run = 0; int best = -1;
        std::vector<double>& cm = d;   // normaliza no lugar
        for (int tau = 1; tau <= tauMax; ++tau) { run += cm[(size_t) tau]; cm[(size_t) tau] = run > 0 ? cm[(size_t) tau] * tau / run : 1.0; }
        const double thr = 0.15;
        for (int tau = std::max (2, tauMin); tau < tauMax; ++tau)
            if (cm[(size_t) tau] < thr) { while (tau + 1 < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; }
        if (best < 0)
        {   // sem passar o limiar: pega o mínimo global (marcado como pouco confiável)
            best = std::max (2, tauMin);
            for (int tau = best; tau < tauMax; ++tau) if (cm[(size_t) tau] < cm[(size_t) best]) best = tau;
        }
        // erro de oitava: se existe um vale bom no período menor (1/2 ou 1/3), a nota é a de cima
        for (int k = 3; k >= 2; --k)
        {
            const int cand = (int) std::lround ((double) best / k);
            if (cand - 2 < std::max (2, tauMin)) continue;
            int mn = cand;
            for (int t = cand - 2; t <= cand + 2; ++t) if (cm[(size_t) t] < cm[(size_t) mn]) mn = t;
            if (cm[(size_t) mn] < std::max (0.22, cm[(size_t) best] * 1.8)) { best = mn; break; }
        }
        const double a = cm[(size_t) best - 1], b = cm[(size_t) best], c = cm[(size_t) best + 1];
        const double den = a - 2 * b + c, off = std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0;
        f0[(size_t) fr] = fs / (best + std::clamp (off, -1.0, 1.0));
        ap[(size_t) fr] = b;
    }

    // ---- 3. voz / sem voz e afinação em MIDI (mediana de 5 quadros)
    const double gate = maxRms * std::pow (10.0, (fullMix ? -26.0 : -34.0) / 20.0);
    const double apMax = fullMix ? 0.22 : 0.3;
    std::vector<double> mid ((size_t) nFrames, -1.0);
    for (int fr = 0; fr < nFrames; ++fr)
        if (rms[(size_t) fr] > gate && ap[(size_t) fr] < apMax && f0[(size_t) fr] > 60 && f0[(size_t) fr] < 1200)
            mid[(size_t) fr] = 69.0 + 12.0 * std::log2 (f0[(size_t) fr] / 440.0);
    std::vector<double> ms = mid;
    for (int fr = 2; fr + 2 < nFrames; ++fr)
    {
        std::vector<double> w;
        for (int k = -2; k <= 2; ++k) if (mid[(size_t) (fr + k)] > 0) w.push_back (mid[(size_t) (fr + k)]);
        if (mid[(size_t) fr] > 0 && w.size() >= 3) ms[(size_t) fr] = median (w);
    }
    // energia em dB para achar ataques (pontadas repetidas na mesma nota)
    std::vector<double> db ((size_t) nFrames);
    for (int fr = 0; fr < nFrames; ++fr) db[(size_t) fr] = 20.0 * std::log10 (rms[(size_t) fr] + 1e-9);
    auto onsetAt = [&] (int fr) { return fr >= 3 && db[(size_t) fr] - db[(size_t) fr - 3] > 6.0 && db[(size_t) fr] >= db[(size_t) fr - 1]; };

    // ---- 4. segmentação em notas
    const double frameSec = hop / fs;
    const int minFrames = std::max (3, (int) std::ceil (0.045 / frameSec));
    int fr = 0;
    while (fr < nFrames)
    {
        if (ms[(size_t) fr] < 0) { ++fr; continue; }
        const int start = fr;
        std::vector<double> pv { ms[(size_t) fr] };
        double ref = ms[(size_t) fr];
        ++fr;
        int jump = 0;
        while (fr < nFrames && ms[(size_t) fr] > 0)
        {
            if (fr - start > minFrames && onsetAt (fr)) break;
            if (std::abs (ms[(size_t) fr] - ref) > 0.75)
            {   // possível nota nova: só conta como nota nova se a altura ficar lá por 3 quadros
                if (++jump >= 3) { fr -= 2; break; }
                ++fr; continue;
            }
            if (jump > 0) { for (int k = jump; k > 0; --k) pv.push_back (ms[(size_t) (fr - k)]); jump = 0; }   // era só um tremido
            pv.push_back (ms[(size_t) fr]);
            if (pv.size() > 4) ref = median (std::vector<double> (pv.end() - 4, pv.end()));
            ++fr;
        }
        const int end = fr;
        if (end - start < minFrames) continue;
        // altura = mediana do "miolo" da nota (ignora entrada e saída)
        const int n = (int) pv.size();
        const int a0 = n / 5, a1 = std::max (a0 + 1, n - n / 4);
        const double body = median (std::vector<double> (pv.begin() + a0, pv.begin() + a1));
        TranscribedNote nt;
        nt.t0 = start * frameSec + W / fs * 0.5; nt.t1 = end * frameSec + W / fs * 0.5;   // centro da janela
        nt.midi = (int) std::lround (body);
        double pk = 0; for (int k = start; k < end; ++k) pk = std::max (pk, rms[(size_t) k]);
        // fim "tocado" da nota: último quadro a até 12 dB do pico (a cauda da sala e do eco não conta)
        int lastLoud = start;
        double pkS = 0; for (int k = start; k < end; ++k) pkS = std::max (pkS, rmsS[(size_t) k]);
        for (int k = start; k < end; ++k) if (rmsS[(size_t) k] > pkS * 0.25) lastLoud = k;
        nt.t1 = std::max (nt.t0 + frameSec, (lastLoud + 1) * frameSec + W / fs * 0.5);
        nt.level = pk;
        nt.vel = (float) std::clamp (0.35 + 0.65 * (20.0 * std::log10 (pk / maxRms + 1e-9) + 30.0) / 30.0, 0.3, 1.0);
        // articulação pela curva de afinação
        const int edge = std::max (2, std::min (n / 4, (int) std::ceil (0.08 / frameSec)));
        const double head = median (std::vector<double> (pv.begin(), pv.begin() + std::min (n, edge)));
        const double tail = median (std::vector<double> (pv.end() - std::min (n, edge), pv.end()));
        const double dur = nt.t1 - nt.t0;
        // (todo músico entra um pouquinho por baixo; scoop de verdade é mais de 1 semitom)
        if (dur < std::max (0.11, 0.95 * stepSec)) nt.artic = body - head > 1.5 ? Scoop : Stac;
        else if (tail - body < -1.3 && n > 10) nt.artic = Fall;
        else if (tail - body > 1.3 && n > 10) nt.artic = Doit;
        else if (body - head > 1.0) nt.artic = Scoop;
        res.notes.push_back (nt);
    }

    // ---- 4b. limpeza: repetição do eco (mesma nota, bem mais baixa, logo depois) e, na música inteira,
    //          o que está muito abaixo do metal (teclado, voz de fundo)
    {
        double loud = 0; for (const auto& nt : res.notes) loud = std::max (loud, nt.level);
        std::vector<TranscribedNote> keep;
        for (const auto& nt : res.notes)
        {
            if (fullMix && (nt.level < loud * 0.28 || nt.t1 - nt.t0 < 0.07)) continue;
            bool echo = false;
            for (auto it = keep.rbegin(); it != keep.rend() && nt.t0 - it->t0 < 1.6; ++it)
                if (it->midi == nt.midi && nt.level < it->level * 0.6) { echo = true; break; }
            if (! echo) keep.push_back (nt);
        }
        res.notes.swap (keep);
    }

    // ---- 5. quantiza em semicolcheias e monta a frase (até 4 compassos, 32 notas)
    if (res.notes.empty()) return res;
    const double s16 = 60.0 / std::clamp (bpm, 40.0, 200.0) / 4.0;
    const double first = res.notes.front().t0;
    const int firstStep = (int) std::lround (first / s16);
    const int shift = (firstStep / 16) * 16;   // começa no compasso da primeira nota
    int lastEnd = 0;
    Phrase& ph = res.phrase;
    ph.name = "Transcrição";
    for (const auto& nt : res.notes)
    {
        const int s = (int) std::lround (nt.t0 / s16) - shift;
        int du = std::max (1, (int) std::lround ((nt.t1 - nt.t0) / s16 + (nt.artic == Stac ? 0.0 : 0.25)));
        if (s < 0 || s >= 64 || (int) ph.events.size() >= kMaxEvents) { ++res.dropped; continue; }
        if (! ph.events.empty() && ph.events.back().start == s) { ++res.dropped; continue; }   // duas notas no mesmo passo: fica a primeira
        if (! ph.events.empty())
        {   // nota anterior não passa por cima desta
            auto& pe = ph.events.back();
            pe.dur = std::max (1, std::min (pe.dur, s - pe.start));
        }
        du = std::min (du, 64 - s);
        PhraseEvent e; e.start = s; e.dur = du; e.note = std::clamp (nt.midi, 36, 96); e.artic = nt.artic; e.degree = 0;
        ph.events.push_back (e);
        lastEnd = std::max (lastEnd, s + du);
    }
    ph.length = lastEnd > 32 ? 64 : (lastEnd > 16 ? 32 : 16);
    return res;
}
} // namespace horns
