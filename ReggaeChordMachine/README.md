# Mastor Reggae Chord Machine MRCM-1240 1.5

← [Voltar para a página inicial do repositório](../README.md)

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRCM-1240-1.5/MRCM-1240-Windows.zip)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRCM-1240-1.5/MRCM-1240-macOS.zip)**

![Interface do Mastor Reggae Chord Machine MRCM-1240](docs/interface.png)

(Antes se chamava **DubMastor Dub Chords** e, na 1.4, **Mastor Dub Chords Machine MRCM-1240**; a partir da 1.5 é o **Mastor Reggae Chord Machine MRCM-1240**.)

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
| **INVERSÃO** | **AUTO** (padrão) escolhe sozinho a inversão com as notas mais próximas do acorde anterior; **FUND**, **1ª** e **2ª** fixam a inversão. A **seta âmbar** sempre marca a inversão sugerida, mesmo quando você escolhe outra |
| **DURAÇÃO** | Janela de contador: **1/2 compasso**, 1, 2, 3, 4 ou 8 compassos. Clique à direita para aumentar, à esquerda para diminuir |
| **ON** (acordes 2, 3 e 4) | Liga/desliga o acorde no loop. O acorde 1 fica sempre ligado. Ao abrir, só os acordes 1 e 2 vêm ligados (Am e Dm), com **1/2 compasso** cada |

O display âmbar mostra o acorde e a inversão que está tocando (ex.: "D MENOR · 2ª INV"). Os acordes ligados formam o loop, que se repete sozinho
e aparece no visor **LOOP** com o cursor de reprodução. O plugin escolhe a inversão mais próxima do acorde
anterior (condução de vozes), inclusive do último acorde para o primeiro, para o loop fechar sem salto.

## RITMO
| Controle | O que faz |
|---|---|
| **PADRÃO DO SKANK** | Contador clicável: Contratempo · One drop (2 e 4) · Double skank · Steppers |
| **TAMANHO DA NOTA** | Contador clicável: 1/32 (padrão), 1/16, 1/8, 1/4 ou 1/2. A nota nunca encosta no próximo ataque |
| **REGISTRO** | Onde fica a mão esquerda do skank (C3 a C5). A direita fica uma oitava acima, o bubble uma abaixo |
| **VELOCITY / SWING** | Força das notas e atraso das semicolcheias |

## SOM
| Controle | O que faz |
|---|---|
| **PREVIEW** | Botão grande: toca o loop sem a DAW rodando, no **BPM** do knob. Aperte de novo para parar |
| **BUBBLE** | Liga o bubble de órgão Hammond (semicolcheias fora do tempo, uma oitava abaixo do skank). Vem desligado. No som interno, usa o timbre **Roots Percussion** do Roots Keys (com percussão) |
| **SOM INTERNO** | Liga/desliga o som. Desligue quando mandar o MIDI para os seus instrumentos |
| **PIANO M1 / HAMMOND** | Volume de cada timbre (os mesmos do Dub Station Roots Keys, sintetizados, sem samples) |

## MIDI
- **ARRASTE O MIDI:** clique e arraste a área âmbar para a timeline. Vêm duas faixas (skank e bubble), no BPM do projeto, com o nome de cada acorde como marcador.
- **Cada acorde em uma cor:** no arquivo, as notas do acorde 1 vão no canal MIDI 1, as do acorde 2 no canal 2, e assim por diante. Na DAW, peça para colorir as notas por canal: no Cubase, no Key Editor, **Cores › Canal**; no REAPER, *Color notes by channel*; no FL Studio as cores já vêm separadas.
- **EXPORTAR:** salva o `.mid` (padrão: Documentos/Mastor Reggae Chord Machine). **REPETIR** exporta o loop 1, 2, 4 ou 8 vezes.
- **MIDI ao vivo:** o plugin também manda as notas como MIDI para outros instrumentos: canal 1 = skank, canal 3 = bubble.

## Para desenvolvedores
- **Versão:** muda só em `CMakeLists.txt` (`project(DubChords VERSION x.y.z)`). Ela aparece na plaqueta do plugin, e cada push na `main` publica a release **Mastor Reggae Chord Machine MRCM-1240 x.y** (tag `MRCM-1240-x.y`) com as notas de `RELEASE_NOTES.md`. Lembre de atualizar os links `MRCM-1240-x.y` no site e nos READMEs.
- `Source/DubEngine.h`: lógica da harmonia (C++ puro). É aqui que se adiciona padrão novo.
- `Source/PluginProcessor.*`: parâmetros, sincronia com a DAW, saída MIDI, exportação.
- `Source/PluginEditor.*`: interface (LookAndFeel do Big Knob + teclado, botões e contadores clicáveis).
- `Source/DubSynth.h`: som interno; usa o motor do Roots Keys (`../RootsKeys/Source/RootsKeysEngine.h`).
- `tests/engine_test.cpp`: teste do motor (roda no CI antes do build).
- `tests/snapshot.cpp`: gera o PNG da interface sem DAW (`-DDUBCHORDS_SNAPSHOT=ON`); com um nome `.wav`, grava o PREVIEW e um `.mid` exportado.
