// GERADO por tools/frases.py — biblioteca de frases ORIGINAIS por década e estilo de reggae.
// Mesma tabela da versão web (site/web/dubhorns). Não edite à mão: edite tools/frases.py.
#pragma once
#include "Phrases.h"
namespace horns
{
struct PhraseStyle { const char* name; const char* era; const char* desc; std::vector<PatternDef> pats; };
inline const std::vector<PhraseStyle>& phraseStyles()
{
    static const std::vector<PhraseStyle> s {
        { "SKA", "1960-66", "Andamento rápido, naipe em uníssono, contratempo e corridas de jazz.", {
            { "Contratempo ska", "2 comp.", "Todas as colcheias do contratempo, como o naipe dos anos 60.", 32,
              { { 2, 1, 4, Stac }, { 6, 1, 4, Stac }, { 10, 1, 4, Stac }, { 14, 1, 4, Stac }, { 18, 1, 3, Stac }, { 22, 1, 3, Stac }, { 26, 1, 4, Stac }, { 30, 1, 4, Stac } } },
            { "Corrida em uníssono", "2 comp.", "Colcheias subindo o arpejo e resposta que termina em fall.", 32,
              { { 0, 2, 0, Ten }, { 2, 2, 2, Ten }, { 4, 2, 4, Ten }, { 6, 2, 5, Ten }, { 8, 2, 4, Stac }, { 10, 2, 2, Stac }, { 12, 4, 0, Ten }, { 16, 1, 4, Stac }, { 18, 1, 4, Stac }, { 20, 2, 5, Ten }, { 22, 2, 4, Ten }, { 24, 6, 7, Fall } } },
            { "Riff de dois tempos", "2 comp.", "Frase curta repetida com variação no fim, para dobrar a guitarra.", 32,
              { { 0, 1, 0, Stac }, { 2, 1, 2, Stac }, { 4, 2, 4, Ten }, { 8, 1, 4, Stac }, { 10, 1, 2, Stac }, { 12, 2, 0, Ten }, { 16, 1, 0, Stac }, { 18, 1, 2, Stac }, { 20, 2, 5, Ten }, { 24, 1, 4, Stac }, { 26, 1, 2, Stac }, { 28, 3, 0, Fall } } },
            { "Toque de clarim", "2 comp.", "Repetição na tônica abrindo para a 5ª: chamada para a banda entrar.", 32,
              { { 0, 1, 0, Stac }, { 1, 1, 0, Stac }, { 2, 2, 0, Ten }, { 4, 4, 4, Ten }, { 12, 1, 0, Stac }, { 13, 1, 0, Stac }, { 14, 2, 0, Ten }, { 16, 4, 4, Ten }, { 20, 10, 7, Fall } } },
            { "Contratempo seco", "2 comp.", "Todos os 'e' do primeiro compasso, respondido no segundo.", 32,
              { { 2, 1, 0, Stac }, { 6, 1, 2, Stac }, { 10, 1, 0, Stac }, { 14, 1, 2, Stac }, { 18, 1, 4, Stac }, { 22, 1, 4, Stac }, { 26, 2, 2, Ten } } },
        } },
        { "ROCKSTEADY", "1966-68", "Mais lento e cantado: notas longas, terças e cauda suave.", {
            { "Fanfarra rocksteady", "4 comp.", "Abertura em arpejo, segura no topo e resolve na tônica.", 64,
              { { 0, 4, 0, Scoop }, { 4, 4, 2, Ten }, { 8, 4, 4, Ten }, { 12, 4, 7, Ten }, { 16, 12, 6, Fall }, { 32, 2, 4, Stac }, { 34, 2, 4, Stac }, { 36, 4, 5, Ten }, { 40, 8, 4, Ten }, { 48, 2, 2, Ten }, { 50, 2, 1, Ten }, { 52, 12, 0, Fall } } },
            { "Linha em terças", "2 comp.", "Melodia longa para o naipe harmonizado em terças.", 32,
              { { 0, 6, 2, Ten }, { 6, 2, 3, Ten }, { 8, 8, 4, Ten }, { 16, 4, 5, Ten }, { 20, 4, 4, Ten }, { 24, 8, 2, Fall } } },
            { "Resposta suave", "2 comp.", "Silêncio, um pickup e a resposta descendo devagar.", 32,
              { { 12, 1, 4, Stac }, { 13, 1, 5, Stac }, { 14, 2, 4, Ten }, { 16, 6, 2, Ten }, { 22, 2, 1, Ten }, { 24, 8, 0, Fall } } },
            { "Pontada com cauda", "2 comp.", "Notas no 2 e no 4 que seguram um pouco, sem pressa.", 32,
              { { 4, 2, 4, Ten }, { 12, 2, 4, Ten }, { 20, 2, 5, Ten }, { 28, 4, 4, Fall } } },
            { "Escada", "4 comp.", "Colcheias subindo a escala, doit no topo, desce e termina em fall.", 64,
              { { 0, 2, 0, Ten }, { 2, 2, 1, Ten }, { 4, 2, 2, Ten }, { 6, 2, 3, Ten }, { 8, 2, 4, Ten }, { 10, 2, 5, Ten }, { 12, 2, 6, Ten }, { 14, 2, 7, Ten }, { 16, 14, 7, Doit }, { 32, 2, 7, Ten }, { 34, 2, 6, Ten }, { 36, 2, 5, Ten }, { 38, 2, 4, Ten }, { 40, 2, 3, Ten }, { 42, 2, 2, Ten }, { 44, 2, 1, Ten }, { 46, 2, 0, Ten }, { 48, 14, 0, Fall } } },
        } },
        { "EARLY REGGAE", "1968-72", "O reggae saltitante do fim dos 60: riffs curtos e pontadas junto com o órgão.", {
            { "Skank de metais (2 e 4)", "2 comp.", "Pontadas secas no 2 e no 4, junto com a guitarra. Base de tudo.", 32,
              { { 4, 1, 4, Stac }, { 12, 1, 4, Stac }, { 20, 1, 2, Stac }, { 28, 1, 2, Stac } } },
            { "Riff saltitante", "2 comp.", "Duas semicolcheias empurrando para o 2, com resposta caindo.", 32,
              { { 2, 1, 0, Stac }, { 3, 1, 0, Stac }, { 4, 2, 2, Ten }, { 10, 1, 4, Stac }, { 12, 2, 4, Ten }, { 18, 1, 5, Stac }, { 19, 1, 5, Stac }, { 20, 2, 4, Ten }, { 26, 1, 2, Stac }, { 28, 3, 0, Fall } } },
            { "Ataque duplo", "2 comp.", "Duas semicolcheias no 'e' do 2, a segunda segura; resposta caindo.", 32,
              { { 6, 1, 4, Stac }, { 7, 2, 4, Ten }, { 22, 1, 5, Stac }, { 23, 3, 4, Fall } } },
            { "Pergunta curta", "2 comp.", "Três notas subindo, resposta descendo com fall.", 32,
              { { 12, 1, 2, Stac }, { 13, 1, 3, Stac }, { 14, 2, 4, Ten }, { 28, 1, 4, Stac }, { 29, 1, 3, Stac }, { 30, 2, 2, Fall } } },
            { "Chamada de três notas", "2 comp.", "Desce da oitava em três notas, duas vezes.", 32,
              { { 0, 2, 7, Stac }, { 2, 2, 6, Ten }, { 4, 4, 4, Ten }, { 16, 2, 7, Stac }, { 18, 2, 6, Ten }, { 20, 6, 5, Fall } } },
        } },
        { "ROOTS ONE DROP", "1972-78", "O tempo 3 é o centro: chamadas menores, pergunta e resposta, hinos.", {
            { "Chamada menor", "4 comp.", "Linha descendente melancólica, ótima em menor ou dórico.", 64,
              { { 0, 6, 4, Scoop }, { 6, 2, 3, Ten }, { 8, 4, 2, Ten }, { 12, 4, 0, Ten }, { 16, 14, 1, Fall }, { 32, 2, 4, Stac }, { 34, 2, 5, Stac }, { 36, 2, 4, Stac }, { 38, 2, 3, Stac }, { 40, 8, 2, Ten }, { 48, 3, 1, Ten }, { 51, 3, 0, Ten }, { 54, 10, 0, Fall } } },
            { "Pickup no 3 (one drop)", "2 comp.", "Três semicolcheias levando ao tempo 3, onde cai o bumbo.", 32,
              { { 5, 1, 2, Stac }, { 6, 1, 3, Stac }, { 7, 1, 4, Stac }, { 8, 4, 4, Ten }, { 21, 1, 4, Stac }, { 22, 1, 5, Stac }, { 23, 1, 6, Stac }, { 24, 6, 7, Fall } } },
            { "Pergunta e resposta", "4 comp.", "Dois compassos perguntam subindo, dois respondem descendo.", 64,
              { { 2, 2, 0, Ten }, { 4, 2, 2, Ten }, { 6, 6, 4, Ten }, { 14, 1, 4, Stac }, { 16, 8, 5, Fall }, { 34, 2, 4, Ten }, { 36, 2, 2, Ten }, { 38, 6, 1, Ten }, { 46, 1, 1, Stac }, { 48, 12, 0, Fall } } },
            { "Lamento no 3", "2 comp.", "Uma nota que entra por baixo no 3 e cai no compasso seguinte.", 32,
              { { 8, 6, 4, Scoop }, { 14, 2, 3, Ten }, { 24, 8, 2, Fall } } },
            { "Hino roots", "4 comp.", "Melodia lenta em mínimas, para introdução de música roots.", 64,
              { { 0, 8, 0, Scoop }, { 8, 8, 2, Ten }, { 16, 4, 4, Ten }, { 20, 4, 3, Ten }, { 24, 8, 2, Ten }, { 32, 8, 4, Ten }, { 40, 8, 5, Ten }, { 48, 4, 4, Ten }, { 52, 4, 2, Ten }, { 56, 8, 0, Fall } } },
            { "Pontada com fall", "2 comp.", "Uma nota forte que despenca: o gesto do sax tenor no reggae.", 32,
              { { 10, 3, 7, Fall }, { 26, 3, 6, Fall } } },
        } },
        { "ROCKERS", "1975-80", "Mais duro e militante: pontadas dobradas, empurrões e cortes secos.", {
            { "Dobrada rockers", "2 comp.", "Duas pontadas coladas no 2 e no 4, como o naipe de Channel One.", 32,
              { { 3, 1, 4, Stac }, { 4, 1, 4, Stac }, { 11, 1, 4, Stac }, { 12, 1, 4, Stac }, { 19, 1, 5, Stac }, { 20, 1, 5, Stac }, { 27, 1, 4, Stac }, { 28, 2, 4, Ten } } },
            { "Militante", "2 comp.", "Colcheias marcadas na tônica e subida para a oitava.", 32,
              { { 0, 1, 0, Stac }, { 2, 1, 0, Stac }, { 4, 1, 0, Stac }, { 6, 1, 0, Stac }, { 8, 4, 2, Ten }, { 12, 4, 4, Ten }, { 16, 1, 0, Stac }, { 18, 1, 0, Stac }, { 20, 1, 0, Stac }, { 22, 1, 0, Stac }, { 24, 8, 7, Fall } } },
            { "Empurrão no 'a'", "2 comp.", "Notas antecipadas uma semicolcheia antes do tempo, empurrando o groove.", 32,
              { { 7, 3, 4, Ten }, { 15, 3, 5, Ten }, { 23, 3, 4, Ten }, { 31, 1, 2, Stac } } },
            { "Corte seco", "2 comp.", "Pares de pontadas e um fall para fechar.", 32,
              { { 4, 1, 4, Stac }, { 6, 1, 4, Stac }, { 12, 1, 2, Stac }, { 14, 1, 2, Stac }, { 20, 1, 4, Stac }, { 22, 1, 5, Stac }, { 28, 4, 4, Fall } } },
            { "Scoop + corte", "2 comp.", "Nota que entra por baixo (scoop) e um corte seco logo depois.", 32,
              { { 8, 2, 4, Scoop }, { 11, 1, 4, Stac }, { 24, 2, 6, Scoop }, { 27, 1, 4, Stac } } },
        } },
        { "DUB", "1973-80", "Pouca nota e muito espaço: feitas para jogar no Tape Echo e no Dub Throw.", {
            { "Entrada de dub (para o eco)", "4 comp.", "Notas isoladas e secas, feitas para jogar no Tape Echo.", 64,
              { { 0, 1, 4, Stac }, { 8, 1, 4, Stac }, { 24, 8, 7, Fall }, { 40, 1, 2, Stac }, { 44, 1, 4, Stac }, { 48, 12, 0, Scoop } } },
            { "Nota solta no 3", "2 comp.", "Uma pontada por compasso. O eco faz o resto.", 32,
              { { 8, 1, 4, Stac }, { 24, 1, 2, Stac } } },
            { "Throw de duas notas", "2 comp.", "Duas notas coladas para disparar o Dub Throw.", 32,
              { { 2, 1, 7, Stac }, { 3, 1, 4, Stac }, { 18, 1, 5, Stac }, { 19, 1, 2, Stac } } },
            { "Fall longo para a sala", "4 comp.", "Notas longas que caem devagar e deixam a sala soar.", 64,
              { { 0, 12, 7, Scoop }, { 16, 14, 4, Fall }, { 40, 1, 2, Stac }, { 48, 14, 0, Fall } } },
            { "Doit sustentado", "2 comp.", "Nota longa que sobe no final. Deixe o eco com intensidade alta.", 32,
              { { 4, 4, 4, Doit }, { 20, 4, 5, Doit } } },
        } },
        { "DANCEHALL", "1979-85", "O começo do dancehall: andamento lento, riffs curtos repetidos e muito espaço para o DJ.", {
            { "Riff repetido", "2 comp.", "Uma ideia curta tocada igual nos dois compassos.", 32,
              { { 0, 1, 4, Stac }, { 2, 1, 4, Stac }, { 3, 2, 5, Ten }, { 6, 1, 4, Stac }, { 8, 4, 2, Ten }, { 16, 1, 4, Stac }, { 18, 1, 4, Stac }, { 19, 2, 5, Ten }, { 22, 1, 4, Stac }, { 24, 4, 2, Ten } } },
            { "Resposta ao DJ", "2 comp.", "Silêncio no primeiro compasso e uma resposta no segundo.", 32,
              { { 20, 1, 4, Stac }, { 22, 1, 4, Stac }, { 24, 2, 5, Ten }, { 26, 2, 4, Ten }, { 28, 4, 2, Fall } } },
            { "Pontada no 1", "2 comp.", "Marca a cabeça do compasso e sobe para a 5ª.", 32,
              { { 0, 1, 0, Stac }, { 16, 1, 0, Stac }, { 18, 1, 2, Stac }, { 20, 4, 4, Fall } } },
            { "Sax de rua", "2 comp.", "Frase de sax com semicolcheias, scoop e fall no fim.", 32,
              { { 0, 1, 4, Ten }, { 1, 1, 5, Ten }, { 2, 1, 4, Ten }, { 3, 1, 2, Ten }, { 4, 4, 0, Ten }, { 12, 1, 2, Stac }, { 14, 2, 4, Scoop }, { 16, 6, 5, Ten }, { 22, 2, 4, Ten }, { 24, 8, 2, Fall } } },
            { "Contracanto lento", "4 comp.", "Notas longas descendo por baixo da voz.", 64,
              { { 0, 12, 2, Scoop }, { 16, 12, 1, Ten }, { 32, 12, 0, Ten }, { 48, 14, 2, Fall } } },
        } },
        { "DIGITAL & RAGGA", "1985-95", "Reggae digital: riffs sincopados (3+3+2), pontadas curtas e frases de sax anos 80.", {
            { "Riff 3+3+2", "2 comp.", "A divisão sincopada do riddim digital.", 32,
              { { 0, 1, 4, Stac }, { 3, 1, 4, Stac }, { 6, 2, 5, Ten }, { 16, 1, 4, Stac }, { 19, 1, 4, Stac }, { 22, 2, 2, Ten }, { 28, 4, 0, Fall } } },
            { "Pontadas digitais", "2 comp.", "Contratempos curtinhos com um empurrão no fim de cada compasso.", 32,
              { { 2, 1, 4, Stac }, { 6, 1, 4, Stac }, { 10, 1, 4, Stac }, { 11, 1, 5, Stac }, { 12, 2, 4, Ten }, { 18, 1, 2, Stac }, { 22, 1, 2, Stac }, { 26, 1, 2, Stac }, { 27, 1, 4, Stac }, { 28, 2, 2, Ten } } },
            { "Frase de sax anos 80", "4 comp.", "Melodia cantada de sax tenor, com scoop na entrada e fall nas pontas.", 64,
              { { 0, 2, 4, Scoop }, { 2, 2, 5, Ten }, { 4, 4, 7, Ten }, { 8, 2, 5, Ten }, { 10, 2, 4, Ten }, { 12, 4, 2, Ten }, { 16, 10, 4, Fall }, { 32, 2, 2, Ten }, { 34, 2, 4, Ten }, { 36, 4, 5, Ten }, { 40, 2, 4, Ten }, { 42, 2, 2, Ten }, { 44, 4, 1, Ten }, { 48, 12, 0, Fall } } },
            { "Ragga hit", "2 comp.", "Pontadas na oitava a cada tempo forte e descida rápida.", 32,
              { { 0, 1, 7, Stac }, { 8, 1, 7, Stac }, { 16, 1, 7, Stac }, { 24, 1, 6, Stac }, { 26, 1, 5, Stac }, { 28, 3, 4, Fall } } },
            { "Doit de resposta", "2 comp.", "Subida curta que termina em doit, resposta que cai.", 32,
              { { 12, 1, 4, Stac }, { 13, 1, 5, Stac }, { 14, 6, 7, Doit }, { 28, 1, 4, Stac }, { 29, 1, 2, Stac }, { 30, 2, 0, Fall } } },
        } },
    };
    return s;
}
} // namespace horns

