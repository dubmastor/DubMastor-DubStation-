# DubMastor Dub Chords 1.1

← [Voltar para a página inicial do repositório](../README.md)

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubChords-1.1/DubMastorDubChords-Windows.zip)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubChords-1.1/DubMastorDubChords-macOS.zip)**

![Interface do Dub Chords](docs/interface.png)

Gerador de harmonia para começar a programar um riddim de reggae. Você escolhe até 4 acordes e quanto
tempo cada um dura, e o plugin toca o skank de piano **com as duas mãos** (o acorde em duas oitavas) e o
**bubble de órgão uma oitava abaixo**, sincronizados com o andamento da DAW. Sai como MIDI (arrastar para a
timeline, exportar ou MIDI ao vivo) e tem som interno com o **piano Korg M1** e o **Hammond** do Roots Keys.
Formatos: **VST3** (Windows/macOS), **AU** (macOS) e **Standalone**. É um **instrumento**: no Cubase,
carregue numa *Instrument Track*.

## ACORDE 1 a 4
| Controle | O que faz |
|---|---|
| **TÔNICA** | Teclado mecânico de uma oitava. Clique na nota: a tecla afunda e a lâmpada acende |
| **MAIOR / MENOR** | Dois botões de pressão intertravados |
| **DURAÇÃO** | Janela de contador: **1/2 compasso**, 1, 2, 3, 4 ou 8 compassos. Clique à direita para aumentar, à esquerda para diminuir |
| **ON** (acordes 2, 3 e 4) | Liga/desliga o acorde no loop. O acorde 1 fica sempre ligado |

O display âmbar mostra o acorde (ex.: "A MENOR"). Os acordes ligados formam o loop, que se repete sozinho
e aparece no visor **LOOP** com o cursor de reprodução. O plugin escolhe a inversão mais próxima do acorde
anterior (condução de vozes), então a harmonia não fica pulando.

## RITMO
| Controle | O que faz |
|---|---|
| **PADRÃO DO SKANK** | Contador clicável: Contratempo · One drop (2 e 4) · Double skank · Steppers |
| **TAMANHO DA NOTA** | Contador clicável: 1/32, 1/16, 1/8, 1/4 ou 1/2. A nota nunca encosta no próximo ataque |
| **REGISTRO** | Onde fica a mão esquerda do skank (C3 a C5). A direita fica uma oitava acima, o bubble uma abaixo |
| **VELOCITY / SWING** | Força das notas e atraso das semicolcheias |

## SOM
| Controle | O que faz |
|---|---|
| **PREVIEW** | Botão grande: toca o loop sem a DAW rodando, no **BPM** do knob. Aperte de novo para parar |
| **BUBBLE** | Liga o bubble de órgão Hammond (semicolcheias fora do tempo, uma oitava abaixo do skank) |
| **BAIXO GUIA** | Tônica no 1 e no "e" do 3, como referência para a linha de baixo |
| **SOM INTERNO** | Liga/desliga o som. Desligue quando mandar o MIDI para os seus instrumentos |
| **PIANO M1 / HAMMOND** | Volume de cada timbre (os mesmos do Dub Station Roots Keys, sintetizados, sem samples) |

## MIDI
- **ARRASTE O MIDI:** clique e arraste a área âmbar para a timeline. Vêm três faixas (skank, bubble e baixo guia), no BPM do projeto, com o nome de cada acorde como marcador.
- **Cada acorde em uma cor:** no arquivo, as notas do acorde 1 vão no canal MIDI 1, as do acorde 2 no canal 2, e assim por diante. Na DAW, peça para colorir as notas por canal: no Cubase, no Key Editor, **Cores › Canal**; no REAPER, *Color notes by channel*; no FL Studio as cores já vêm separadas.
- **EXPORTAR:** salva o `.mid` (padrão: Documentos/Dub Chords). **REPETIR** exporta o loop 1, 2, 4 ou 8 vezes.
- **MIDI ao vivo:** o plugin também manda as notas como MIDI para outros instrumentos: canal 1 = skank, canal 2 = baixo guia, canal 3 = bubble.

## Para desenvolvedores
- `Source/DubEngine.h`: lógica da harmonia (C++ puro). É aqui que se adiciona padrão novo.
- `Source/PluginProcessor.*`: parâmetros, sincronia com a DAW, saída MIDI, exportação.
- `Source/PluginEditor.*`: interface (LookAndFeel do Big Knob + teclado, botões e contadores clicáveis).
- `Source/DubSynth.h`: som interno; usa o motor do Roots Keys (`../RootsKeys/Source/RootsKeysEngine.h`).
- `tests/engine_test.cpp`: teste do motor (roda no CI antes do build).
- `tests/snapshot.cpp`: gera o PNG da interface sem DAW (`-DDUBCHORDS_SNAPSHOT=ON`); com um nome `.wav`, grava o PREVIEW e um `.mid` exportado.
