// Renderiza demos WAV usando o motor (teste offline, sem JUCE)
// g++ -O2 -std=c++17 -Wall tests/render_demo.cpp -o render_demo && ./render_demo
// Falha (exit 1) se aparecer NaN/Inf ou se a saída passar de 0 dBFS.
#include "../Source/RootsKeysEngine.h"
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>

static bool g_ok = true;
static void check (const char* name, const std::vector<float>& L, const std::vector<float>& R)
{
    for (size_t i = 0; i < L.size(); ++i)
        if (! std::isfinite (L[i]) || ! std::isfinite (R[i]) || std::fabs (L[i]) > 1.0f || std::fabs (R[i]) > 1.0f)
        { std::printf ("FALHA em %s (amostra %zu)\n", name, i); g_ok = false; return; }
}

struct Ev { double t; int type; int note; float vel; }; // type 0=on 1=off 2=leslieFast 3=leslieSlow

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4, riff = 36 + dataBytes;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (riff); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) (rk::clampf (L[i], -1, 1) * 32767), b = (int16_t) (rk::clampf (R[i], -1, 1) * 32767);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

static void render (const std::string& name, rk::Params p, std::vector<Ev> evs, double seconds)
{
    const int sr = 48000;
    rk::Engine e; e.params = p; e.prepare (sr);
    std::sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
    const int total = (int) (seconds * sr);
    std::vector<float> L ((size_t) total), R ((size_t) total);
    int pos = 0; size_t ei = 0;
    while (pos < total)
    {
        int next = total;
        if (ei < evs.size()) next = std::min (total, (int) (evs[ei].t * sr));
        if (next > pos) { e.render (&L[(size_t) pos], &R[(size_t) pos], next - pos); pos = next; }
        while (ei < evs.size() && (int) (evs[ei].t * sr) <= pos)
        {
            const Ev& v = evs[ei++];
            if (v.type == 0) e.noteOn (v.note, v.vel);
            else if (v.type == 1) e.noteOff (v.note);
            else if (v.type == 2) e.setModWheelLeslie (true);
            else e.setModWheelLeslie (false);
        }
    }
    float peak = 0; double rms = 0;
    for (int i = 0; i < total; ++i) { peak = std::max (peak, std::max (std::fabs (L[i]), std::fabs (R[i]))); rms += L[i] * L[i]; }
    std::printf ("%-28s peak=%.3f rms=%.4f\n", name.c_str(), peak, std::sqrt (rms / total));
    check (name.c_str(), L, R);
    writeWav (name, L, R, sr);
}

static void chord (std::vector<Ev>& ev, double t, double dur, std::initializer_list<int> notes, float vel)
{
    for (int n : notes) { ev.push_back ({ t, 0, n, vel }); ev.push_back ({ t + dur, 1, n, 0 }); }
}

