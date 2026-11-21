#include "PatternGenerator.h"

#include <random>
#include <algorithm>
#include <cmath>

namespace dubdrums
{

namespace
{
    constexpr double kStep = 0.25;           // semicolcheia em semínimas
    constexpr int    kStepsPerBar = 16;

    // Cada peça tem seu próprio gerador aleatório: mexer na aleatoriedade
    // do hi-hat não muda o bumbo, e a mesma semente sempre gera o mesmo groove.
    struct Rng
    {
        std::mt19937 eng;
        explicit Rng (uint32_t seed) : eng (seed) {}
        double uni()                       { return std::uniform_real_distribution<double> (0.0, 1.0) (eng); }
        double range (double a, double b)  { return a + (b - a) * uni(); }
        bool   chance (double p)           { return uni() < p; }
        int    pick (int n)                { return std::uniform_int_distribution<int> (0, n - 1) (eng); }
    };

    double swingRatio (Swing s)
    {
        switch (s)
        {
            case Swing::Light:  return 0.54;
            case Swing::Medium: return 0.58;
            case Swing::Heavy:  return 0.62;
            default:            return 0.50;
        }
    }

    bool nearly (double a, double b) { return std::abs (a - b) < 1.0e-6; }

    // Desloca as notas fora do tempo de acordo com o tipo de swing.
    // Só mexe em posições da grade de semicolcheias.
    double applySwing (double beat, Swing swing, double drummerDelta)
    {
        if (swing == Swing::Straight && drummerDelta <= 0.0)
            return beat;

        const double whole = std::floor (beat);
        const double frac  = beat - whole;

        if (swing == Swing::Shuffle)
        {
            // sensação de tercina: o contratempo vai para 2/3 do tempo
            if (nearly (frac, 0.25)) return whole + 1.0 / 3.0;
            if (nearly (frac, 0.50)) return whole + 2.0 / 3.0;
            if (nearly (frac, 0.75)) return whole + 5.0 / 6.0;
            return beat;
        }

        // swing de semicolcheia: a 2ª semicolcheia de cada colcheia atrasa
        const double r = std::clamp (swingRatio (swing) + drummerDelta, 0.5, 0.67);
        if (nearly (frac, 0.25)) return whole + 0.5 * r;
        if (nearly (frac, 0.75)) return whole + 0.5 + 0.5 * r;
        return beat;
    }

    // Como cada baterista "senta" no groove
    struct Feel
    {
        double swingDelta;      // soma na proporção do swing (0.5 = reto)
        double late[4];         // microtempo fixo por peça (bumbo, caixa, hi-hat, tons), em semínimas
        double hatOrnaments;    // multiplica os enfeites de semicolcheia no hi-hat
        double hatOrnamentBase; // enfeites mesmo com aleatoriedade zero
        double openHat;         // chance extra de hi-hat aberto no contratempo
        double ghosts;          // multiplica as ghost notes da caixa
    };

    Feel feelOf (Drummer d)
    {
        switch (d)
        {
            // Roots Radics: reto, empurrando um pouquinho, aro seco, hi-hat aberto "latindo"
            case Drummer::StyleScott:     return { -0.02, { 0.000, -0.004, -0.004, 0.000 }, 1.0, 0.00, 0.10, 0.7 };
            // Soul Syndicate: mais balanço, muitos enfeites e ghost notes
            case Drummer::SantaDavis:     return {  0.02, { 0.000,  0.006,  0.000, 0.004 }, 1.7, 0.06, 0.05, 1.4 };
            // Wailers: atrás do tempo, balanço de tercina, hi-hat econômico
            case Drummer::CarltonBarrett: return {  0.04, { 0.006,  0.018,  0.006, 0.010 }, 0.6, 0.00, 0.00, 1.2 };
        }
        return { 0.0, { 0, 0, 0, 0 }, 1.0, 0.0, 0.0, 1.0 };
    }

    struct Builder
    {
        const Settings& s;
        std::vector<NoteEvent> ev;

        void add (double beat, int note, int vel, double len = 0.1)
        {
            ev.push_back ({ beat, len, note, std::clamp (vel, 1, 127), (int) std::floor (beat / 4.0) });
        }

        bool has (double beat, int note) const
        {
            for (auto& e : ev)
                if (e.note == note && nearly (e.beat, beat))
                    return true;
            return false;
        }

