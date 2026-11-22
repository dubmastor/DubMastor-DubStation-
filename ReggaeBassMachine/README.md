# Mastor Reggae Bass Machine MRBM-69 2.4 — Gerador de Reggae Bass Lines

← [Voltar para a página inicial do repositório](../README.md)

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRBM-69-2.4/MRBM-69-Windows.zip)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRBM-69-2.4/MRBM-69-macOS.zip)**

![Interface do Reggae Bass Machine](docs/interface.png)

Gerador de **sugestões de linha de baixo** em MIDI para reggae, dub e dancehall, da linha **Dub Station**.
Você escolhe a harmonia e o baixista, ouve com o som do **Dub Bass**, liga a **bateria** e o **teclado do
Roots Keys** para ouvir no contexto, e arrasta o MIDI do baixo para a timeline para editar à vontade.
Formatos: **VST3** (Windows/macOS), **AU** (macOS) e **Standalone**. É um **instrumento**: no Cubase,
carregue numa *Instrument Track*.

## ESTILO (botoeira de 7 teclas)
Teclas intertravadas como as de um rádio valvulado: apertar uma solta a outra.

| Tecla | Baixista | O que o gerador faz |
|---|---|---|
| **FAMILY MAN** | Aston "Family Man" Barrett (Wailers) | One drop: o tempo 1 fica vazio. Linha melódica (terça, quinta, sexta, oitava), colcheias com swing e tercinas "puxadas para trás" no fim da frase |
| **FLABBA** | Errol "Flabba" Holt (Roots Radics) | Rub-a-dub: fundamental no 1, notas longas e graves, quinta e sétima abaixo. Um riff curto repetido de forma hipnótica |
| **DERRICK BARNETT** | Derrick Barnett (Sagittarius Band) | Dancehall ao vivo dos anos 80/90: ataque curto, síncope 3-3-2, oitavas estaladas, corridas de semicolcheias nas viradas |
| **ROBBIE** | Robbie Shakespeare (Sly & Robbie) | Rockers da Channel One: sincopado, firme, quase mecânico, com o balanço do funk e do soul; oitavas e subidas cromáticas até o próximo acorde |
| **LEROY SIBBLES** | Leroy Sibbles (Heptones, Studio One) | Linhas cantadas: notas esticadas, frases que descem pela escala, espaço, e cada nota um pouco atrás do tempo |
| **FULLY FULLWOOD** | George "Fully" Fullwood (Soul Syndicate) | Riff de riddim dos anos 70: gancho melódico com anacruse de semicolcheia e salto de oitava, repetido como assinatura |
| **JAMMYS** | King Jammy, *Sleng Teng* (1985) | O primeiro riddim digital: riff de teclado em semicolcheias, curto e staccato, em volta da tônica (combina com o timbre **SUPER TONE** e a bateria **DIGITAL**) |

| Controle | O que faz |
|---|---|
| **DENSIDADE** | Quantas notas opcionais (ghost notes, repetições) entram na linha |
| **VARIAÇÃO** | Chance de virada no fim da frase e de cada acorde ganhar uma célula diferente |
| **SWING** | Atraso das colcheias e semicolcheias do contratempo (somado ao balanço do estilo; vale também para a bateria e o teclado) |
| **REGISTRO** | Nota mais grave da fundamental (E1 a E2) |

## HARMONIA
Até 4 acordes: **TÔNICA** (teclado mecânico), **TIPO** (maior, menor, 7, m7), **DURAÇÃO** (1/2 a 8 compassos)
e **ON** nos acordes 2 a 4. O plugin abre com **dois acordes ligados, Am e Dm, de 1/2 compasso cada**. As notas de aproximação caem meio tom (ou um tom) da tônica que vem a seguir.

## LINHA DE BAIXO
| Controle | O que faz |
|---|---|
| **IDEIA** | Número da sugestão. A mesma ideia gera sempre a mesma linha: clique nas setas para voltar a uma de que você gostou (Shift = de 10 em 10) |
| **NOVA IDEIA** | Sorteia outra linha com os mesmos acordes e o mesmo estilo |
| **Visor** | Rolo de piano âmbar com as notas, os acordes e o cursor. Notas douradas são de virada |
| **VELOCITY / SUSTENTA** | Força das notas e quanto cada nota segura |

