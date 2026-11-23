# Mastor Reggae Bass Machine MRBM-69 — 3.1 · Gerador de Reggae Bass Lines

- **BANCO ÚNICO DE RIDDIMS no visor dos riddims:** 356 músicas e 155 riddims, o mesmo banco do MRCM-1240, do MRDM-12 e do MRM-0420 (`RiddimBank/`), por **ESTILOS**, **RIDDIMS A–Z** ou **MÚSICAS A–Z**, com as luzes **H B D** e a opção **Só completos**
- A música carrega a harmonia conferida e o BPM dela; o baixo é o do riddim do MRBM-69 quando ele existe (baixista, densidade, timbre, bateria), senão o baixista mais próximo de quem gravou. O visor mostra `=BANCO= n/356` e as setas passam de música em música
- Os 160 riddims continuam no mesmo menu e na mesma ordem (os projetos antigos abrem igual)
- Versão para celular 3.1 com o mesmo menu

# Mastor Reggae Bass Machine MRBM-69 — 3.0 · Gerador de Reggae Bass Lines

- **BPM dos riddims corrigido:** os 10 riddims de dancehall que estavam acima de 100 BPM (Speed, Hard Drive, Coolie Dance, Bad Company, Applause, Stepz, Red Bull & Guinness, Power Cut, Wipe Out, Tremor) estavam dobrados e agora ficam na metade (51 e 52)
- BPM interno a partir de 50
- Versão para celular igual

# Mastor Reggae Bass Machine MRBM-69 — 2.9 · Gerador de Reggae Bass Lines

- **Visor de riddims em LCD azul estilo MPC**: moldura cinza, tela azul com letras claras de matriz (=RIDDIM= 081/160, nome, BASS e TEMPO); setas < > dentro da tela
- **Visual de console anos 70**: faixa terracota no alto da placa creme, com o nome em letras creme; cada machine da DubStation tem a sua cor (MRDM-12 oliva, MRCM-1240 mostarda, MRBM-69 terracota, MRM-0420 verde-petróleo)
- **Placa de latão com a sigla MRBM-69** no lugar de "DUB STATION"
- Mesmo visual na versão para navegador

# Mastor Reggae Bass Machine MRBM-69 — 2.8 · Gerador de Reggae Bass Lines

- **RIDDIM CLÁSSICO: 160 presets, 20 por estilo de reggae** — Rocksteady (Studio One e Treasure Isle, 1966-69), Early Reggae e Roots (1969-75), Roots e Rockers (Channel One, Joe Gibbs, Black Ark, 1975-79), Rub a Dub (Roots Radics e Taxi, 1979-84), **Digital King Jammys (1985-89)**, Ragga (Steely & Clevie, Bobby Digital, Sly & Robbie, 1990-95), Dancehall 1995-2001 e Dancehall 2002-07
- Visor âmbar no alto do painel: clique para escolher **por estilo, por baixista ou por década**; as setas passam para o anterior / próximo
- Cada preset traz o riddim, a gravação de origem e o ano, o produtor e quem tocou (ou programou) o baixo — Boris Gardiner, Leroy Sibbles, Family Man, Robbie Shakespeare, Flabba Holt, Noel Davey, Steely & Clevie, Dave Kelly e outros — e carrega a harmonia, o BPM, a bateria, o timbre e o estilo do gerador mais próximo do baixista
- O baixo é gerado pelo MRBM-69 no estilo de quem tocou; não é transcrição das linhas originais. Harmonias marcadas como aproximadas foram tiradas de ouvido: confira o tom no disco e acerte com a TÔNICA
- Lista completa em `docs/RIDDIMS.md`; mesmo banco na versão para navegador

## 2.7

- **Edição só na pentatônica do acorde**: no visor, as linhas fora da pentatônica do acorde de cada trecho ficam escurecidas e desabilitadas; nota nova ou arrastada encaixa na nota permitida mais próxima (pentatônica menor em acordes menor e m7, maior em maior e 7). O nome da escala aparece ao lado do acorde (PENTA / PENTA m)
- Mesma regra na versão para navegador

## 2.6

- **Bateria corrigida**: em todos os estilos (Roots, One Drop, Digital e Dancehall) a caixa cai sempre no 2 e no 4; teste novo no CI confere isso em todo compasso
- **PLAY sem animação**: só a lâmpada, acesa tocando e apagada parado
- **808 do preview com cauda longa**: decay 1,6 s e release 1,2 s (antes 750 ms / 220 ms); a nota seguinte corta a anterior, como num baixo mono
- Mesmas mudanças na versão para navegador

