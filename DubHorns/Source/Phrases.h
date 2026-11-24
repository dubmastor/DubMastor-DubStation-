// =============================================================================
//  Mastor Dub Horns — sugestões de frases (ataques curtos e introduções)
//  Cada sugestão traz só ritmo, duração, articulação e um grau sugerido.
//  As notas são escolhidas pelo usuário (ou sugeridas no tom, se ele quiser).
// =============================================================================
#pragma once
#include <vector>
#include <string>
#include "Music.h"

namespace horns
{
enum Artic { Ten = 0, Stac, Fall, Scoop, Doit, kNumArtic };

inline const char* articName (int a)
{
    static const char* n[kNumArtic] = { "normal", "staccato", "fall", "scoop", "doit" };
    return n[std::clamp (a, 0, kNumArtic - 1)];
}

struct PhraseEvent
{
    int start = 0;     // semicolcheia (0..len-1)
    int dur = 2;       // em semicolcheias
    int note = -1;     // MIDI escolhido pelo usuário (-1 = ainda sem nota)
    int artic = Ten;
    int degree = 0;    // grau sugerido (para "reaplicar sugestão no tom")
};

struct Phrase
{
    std::string name;
    int length = 32;   // semicolcheias (32 = 2 compassos 4/4)
    std::vector<PhraseEvent> events;
};

struct PatternDef
{
    const char* name; const char* tag; const char* desc; int len;
    std::vector<std::array<int, 4>> ev;   // start, dur, grau, articulação
};

constexpr int kMaxEvents = 32;

inline const std::vector<PatternDef>& stabPatterns()
{
    static const std::vector<PatternDef> p {
        { "Skank de metais (2 e 4)", "2 comp.", "Pontadas secas no 2 e no 4, junto com a guitarra. Base de tudo.", 32,
          { { 4, 1, 4, Stac }, { 12, 1, 4, Stac }, { 20, 1, 2, Stac }, { 28, 1, 2, Stac } } },
        { "Ataque duplo", "2 comp.", "Duas semicolcheias no 'e' do 2, a segunda segura; resposta caindo.", 32,
          { { 6, 1, 4, Stac }, { 7, 2, 4, Ten }, { 22, 1, 5, Stac }, { 23, 3, 4, Fall } } },
        { "Pontada com fall", "2 comp.", "Uma nota forte que despenca: o gesto do sax tenor no dub.", 32,
          { { 10, 3, 7, Fall }, { 26, 3, 6, Fall } } },
        { "Pergunta curta", "2 comp.", "Tres notas subindo, resposta descendo com fall.", 32,
          { { 12, 1, 2, Stac }, { 13, 1, 3, Stac }, { 14, 2, 4, Ten }, { 28, 1, 4, Stac }, { 29, 1, 3, Stac }, { 30, 2, 2, Fall } } },
        { "Contratempo seco", "2 comp.", "Todos os 'e' do primeiro compasso, respondido no segundo.", 32,
          { { 2, 1, 0, Stac }, { 6, 1, 2, Stac }, { 10, 1, 0, Stac }, { 14, 1, 2, Stac }, { 18, 1, 4, Stac }, { 22, 1, 4, Stac }, { 26, 2, 2, Ten } } },
        { "Scoop + corte", "2 comp.", "Nota que entra por baixo (scoop) e um corte seco logo depois.", 32,
          { { 8, 2, 4, Scoop }, { 11, 1, 4, Stac }, { 24, 2, 6, Scoop }, { 27, 1, 4, Stac } } },
        { "Pickup na batida (one drop)", "2 comp.", "Tres semicolcheias levando a batida do one drop (bumbo e aro juntos): no 2 e depois no 4.", 32,
          { { 1, 1, 2, Stac }, { 2, 1, 3, Stac }, { 3, 1, 4, Stac }, { 4, 4, 4, Ten }, { 25, 1, 4, Stac }, { 26, 1, 5, Stac }, { 27, 1, 6, Stac }, { 28, 4, 7, Fall } } },
        { "Doit sustentado", "2 comp.", "Nota longa que sobe no final. Deixe o eco com intensidade alta.", 32,
          { { 4, 4, 4, Doit }, { 20, 4, 5, Doit } } },
    };
    return p;
}

inline const std::vector<PatternDef>& introPatterns()
{
    static const std::vector<PatternDef> p {
        { "Fanfarra rocksteady", "4 comp.", "Abertura em arpejo, segura no topo e resolve na tonica.", 64,
          { { 0, 4, 0, Scoop }, { 4, 4, 2, Ten }, { 8, 4, 4, Ten }, { 12, 4, 7, Ten }, { 16, 12, 6, Fall }, { 32, 2, 4, Stac }, { 34, 2, 4, Stac },
            { 36, 4, 5, Ten }, { 40, 8, 4, Ten }, { 48, 2, 2, Ten }, { 50, 2, 1, Ten }, { 52, 12, 0, Fall } } },
        { "Chamada menor", "4 comp.", "Linha descendente melancolica, otima em menor ou dorico.", 64,
          { { 0, 6, 4, Scoop }, { 6, 2, 3, Ten }, { 8, 4, 2, Ten }, { 12, 4, 0, Ten }, { 16, 14, 1, Fall }, { 32, 2, 4, Stac }, { 34, 2, 5, Stac },
            { 36, 2, 4, Stac }, { 38, 2, 3, Stac }, { 40, 8, 2, Ten }, { 48, 3, 1, Ten }, { 51, 3, 0, Ten }, { 54, 10, 0, Fall } } },
        { "Escada", "4 comp.", "Colcheias subindo a escala, doit no topo, desce e termina em fall.", 64,
          { { 0, 2, 0, Ten }, { 2, 2, 1, Ten }, { 4, 2, 2, Ten }, { 6, 2, 3, Ten }, { 8, 2, 4, Ten }, { 10, 2, 5, Ten }, { 12, 2, 6, Ten }, { 14, 2, 7, Ten },
            { 16, 14, 7, Doit }, { 32, 2, 7, Ten }, { 34, 2, 6, Ten }, { 36, 2, 5, Ten }, { 38, 2, 4, Ten }, { 40, 2, 3, Ten }, { 42, 2, 2, Ten },
            { 44, 2, 1, Ten }, { 46, 2, 0, Ten }, { 48, 14, 0, Fall } } },
        { "Pergunta e resposta", "4 comp.", "Dois compassos perguntam subindo, dois respondem descendo.", 64,
          { { 2, 2, 0, Ten }, { 4, 2, 2, Ten }, { 6, 6, 4, Ten }, { 14, 1, 4, Stac }, { 16, 8, 5, Fall }, { 34, 2, 4, Ten }, { 36, 2, 2, Ten },
            { 38, 6, 1, Ten }, { 46, 1, 1, Stac }, { 48, 12, 0, Fall } } },
        { "Entrada de dub (para o eco)", "4 comp.", "Notas isoladas e secas, feitas para jogar no Tape Echo.", 64,
          { { 0, 1, 4, Stac }, { 8, 1, 4, Stac }, { 24, 8, 7, Fall }, { 40, 1, 2, Stac }, { 44, 1, 4, Stac }, { 48, 12, 0, Scoop } } },
        { "Toque de clarim", "2 comp.", "Repeticao na tonica abrindo para a 5a: chamada para a banda entrar.", 32,
          { { 0, 1, 0, Stac }, { 1, 1, 0, Stac }, { 2, 2, 0, Ten }, { 4, 4, 4, Ten }, { 12, 1, 0, Stac }, { 13, 1, 0, Stac }, { 14, 2, 0, Ten },
            { 16, 4, 4, Ten }, { 20, 10, 7, Fall } } },
    };
    return p;
}

// Carrega uma sugestão. suggestNotes = false: só ritmo (todas as notas ficam para o usuário escolher)
inline Phrase makePhrase (const PatternDef& p, bool suggestNotes, int key, int scale)
{
    Phrase ph;
    ph.name = p.name;
    ph.length = p.len;
    for (const auto& e : p.ev)
    {
        PhraseEvent ev;
        ev.start = e[0]; ev.dur = e[1]; ev.degree = e[2]; ev.artic = e[3];
        ev.note = suggestNotes ? degreeToMidi (e[2], key, scale) : -1;
        ph.events.push_back (ev);
    }
    return ph;
}

inline void sortEvents (Phrase& p)
{
    std::stable_sort (p.events.begin(), p.events.end(), [] (const PhraseEvent& a, const PhraseEvent& b) { return a.start < b.start; });
}
} // namespace horns
