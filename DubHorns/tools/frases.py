# Biblioteca de FRASES por década e estilo de reggae — todas ORIGINAIS (ritmo, duração, grau e articulação).
# Gera DubHorns/Source/PhraseStyles.h e o bloco JS da versão web (site/web/dubhorns).
# Uso (na pasta DubHorns):  python3 tools/frases.py cpp > Source/PhraseStyles.h   ·   python3 tools/frases.py js
# Posições em semicolcheias (0 = tempo 1 do compasso 1; 4 = tempo 2; 8 = tempo 3; 16 = compasso 2).
# Graus: 0 = tônica, 2 = 3ª, 4 = 5ª, 7 = oitava (sempre dentro do tom e escala escolhidos).
import json, sys
T,S,F,C,D = 'Ten','Stac','Fall','Scoop','Doit'
STYLES = [
 ('SKA', '1960-66', 'Andamento rápido, naipe em uníssono, contratempo e corridas de jazz.', [
   ('Contratempo ska', '2 comp.', 'Todas as colcheias do contratempo, como o naipe dos anos 60.', 32,
    [(2,1,4,S),(6,1,4,S),(10,1,4,S),(14,1,4,S),(18,1,3,S),(22,1,3,S),(26,1,4,S),(30,1,4,S)]),
   ('Corrida em uníssono', '2 comp.', 'Colcheias subindo o arpejo e resposta que termina em fall.', 32,
    [(0,2,0,T),(2,2,2,T),(4,2,4,T),(6,2,5,T),(8,2,4,S),(10,2,2,S),(12,4,0,T),(16,1,4,S),(18,1,4,S),(20,2,5,T),(22,2,4,T),(24,6,7,F)]),
   ('Riff de dois tempos', '2 comp.', 'Frase curta repetida com variação no fim, para dobrar a guitarra.', 32,
    [(0,1,0,S),(2,1,2,S),(4,2,4,T),(8,1,4,S),(10,1,2,S),(12,2,0,T),(16,1,0,S),(18,1,2,S),(20,2,5,T),(24,1,4,S),(26,1,2,S),(28,3,0,F)]),
   ('Toque de clarim', '2 comp.', 'Repetição na tônica abrindo para a 5ª: chamada para a banda entrar.', 32,
    [(0,1,0,S),(1,1,0,S),(2,2,0,T),(4,4,4,T),(12,1,0,S),(13,1,0,S),(14,2,0,T),(16,4,4,T),(20,10,7,F)]),
   ('Contratempo seco', '2 comp.', "Todos os 'e' do primeiro compasso, respondido no segundo.", 32,
    [(2,1,0,S),(6,1,2,S),(10,1,0,S),(14,1,2,S),(18,1,4,S),(22,1,4,S),(26,2,2,T)]),
 ]),
 ('ROCKSTEADY', '1966-68', 'Mais lento e cantado: notas longas, terças e cauda suave.', [
   ('Fanfarra rocksteady', '4 comp.', 'Abertura em arpejo, segura no topo e resolve na tônica.', 64,
    [(0,4,0,C),(4,4,2,T),(8,4,4,T),(12,4,7,T),(16,12,6,F),(32,2,4,S),(34,2,4,S),(36,4,5,T),(40,8,4,T),(48,2,2,T),(50,2,1,T),(52,12,0,F)]),
   ('Linha em terças', '2 comp.', 'Melodia longa para o naipe harmonizado em terças.', 32,
    [(0,6,2,T),(6,2,3,T),(8,8,4,T),(16,4,5,T),(20,4,4,T),(24,8,2,F)]),
   ('Resposta suave', '2 comp.', 'Silêncio, um pickup e a resposta descendo devagar.', 32,
    [(12,1,4,S),(13,1,5,S),(14,2,4,T),(16,6,2,T),(22,2,1,T),(24,8,0,F)]),
   ('Pontada com cauda', '2 comp.', 'Notas no 2 e no 4 que seguram um pouco, sem pressa.', 32,
    [(4,2,4,T),(12,2,4,T),(20,2,5,T),(28,4,4,F)]),
   ('Escada', '4 comp.', 'Colcheias subindo a escala, doit no topo, desce e termina em fall.', 64,
    [(0,2,0,T),(2,2,1,T),(4,2,2,T),(6,2,3,T),(8,2,4,T),(10,2,5,T),(12,2,6,T),(14,2,7,T),(16,14,7,D),(32,2,7,T),(34,2,6,T),(36,2,5,T),(38,2,4,T),(40,2,3,T),(42,2,2,T),(44,2,1,T),(46,2,0,T),(48,14,0,F)]),
 ]),
 ('EARLY REGGAE', '1968-72', 'O reggae saltitante do fim dos 60: riffs curtos e pontadas junto com o órgão.', [
   ('Skank de metais (2 e 4)', '2 comp.', 'Pontadas secas no 2 e no 4, junto com a guitarra. Base de tudo.', 32,
    [(4,1,4,S),(12,1,4,S),(20,1,2,S),(28,1,2,S)]),
   ('Riff saltitante', '2 comp.', 'Duas semicolcheias empurrando para o 2, com resposta caindo.', 32,
    [(2,1,0,S),(3,1,0,S),(4,2,2,T),(10,1,4,S),(12,2,4,T),(18,1,5,S),(19,1,5,S),(20,2,4,T),(26,1,2,S),(28,3,0,F)]),
   ('Ataque duplo', '2 comp.', "Duas semicolcheias no 'e' do 2, a segunda segura; resposta caindo.", 32,
    [(6,1,4,S),(7,2,4,T),(22,1,5,S),(23,3,4,F)]),
   ('Pergunta curta', '2 comp.', 'Três notas subindo, resposta descendo com fall.', 32,
    [(12,1,2,S),(13,1,3,S),(14,2,4,T),(28,1,4,S),(29,1,3,S),(30,2,2,F)]),
   ('Chamada de três notas', '2 comp.', 'Desce da oitava em três notas, duas vezes.', 32,
    [(0,2,7,S),(2,2,6,T),(4,4,4,T),(16,2,7,S),(18,2,6,T),(20,6,5,F)]),
 ]),
 ('ROOTS ONE DROP', '1972-78', 'O tempo 3 é o centro: chamadas menores, pergunta e resposta, hinos.', [
   ('Chamada menor', '4 comp.', 'Linha descendente melancólica, ótima em menor ou dórico.', 64,
    [(0,6,4,C),(6,2,3,T),(8,4,2,T),(12,4,0,T),(16,14,1,F),(32,2,4,S),(34,2,5,S),(36,2,4,S),(38,2,3,S),(40,8,2,T),(48,3,1,T),(51,3,0,T),(54,10,0,F)]),
   ('Pickup no 3 (one drop)', '2 comp.', 'Três semicolcheias levando ao tempo 3, onde cai o bumbo.', 32,
    [(5,1,2,S),(6,1,3,S),(7,1,4,S),(8,4,4,T),(21,1,4,S),(22,1,5,S),(23,1,6,S),(24,6,7,F)]),
   ('Pergunta e resposta', '4 comp.', 'Dois compassos perguntam subindo, dois respondem descendo.', 64,
    [(2,2,0,T),(4,2,2,T),(6,6,4,T),(14,1,4,S),(16,8,5,F),(34,2,4,T),(36,2,2,T),(38,6,1,T),(46,1,1,S),(48,12,0,F)]),
   ('Lamento no 3', '2 comp.', 'Uma nota que entra por baixo no 3 e cai no compasso seguinte.', 32,
    [(8,6,4,C),(14,2,3,T),(24,8,2,F)]),
   ('Hino roots', '4 comp.', 'Melodia lenta em mínimas, para introdução de música roots.', 64,
    [(0,8,0,C),(8,8,2,T),(16,4,4,T),(20,4,3,T),(24,8,2,T),(32,8,4,T),(40,8,5,T),(48,4,4,T),(52,4,2,T),(56,8,0,F)]),
   ('Pontada com fall', '2 comp.', 'Uma nota forte que despenca: o gesto do sax tenor no reggae.', 32,
    [(10,3,7,F),(26,3,6,F)]),
 ]),
 ('ROCKERS', '1975-80', 'Mais duro e militante: pontadas dobradas, empurrões e cortes secos.', [
   ('Dobrada rockers', '2 comp.', 'Duas pontadas coladas no 2 e no 4, como o naipe de Channel One.', 32,
    [(3,1,4,S),(4,1,4,S),(11,1,4,S),(12,1,4,S),(19,1,5,S),(20,1,5,S),(27,1,4,S),(28,2,4,T)]),
   ('Militante', '2 comp.', 'Colcheias marcadas na tônica e subida para a oitava.', 32,
    [(0,1,0,S),(2,1,0,S),(4,1,0,S),(6,1,0,S),(8,4,2,T),(12,4,4,T),(16,1,0,S),(18,1,0,S),(20,1,0,S),(22,1,0,S),(24,8,7,F)]),
   ("Empurrão no 'a'", '2 comp.', 'Notas antecipadas uma semicolcheia antes do tempo, empurrando o groove.', 32,
    [(7,3,4,T),(15,3,5,T),(23,3,4,T),(31,1,2,S)]),
   ('Corte seco', '2 comp.', 'Pares de pontadas e um fall para fechar.', 32,
    [(4,1,4,S),(6,1,4,S),(12,1,2,S),(14,1,2,S),(20,1,4,S),(22,1,5,S),(28,4,4,F)]),
   ('Scoop + corte', '2 comp.', 'Nota que entra por baixo (scoop) e um corte seco logo depois.', 32,
    [(8,2,4,C),(11,1,4,S),(24,2,6,C),(27,1,4,S)]),
 ]),
 ('DUB', '1973-80', 'Pouca nota e muito espaço: feitas para jogar no Tape Echo e no Dub Throw.', [
   ('Entrada de dub (para o eco)', '4 comp.', 'Notas isoladas e secas, feitas para jogar no Tape Echo.', 64,
    [(0,1,4,S),(8,1,4,S),(24,8,7,F),(40,1,2,S),(44,1,4,S),(48,12,0,C)]),
   ('Nota solta no 3', '2 comp.', 'Uma pontada por compasso. O eco faz o resto.', 32,
    [(8,1,4,S),(24,1,2,S)]),
   ('Throw de duas notas', '2 comp.', 'Duas notas coladas para disparar o Dub Throw.', 32,
    [(2,1,7,S),(3,1,4,S),(18,1,5,S),(19,1,2,S)]),
   ('Fall longo para a sala', '4 comp.', 'Notas longas que caem devagar e deixam a sala soar.', 64,
    [(0,12,7,C),(16,14,4,F),(40,1,2,S),(48,14,0,F)]),
   ('Doit sustentado', '2 comp.', 'Nota longa que sobe no final. Deixe o eco com intensidade alta.', 32,
    [(4,4,4,D),(20,4,5,D)]),
 ]),
 ('DANCEHALL', '1979-85', 'O começo do dancehall: andamento lento, riffs curtos repetidos e muito espaço para o DJ.', [
   ('Riff repetido', '2 comp.', 'Uma ideia curta tocada igual nos dois compassos.', 32,
    [(0,1,4,S),(2,1,4,S),(3,2,5,T),(6,1,4,S),(8,4,2,T),(16,1,4,S),(18,1,4,S),(19,2,5,T),(22,1,4,S),(24,4,2,T)]),
   ('Resposta ao DJ', '2 comp.', 'Silêncio no primeiro compasso e uma resposta no segundo.', 32,
    [(20,1,4,S),(22,1,4,S),(24,2,5,T),(26,2,4,T),(28,4,2,F)]),
   ('Pontada no 1', '2 comp.', 'Marca a cabeça do compasso e sobe para a 5ª.', 32,
    [(0,1,0,S),(16,1,0,S),(18,1,2,S),(20,4,4,F)]),
   ('Sax de rua', '2 comp.', 'Frase de sax com semicolcheias, scoop e fall no fim.', 32,
    [(0,1,4,T),(1,1,5,T),(2,1,4,T),(3,1,2,T),(4,4,0,T),(12,1,2,S),(14,2,4,C),(16,6,5,T),(22,2,4,T),(24,8,2,F)]),
   ('Contracanto lento', '4 comp.', 'Notas longas descendo por baixo da voz.', 64,
    [(0,12,2,C),(16,12,1,T),(32,12,0,T),(48,14,2,F)]),
 ]),
 ('DIGITAL & RAGGA', '1985-95', 'Reggae digital: riffs sincopados (3+3+2), pontadas curtas e frases de sax anos 80.', [
   ('Riff 3+3+2', '2 comp.', 'A divisão sincopada do riddim digital.', 32,
    [(0,1,4,S),(3,1,4,S),(6,2,5,T),(16,1,4,S),(19,1,4,S),(22,2,2,T),(28,4,0,F)]),
   ('Pontadas digitais', '2 comp.', 'Contratempos curtinhos com um empurrão no fim de cada compasso.', 32,
    [(2,1,4,S),(6,1,4,S),(10,1,4,S),(11,1,5,S),(12,2,4,T),(18,1,2,S),(22,1,2,S),(26,1,2,S),(27,1,4,S),(28,2,2,T)]),
   ('Frase de sax anos 80', '4 comp.', 'Melodia cantada de sax tenor, com scoop na entrada e fall nas pontas.', 64,
    [(0,2,4,C),(2,2,5,T),(4,4,7,T),(8,2,5,T),(10,2,4,T),(12,4,2,T),(16,10,4,F),(32,2,2,T),(34,2,4,T),(36,4,5,T),(40,2,4,T),(42,2,2,T),(44,4,1,T),(48,12,0,F)]),
   ('Ragga hit', '2 comp.', 'Pontadas na oitava a cada tempo forte e descida rápida.', 32,
    [(0,1,7,S),(8,1,7,S),(16,1,7,S),(24,1,6,S),(26,1,5,S),(28,3,4,F)]),
   ('Doit de resposta', '2 comp.', 'Subida curta que termina em doit, resposta que cai.', 32,
    [(12,1,4,S),(13,1,5,S),(14,6,7,D),(28,1,4,S),(29,1,2,S),(30,2,0,F)]),
 ]),
]
ART={'Ten':0,'Stac':1,'Fall':2,'Scoop':3,'Doit':4}
def cs(s): return '"'+s.replace('\\','\\\\').replace('"','\\"')+'"'
def check():
    for st in STYLES:
        for p in st[3]:
            n,tag,desc,ln,ev=p
            assert len(ev)<=32 and ln in (32,64),n
            for s,d,g,a in ev: assert 0<=s<ln and d>=1 and 0<=g<=7,(n,s)