        void removeRange (double from, double to, std::initializer_list<int> notes)
        {
            ev.erase (std::remove_if (ev.begin(), ev.end(), [&] (const NoteEvent& e)
            {
                if (e.beat < from - 1.0e-6 || e.beat >= to - 1.0e-6) return false;
                for (int n : notes) if (e.note == n) return true;
                return false;
            }), ev.end());
        }
    };

    double barStart (int bar) { return bar * 4.0; }
    double stepPos (int bar, int step) { return barStart (bar) + step * kStep; }

    // ---------------------------------------------------------------- BUMBO
    void buildKick (Builder& b, Rng& rng)
    {
        const auto& s = b.s;
        const double r = s.randKick;

        for (int bar = 0; bar < s.bars; ++bar)
        {
            switch (s.style)
            {
                case Style::OneDrop:
                    // bumbo só no 3 — o "1" fica vazio, é a marca do one drop
                    b.add (stepPos (bar, 8), gm::Kick, 112);
                    if (rng.chance (r * 0.20)) b.add (stepPos (bar, 14), gm::Kick, 80);
                    if (rng.chance (r * 0.12)) b.add (stepPos (bar, 7),  gm::Kick, 70);
                    break;

                case Style::Rockers:
                    // bumbo no 1 e no 3
                    b.add (stepPos (bar, 0), gm::Kick, 110);
                    b.add (stepPos (bar, 8), gm::Kick, 112);
                    for (int st : { 6, 14, 3, 11 })
                        if (rng.chance (r * 0.25)) b.add (stepPos (bar, st), gm::Kick, 78);
                    break;

                case Style::Steppa:
                    // bumbo em todos os tempos
                    for (int st : { 0, 4, 8, 12 })
                        b.add (stepPos (bar, st), gm::Kick, st == 0 ? 114 : 106);
                    for (int st : { 2, 6, 10, 14 })
                        if (rng.chance (r * 0.15)) b.add (stepPos (bar, st), gm::Kick, 72);
                    break;
            }
        }
    }

    // ---------------------------------------------------------------- CAIXA / ARO
    void addBackbeat (Builder& b, double pos, Backbeat bb, int vel)
    {
        switch (bb)
        {
            case Backbeat::Snare: b.add (pos, gm::Snare, vel); break;
            case Backbeat::Rim:   b.add (pos, gm::SideStick, vel - 4); break;
            case Backbeat::Both:  b.add (pos, gm::SideStick, vel - 10); b.add (pos, gm::Snare, vel - 2); break;
        }
    }

    void buildSnare (Builder& b, Rng& rng)
    {
        const auto& s = b.s;
        const double r = s.randSnare;

        // Roots Radics (Style Scott) no rockers: bumbo no 1 e no 3 e caixa no 2 e no 4
        const bool scottRockers = s.drummer == Drummer::StyleScott && s.style == Style::Rockers;
        const std::vector<int> backbeats = scottRockers ? std::vector<int> { 4, 12 } : std::vector<int> { 8 };

        for (int bar = 0; bar < s.bars; ++bar)
        {
            const bool lastBar = (bar == s.bars - 1);

            for (int st : backbeats)
            {
                // no compasso final, às vezes a caixa vem "cheia" com flam
                if (lastBar && s.bars > 1 && st == backbeats.back() && rng.chance (r * 0.5))
                {
                    b.add (stepPos (bar, st) - 0.03, gm::Snare, 60);
                    b.add (stepPos (bar, st), gm::Snare, 114);
                }
                else
                {
                    addBackbeat (b, stepPos (bar, st), s.backbeat, 110);
                }
            }

            // ghost notes (longe da batida principal)
            const double ghost = feelOf (s.drummer).ghosts;
            for (int st : { 3, 6, 7, 11, 14, 15 })
            {
                bool nearBackbeat = false;
                for (int bb : backbeats) nearBackbeat |= std::abs (st - bb) <= 1;
                if (! nearBackbeat && rng.chance (r * 0.30 * ghost))
                    b.add (stepPos (bar, st), gm::Snare, (int) rng.range (26, 44));
            }

            // steppa: às vezes reforça o 2 e o 4 com aro
            if (s.style == Style::Steppa)
                for (int st : { 4, 12 })
                    if (rng.chance (r * 0.20)) b.add (stepPos (bar, st), gm::SideStick, 70);
        }
    }

    // ---------------------------------------------------------------- HI-HAT
    // Assinatura de hi-hat de cada baterista, aplicada por cima da figura escolhida
    bool isHat (int n) { return n == gm::ClosedHat || n == gm::OpenHat || n == gm::PedalHat; }