## PREVIEW e TIMBRE (Dub Bass)
| Controle | O que faz |
|---|---|
| **PLAY / STOP** | Toca o loop sem a DAW, no **BPM** do knob. Com a DAW tocando, segue o andamento dela |
| **TIMBRE** | Duas teclas com os timbres do **Dub Bass 1.3**: **ROOTS (SINE)** (Classic Sine) e **SUPER TONE** (o baixo digital da era King Jammy). O preview usa a mesma voz, filtro e maximizer do Dub Bass com os valores iniciais dele: Harm 2 15 %, Harm 3 8 %, Glide 20 ms, ataque 3 ms, decay 600 ms, sustain 85 %, release 180 ms, cutoff 900 Hz, ressonância 0,8, **sem saturação e sem reverb**, EQ plano, sem bass boost, maximizer +3 dB com teto −0,3 dB |
| **GANHO** | Ganho de entrada (−12 a +12 dB) antes da saturação e do maximizer. Duplo clique volta a 0 dB |
| **TONE / DRIVE / VOLUME** | Cutoff (±1,5 oitava; 50 % = 900 Hz, como no Dub Bass), saturação da válvula TUBE do Dub Bass (começa em 0 = desligada, como nele) e volume geral do preview |
| **SOM INTERNO** | Desligue quando mandar o MIDI para o seu próprio baixo |
| **OSCILOSCÓPIO** | Tubo verde com a onda do baixo |

## ACOMPANHAMENTO (só no preview)
| Controle | O que faz |
|---|---|
| **BATERIA** | Liga a bateria. **ROOTS** (rockers da Channel One) e **ONE DROP** usam o kit acústico do Dub Drums; **DIGITAL** (caixa de ritmos de 1985, palmas no 2 e no 4) e **DANCEHALL** (bumbo nos quatro, caixa "dem bow", timbales) usam um kit eletrônico anos 80. Uma virada no último compasso do loop |
| **TECLADO** | Liga o teclado do **Mastor Roots Keys**: skank do **PIANO M1** no contratempo, bubble do **ÓRGÃO B3** ou **AMBOS**, seguindo os acordes da HARMONIA |
| **VOLUME** | Volume de cada um |

O MIDI arrastado/exportado continua sendo **só o baixo**. O nível do baixo fica 5 dB abaixo do Dub Bass, e um limitador suave segura os picos da mistura.

## MIDI
- **ARRASTE O MIDI:** clique e arraste a área âmbar para a timeline. Vem a faixa de baixo no BPM do projeto.
- **EXPORTAR:** salva o `.mid` (padrão: Documentos/Reggae Bass Machine). **REPETIR** exporta o loop 1, 2, 4 ou 8 vezes.
- **MIDI ao vivo:** as notas do baixo também saem como MIDI do plugin (canal 1).

## Para desenvolvedores
- `Source/BassEngine.h`: lógica das linhas (C++ puro). Cada estilo é um "livro" de células de um compasso (grooves e viradas) com graus (R, 3, 5, 7, oitava, quinta grave, aproximação...), swing e microtempo próprios.
- `Source/AccompEngine.h`: padrões da bateria e do teclado e o kit eletrônico (C++ puro).
- `Source/DubBassTimbre.h`: preview com o sistema de timbres do Dub Bass (inclui `ReggaeDubBassInstrument/Source/BassVoice.h` e `Maximizer.h`).
- Bateria acústica: `DubDrumsMachine/Source/DrumSynth.h`. Teclado: `ReggaeChordMachine/Source/DubSynth.h` + `RootsKeys/Source/RootsKeysEngine.h`.
- `Source/PluginProcessor.*`: parâmetros, sincronia com a DAW, PLAY, MIDI, mixagem do preview.
- `Source/PluginEditor.*`: interface (LookAndFeel do Big Knob / Dub Chords, osciloscópio do Mastor Fone, botoeira intertravada).
- `tests/engine_test.cpp`: teste dos 7 estilos e do acompanhamento.
- `tests/snapshot.cpp`: gera o PNG da interface e um WAV do preview, sem DAW (`-DBASSMACHINE_SNAPSHOT=ON -DBASSMACHINE_PLUGIN=OFF`).
