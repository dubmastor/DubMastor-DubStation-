## Mastor Riddim Machine MRM-0420 2.8 — DubStation

- **Aba ÓRGÃO, nova (depois da RIDDIM):** o Mastor Reggae Organ MRO-68 1.3 inteiro dentro do MRM-0420, compilado direto da pasta dele: os grooves dos organistas (Mittoo, Wright, Downie, Gladdy…), grade, drawbars, Leslie animada, os pianos elétricos (Rhodes, Suitcase, Wurlitzer, DX EP), a aba EQ e os **timbres Midnite** (MIDNITE, ABAFADO, CORPO e AO VIVO)
  - toca com a **harmonia e o BPM do riddim** (os 4 acordes ligados ao MRCM-1240 e ao MRBM-69) e entra junto com a harmonia, depois da intro da bateria
  - embaixo do painel: **ÓRGÃO LIGADO**, **MUTE**, **SOLO**, **VOLUME** (começa em −3 dB) e **PAN**
  - no **ARRANJO** o órgão toca nas partes em que a harmonia (RIDDIM) toca
- **MIXER** com o quarto canal, ÓRGÃO, e **CONTROL ROOM** com o mute do órgão
- Tudo na contagem com a caixa no 2 e no 4; o Tape Echo e o saturador do órgão começam desligados, como no MRO-68
- **Projetos antigos** abrem com o órgão desligado (soam igual ao que eram) e na mesma aba
- Versão para celular 2.8: o órgão ganhou os timbres DX EP e Midnite do MRO-68

## Mastor Riddim Machine MRM-0420 2.7 — DubStation

- **Ordem das máquinas sempre BATERIA, BAIXO e RIDDIM**: nas abas (PRINCIPAL, BATERIA, BAIXO, RIDDIM, ARRANJO, MIXER, SHOW, CONTROL ROOM), nas linhas da aba PRINCIPAL e no mixer. A aba da harmonia (Mastor Reggae Keys MRK-80) passa a se chamar RIDDIM
- **Aba ARRANJO**: partes da música (Intro, Verso, Refrão, Ponte, Dub, Outro e partes novas) com quantos compassos e quais máquinas tocam em cada uma; a ordem da música em quadrados (arraste ou ◀ ▶ ×); TOCAR O ARRANJO segue a DAW ou o PLAY RIDDIM, a harmonia recomeça no início de cada parte e a música para no fim (ou repete); o quadrado que está tocando fica aceso, com o compasso; clicar num quadrado com o PLAY RIDDIM pula para ele. O MIDI ao vivo só sai das máquinas que tocam na parte
- **EXPORTAR WAV + MIDI**: a música inteira na ordem do arranjo, renderizada fora do tempo real (WAV 24 bits 44,1 kHz com o rabo do eco) e o MIDI de cada máquina com o nome das partes
- **Aba SHOW**: 10 riddims salvos no projeto (harmonia, BPM, músicos, edições, mixer, efeitos e arranjo); ▶ TOCAR carrega e toca; renomear, apagar, EXPORTAR e IMPORTAR o show (.mrmshow)
- **Aba MIXER**: volume (dB), pan, mute e solo de cada máquina, com medidor; o baixo começa 2 dB mais baixo
- **Sirene com o Mastor Tape Echo MTE-12**: o mesmo motor do plugin (3 cabeças, mola, wow & flutter, saturação e chiado de fita), ligado de fábrica, Mode 7 e tempo livre pelo Repeat Rate; FIRE, HOLD, TAPE ECHO, REPEAT e INTENSITY na aba PRINCIPAL e todos os controles na aba MIXER. A fita para 10 s depois da sirene: nada de chiado no riddim
- Projetos antigos abrem igual (o HOLD da sirene do Big Knob passa para a sirene nova; o arranjo começa desligado)
- Testes novos `mrm_render arranjo`, `show`, `exportar`, `sirene` e `mixer`
- Versão para celular 2.7

### Mastor Riddim Machine MRM-0420 2.6

- Correção no motor de piano compartilhado com o Reggae Keys MRK-80 (fase de timbres em notas muito agudas).

### Mastor Riddim Machine MRM-0420 2.5

- O skank do piano agora toca no **M1 COMP** (o Piano 16 do Korg M1 comprimido), o novo timbre de fábrica do Reggae Keys MRK-80 3.1.

### Mastor Riddim Machine MRM-0420 2.4