    void hatSignature (Builder& b, int bar, Rng& rng, double r)
    {
        const auto& s = b.s;
        const double t0 = barStart (bar);
        auto removeHats = [&] (double from, double to) { b.removeRange (t0 + from, t0 + to, { gm::ClosedHat, gm::OpenHat, gm::PedalHat }); };
        auto hatAt = [&] (double beat)
        {
            for (auto& e : b.ev) if (isHat (e.note) && nearly (e.beat, t0 + beat)) return true;
            return false;
        };

        if (s.hatPattern == HatPattern::Triplets)
        {
            // Carlton: acento na última nota da tercina, puxando para o tempo seguinte
            if (s.drummer == Drummer::CarltonBarrett)
                for (auto& e : b.ev)
                    if (isHat (e.note) && e.beat >= t0 && e.beat < t0 + 4.0
                        && nearly (e.beat - std::floor (e.beat), 2.0 / 3.0))
                        e.velocity = std::min (127, e.velocity + 18);
            return;
        }

        switch (s.drummer)
        {
            case Drummer::CarltonBarrett:
            {
                // tercina "quebrada": em alguns tempos o hi-hat vira tercina (cheia ou só 1ª e 3ª nota)
                for (int beat : { 1, 2 })
                {
                    const double p = beat == 1 ? 0.40 + r * 0.40 : 0.20 + r * 0.30;
                    if (! rng.chance (p)) continue;
                    removeHats (beat, beat + 1.0);
                    const bool full = rng.chance (0.5);
                    b.add (t0 + beat, gm::ClosedHat, 72);
                    if (full) b.add (t0 + beat + 1.0 / 3.0, gm::ClosedHat, 54);
                    b.add (t0 + beat + 2.0 / 3.0, gm::ClosedHat, 88);   // acento puxando o tempo seguinte
                }
                // no one drop o hi-hat também "cai" do 1, como o bumbo
                if (s.style == Style::OneDrop)
                    removeHats (0.0, 0.01);
                // hi-hat aberto na última colcheia, levando ao compasso seguinte
                if (rng.chance (0.80 + r * 0.15))
                {
                    removeHats (3.5, 4.0);
                    b.add (t0 + 3.5, gm::OpenHat, 92, 0.45);
                }
                break;
            }

            case Drummer::SantaDavis:
            {
                // "flying cymbal": hi-hat abre no 2 e no 4 e fecha no contratempo seguinte,
                // com semicolcheias leves no meio
                for (int st = 1; st < kStepsPerBar; st += 2)
                    if (st != 5 && st != 13 && ! hatAt (st * kStep) && rng.chance (0.55 + r * 0.30))
                        b.add (t0 + st * kStep, gm::ClosedHat, (int) rng.range (34, 46));
                for (double beat : { 1.0, 3.0 })
                {
                    removeHats (beat, beat + 0.5 + 0.01);
                    b.add (t0 + beat, gm::OpenHat, 102, 0.45);
                    b.add (t0 + beat + 0.5, gm::ClosedHat, 62);   // o pé fecha: "tsss-chk"
                }
                break;
            }

            case Drummer::StyleScott:
            {
                // disciplinado: acento forte no contratempo, tempos leves, quase sem enfeite
                for (auto& e : b.ev)
                {
                    if (! isHat (e.note) || e.beat < t0 || e.beat >= t0 + 4.0) continue;
                    const double frac = e.beat - std::floor (e.beat);
                    if (nearly (frac, 0.5))       e.velocity = 104;
                    else if (nearly (frac, 0.0))  e.velocity = 56;
                    else                          e.velocity = 38;
                }
                b.ev.erase (std::remove_if (b.ev.begin(), b.ev.end(), [&] (const NoteEvent& e)
                {
                    if (! isHat (e.note) || e.beat < t0 || e.beat >= t0 + 4.0) return false;
                    const double frac = e.beat - std::floor (e.beat);
                    const bool sixteenth = nearly (frac, 0.25) || nearly (frac, 0.75);
                    return sixteenth && s.hatPattern != HatPattern::Sixteenths && rng.chance (0.7);
                }), b.ev.end());
                for (double beat : { 0.5, 1.5, 2.5, 3.5 })
                    if (! hatAt (beat)) b.add (t0 + beat, gm::ClosedHat, 104);
                // latido curto do hi-hat aberto no "e" do 4
                if (rng.chance (0.30 + r * 0.30))
                {
                    removeHats (3.5, 3.76);
                    b.add (t0 + 3.5, gm::OpenHat, 106, 0.14);
                }
                break;
            }
        }
    }

