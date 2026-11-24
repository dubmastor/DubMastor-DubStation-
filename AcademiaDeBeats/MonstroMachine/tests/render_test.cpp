// Teste offline do motor da Monstro Machine MM-2500 (sem JUCE):
// todos os pads de todos os kits soam e terminam, os grooves de reggae ficam na contagem
// (caixa/aro no 2 e no 4, nada de caixa no 3), efeitos começam em bypass e o MIDI sai válido.
#include "MonstroEngine.h"
#include <cstdio>
#include <cstdlib>

static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { std::printf ("FALHA: " __VA_ARGS__); std::printf ("\n"); ++fails; } } while (0)

int main()
{
    mm::Engine e;
    e.prepare (48000);
    CHECK (! e.satOn && ! e.echoOn, "saturador e Tape Echo precisam começar em bypass");
    std::vector<float> L (48000 * 3), R (L.size());
    for (int k = 0; k < (int) mm::kits().size(); ++k)
    {
        e.setKit (k);
        for (int p = 0; p < mm::kPads; ++p)
        {
            std::fill (L.begin(), L.end(), 0.f); std::fill (R.begin(), R.end(), 0.f);
            e.trigger (p, .9f);
            e.render (L.data(), R.data(), (int) L.size());
            double peak = 0, tail = 0;
            for (size_t i = 0; i < L.size(); ++i) { CHECK (std::isfinite (L[i]), "NaN kit %d pad %d", k, p); peak = std::fmax (peak, std::fabs (L[i])); }
            for (size_t i = L.size() - 4800; i < L.size(); ++i) tail = std::fmax (tail, std::fabs (L[i]));
            CHECK (peak > .02, "kit %s pad %s mudo (%.4f)", mm::kits()[(size_t) k].name, mm::padName (p), peak);
            CHECK (peak <= 1.0, "kit %d pad %d estourou", k, p);
            CHECK (tail < .01, "kit %s pad %s não termina", mm::kits()[(size_t) k].name, mm::padName (p));
        }
    }
    // reggae: caixa (2) ou aro (4) só nos passos 4 e 12 (tempos 2 e 4); nunca no passo 8 (o "drop" no 3)
    auto rg = mm::stylePatterns (mm::reggae);
    for (int s = 0; s < mm::kSeqs; ++s)
    {
        bool two = false, four = false;
        for (int pad : { 2, 4 })
            for (int st = 0; st < mm::kSteps; ++st)
                if (rg[(size_t) s][(size_t) pad][(size_t) st])
                {
                    CHECK (st == 4 || st == 12, "reggae SEQ %d: caixa/aro fora do 2 e do 4 (passo %d)", s, st + 1);
                    two |= st == 4; four |= st == 12;
                }
        CHECK (two || four, "reggae SEQ %d sem caixa/aro no 2 e no 4", s);
    }
    CHECK (mm::kits()[0].style == mm::reggae && std::string (mm::kits()[0].name) == "DubMastor", "kit inicial é o DubMastor (reggae)");
    // efeitos ligados não podem gerar NaN
    e.satOn = e.echoOn = true; e.setKit (1);
    std::fill (L.begin(), L.end(), 0.f); std::fill (R.begin(), R.end(), 0.f);
    for (int p = 0; p < mm::kPads; ++p) e.trigger (p, 1.f, p * 1000);
    e.render (L.data(), R.data(), (int) L.size());
    for (float x : L) CHECK (std::isfinite (x) && std::fabs (x) <= 1.0f, "efeitos geraram valor inválido");
    // MIDI
    auto pat = mm::stylePatterns (mm::trap);
    auto mid = mm::patternToMidi (pat[0], 140);
    CHECK (mid.size() > 30 && mid[0] == 'M' && mid[14] == 'M', "MIDI inválido");
    std::printf ("%zu kits x %d pads conferidos · %d falha(s)\n", mm::kits().size(), mm::kPads, fails);
    return fails ? 1 : 0;
}
