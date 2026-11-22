// Despeja padrões do gerador C++ em JSON (uma linha por combinação de controles).
// O teste tests/web_generator_test.js gera as mesmas combinações com site/drums/generator.js
// e confere nota a nota que a versão do navegador toca igual ao plugin.
#include "../Source/PatternGenerator.h"
#include <cstdio>

using namespace dubdrums;

static void dumpEvents (const std::vector<NoteEvent>& evs)
{
    std::printf ("[");
    for (size_t i = 0; i < evs.size(); ++i)
        std::printf ("%s[%.9f,%.4f,%d,%d,%d]", i ? "," : "", evs[i].beat, evs[i].length, evs[i].note, evs[i].velocity, evs[i].bar);
    std::printf ("]");
}

int main()
{
    const float rands[] = { 0.0f, 0.15f, 0.35f, 0.6f, 1.0f };
    int n = 0;
    for (int style = 0; style < 3; ++style)
    for (int drummer = 0; drummer < numDrummers; ++drummer)
    for (int hat = 0; hat < 4; ++hat)
    for (int k = 0; k < 6; ++k)
    {
        Settings s;
        s.style = (Style) style;
        s.kickMask = defaultKickMask (s.style); s.snareMask = defaultSnareMask (s.style);
        if (k == 5) { s.kickMask = 0x83; s.snareMask = 0x50; }
        s.drummer = (Drummer) drummer;
        s.hatPattern = (HatPattern) hat;
        s.swing = (Swing) ((k + hat) % 5);
        s.swingAll = (k % 2) == 0;
        s.dynamics = (Dynamics) ((k + style) % 6);
        s.backbeat = (Backbeat) (k % 3);
        s.bars = 1 + (k + drummer) % 4;
        s.fill = (k % 3) != 0;
        s.intro = (k % 2) == 1;
        s.machine = k == 4;
        s.randKick = rands[k % 5]; s.randSnare = rands[(k + 1) % 5]; s.randHat = rands[(k + 2) % 5]; s.randPerc = rands[(k + 3) % 5];
        s.seed = (uint32_t) (1 + n * 37 % 9999);
        s.strict = (n % 3) == 2;   // modo "de livro" do MRM-0420
        const auto p = generate (s);
        std::printf ("{\"s\":{\"style\":%d,\"kickMask\":%d,\"snareMask\":%d,\"bars\":%d,\"hatPattern\":%d,\"swing\":%d,\"swingAll\":%s,"
                     "\"dynamics\":%d,\"randKick\":%.9g,\"randSnare\":%.9g,\"randHat\":%.9g,\"randPerc\":%.9g,\"drummer\":%d,\"backbeat\":%d,"
                     "\"machine\":%s,\"fill\":%s,\"intro\":%s,\"strict\":%s,\"seed\":%u},\"len\":%.3f,\"introLen\":%.3f,\"events\":",
                     style, s.kickMask, s.snareMask, s.bars, hat, (int) s.swing, s.swingAll ? "true" : "false", (int) s.dynamics,
                     (double) s.randKick, (double) s.randSnare, (double) s.randHat, (double) s.randPerc, drummer, (int) s.backbeat,
                     s.machine ? "true" : "false", s.fill ? "true" : "false", s.intro ? "true" : "false", s.strict ? "true" : "false", s.seed,
                     p.lengthInBeats, p.introLengthInBeats);
        dumpEvents (p.events);
        std::printf (",\"intro\":");
        dumpEvents (p.intro);
        std::printf ("}\n");
        ++n;
    }
    return 0;
}