    void buildHats (Builder& b, Rng& rng)
    {
        const auto& s = b.s;
        const double r = s.randHat;
        const Feel feel = feelOf (s.drummer);
        const double orn = feel.hatOrnamentBase + r * feel.hatOrnaments;

        for (int bar = 0; bar < s.bars; ++bar)
        {
            if (s.hatPattern == HatPattern::Triplets)
            {
                for (int beat = 0; beat < 4; ++beat)
                    for (int k = 0; k < 3; ++k)
                    {
                        if (k > 0 && rng.chance (r * 0.12)) continue;
                        const double pos = barStart (bar) + beat + k / 3.0;
                        const int vel = (k == 0) ? 88 : (k == 2 ? 70 : 56);
                        const bool open = (k == 2 && rng.chance (r * 0.12));
                        b.add (pos, open ? gm::OpenHat : gm::ClosedHat, vel, open ? 0.25 : 0.1);
                    }
                hatSignature (b, bar, rng, r);
                continue;
            }

            bool skipNext = false;
            for (int st = 0; st < kStepsPerBar; ++st)
            {
                if (skipNext) { skipNext = false; continue; }

                const bool onBeat   = (st % 4 == 0);
                const bool offEight = (st % 4 == 2);
                const bool sixteen  = (st % 2 == 1);

                bool play = false;
                int  vel  = 0;

                switch (s.hatPattern)
                {
                    case HatPattern::Eighths:
                        play = ! sixteen;
                        vel  = onBeat ? 88 : 72;
                        if (sixteen && rng.chance (orn * 0.18)) { play = true; vel = 46; } // enfeite
                        break;

                    case HatPattern::Sixteenths:
                        play = true;
                        vel  = onBeat ? 90 : (offEight ? 74 : 56);
                        break;

                    case HatPattern::Offbeat:
                        play = offEight;
                        vel  = 94;
                        if (sixteen && rng.chance (orn * 0.15)) { play = true; vel = 48; }
                        break;

                    case HatPattern::Triplets:
                        break;
                }

                if (! play) continue;

                // pode "esquecer" algumas notas fora do tempo
                if (! onBeat && rng.chance (r * 0.10)) continue;

                // hi-hat aberto em alguns contratempos
                bool open = false;
                if (offEight)
                {
                    const double baseOpen = (s.hatPattern == HatPattern::Offbeat && st == 14) ? 0.35 : 0.0;
                    open = rng.chance (baseOpen + feel.openHat * 0.5 + r * 0.10);
                }

                if (open)
                {
                    b.add (stepPos (bar, st), gm::OpenHat, vel + 4, 0.22);
                    skipNext = true; // deixa o aberto soar
                }
                else
                {
                    b.add (stepPos (bar, st), gm::ClosedHat, vel);
                }
            }

            hatSignature (b, bar, rng, r);
        }
    }

    // ---------------------------------------------------------------- VIRADAS
    struct Hit { double beat; int note; int vel; };   // beat: posição dentro do compasso (0..4)
    using Fill = std::vector<Hit>;

    // Viradas características de cada baterista (sempre resolvendo no "1" seguinte)
    const std::vector<Fill>& fillsOf (Drummer d)
    {
        static const std::vector<Fill> scott =
        {
            // aro estalado e tons em semicolcheia
            { {3.0, gm::Snare, 110}, {3.25, gm::Snare, 72}, {3.5, gm::TomMid, 100}, {3.75, gm::TomLow, 110} },
            // estalos sincopados de caixa (Roots Radics)
            { {2.75, gm::Snare, 106}, {3.25, gm::Snare, 110}, {3.5, gm::Snare, 76}, {3.75, gm::TomLow, 112} },
            // tons descendo em semicolcheia
            { {3.0, gm::TomHigh, 100}, {3.25, gm::TomHigh, 88}, {3.5, gm::TomMid, 102}, {3.75, gm::TomLow, 114} },
        };
        static const std::vector<Fill> santa =
        {
            // rufo de fusas em crescendo
            { {3.0, gm::Snare, 50}, {3.125, gm::Snare, 58}, {3.25, gm::Snare, 66}, {3.375, gm::Snare, 74},
              {3.5, gm::Snare, 82}, {3.625, gm::Snare, 92}, {3.75, gm::Snare, 102}, {3.875, gm::Snare, 114} },
            // semicolcheias misturando caixa e tons
            { {2.5, gm::Snare, 80}, {2.75, gm::Snare, 70}, {3.0, gm::TomHigh, 96}, {3.25, gm::TomHigh, 86},
              {3.5, gm::TomMid, 100}, {3.75, gm::TomLow, 112} },
            // flams em sequência
            { {2.97, gm::Snare, 50}, {3.0, gm::Snare, 104}, {3.47, gm::Snare, 50}, {3.5, gm::Snare, 108}, {3.75, gm::TomLow, 110} },
        };
        static const std::vector<Fill> carlton =
        {
            // rufo em tercina no 4, com flam
            { {2.97, gm::Snare, 48}, {3.0, gm::Snare, 72}, {3.333, gm::Snare, 86}, {3.667, gm::Snare, 102} },
            // tercinas nos tons
            { {3.0, gm::TomHigh, 92}, {3.333, gm::TomHigh, 84}, {3.667, gm::TomMid, 100} },
            // entrada do one drop: caixa do "e" do 3 até o surdo
            { {2.667, gm::Snare, 60}, {2.97, gm::Snare, 48}, {3.0, gm::Snare, 84}, {3.333, gm::TomMid, 92}, {3.667, gm::TomLow, 106} },
        };
        switch (d)
        {
            case Drummer::StyleScott: return scott;
            case Drummer::SantaDavis: return santa;
            default:                  return carlton;
        }
    }