- **BANCO ÚNICO DE RIDDIMS nos PRESETS da aba RIDDIM:** 356 músicas em 10 estilos e 155 riddims de A a Z, o mesmo banco do MRCM-1240, do MRDM-12 e do MRBM-69 (`RiddimBank/`). A música vai inteira para as três máquinas: harmonia conferida, BPM, kit, baterista e baixista mais próximos de quem gravou
- Visor azul com o estilo e os anos, a música, os acordes, o BPM (ORIGINAL quando medido), o kit, os músicos e as **luzes H B D** (harmonia, baixo e bateria da gravação original). ◀ ▶ passam por estilo e por música; LISTA e PRESETS abrem **ESTILOS · RIDDIMS A–Z · MÚSICAS A–Z** e a opção **Só completos**
- **TIMBRE do baixo na aba RIDDIM:** ROOTS, **SUPER TONE** (o baixo digital Casio da era Sleng Teng) ou 808, direto na linha do BAIXO, acima dos baixistas. Cada música do banco já escolhe o timbre da época: SUPER TONE no digital e no ragga, 808 no dancehall, ROOTS no resto (dá para trocar na hora)
- **Bateria do Mastor Reggae Drums MRD-12:** a aba e a plaqueta da BATERIA passam a mostrar o MRD-12 (o MRDM-12 virou a aba DUB DRUMS dele); o BAIXO mostra o Mastor Reggae Bass MRB-69 (o MRBM-69 virou a aba BASS MACHINE dele) e a HARMONIA mostra o Mastor Reggae Keys MRK-80 (o MRCM-1240 virou a aba CHORD MACHINE dele); balanço humano calibrado pela medição de bateria de reggae (Wright & Berdahl, Stanford 2005)
- **Créditos no visor:** a linha de baixo do visor azul alterna a cada 4 s entre os músicos e os créditos da gravação (produção, estúdio, baixo e bateria)
- **ALAVANCA** sorteia uma música do banco (só ROCKERS e ONE DROP: caixa no 2 e no 4)
- Projetos antigos abrem igual: o preset antigo aparece como a música certa do banco
- Teste novo `mrm_render banco`: as 321 músicas com harmonia carregam certo nas três máquinas e ficam salvas no projeto
- Versão para celular 2.4 com o mesmo banco

---

## Mastor Riddim Machine MRM-0420 2.3 — DubStation

- **Link de harmonia com o novo Mastor Tune MT-0420**: com os dois abertos na mesma DAW, o MRM publica os 4 acordes, o acorde que está tocando e o BPM, e o Mastor Tune afina a voz pela harmonia do riddim, sem roteamento
- O resto continua igual

É um instrumento: no Cubase, carregue numa *Instrument Track*.

---

## Mastor Riddim Machine MRM-0420 2.2 — DubStation

- **118 GROOVES do MRDM-12 2.7 na aba BATERIA** (antes 21): músicas de Style Scott, Santa Davis, Carlton Barrett, Sly Dunbar e DIGIKAL
- Estilo de reggae **DEMBOW** nas teclas ESTILO DE REGGAE, e hi-hats **FLYING CYMBAL** e **ABRE NO 1**
- Versão para celular igual

É um instrumento: no Cubase, carregue numa *Instrument Track*.

---

## Mastor Riddim Machine MRM-0420 2.1 — DubStation

- **GROOVES do MRDM-12 2.6 na aba BATERIA**: visor azul de MPC no módulo BATERISTA com 21 presets de músicas de cada baterista
  (Style Scott, Santa Davis, Carlton Barrett, Sly Dunbar e DIGIKAL). ◀ ▶ passa pelas músicas do baterista; clique no visor para a lista.
  O groove ajusta a bateria e o BPM do riddim todo; a harmonia e o baixo continuam como estão
- **Sem flam** na bateria: duas notas coladas da mesma peça viram uma só, no tempo certo

É um instrumento: no Cubase, carregue numa *Instrument Track*.

---

## Mastor Riddim Machine MRM-0420 2.0 — DubStation

- **BPM corrigido em todos os presets:** nada passa de 100 BPM. Os riddims que estavam acima estavam dobrados e agora ficam na metade (ex.: Could You Be Loved 52, Temperature 63); os presets de BPM DANCEHALL 2000 (52) e STEPPERS UK (65 e 70) também
- BPM de 50 a 180 (antes 60), em todas as máquinas
- Versão para celular igual

É um instrumento: no Cubase, carregue numa *Instrument Track*.

---

## Mastor Riddim Machine MRM-0420 1.9 — DubStation

- **Visual de console anos 70**: faixa verde-petróleo atrás do nome; dentro das abas, cada máquina mostra a sua cor e a placa de latão com a própria sigla (MRDM-12 oliva, MRCM-1240 mostarda, MRBM-69 terracota)
- Aba BAIXO com o visor de riddims em LCD azul estilo MPC do MRBM-69 2.9
- Mesma faixa na versão para navegador

