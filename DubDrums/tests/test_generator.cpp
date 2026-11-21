// Teste do gerador sem JUCE: imprime a grade e grava arquivos .mid de exemplo.
// g++ -std=c++17 -O2 -I../Source ../Source/PatternGenerator.cpp test_generator.cpp -o test_generator
#include "PatternGenerator.h"

#include <cstdio>
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <algorithm>

using namespace dubdrums;

static void printGrid (const Pattern& p, const char* title)
{
    std::printf ("\n== %s (%g tempos, %zu notas)\n", title, p.lengthInBeats, p.events.size());
    const std::pair<int, const char*> rows[] = {
        { gm::Crash, "Prato " }, { gm::OpenHat, "HH ab " }, { gm::ClosedHat, "HH fec" },
        { gm::TomHigh, "Tom 1 " }, { gm::TomMid, "Tom 2 " }, { gm::TomLow, "Surdo " },
        { gm::Snare, "Caixa " }, { gm::SideStick, "Aro   " }, { gm::Kick, "Bumbo " } };

    const int steps = (int) std::lround (p.lengthInBeats * 4);
    for (auto& [note, name] : rows)
    {
        std::string line (steps, '.');
        for (int i = 0; i < steps; ++i) if (i % 4 == 0) line[i] = ':';
        bool any = false;
        for (auto& e : p.events)
            if (e.note == note)
            {
                int i = std::clamp ((int) std::lround (e.beat * 4), 0, steps - 1);
                line[i] = e.velocity > 95 ? 'X' : (e.velocity > 60 ? 'x' : 'o');
                any = true;
            }
        if (! any) continue;
        std::string out;
        for (int i = 0; i < steps; ++i) { if (i && i % 16 == 0) out += '|'; out += line[i]; }
        std::printf ("%s %s\n", name, out.c_str());
    }
}

// --- escritor SMF mínimo (formato 0, 960 ticks por semínima)
static void vlq (std::vector<uint8_t>& d, uint32_t v)
{
    uint8_t buf[5]; int n = 0;
    buf[n++] = v & 0x7f;
    while (v >>= 7) buf[n++] = (uint8_t) ((v & 0x7f) | 0x80);
    while (n) d.push_back (buf[--n]);
}

static void writeMidi (const Pattern& p, const std::string& path)
{
    struct M { uint32_t tick; int order; uint8_t a, b, c; };
    std::vector<M> msgs;
    for (auto& e : p.events)
    {
        auto on  = (uint32_t) std::lround (e.beat * 960);
        auto off = (uint32_t) std::lround ((e.beat + e.length) * 960);
        msgs.push_back ({ on, 1, 0x99, (uint8_t) e.note, (uint8_t) e.velocity });
        msgs.push_back ({ off, 0, 0x89, (uint8_t) e.note, 0 });
    }
    std::stable_sort (msgs.begin(), msgs.end(), [] (auto& x, auto& y)
                      { return x.tick != y.tick ? x.tick < y.tick : x.order < y.order; });

    std::vector<uint8_t> t;
    // andamento 75 bpm
    uint32_t tempo = 60000000 / 75;
    t.insert (t.end(), { 0x00, 0xFF, 0x51, 0x03, (uint8_t) (tempo >> 16), (uint8_t) (tempo >> 8), (uint8_t) tempo });
    uint32_t last = 0;
    for (auto& m : msgs) { vlq (t, m.tick - last); last = m.tick; t.insert (t.end(), { m.a, m.b, m.c }); }
    vlq (t, (uint32_t) std::lround (p.lengthInBeats * 960) - std::min (last, (uint32_t) std::lround (p.lengthInBeats * 960)));
    t.insert (t.end(), { 0xFF, 0x2F, 0x00 });

    std::ofstream f (path, std::ios::binary);
    const uint8_t hdr[] = { 'M','T','h','d',0,0,0,6,0,0,0,1,0x03,0xC0,'M','T','r','k' };
    f.write ((const char*) hdr, sizeof hdr);
    uint32_t len = (uint32_t) t.size();
    uint8_t l[4] = { (uint8_t) (len >> 24), (uint8_t) (len >> 16), (uint8_t) (len >> 8), (uint8_t) len };
    f.write ((const char*) l, 4);
    f.write ((const char*) t.data(), (std::streamsize) t.size());
}