    // Coloca uma virada no compasso `bar` do builder. Devolve onde ela começa.
    double placeFill (Builder& b, int bar, Rng& rng, double r)
    {
        const auto& all = fillsOf (b.s.drummer);
        Fill f = all[(size_t) rng.pick ((int) all.size())];

        // virada longa: começa meio tempo antes
        if (rng.chance (0.20 + r * 0.5))
            f.insert (f.begin(), { 2.5, gm::Snare, 66 });

        double first = 4.0;
        for (auto& h : f) first = std::min (first, h.beat);
        const double from = barStart (bar) + std::floor (first * 4.0 - 1.0e-6) / 4.0;

        b.removeRange (from, barStart (bar + 1),
                       { gm::ClosedHat, gm::OpenHat, gm::Snare, gm::SideStick, gm::TomHigh, gm::TomMid, gm::TomLow });

        for (size_t i = 0; i < f.size(); ++i)
        {
            const auto& h = f[i];
            if (i > 0 && i + 1 < f.size() && rng.chance (r * 0.12)) continue; // deixa um buraco
            b.add (barStart (bar) + h.beat, h.note, h.vel + (int) rng.range (-6, 6));
        }
        return from;
    }

    void buildPerc (Builder& b, Rng& rng)
    {
        const auto& s = b.s;
        const double r = s.randPerc;

        // tons soltos entrando no 3 (bem comum no one drop)
        for (int bar = 0; bar < s.bars; ++bar)
        {
            const bool fillBar = s.fill && bar == s.bars - 1;
            if (! fillBar && rng.chance (r * 0.30))
            {
                b.add (stepPos (bar, 6), gm::TomHigh, (int) rng.range (70, 90));
                if (rng.chance (0.5)) b.add (stepPos (bar, 7), gm::TomMid, (int) rng.range (70, 90));
            }
        }

        if (s.fill)
            placeFill (b, s.bars - 1, rng, r);

        // prato no "1" do loop, resolvendo a virada (ou a introdução)
        if (s.fill || s.intro)
            b.add (0.0, gm::Crash, 104, 1.0);
    }

    // Introdução: um compasso com a contagem no hi-hat e a virada do baterista no fim
    void buildIntro (Builder& b, Rng& rng)
    {
        const double from = placeFill (b, 0, rng, b.s.randPerc);
        for (int beat = 0; beat < 4; ++beat)
            if (beat + 0.001 < from)
                b.add ((double) beat, gm::ClosedHat, beat == 0 ? 70 : 58);
    }

    // ---------------------------------------------------------------- DINÂMICA
    double dynamicsScale (const Settings& s, double beat, double loopLen)
    {
        const int bar = (int) (beat / 4.0);
        switch (s.dynamics)
        {
            case Dynamics::Soft:      return 0.62;
            case Dynamics::Medium:    return 0.85;
            case Dynamics::Loud:      return 1.08;
            case Dynamics::Accented:  return 0.92;
            case Dynamics::Crescendo: return 0.55 + 0.50 * (beat / loopLen);
            case Dynamics::Breathing: return (bar % 2 == 0) ? 0.98 : 0.72;
        }
        return 1.0;
    }