## Mastor Riddim Machine MRM-0420 1.8 — DubStation

- **PRESETS no visor azul de MPC** (aba RIDDIM): **ESTILO ◀ ▶** e **MÚSICA ◀ ▶** carregam o riddim na hora, como os presets do Dub Battery Reggae; o visor mostra o estilo, a posição (ex.: 05/21), a música, os acordes, o BPM, o kit e os músicos; clique no visor ou em **LISTA** para escolher por estilo
- **Presets separados só por estilo de reggae:** EARLY REGGAE, ROOTS, ROCKERS, LOVERS ROCK, RUB A DUB, DIGITAL, DANCEHALL e NEW ROOTS (o Gregory Isaacs entra no estilo de cada fase), músicas em ordem alfabética
- **BPM original de cada música:** 99 dos 180 riddims agora vêm com o BPM medido da gravação (SongBPM/GetSongBPM/Tunebat, contado com a caixa no 2 e no 4); os outros mostram "~" (andamento típico do estilo, sem medição)
- **PRESET DE BPM no visor azul** (igual ao do Dub Battery Reggae): ◀ ▶ passa pelos 24 BPMs de época e estilo; as 24 teclas continuam embaixo
- **NOVA LINHA** na bateria e no baixo: outra sugestão só daquela máquina (semente própria), sem mexer na outra nem no JACKPOT
- Banco do MRCM-1240 2.2; versão para celular igual (visor azul, NOVA LINHA e preset de BPM)

É um instrumento: no Cubase, carregue numa *Instrument Track*.

---

## Mastor Riddim Machine MRM-0420 1.7 — DubStation

- **Aba BAIXO com os 160 riddims clássicos do MRBM-69 2.8** (20 por estilo de reggae, por baixista e por década, com a era digital do King Jammys): o visor âmbar no alto da aba carrega harmonia, BPM e estilo do baixista, e a harmonia vale para o MRM-0420 inteiro

## Mastor Riddim Machine MRM-0420 1.6 — DubStation

- **Seção BPM na aba RIDDIM:** BPM grande que você **clica e digita** (60 a 180), também arrasta ou gira a roda do mouse; teclas **−1, +1, −5 e +5**; **TAP** (bata no tempo e ele acha o BPM)
- **24 teclas de BPM por época e estilo de reggae**, todas à vista: ROCKSTEADY, EARLY REGGAE, ROOTS, ROCKERS, RUB A DUB, DIGITAL, DANCEHALL 90s e 2000s, LOVERS ROCK, NEW ROOTS, DUB e STEPPERS UK; a tecla escolhida acende
- **PRESETS com subdivisões:** os 180 riddims agora abrem em pasta (estilo e época) > **letras** (A - G...) > música; abre passando o mouse ou clicando, e a pasta e o grupo da música atual ficam marcados
- Banco do MRCM-1240 2.1 (180 riddims, 20 por estilo e época)
- Versão para celular igual: seção BPM com campo para digitar, TAP e as teclas de estilo, e o menu de presets em cascata

É um instrumento: no Cubase, carregue numa *Instrument Track*.

---

## Mastor Riddim Machine MRM-0420 1.0 — DubStation

Nova geração do MRM-0420: as máquinas da DubStation num instrumento só.