int main()
{
    // 1) Piano: arpejo + acordes
    {
        rk::Params p; p.mode = 0; p.pianoBright = 0.65f; p.pianoDecay = 0.55f; p.pianoChorus = 0.5f; p.reverb = 0.22f;
        std::vector<Ev> ev;
        int arp[] = { 48, 52, 55, 60, 64, 67, 72, 76, 79, 84 };
        for (int i = 0; i < 10; ++i) chord (ev, i * 0.18, 0.5, { arp[i] }, 0.55f + 0.04f * i);
        chord (ev, 2.2, 1.6, { 45, 57, 60, 64, 69 }, 0.8f);  // Am
        chord (ev, 4.0, 1.6, { 50, 57, 62, 66, 69 }, 0.8f);  // D
        chord (ev, 5.8, 2.2, { 43, 55, 59, 62, 67 }, 0.9f);  // G
        render ("demo_piano_m1.wav", p, ev, 9.0);
    }
    // 2) Órgão: acordes com Leslie lento -> rápido -> lento
    {
        rk::Params p; p.mode = 1; float db[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
        std::memcpy (p.drawbar, db, sizeof db); p.drive = 0.35f; p.click = 0.4f; p.reverb = 0.15f;
        std::vector<Ev> ev;
        chord (ev, 0.0, 2.9, { 57, 60, 64 }, 0.8f);
        chord (ev, 3.0, 2.9, { 57, 62, 66 }, 0.8f);
        ev.push_back ({ 3.5, 2, 0, 0 });
        chord (ev, 6.0, 2.9, { 55, 59, 62, 67 }, 0.8f);
        ev.push_back ({ 7.5, 3, 0, 0 });
        render ("demo_organ_leslie.wav", p, ev, 10.0);
    }
    // 3) Órgão com percussão (estilo gospel/jazz) - frase
    {
        rk::Params p; p.mode = 1; float db[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
        std::memcpy (p.drawbar, db, sizeof db); p.percOn = true; p.perc3rd = true; p.percFast = true; p.drive = 0.3f; p.click = 0.5f;
        std::vector<Ev> ev;
        int mel[] = { 69, 72, 74, 76, 79, 76, 74, 72, 69, 67, 69 };
        for (int i = 0; i < 11; ++i) chord (ev, i * 0.22, 0.18, { mel[i] }, 0.8f);
        chord (ev, 2.6, 1.8, { 57, 60, 64, 69 }, 0.8f);
        render ("demo_organ_perc.wav", p, ev, 5.0);
    }
    // 4) Reggae: skank de piano + bubble de órgão (75 BPM)
    {
        const double beat = 60.0 / 75.0, e8 = beat / 2, s16 = beat / 4;
        std::vector<Ev> pianoEv, organEv;
        // progressão Am | D | Am | Em  (2 compassos cada? 1 compasso cada, x2)
        std::vector<std::vector<int>> chordsHi = { { 69, 72, 76 }, { 69, 74, 78 }, { 69, 72, 76 }, { 67, 71, 76 } };
        std::vector<std::vector<int>> chordsLo = { { 57, 60, 64 }, { 57, 62, 66 }, { 57, 60, 64 }, { 55, 59, 64 } };
        for (int bar = 0; bar < 8; ++bar)
        {
            const auto& hi = chordsHi[bar % 4];
            const auto& lo = chordsLo[bar % 4];
            const double t0 = bar * 4 * beat;
            for (int b = 1; b <= 3; b += 2)   // skank nos tempos 2 e 4
                for (int n : hi) { pianoEv.push_back ({ t0 + b * beat, 0, n, 0.85f }); pianoEv.push_back ({ t0 + b * beat + 0.12, 1, n, 0 }); }
            // bubble: grave no contratempo das semicolcheias, acorde nos "e"
            for (int b = 0; b < 4; ++b)
            {
                const double tb = t0 + b * beat;
                for (int n : lo) { organEv.push_back ({ tb + e8, 0, n, 0.8f }); organEv.push_back ({ tb + e8 + s16 * 0.8, 1, n, 0 }); }
                organEv.push_back ({ tb + s16, 0, lo[0] - 12, 0.7f }); organEv.push_back ({ tb + s16 + s16 * 0.7, 1, lo[0] - 12, 0 });
                organEv.push_back ({ tb + 3 * s16, 0, lo[0] - 12, 0.7f }); organEv.push_back ({ tb + 3 * s16 + s16 * 0.7, 1, lo[0] - 12, 0 });
            }
        }
        // como o motor toca ambos no modo layer, renderizamos os dois instrumentos separados e somamos
        rk::Params pp; pp.mode = 0; pp.pianoBright = 0.8f; pp.pianoDecay = 0.25f; pp.pianoChorus = 0.6f; pp.reverb = 0.3f; pp.master = 0.7f;
        rk::Params po; po.mode = 1; float db[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 }; std::memcpy (po.drawbar, db, sizeof db);
        po.drive = 0.25f; po.click = 0.35f; po.reverb = 0.2f; po.organLevel = 0.6f; po.master = 0.7f;

        const int sr = 48000; const double secs = 8 * 4 * beat + 2.0; const int total = (int) (secs * sr);
        auto run = [&] (rk::Params p, std::vector<Ev> evs, std::vector<float>& L, std::vector<float>& R)
        {
            rk::Engine e; e.params = p; e.prepare (sr);
            std::sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
            L.assign ((size_t) total, 0); R.assign ((size_t) total, 0);
            int pos = 0; size_t ei = 0;
            while (pos < total)
            {
                int next = total; if (ei < evs.size()) next = std::min (total, (int) (evs[ei].t * sr));
                if (next > pos) { e.render (&L[(size_t) pos], &R[(size_t) pos], next - pos); pos = next; }
                while (ei < evs.size() && (int) (evs[ei].t * sr) <= pos)
                { const Ev& v = evs[ei++]; if (v.type == 0) e.noteOn (v.note, v.vel); else e.noteOff (v.note); }
            }
        };
        std::vector<float> aL, aR, bL, bR;
        run (pp, pianoEv, aL, aR); run (po, organEv, bL, bR);
        float peak = 0;
        for (int i = 0; i < total; ++i) { aL[i] = aL[i] * 0.8f + bL[i] * 0.8f; aR[i] = aR[i] * 0.8f + bR[i] * 0.8f; peak = std::max (peak, std::fabs (aL[i])); }
        std::printf ("%-28s peak=%.3f\n", "demo_reggae_skank_bubble.wav", peak);
        check ("reggae", aL, aR);
        writeWav ("demo_reggae_skank_bubble.wav", aL, aR, sr);
    }
    // 5) Dub: skank de piano no eco de fita FitaEco TE-12, intensidade alta (auto-oscilação)
    {
        rk::Params p; p.mode = 0; p.pianoBright = 0.85f; p.pianoDecay = 0.2f; p.pianoChorus = 0.65f; p.reverb = 0.3f;
        p.echoOn = true; p.echoMode = 7; p.echoRate = 0.45f; p.echoIntensity = 0.78f; p.echoVol = 0.7f; p.echoSpring = 0.4f; p.echoWow = 0.4f; p.echoAge = 0.4f;
        std::vector<Ev> ev;
        const double beat = 60.0 / 75.0;
        for (int bar = 0; bar < 2; ++bar)
            for (int b = 1; b <= 3; b += 2)
                chord (ev, bar * 4 * beat + b * beat, 0.12, { 69, 72, 76 }, 0.85f);
        rk::Engine e; e.params = p; e.prepare (48000);
        // render manual para usar bpm
        const int sr = 48000, total = (int) (12.0 * sr);
        std::vector<float> L ((size_t) total), R ((size_t) total);
        std::sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
        int pos = 0; size_t ei = 0;
        while (pos < total)
        {
            int next = total; if (ei < ev.size()) next = std::min (total, (int) (ev[ei].t * sr));
            if (next > pos) { e.render (&L[(size_t) pos], &R[(size_t) pos], next - pos); pos = next; }
            while (ei < ev.size() && (int) (ev[ei].t * sr) <= pos)
            { const Ev& v = ev[ei++]; if (v.type == 0) e.noteOn (v.note, v.vel); else e.noteOff (v.note); }
        }
        float peak = 0, tail = 0;
        for (int i = 0; i < total; ++i) peak = std::max (peak, std::fabs (L[i]));
        for (int i = total - sr; i < total; ++i) tail = std::max (tail, std::fabs (L[i]));
        std::printf ("%-28s peak=%.3f tail(last 1s)=%.3f\n", "demo_dub_tape_echo.wav", peak, tail);
        check ("dub echo", L, R);
        writeWav ("demo_dub_tape_echo.wav", L, R, sr);
    }
    // 6) Flanger dub (Mad Professor): órgão + eco, varredura lenta com regeneração no máximo
    for (float fb : { 0.95f, -0.95f })
    {
        rk::Params p; p.mode = 1; float db[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
        std::memcpy (p.drawbar, db, sizeof db); p.drive = 0.3f; p.reverb = 0.3f;
        p.echoOn = true; p.echoIntensity = 0.7f;
        p.flOn = true; p.flRate = 0.15f; p.flDepth = 1.0f; p.flManual = 0.2f; p.flFeedback = fb; p.flMix = 0.6f;
        std::vector<Ev> ev;
        chord (ev, 0.0, 3.5, { 57, 60, 64 }, 0.8f);
        chord (ev, 4.0, 3.5, { 55, 59, 62 }, 0.8f);
        render (fb > 0 ? "demo_flanger_dub.wav" : "demo_flanger_dub_neg.wav", p, ev, 10.0);
    }
    // 7) Timbres novos do piano: Grand, MT-40 (Casio) e FM EP (Yamaha DX)
    {
        const char* names[10] = { "demo_piano_m1_ref.wav", "demo_piano_grand.wav", "demo_piano_mt40.wav", "demo_piano_fm_ep.wav",
                                 "demo_piano_yamaha_cp80.wav", "demo_piano_yamaha_c7.wav", "demo_piano_m1_grand.wav",
                                 "demo_piano_m1_comp.wav", "demo_piano_honky_tonk.wav", "demo_piano_casio_cz101.wav" };
        for (int t = 1; t <= 9; ++t)
        {
            rk::Params p; p.mode = 0; p.pianoType = t; p.reverb = 0.22f;
            std::vector<Ev> ev;
            int arp[] = { 48, 52, 55, 60, 64, 67, 72, 76 };
            for (int i = 0; i < 8; ++i) chord (ev, i * 0.2, 0.5, { arp[i] }, 0.45f + 0.07f * i);
            chord (ev, 2.0, 1.8, { 45, 57, 60, 64, 69 }, 0.85f);
            chord (ev, 4.0, 2.4, { 50, 57, 62, 66, 69 }, 0.6f);
            render (names[t], p, ev, 7.5);
        }
    }
    // 8) Pitch bend: +1 = +2 semitons (razão 1,1225) em todos os timbres e no órgão
    static const char* kNomes[11] = { "M1", "Grand", "MT-40", "FM EP", "CP-80", "C7", "M1 Gr", "M1 Comp", "Honky", "CZ-101", "orgao" };
    for (int t = 0; t <= 10; ++t)
    {
        auto pitchOf = [&] (float bend)
        {
            rk::Params p; p.mode = t == 10 ? 1 : 0; p.pianoType = t == 10 ? 0 : t; p.reverb = 0; p.maxOn = false;
            float db[9] = { 0, 0, 8, 0, 0, 0, 0, 0, 0 }; std::memcpy (p.drawbar, db, sizeof db); p.leslieMix = 0; p.drive = 0;
            rk::Engine e; e.params = p; e.prepare (48000);
            e.setPitchBend (bend);
            e.noteOn (57, 0.8f);   // A3 = 220 Hz
            std::vector<float> L (24000), R (24000);
            e.render (L.data(), R.data(), 24000);
            double best = 0; int bestLag = 0;
            for (int lag = 48000 / 300; lag <= 48000 / 160; ++lag)
            {
                double acc = 0;
                for (int i = 4000; i < 20000; ++i) acc += (double) L[(size_t) i] * L[(size_t) (i + lag)];
                if (acc > best) { best = acc; bestLag = lag; }
            }
            return bestLag > 0 ? 48000.0 / bestLag : 0.0;
        };
        const double f0 = pitchOf (0.0f), f1 = pitchOf (1.0f);
        const double ratio = f0 > 0 ? f1 / f0 : 0;
        const bool ok = std::abs (ratio - 1.1225) < 0.02;
        std::printf ("pitch bend %-6s %.1f Hz -> %.1f Hz (razao %.3f) %s\n",
                     kNomes[t], f0, f1, ratio, ok ? "ok" : "FALHOU");
        if (! ok) g_ok = false;
    }
    std::printf (g_ok ? "OK: todos os testes passaram\n" : "ERRO: algum teste falhou\n");
    return g_ok ? 0 : 1;
}
