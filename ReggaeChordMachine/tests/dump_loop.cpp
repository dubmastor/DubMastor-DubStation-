// Gera os eventos do motor C++ para cenários lidos da entrada padrão (usado pelo web_engine_test.js).
// Cada linha: pattern center velocity noteSize swing bubble [pianist] [fill] | root qual beats inv ; root qual beats inv ; ...
// Saída por cenário: "L <loopBeats>" e uma linha por evento: start length note vel channel slot, depois "END".
#include "../Source/DubEngine.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
using namespace dub;

int main()
{
    std::string line;
    while (std::getline (std::cin, line))
    {
        if (line.empty()) continue;
        const auto bar = line.find ('|');
        std::istringstream head (line.substr (0, bar));
        Settings s; int bubble = 0;
        head >> s.pattern >> s.centerNote >> s.velocity >> s.noteSize >> s.swing >> bubble;
        int pianist = 0;
        if (head >> pianist) s.pianist = pianist;   // PIANISTAS (opcional no fim da linha)
        int fill = 0;
        if (head >> fill) s.fill = fill;            // FILLS (opcional)
        s.bubble = bubble != 0;
        std::vector<ChordSlot> slots;
        std::istringstream rest (line.substr (bar + 1));
        std::string part;
        while (std::getline (rest, part, ';'))
        {
            std::istringstream ps (part);
            ChordSlot c;
            if (ps >> c.root >> c.quality >> c.beats >> c.inversion) slots.push_back (c);
        }
        double L = 0;
        const auto ev = buildLoop (slots, s, L);
        std::printf ("L %.9f\n", L);
        for (auto& e : ev) std::printf ("%.9f %.9f %d %d %d %d\n", e.start, e.length, e.note, e.velocity, e.channel, e.slot);
        const auto v = voiceProgression (slots, s.centerNote);
        std::printf ("S");
        for (size_t i = 0; i < slots.size(); ++i) std::printf (" %d/%d", v.suggested[i], v.used[i]);
        std::printf ("\nEND\n");
    }
}