int main()
{
    int failures = 0;
    auto check = [&] (bool ok, const char* what) { if (! ok) { std::printf ("FALHOU: %s\n", what); ++failures; } };

    for (int st = 0; st < 3; ++st)
    {
        Settings s;
        s.style = (Style) st;
        s.bars = 4;
        s.hatPattern = st == 2 ? HatPattern::Sixteenths : HatPattern::Eighths;
        s.swing = st == 0 ? Swing::Shuffle : Swing::Light;
        s.randKick = s.randSnare = s.randHat = s.randPerc = 0.4f;
        s.seed = 42;
        auto p = generate (s);
        printGrid (p, styleName (s.style));
        std::string name = std::string ("exemplo_") + (st == 0 ? "onedrop" : st == 1 ? "rockers" : "steppa") + ".mid";
        writeMidi (p, name);

        // regras de estilo (sem aleatoriedade)
        Settings clean = s; clean.randKick = clean.randSnare = clean.randHat = clean.randPerc = 0; clean.fill = false;
        clean.swing = Swing::Straight;
        auto c = generate (clean);
        auto kickAt = [&] (double b) { for (auto& e : c.events) if (e.note == gm::Kick && std::abs (e.beat - b) < 0.02) return true; return false; };
        for (int bar = 0; bar < 4; ++bar)
        {
            double o = bar * 4.0;
            if (s.style == Style::OneDrop) { check (! kickAt (o), "one drop sem bumbo no 1"); check (kickAt (o + 2), "one drop bumbo no 3"); }
            if (s.style == Style::Rockers) { check (kickAt (o) && kickAt (o + 2), "rockers bumbo 1 e 3"); }
            if (s.style == Style::Steppa)  { for (int b = 0; b < 4; ++b) check (kickAt (o + b), "steppa bumbo em todos os tempos"); }
        }
        // determinismo
        auto p2 = generate (s);
        check (p2.events.size() == p.events.size(), "mesma semente = mesmo padrão");
        for (auto& e : p.events) check (e.beat >= 0 && e.beat < p.lengthInBeats && e.velocity >= 1 && e.velocity <= 127, "limites");
    }

    // 1 a 4 compassos
    for (int bars = 1; bars <= 4; ++bars)
    {
        Settings s; s.bars = bars; s.randPerc = 1; s.randHat = 1; s.randKick = 1; s.randSnare = 1;
        for (uint32_t seed = 1; seed < 80; ++seed)
        {
            s.seed = seed;
            for (int d = 0; d < 6; ++d) for (int h = 0; h < 4; ++h) for (int sw = 0; sw < 5; ++sw) for (int dr = 0; dr < 3; ++dr)
            {
                s.dynamics = (Dynamics) d; s.hatPattern = (HatPattern) h; s.swing = (Swing) sw; s.drummer = (Drummer) dr; s.intro = (seed % 2) == 0;
                auto p = generate (s);
                check (p.lengthInBeats == bars * 4.0, "tamanho do loop");
                for (auto& e : p.events)
                    if (! (e.beat >= 0 && e.beat < p.lengthInBeats && e.velocity >= 1 && e.velocity <= 127)) { check (false, "limites (fuzz)"); break; }
            }
        }
    }

    Settings shuf; shuf.style = Style::OneDrop; shuf.bars = 1; shuf.fill = false; shuf.swing = Swing::Shuffle;
    auto sp = generate (shuf);
    bool foundTriplet = false;
    for (auto& e : sp.events) if (e.note == gm::ClosedHat && std::abs (e.beat - 2.0 / 3.0) < 0.02) foundTriplet = true;
    check (foundTriplet, "shuffle desloca contratempo para 2/3");

    // bateristas: cada um tem sua sensação de swing e suas viradas
    {
        auto hatOffset = [] (Drummer d)
        {
            Settings s; s.bars = 1; s.fill = false; s.hatPattern = HatPattern::Sixteenths; s.swing = Swing::Light; s.drummer = d;
            auto p = generate (s);
            for (auto& e : p.events) if (e.note == gm::ClosedHat && e.beat > 0.2 && e.beat < 0.45) return e.beat;
            return -1.0;
        };
        const double scott = hatOffset (Drummer::StyleScott), santa = hatOffset (Drummer::SantaDavis), carlton = hatOffset (Drummer::CarltonBarrett);
        std::printf ("\nswing da 2a semicolcheia: Scott %.3f  Santa %.3f  Carlton %.3f\n", scott, santa, carlton);
        check (scott > 0 && scott < santa && santa < carlton, "Carlton mais atrasado que Santa, Santa mais que Scott");

        for (int d = 0; d < 3; ++d)
        {
            Settings s; s.bars = 2; s.fill = true; s.intro = true; s.drummer = (Drummer) d; s.seed = 7; s.randPerc = 0.3f;
            auto p = generate (s);
            check (p.introLengthInBeats == 4.0 && ! p.intro.empty(), "intro presente");
            bool fillInIntro = false, crash = false;
            for (auto& e : p.intro)
            {
                check (e.bar == -1 && e.beat >= 0 && e.beat < 4.0, "intro dentro do compasso de intro");
                if (e.beat >= 2.4 && (e.note == gm::Snare || e.note == gm::TomLow || e.note == gm::TomMid || e.note == gm::TomHigh)) fillInIntro = true;
            }
            for (auto& e : p.events) { if (e.note == gm::Crash && e.beat < 0.01) crash = true; check (e.bar >= 0 && e.bar < 2, "compasso de cada nota"); }
            check (fillInIntro, "intro termina com virada");
            check (crash, "prato no 1 depois da intro");
            printGrid (p, (std::string ("Intro + loop - ") + drummerName ((Drummer) d)).c_str());
            Pattern introOnly; introOnly.events = p.intro; introOnly.lengthInBeats = 4.0;
            printGrid (introOnly, "  (intro)");
        }
        Settings s; s.intro = false;
        check (generate (s).intro.empty() && generate (s).introLengthInBeats == 0.0, "sem intro quando desligada");
    }

    std::printf ("\n%s (%d falhas)\n", failures ? "TESTES FALHARAM" : "TODOS OS TESTES OK", failures);
    return failures ? 1 : 0;
}