check()
if sys.argv[1]=='cpp':
    o=['// GERADO por tools/frases.py — biblioteca de frases ORIGINAIS por década e estilo de reggae.',
       '// Mesma tabela da versão web (site/web/dubhorns). Não edite à mão: edite tools/frases.py.',
       '#pragma once','#include "Phrases.h"','namespace horns','{',
       'struct PhraseStyle { const char* name; const char* era; const char* desc; std::vector<PatternDef> pats; };',
       'inline const std::vector<PhraseStyle>& phraseStyles()','{','    static const std::vector<PhraseStyle> s {']
    for name,era,desc,pats in STYLES:
        o.append('        { %s, %s, %s, {'%(cs(name),cs(era),cs(desc)))
        for n,tag,d,ln,ev in pats:
            evs=', '.join('{ %d, %d, %d, %s }'%(s,du,g,a) for s,du,g,a in ev)
            o.append('            { %s, %s, %s, %d,\n              { %s } },'%(cs(n),cs(tag),cs(d),ln,evs))
        o.append('        } },')
    o+=['    };','    return s;','}','} // namespace horns','']
    print('\n'.join(o))
else:
    out=[dict(n=n,era=e,d=d,p=[dict(n=pn,tag=t,d=pd,len=ln,e=[[s,du,g,a.lower() if a!='Ten' else 'ten'] for s,du,g,a in ev]) for pn,t,pd,ln,ev in pats]) for n,e,d,pats in STYLES]
    print('const FRASE_ESTILOS = '+json.dumps(out,ensure_ascii=False,separators=(',',':'))+';')
