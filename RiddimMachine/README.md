# Mastor Riddim Machine MRM-0420 1.0 — DubStation

← [Voltar para a página inicial do repositório](../README.md)

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRM-0420-1.0/MRM-0420-Windows.zip)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRM-0420-1.0/MRM-0420-macOS.zip)** · **📱 [Versão para celular](https://dubmastor.github.io/DubMastor-DubStation-/riddim/)**

![MRM-0420, aba RIDDIM](docs/interface-riddim.png)

As máquinas da DubStation num instrumento só, para começar um riddim em segundos:

| Máquina | Parte | Aba |
|---|---|---|
| **Mastor Reggae Drum Machine MRDM-12** | bateria, com os samples DM (`DubDrums/Samples`) | BATERIA |
| **Mastor Reggae Chord Machine MRCM-1240** | skank de piano M1 e bubble de órgão | HARMONIA |
| **Mastor Reggae Bass Machine MRBM-69** | linha de baixo, com os timbres do MRDBI-07 | BAIXO |
| **Mastor Big Knob** | filtro, tape echo, sirene e maximizer no master | CONTROL ROOM |

A **harmonia é uma só** para o MRCM-1240 e o MRBM-69 (mexer nos acordes em qualquer aba muda as duas) e o
**BPM é um só** para todas. Formatos: **VST3** (Windows/macOS), **AU** (macOS) e **Standalone**. É um
**instrumento**: no Cubase, carregue numa *Instrument Track*.

## Aba RIDDIM (a principal)
| Parte | O que faz |
|---|---|
| **HARMONIA** | Os 4 acordes no padrão do MRCM-1240: teclado, MAIOR/MENOR, inversão (com a sugestão automática) e duração. Valem para o skank e para o baixo |
| **MÁQUINAS** | Cada máquina com a chave **LIGADA/DESLIGADA**, os músicos clicáveis (bateristas, padrões do skank e baixistas), o **MIDI na tela, editável**, e o **ARRASTE** só daquela máquina |
| **JACKPOT** | A semente nos rolos de caça-níquel e o botão vermelho **NOVO RIDDIM**: sorteia outra semente para a bateria e o baixo (os rolos giram e param um a um, como no MRDM-12). Arraste os rolos ou use a rodinha para escolher a semente na mão |
| **RIDDIM** | Tamanho (4 a 64 compassos), **ARRASTE O RIDDIM** (as máquinas ligadas num `.mid` só, alinhadas), **EXPORTAR**, a chave **CONTROL ROOM** e a chave **TAPE ECHO** |
| Alto-falante | Caixa de madeira com grade de metal perfurada; o cone mexe com o som |

### MIDI editável na tela
- **Bateria:** clique numa semicolcheia para pôr/tirar bumbo, caixa, aro, hi-hat ou hi-hat aberto; arrastar pinta várias; a rodinha muda a força. São as mesmas edições da grade PADRÃO do MRDM-12.
- **Harmonia:** clique num espaço vazio do SKANK ou do BUBBLE para pôr um ataque do acorde (com as mesmas notas do ataque mais próximo); clique num ataque para tirar; a rodinha muda a força.
- **Baixo:** o visor do MRBM-69: clique põe nota (na pentatônica do acorde), arraste move, borda estica, duplo clique apaga.
- **DESFAZER EDIÇÃO** aparece na máquina editada. A edição fica salva no projeto e vale até um controle do gerador daquela máquina ser mexido (como nas máquinas separadas).
- O que você vê é o que toca e o que é arrastado.

## Cabeçalho (vale para todas as abas)
| Controle | O que faz |
|---|---|
| **ABAS** | RIDDIM, HARMONIA, BATERIA, BAIXO e CONTROL ROOM |
| **PLAY RIDDIM** | Toca todas as máquinas juntas, sem a DAW rodando, começando pela intro |
| **BPM** | Um BPM para todas. Com a DAW tocando, mostra o BPM da DAW |

## Abas das máquinas
Cada aba mostra o painel completo da máquina, igual ao plugin separado (com o mesmo código: o que muda nas
máquinas entra no MRM-0420 no próximo build). Na aba BAIXO, o acompanhamento interno do MRBM-69 fica desligado:
no MRM-0420 a bateria e o teclado tocam de verdade.

## Aba CONTROL ROOM
![MRM-0420, aba CONTROL ROOM](docs/interface-control-room.png)

- O **Mastor Big Knob** inteiro no master: filtro big knob, tape echo, sirene de dub, VUs e maximizer.
- **MUTE DAS MÁQUINAS**, na ordem **DRUMS** (verde), **BASS** (amarelo) e **RIDDIM** (vermelho, o skank e o bubble do MRCM-1240). O mute tira só o som: o MIDI continua saindo e a máquina continua no riddim arrastado.
- **CONTROL ROOM** liga/desliga o Big Knob no master; **TAPE ECHO** liga/desliga o eco de fita.

## Como as máquinas andam juntas
- **Intro:** com a VIRADA DE INTRO do MRDM-12 ligada, a bateria toca um compasso sozinha e a harmonia e o baixo entram no compasso seguinte (no PLAY RIDDIM, com a DAW e no MIDI).
- **Loops de tamanhos diferentes** se repetem cada um no seu tamanho até completar o riddim.
- **Cores no MIDI:** harmonia e baixo com um canal por acorde, bateria com um canal por compasso (MIDI COLORIDO). No Cubase: Key Editor, Cores › Canal.
- **MIDI ao vivo** (DAW tocando): skank no canal 1, baixo no 2, bubble no 3 e a bateria no canal do MRDM-12 (10).
- **Projeto salvo:** a aba aberta, o BPM, a semente, as chaves, os mutes, as edições e os ajustes de cada máquina (inclusive o Big Knob).

## No celular
A [versão para celular](https://dubmastor.github.io/DubMastor-DubStation-/riddim/) tem a aba RIDDIM: harmonia,
máquinas com músicos e MIDI editável (toque para pôr/tirar), JACKPOT, tamanho, mute DRUMS / BASS / RIDDIM,
CONTROL ROOM e TAPE ECHO, e baixa o MIDI de cada máquina ou do riddim inteiro. A bateria usa os mesmos samples DM.

## Para desenvolvedores
- `Source/MrmProcessor.*`: roda as quatro máquinas no mesmo relógio, atrasa harmonia e baixo pela intro (dividindo o bloco na amostra exata), liga harmonia/BPM entre as máquinas, aplica chaves e mutes, passa a mistura pelo Big Knob, salva o estado e escreve o MIDI.
- `Source/MrmMain.*`: aba RIDDIM. `Source/MrmEditor.*`: cabeçalho, abas e CONTROL ROOM. `Source/MrmStyle.h`: desenho DubStation.
- `Source/*Unit.cpp` e `*Names.h`: compilam o código das máquinas direto das pastas delas, renomeando as classes com nomes repetidos.
- `tests/render_test.cpp`: PNG das 5 abas, MIDI do riddim e de cada máquina, harmonia/BPM/semente ligados, edições, mute, intro alinhada, samples DM e estado. Roda no CI antes do build.
- **Regra do repositório:** mudou o MRCM-1240, o MRDM-12, o MRBM-69, o MRDBI-07 ou o Big Knob? O workflow do MRM-0420 roda junto (veja `paths` em `.github/workflows/build-mrm0420.yml`); confira as abas e atualize as notas do MRM-0420.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
# teste offline (Linux): -DMRM_FORMATS=VST3 -DMRM_RENDER_TEST=ON e --target mrm_render
```
