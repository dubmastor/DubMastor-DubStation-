# Mastor Reggae Chord Machine MRCM-1240 1.7

← [Voltar para a página inicial do repositório](../README.md)

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRCM-1240-1.7/MastorReggaeChordMachine-Windows.zip)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MRCM-1240-1.7/MastorReggaeChordMachine-macOS.zip)**

**📱 [Versão para celular](https://dubmastor.github.io/DubMastor-DubStation-/chords/)**: monte os acordes, ouça e baixe o MIDI direto no navegador (mesmo motor de acordes do plugin).

![Interface do Mastor Reggae Chord Machine MRCM-1240](docs/interface.png)

(Antes se chamava **DubMastor Dub Chords** e, na 1.4, **Mastor Dub Chords Machine MRCM-1240**; a partir da 1.5 é o **Mastor Reggae Chord Machine MRCM-1240**.)

Gerador de harmonia para começar a programar um riddim de reggae. Você escolhe até 4 acordes e quanto
tempo cada um dura, e o plugin toca o skank de piano **com as duas mãos** (o acorde em duas oitavas) e o
**bubble de Hammond com as duas mãos**, sincronizados com o andamento da DAW. Sai como MIDI (arrastar para a
timeline, exportar ou MIDI ao vivo) e tem som interno tocado pelo próprio motor do **Roots Keys**: o piano
**Korg M1** com o timbre inicial do Roots Keys e o Hammond **Roots Percussion**.
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
| **PADRÃO DO SKANK** | Sempre **contratempo**: o skank nos "e" de cada tempo, o jeito clássico do reggae (os padrões One drop, Double skank e Steppers saíram na 1.7) |
| **DURAÇÃO DAS NOTAS** | Sempre **1/32 (fusa)**, no skank e nas duas mãos do bubble |
| **REGISTRO** | Onde fica a mão esquerda do skank (C3 a C5). A direita fica uma oitava acima, o bubble uma abaixo |
| **VELOCITY / SWING** | Força das notas e atraso das semicolcheias |

## SOM
| Controle | O que faz |
|---|---|
| **PREVIEW** | Botão grande: toca o loop sem a DAW rodando, no **BPM** do knob. Aperte de novo para parar |
| **BUBBLE** | Liga o bubble de órgão Hammond **com as duas mãos**, como os organistas de reggae tocam: em cada tempo, **mão esquerda** no "e" e no "a" (grave) e **mão direita** no contratempo ("&", com o acorde, junto do skank). A mão direita fica uma oitava abaixo da mão direita do skank e a esquerda uma oitava abaixo dela. Vem desligado. No som interno, usa o timbre **Roots Percussion** do Roots Keys (com percussão) |
| **SOM INTERNO** | Liga/desliga o som. Desligue quando mandar o MIDI para os seus instrumentos |
| **PIANO M1 / HAMMOND** | Volume de cada timbre (com rampa: girar o knob com o preview tocando não estala) |

Mexer em qualquer controle com o **PREVIEW** tocando é seguro: o loop novo é montado fora da thread de áudio e, na troca, as notas do loop antigo são soltas (nada fica soando por cima).

## MIDI
- **ARRASTE O MIDI:** clique e arraste a área âmbar para a timeline. Vêm duas faixas (skank e bubble), no BPM do projeto, com o nome de cada acorde como marcador.
- **Cada acorde em uma cor:** no arquivo, as notas do acorde 1 vão no canal MIDI 1, as do acorde 2 no canal 2, e assim por diante. Na DAW, peça para colorir as notas por canal: no Cubase, no Key Editor, **Cores › Canal**; no REAPER, *Color notes by channel*; no FL Studio as cores já vêm separadas.
- **EXPORTAR:** salva o `.mid` (padrão: Documentos/Mastor Reggae Chord Machine). **REPETIR** exporta o loop 1, 2, 4 ou 8 vezes.
- **MIDI ao vivo:** o plugin também manda as notas como MIDI para outros instrumentos: canal 1 = skank, canal 3 = bubble.

## PRESETS (banco de harmonias clássicas)
O botão verde **PRESETS ▾** (na faixa do LOOP) abre o banco de progressões clássicas do reggae, separadas em pastas por estilo.
Escolher um preset ajusta os acordes, a duração de cada um e quais ficam ligados (inversão em AUTO); depois é só mexer à vontade.
As progressões estão no tom em que costumam aparecer nos riddims citados (dá para transpor no teclado de cada acorde).

| Pasta | Época | Exemplos |
|---|---|---|
| **ROOTS** | anos 70 | Satta (Am-Dm-Am-Em), Marcus Garvey (Am-G), Studio One I-IV, Wailers i-VII-VI, Real Rock (Bm-F#m), Nyabinghi de um acorde |
| **ROCKERS** | anos 70 | Stalag (Cm-Bb), Channel One i-VII, Revolutionaries I-IV-V, dórico (Dm-G), melodica (Am-Em) |
| **RUB A DUB** | anos 80 | Night Nurse (Am-G) e a ponte, Cuss Cuss (Dm-F-G), Roots Radics (Fm-Eb), Volcano (Cm-Fm-Bb) |
| **LOVERS ROCK** | 70 a 90 | I-vi-IV-V, I-IV-V, ii-V-I soul, vi-IV-I-V, IV-V-iii-vi, crossover I-V-vi-IV |
| **DANCEHALL DIGITAL** | 80 e 90 | Digital 85 de um acorde, Ring the Alarm (Stalag 85), ragga i-VII, Dancehall 92, Real Rock 94 |
| **NEW ROOTS 2000** | anos 2000 | Full Up (Am-Dm), one drop i-VI-VII, juggling (Gm-Eb-F), conscious (Bm-G-A), pop reggae I-V-vi-IV |

São 39 presets. O mesmo banco está no **MRM-0420** e nas versões para celular. Fonte única: `tools/harmony_presets.py`
(gera `Source/HarmonyPresets.h` e `site/chords/presets.js`).

## Para desenvolvedores

### Checklist de toda atualização
1. **Versão:** só em `CMakeLists.txt` (`project(DubChords VERSION x.y.z)`). Ela aparece no **título** do plugin ("MASTOR REGGAE CHORD MACHINE x.y"); a placa de latão mostra **só "DUB STATION"**.
2. **Nome:** "Mastor Reggae Chord Machine MRCM-1240" em tudo (plugin, zips `MastorReggaeChordMachine-*`, tag `MRCM-1240-x.y`, release "Mastor Reggae Chord Machine MRCM-1240 x.y").
3. **Timbre do piano:** o som interno usa o timbre **inicial** do Roots Keys (e o órgão Roots Percussion). Confira com `python3 DubChords/tests/check_rootskeys_timbre.py` (o CI roda e falha se estiver desatualizado).
4. **Versão para celular:** atualize `site/chords/` junto (o `engine.js` é o porte do `DubEngine.h`; o número da versão fica no `<title>`, no `<h1>` e em `VERSION`). O CI compara o MIDI do JS com o do C++ (`tests/web_engine_test.js`).
5. **Notas:** `RELEASE_NOTES.md`. Cada push na `main` publica a release com elas.
6. **Links:** quando a release sair, troque os links de download do site e dos READMEs para `MRCM-1240-x.y`.

### Arquivos
- `Source/DubEngine.h`: lógica da harmonia (C++ puro). É aqui que se adiciona padrão novo.
- `Source/PluginProcessor.*`: parâmetros, sincronia com a DAW, saída MIDI, exportação.
- `Source/PluginEditor.*`: interface (LookAndFeel do Big Knob + teclado, botões e contadores clicáveis).
- `Source/RootsKeysSound.h`: som interno; toca pelo `rk::Engine` do Roots Keys (`../RootsKeys/Source/RootsKeysEngine.h`) com os valores de `rkdefaults`.
- `Source/DubSynth.h`: som de teclado antigo, mantido porque o Mastor Reggae Bass Machine MRBM-69 usa.
- `tests/engine_test.cpp`: teste do motor (roda no CI antes do build).
- `tests/snapshot.cpp`: gera o PNG da interface sem DAW (`-DDUBCHORDS_SNAPSHOT=ON`); com um nome `.wav`, grava o PREVIEW e um `.mid` exportado; `ruido` e `mexer` testam som preso ao alternar o SOM INTERNO e ao mexer nos controles com o preview tocando.
- `tests/check_rootskeys_timbre.py`: confere o timbre com o do Roots Keys.
- `tests/dump_loop.cpp` + `tests/web_engine_test.js`: comparam a versão de celular com o plugin, nota a nota.
- `../site/chords/`: versão para celular (`index.html` + `engine.js`).