    int pieceOf (int note)
    {
        switch (note)
        {
            case gm::Kick: return 0;
            case gm::Snare: case gm::SideStick: return 1;
            case gm::ClosedHat: case gm::OpenHat: case gm::PedalHat: return 2;
            default: return 3;
        }
    }
} // namespace

Pattern generate (const Settings& in)
{
    Settings s = in;
    s.bars = std::clamp (s.bars, 1, 4);

    Builder b { s, {} };

    Rng kickRng  (s.seed * 7919u + 1u);
    Rng snareRng (s.seed * 7919u + 2u);
    Rng hatRng   (s.seed * 7919u + 3u);
    Rng percRng  (s.seed * 7919u + 4u);
    Rng humanRng (s.seed * 7919u + 5u);
    Rng introRng (s.seed * 7919u + 6u);

    buildKick  (b, kickRng);
    buildSnare (b, snareRng);
    buildHats  (b, hatRng);
    buildPerc  (b, percRng);

    Pattern p;
    p.lengthInBeats = s.bars * 4.0;

    Feel feel = feelOf (s.drummer);
    if (s.machine) { feel.swingDelta = 0.0; for (auto& l : feel.late) l = 0.0; }   // a máquina não "senta" atrás do tempo
    const float rand[4] = { s.randKick, s.randSnare, s.randHat, s.randPerc };
    const double timingAmount[4] = { 0.006, 0.012, 0.014, 0.012 }; // em semínimas, no máximo

    auto finish = [&] (const std::vector<NoteEvent>& raw, double len, int barOverride, std::vector<NoteEvent>& out)
    {
        for (auto e : raw)
        {
            const int piece = pieceOf (e.note);
            const bool isHat = (piece == 2);
            if (barOverride != 0) e.bar = barOverride;

            // swing (o baterista sempre deixa sua marca no balanço)
            if (isHat || s.swingAll)
                e.beat = applySwing (e.beat, s.swing, feel.swingDelta);

            // dinâmica
            double vel = e.velocity;
            if (s.dynamics == Dynamics::Accented)
            {
                const double frac = e.beat - std::floor (e.beat);
                const bool downbeat = frac < 0.02;
                vel = downbeat ? vel * 1.15 : 64.0 + (vel - 64.0) * 0.6 - 10.0;
            }
            vel *= dynamicsScale (s, e.beat, len);

            // humanização: velocidade e microtempo proporcionais à aleatoriedade da peça
            const double r = rand[piece];
            if (s.machine)
            {
                // drum machine: grade exata e acento de dois níveis
                vel = vel >= 92.0 ? 118.0 : (vel >= 50.0 ? 88.0 : 60.0);
            }
            else
            {
                vel += humanRng.range (-18.0, 18.0) * r;
                if (e.beat > 0.0) // não tira o "1" do lugar, para o loop não começar atrasado
                    e.beat += humanRng.range (-1.0, 1.0) * timingAmount[piece] * r + feel.late[piece];
            }

            e.velocity = std::clamp ((int) std::lround (vel), 1, 127);
            e.beat = std::clamp (e.beat, 0.0, len - 0.001);
            out.push_back (e);
        }
        std::stable_sort (out.begin(), out.end(),
                          [] (const NoteEvent& x, const NoteEvent& y) { return x.beat < y.beat; });
    };

    finish (b.ev, p.lengthInBeats, 0, p.events);

    if (s.intro)
    {
        Settings introSettings = s;
        introSettings.bars = 1;
        Builder ib { introSettings, {} };
        buildIntro (ib, introRng);
        p.introLengthInBeats = 4.0;
        finish (ib.ev, p.introLengthInBeats, -1, p.intro);
    }
    return p;
}

const char* styleName (Style s)
{
    switch (s)
    {
        case Style::OneDrop: return "One Drop";
        case Style::Rockers: return "Rockers";
        case Style::Steppa:  return "Steppa";
    }
    return "";
}

const char* drummerName (Drummer d)
{
    switch (d)
    {
        case Drummer::StyleScott:     return "Style Scott";
        case Drummer::SantaDavis:     return "Santa Davis";
        case Drummer::CarltonBarrett: return "Carlton Barrett";
    }
    return "";
}

const char* swingName (Swing s)
{
    switch (s)
    {
        case Swing::Straight: return "Reto";
        case Swing::Light:    return "Leve 54%";
        case Swing::Medium:   return "Medio 58%";
        case Swing::Heavy:    return "Pesado 62%";
        case Swing::Shuffle:  return "Shuffle (tercina)";
    }
    return "";
}

} // namespace dubdrums
