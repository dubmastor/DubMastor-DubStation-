## Mastor Riddim Machine MRM-0420 1.0 — DubStation

Nova geração do MRM-0420: as máquinas da DubStation num instrumento só.

- **MRDM-12** (bateria, samples DM), **MRCM-1240** (skank e bubble), **MRBM-69** (baixo, timbres do MRDBI-07) e **Mastor Big Knob** (master), cada um na sua aba, completo
- **Aba RIDDIM** com o resumo de tudo: a harmonia (4 acordes) que vale para o skank e para o baixo, chave liga/desliga e músicos clicáveis de cada máquina, MIDI de cada máquina na tela, editável e arrastável separadamente
- **JACKPOT**: semente nos rolos e o botão vermelho **NOVO RIDDIM** para gerar outra bateria e outro baixo
- **CONTROL ROOM**: Big Knob no master, mute de cada máquina (DRUMS verde, BASS amarelo, RIDDIM vermelho) e chave **TAPE ECHO** (também na aba RIDDIM)
- **Bateria mais limpa** (gerador do MRDM-12): ghost colada, nota duplicada e flam saem, fica só a nota no tempo certo; nota próxima da mesma peça cai para 35% da força
- Um BPM para todas, PLAY RIDDIM, ARRASTE O RIDDIM (4 a 64 compassos) e EXPORTAR; harmonia e baixo entram depois da intro da bateria
- **ALAVANCA do JACKPOT**: riddim aleatório, mas **sempre reggae**: sorteia um dos 90 riddims clássicos e monta ele inteiro (harmonia, BPM, ROCKERS ou ONE DROP e os músicos certos dele), com a caixa só no 2 e no 4 (sem ghost nem flam), hi-hat reto, skank no contratempo e o baixo tocando só notas do acorde (pentatônica, sem cromatismo); hi-hat e semente sorteados, tudo já na tela, é só dar PLAY
- **BPM POR ÉPOCA E ESTILO**: clique em ESTILO ▾ no visor do BPM: ROCKSTEADY 66-68, EARLY REGGAE 68-72, ROOTS 70s, ROCKERS 75-79, RUB A DUB 80-84, DIGITAL 85-89, DANCEHALL 90s e 2000s, LOVERS ROCK, NEW ROOTS, DUB e STEPPERS UK
- **DUB SIREN** na aba RIDDIM, junto do Big Knob: a sirene do Mastor Big Knob com FIRE (segure; o delay continua depois), HOLD, forma da onda (seno, triângulo, quadrada, sobe, desce), PITCH, RATE, DEPTH, DELAY, REPEATS e LEVEL
- **BYPASS** na seção do Big Knob: desliga o Big Knob inteiro (filtro, eco, sirene e maximizer) para comparar
- Versão para celular com o **som do Big Knob igual ao do plugin**: o filtro por degraus do King Tubby (SVF com saturação, 20 Hz a 10 kHz) e a sirene rodam com o mesmo DSP do Mastor Big Knob
- **PRESETS de riddim**: 90 riddims clássicos, 10 por pasta de estilo e época (EARLY REGGAE, ROOTS, ROCKERS, RUB A DUB/VOLCANO, GREGORY ISAACS, DIGITAL/KING JAMMY, LOVERS ROCK, DANCEHALL, NEW ROOTS). Cada um monta o riddim inteiro: harmonia, BPM sugerido, estilo da bateria e os músicos das máquinas mais próximos de quem gravou. Mesmo banco do MRCM-1240 1.9
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
