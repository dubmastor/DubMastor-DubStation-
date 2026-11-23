// Teste offline do DSP do Mastor Key Scout MKS-0420 (sem JUCE): o "ouvido" do riddim.
//   g++ -std=c++17 -O2 -Wall -Wextra tests/render_test.cpp -o render_test && ./render_test
// Toca progressões de reggae sintéticas (baixo + skank) e confere o tom e o acorde de cada momento.
#include "../../Tune/Source/TuneDSP.h"
#include <cstdio>

using namespace mt;
static int failures = 0;
static void check (bool ok, const char* what, double v)
{
    std::printf ("  [%s] %-56s %.2f\n", ok ? " OK " : "FALHOU", what, v);
    if (! ok) ++failures;
}

struct Prog { const char* name; int key; bool minor; int chords[4][2]; int n; };

int main()
{
    const Prog progs[] = {
        { "Am - Dm - Em (A menor)", 9, true,  { { 9, 1 }, { 2, 1 }, { 4, 1 }, { 0, 0 } }, 3 },
        { "G - C - D (G maior)",    7, false, { { 7, 0 }, { 0, 0 }, { 2, 0 }, { 0, 0 } }, 3 },
        { "Dm - Gm - Am - Dm (D menor)", 2, true, { { 2, 1 }, { 7, 1 }, { 9, 1 }, { 2, 1 } }, 4 },
        { "F - Bb - C (F maior)",   5, false, { { 5, 0 }, { 10, 0 }, { 0, 0 }, { 0, 0 } }, 3 },
    };
    for (double fs : { 44100.0, 48000.0 })
        for (const auto& p : progs)
        {
            std::printf ("\n== %s @ %.0f Hz\n", p.name, fs);
            RiddimEar ear; ear.prepare (fs);
            int ok = 0, total = 0;
            double ph[4] = {};
            for (int bar = 0; bar < 8 * p.n; ++bar)
            {
                const int r = p.chords[bar % p.n][0]; const bool m = p.chords[bar % p.n][1] != 0;
                const int tri[3] = { 0, m ? 3 : 4, 7 };
                for (int i = 0; i < (int) (fs * 1.6); ++i)
                {
                    const double beat = std::fmod ((double) i / fs, 0.4);
                    const float skank = (beat > 0.2 && beat < 0.32) ? 1.0f : 0.15f;     // contratempo
                    float s = 0;
                    for (int k = 0; k < 4; ++k)
                    {
                        const int note = k == 0 ? 36 + r : 60 + r + tri[k - 1];   // baixo a partir do C2, acorde a partir do C4
                        const double hz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
                        ph[k] += hz / fs; if (ph[k] > 1) ph[k] -= 1;
                        const float v = (float) (std::sin (2 * M_PI * ph[k]) + 0.4 * std::sin (4 * M_PI * ph[k]) + 0.2 * std::sin (6 * M_PI * ph[k]));
                        s += k == 0 ? 0.25f * v : 0.08f * v * skank;
                    }
                    if (ear.push (s) && i > (int) (fs * 0.8) && bar >= p.n)
                    {
                        ++total;
                        if (ear.getChordRoot() == r && ear.getChordMinor() == m) ++ok;
                    }
                }
            }
            const bool keyOk = (ear.getKeyRoot() == p.key && ear.getKeyMinor() == p.minor)
                            || (p.minor && ear.getKeyRoot() == (p.key + 3) % 12 && ! ear.getKeyMinor())    // relativa maior
                            || (! p.minor && ear.getKeyRoot() == (p.key + 9) % 12 && ear.getKeyMinor());   // relativa menor
            std::printf ("  tom achado: %d %s\n", ear.getKeyRoot(), ear.getKeyMinor() ? "menor" : "maior");
            check (keyOk, "tom certo (ou a relativa)", ear.getKeyRoot());
            check (total > 0 && ok > total * 0.8, "acorde do momento certo (%)", total ? 100.0 * ok / total : 0.0);
            check (ear.hasSignal() && ear.hasKey(), "sinal e tom aprendido", 1.0);
        }
    // silêncio: some o sinal
    {
        RiddimEar ear; ear.prepare (48000.0);
        for (int i = 0; i < 48000 * 3; ++i) ear.push (0.0f);
        check (! ear.hasSignal(), "silêncio: sem sinal", 0.0);
    }
    std::printf ("\n%s (%d falha%s)\n", failures ? "TESTE FALHOU" : "TESTE OK", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