- **MRDM-12** (bateria, samples DM), **MRCM-1240** (skank e bubble), **MRBM-69** (baixo, timbres do MRDBI-07) e **Mastor Big Knob** (master), cada um na sua aba, completo
- **Aba RIDDIM** com o resumo de tudo: a harmonia (4 acordes) que vale para o skank e para o baixo, chave liga/desliga e músicos clicáveis de cada máquina, MIDI de cada máquina na tela, editável e arrastável separadamente
- **JACKPOT**: semente nos rolos e o botão vermelho **NOVO RIDDIM** para gerar outra bateria e outro baixo
- **CONTROL ROOM**: Big Knob no master, mute de cada máquina (DRUMS verde, BASS amarelo, RIDDIM vermelho) e chave **TAPE ECHO** (também na aba RIDDIM)
- **Bateria mais limpa** (gerador do MRDM-12): ghost colada, nota duplicada e flam saem, fica só a nota no tempo certo; nota próxima da mesma peça cai para 35% da força
- Um BPM para todas, PLAY RIDDIM, ARRASTE O RIDDIM (4 a 64 compassos) e EXPORTAR; harmonia e baixo entram depois da intro da bateria
- **ALAVANCA do JACKPOT**: riddim aleatório, mas **sempre reggae**: sorteia um dos 180 riddims clássicos e monta ele inteiro (harmonia, BPM, ROCKERS ou ONE DROP e os músicos certos dele), com a caixa só no 2 e no 4 (sem ghost nem flam), hi-hat reto, skank no contratempo e o baixo tocando só notas do acorde (pentatônica, sem cromatismo); hi-hat e semente sorteados, tudo já na tela, é só dar PLAY
- **BPM POR ÉPOCA E ESTILO**: clique em ESTILO ▾ no visor do BPM: ROCKSTEADY 66-68, EARLY REGGAE 68-72, ROOTS 70s, ROCKERS 75-79, RUB A DUB 80-84, DIGITAL 85-89, DANCEHALL 90s e 2000s, LOVERS ROCK, NEW ROOTS, DUB e STEPPERS UK
- **DUB SIREN** na aba RIDDIM, junto do Big Knob: a sirene do Mastor Big Knob com FIRE (segure; o delay continua depois), HOLD, forma da onda (seno, triângulo, quadrada, sobe, desce), PITCH, RATE, DEPTH, DELAY, REPEATS e LEVEL
- **BYPASS** na seção do Big Knob: desliga o Big Knob inteiro (filtro, eco, sirene e maximizer) para comparar
- Versão para celular com o **som do Big Knob igual ao do plugin**: o filtro por degraus do King Tubby (SVF com saturação, 20 Hz a 10 kHz) e a sirene rodam com o mesmo DSP do Mastor Big Knob
- **PRESETS de riddim**: 180 riddims clássicos, 20 por pasta de estilo e época (EARLY REGGAE, ROOTS, ROCKERS, RUB A DUB/VOLCANO, GREGORY ISAACS, DIGITAL/KING JAMMY, LOVERS ROCK, DANCEHALL, NEW ROOTS). Cada um monta o riddim inteiro: harmonia, BPM sugerido, estilo da bateria e os músicos das máquinas mais próximos de quem gravou. Mesmo banco do MRCM-1240 1.9
- Harmonia com o skank sempre no **contratempo** (os outros padrões saíram, como no MRCM-1240 1.7)
- **Uma máquina por aba, completa**; a aba RIDDIM resume tudo e ganhou a seção **BIG KNOB · CONTROL ROOM** (botão grande do filtro HIGH/LOW PASS, CONTROL ROOM, TAPE ECHO e mute de cada máquina)
- **ESTILO DE REGGAE** na bateria da aba RIDDIM: ROCKERS, ONE DROP e BOOM BAP (com o bumbo e a caixa de cada estilo)
- **TAPE ECHO começa desligado** no preset inicial
- Configuração inicial: Lá menor e Ré menor, meio compasso cada
- Alto-falante antigo com grade de metal na aba RIDDIM
- **Instaladores:** `.exe` para Windows (VST3 e standalone) e `.pkg` para Mac (VST3, AU e app, Apple Silicon e Intel): Cubase, FL Studio, Ableton, Reaper, Studio One, Bitwig (VST3), Logic e GarageBand (AU). **Pro Tools:** o AAX já compila e entra no instalador assim que for assinado pela PACE (veja PROTOOLS.md)
- Versão para celular: https://dubmastor.github.io/DubMastor-DubStation-/riddim/

Versões das máquinas neste build: veja a aba RIDDIM (cada máquina mostra a versão dela).

**Instalação:** Windows: rode `MRM-0420-Windows-Instalador.exe`. Mac: abra `MRM-0420-macOS-Instalador.pkg` (se o macOS reclamar, botão direito › Abrir).
Pro Tools: o Pro Tools normal só carrega AAX assinado pela PACE; até lá o AAX sai como artefato de teste para o Pro Tools Developer.
Sem instalador (zip), no Mac: copie o `.vst3` para `~/Library/Audio/Plug-Ins/VST3/` e rode no Terminal
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Mastor Riddim Machine MRM-0420.vst3"`

## Como instalar
- **Windows:** baixe o `…-Windows-Instalador.exe` e siga o instalador (se o Windows avisar, clique em *Mais informações → Executar assim mesmo*).
- **Mac:** baixe o `…-macOS-Instalador.pkg` (VST3 + AU, Intel e Apple Silicon). Se o Mac bloquear, vá em *Ajustes do Sistema → Privacidade e Segurança → Abrir Mesmo Assim*.
- Os zips são para instalação manual. Passo a passo para cada DAW: https://dubmastor.github.io/DubMastor-DubStation-/#instalacao
