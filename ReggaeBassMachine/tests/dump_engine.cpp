// Exporta o motor do MRBM-69 em JSON para a versão de navegador (site/bassmachine/engine.js):
//   dump_engine book   -> células dos 7 baixistas (os dados que o engine.js embute)
//   dump_engine lines  -> linhas geradas numa grade de controles, para o teste web_engine_test.js
//   dump_engine drums  -> bateria de acompanhamento dos 4 estilos
// g++ -std=c++17 -O2 tests/dump_engine.cpp -o dump_engine
#include "../Source/BassEngine.h"
#include "../Source/AccompEngine.h"
#include <cstdio>
#include <cstring>
using namespace mbm;

static void printBar (const Bar& b)
{
    std::printf ("[");
    for (size_t i = 0; i < b.size(); ++i)
        std::printf ("%s[%.17g,%.17g,%d,%.9g,%d]", i ? "," : "", b[i].start, b[i].len, b[i].tok, (double) b[i].vel, b[i].opt ? 1 : 0);
    std::printf ("]");
}

int main (int argc, char** argv)
{
    const char* mode = argc > 1 ? argv[1] : "lines";

    if (! std::strcmp (mode, "book"))
    {
        std::printf ("[\n");
        for (int s = 0; s < NumStyles; ++s)
        {
            const auto& bk = book (s);
            std::printf ("%s{\"name\":\"%s\",\"tagline\":\"%s\",\"main\":[", s ? ",\n" : "", styleName (s), styleTagline (s));
            for (size_t i = 0; i < bk.main.size(); ++i) { if (i) std::printf (","); printBar (bk.main[i]); }
            std::printf ("],\"fills\":[");
            for (size_t i = 0; i < bk.fills.size(); ++i) { if (i) std::printf (","); printBar (bk.fills[i]); }
            std::printf ("],\"swing\":%.9g,\"varScale\":%.9g,\"newCellChance\":%.9g,\"phraseBars\":%d,\"lag\":%.17g}",
                         (double) bk.swing, (double) bk.varScale, (double) bk.newCellChance, bk.phraseBars, bk.lag);
        }
        std::printf ("\n]\n");
        return 0;
    }

    if (! std::strcmp (mode, "drums"))
    {
        std::printf ("[");
        bool first = true;
        for (double L : { 2.0, 4.0, 8.0, 18.0, 34.0 })
            for (int ds = 0; ds < NumDrumStyles; ++ds)
                for (float sw : { 0.0f, 0.35f, 1.0f })
                {
                    const auto d = buildDrums (L, ds, sw);
                    std::printf ("%s\n{\"L\":%.17g,\"style\":%d,\"swing\":%.9g,\"ev\":[", first ? "" : ",", L, ds, (double) sw);
                    first = false;
                    for (size_t i = 0; i < d.size(); ++i)
                        std::printf ("%s[%.17g,%d,%d,%d]", i ? "," : "", d[i].start, d[i].note, d[i].velocity, d[i].target);
                    std::printf ("]}");
                }
        std::printf ("\n]\n");
        return 0;
    }

    // lines: harmonias x estilos x ideias x controles
    const std::vector<std::vector<ChordSlot>> harmonies {
        { { 9, Min, 2 }, { 2, Min, 2 } },                                        // padrão de início
        { { 9, Min, 8 }, { 2, Min, 4 }, { 4, Min7, 4 }, { 7, Maj, 2 } },
        { { 0, Maj, 4 }, { 5, Dom7, 4 }, { 7, Dom7, 8 } },
        { { 4, Min7, 32 } },
        { { 10, Maj, 12 }, { 3, Min, 2 }, { 5, Dom7, 16 } },
    };
    std::printf ("[");
    bool first = true;
    for (size_t h = 0; h < harmonies.size(); ++h)
        for (int style = 0; style < NumStyles; ++style)
            for (int seed : { 1, 2, 42, 137, 999 })
                for (float dens : { 0.0f, 0.6f, 1.0f })
                    for (float var : { 0.0f, 0.5f, 1.0f })
                        for (float sw : { 0.0f, 0.3f })
                            for (int base : { 28, 31, 40 })
                            {
                                Settings s; s.style = style; s.seed = seed; s.density = dens; s.variation = var;
                                s.swing = sw; s.base = base; s.velocity = 0.85f; s.gate = (base == 31 ? 0.7f : 0.25f);
                                s.strict = (seed == 42 || seed == 999);   // modo "de livro" do MRM-0420
                                double L = 0;
                                const auto ev = buildLoop (harmonies[h], s, L);
                                std::printf ("%s\n{\"h\":%zu,\"style\":%d,\"seed\":%d,\"density\":%.9g,\"variation\":%.9g,\"swing\":%.9g,\"base\":%d,"
                                             "\"velocity\":%.9g,\"gate\":%.9g,\"strict\":%s,\"L\":%.17g,\"ev\":[",
                                             first ? "" : ",", h, style, seed, (double) dens, (double) var, (double) sw, base,
                                             (double) s.velocity, (double) s.gate, s.strict ? "true" : "false", L);
                                first = false;
                                for (size_t i = 0; i < ev.size(); ++i)
                                    std::printf ("%s[%.17g,%.17g,%d,%d,%d]", i ? "," : "", ev[i].start, ev[i].length, ev[i].note, ev[i].velocity, ev[i].fill ? 1 : 0);
                                std::printf ("]}");
                            }
    std::printf ("\n]\n");
    return 0;
}