## 2.5

- **MIDI editável** no visor: põe, move, estica e apaga notas (grade 1/16, Shift = 1/32); DESFAZER EDIÇÃO; a linha editada fica salva no projeto e sai no MIDI
- **Timbre no PREVIEW** com os timbres do MRDBI-07: Roots (Sine), Super Tone e 808 (preset MÉDIO)
- **PLAY animado**: halo, lâmpada e barrinhas de VU pulsam no tempo; parado, o botão respira
- **Letreiro de palhetas** (split-flap) com o nome do baixista, que gira ao trocar de estilo
- Nome completo no título: MASTOR REGGAE BASS MACHINE MRBM-69
- **Versão para navegador** (site/bassmachine), com o mesmo gerador do plugin conferido nota a nota no CI
- Compila com o Mastor Reggae Chord Machine MRCM-1240 1.6

## 2.4

- Novo nome: **Mastor Reggae Bass Machine MRBM-69** (antes Mastor Bass Machine MBM-710) no título, na DAW, nos instaladores, na release e no site
- Os códigos do plugin continuam os mesmos: projetos que usavam o nome antigo abrem com o novo
- Release e instaladores agora se chamam `MRBM-69-x.y`, `MRBM-69-Windows.zip` e `MRBM-69-macOS.zip`

## 2.3

- Padrão visual atual da linha Dub Station: a versão fica no título ("MASTOR REGGAE BASS MACHINE 2.3") e a plaqueta dourada só com "DUB STATION"

## 2.2

- Preview com o som do **Dub Bass 1.3** e os valores iniciais dele: Classic Sine (Harm 2 15 %, Harm 3 8 %), envelope 3 / 600 ms / 85 % / 180 ms, cutoff 900 Hz, ressonância 0,8, **sem saturação e sem reverb**
- Seletor de timbre com só duas teclas: **ROOTS (SINE)** e **SUPER TONE** (o baixo digital King Jammy do Dub Bass)
- DRIVE começa em 0 (válvula desligada, como no Dub Bass)
- Padrão de início: **dois acordes ligados, Am e Dm, de 1/2 compasso cada**

## 2.1

- Bateria de acompanhamento com o kit acústico do **Dub Drums 1.1**: bumbo abafado de estúdio jamaicano dos anos 70 (no lugar do bumbo com cara de 808) e caixa afinada alta com estalo de aro

## 2.0

- Botoeira de 7 teclas intertravadas no lugar da chave rotativa: **Family Man**, **Flabba**, **Derrick Barnett** (antes "Sagittarius"), e os novos **Robbie** (Shakespeare), **Leroy Sibbles**, **Fully Fullwood** e **Jammys** (Sleng Teng)
- Novo timbre **SLENG TENG** (teclado digital de 1985) no sistema de timbres do Dub Bass
- **Bateria de acompanhamento** no preview: Roots, One Drop (kit acústico do Dub Drums), Digital e Dancehall (kit eletrônico anos 80)
- **Teclado do Mastor Roots Keys** no preview: skank do piano M1, bubble do órgão B3 ou os dois, seguindo a harmonia
- Plaqueta dourada **DUB STATION 2.0** e o nome "Gerador de Reggae Bass Lines"
- As notas de aproximação agora caem sempre colado na tônica seguinte; limitador suave na saída do preview

## 1.0
- Estilos Family Man, Flabba e Sagittarius; harmonia de 4 acordes; IDEIA numerada; PLAY com os timbres do Dub Bass, GANHO de entrada e osciloscópio; arrastar/exportar MIDI

## Como instalar
- **Windows:** baixe o `…-Windows-Instalador.exe` e siga o instalador (se o Windows avisar, clique em *Mais informações → Executar assim mesmo*).
- **Mac:** baixe o `…-macOS-Instalador.pkg` (VST3 + AU, Intel e Apple Silicon). Se o Mac bloquear, vá em *Ajustes do Sistema → Privacidade e Segurança → Abrir Mesmo Assim*.
- Os zips são para instalação manual. Passo a passo para cada DAW: https://dubmastor.github.io/DubMastor-DubStation-/#instalacao
