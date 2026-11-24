# Mastor Reggae Organ MRO-68 1.2

- **Luzes D B R** no banco de riddims, nesta ordem: **D** = bateria, **B** = baixo, **R** = riddim (a harmonia da música; antes era o H). Acesa = a parte veio da gravação original
- O órgão agora mora só aqui: o antigo Reggae Keys virou o **Mastor Reggae Piano MRP-80** (só piano). Toque o skank no MRP-80 e o órgão no MRO-68, cada MIDI na trilha do seu plugin
- **CORES no visor MIDI**: COR POR ACORDE (igual ao MRP-80: cada acorde numa cor e, no MIDI, acorde 1..4 = canal 1..4; a camada fica no contorno da nota) ou COR POR CAMADA (como na 1.0: canal 1 bubble, 2 chop, 3 fills). Projetos antigos abrem por camada. Também na versão para celular
- **Duração da nota: 1/64 (padrão) ou 1/32** — saíram o 1/16 e o 1/8, para o órgão ficar sempre curto e no contratempo (projeto antigo em 1/16 ou 1/8 abre em 1/64)
- **Banco de riddims filtrado:** só as 321 músicas com o riddim (a harmonia) da gravação original (luz R acesa); as setas do visor também só passam por elas
- Versão para celular 1.2

# Mastor Reggae Organ MRO-68 1.1

- **LESLIE nova, animada** (aba ÓRGÃO, no console da mão direita): dois rotores como numa Leslie de verdade, a **corneta** (agudos) e o **tambor** (graves), separados em 800 Hz, cada um com vibrato, tremolo e giro no estéreo. O gabinete na tela gira na velocidade real de cada rotor.
  - **Presets:** SEM LESLIE, 122 CLÁSSICA (a do Hammond B3), ROOTS SUAVE, 147 GRAVE, CHORALE e DUB TREMOLO.
  - **LENTA / RÁPIDA**, **FREIO** (para os rotores) e **PROFUNDIDADE**. A corneta acelera em menos de 1 s e o tambor, pesado, demora 2 a 4 s, como no gabinete de verdade.
  - Vale para o B3 e para os pianos elétricos. Projetos da 1.0 abrem com a Leslie que tinham (lenta → 122 CLÁSSICA, rápida → DUB TREMOLO).
- **INSTRUMENTO** (aba ÓRGÃO, seção SOM): além do **Hammond B3**, o mesmo groove (bubble, chop e fills) pode tocar em 4 pianos elétricos com o motor do Reggae Keys MRK-80: **RHODES** (Rhodes Stage), **SUITC.** (Rhodes Suitcase com tremolo), **WURLI** (Wurlitzer 200A) e **DX EP** (FM EP estilo Yamaha DX7). Drawbars e percussão só valem no B3. Projetos antigos abrem no B3.

# Mastor Reggae Organ MRO-68 1.0

Instrumento novo da linha Dub Station: **o órgão de reggae dos organistas jamaicanos**, pronto no andamento da DAW.
Mesmo Hammond do Mastor Reggae Keys MRK-80, tocado como dois organistas dividindo as partes. Contagem da Dub Station:
**caixa no 2 e no 4** (BPM até 100), mão direita no contratempo, mão esquerda no "e" e no "a", notas de 1/32.

**Downloads**
- Windows: `MRO-68-Windows-Instalador.exe` (VST3 + standalone) ou `MRO-68-Windows.zip`
- Mac: `MRO-68-macOS-Instalador.pkg` (VST3, AU + standalone, Intel e Apple Silicon) ou `MRO-68-macOS.zip`
- Testar sem instalar: https://dubmastor.github.io/DubMastor-DubStation-/web/reggaeorgan/

**Escolha o groove por três caminhos**
- **MÚSICOS** — 8 organistas, cada um com a sua grade, balanço, fills e registros: Theophilus Beckford (ska), Jackie Mittoo (Studio One, o pioneiro do bubble), Ansel Collins (early reggae), Winston Wright (Supersonics), Earl "Wire" Lindo e Tyrone Downie (Wailers), Gladstone Anderson (Roots Radics) e Steely (era digital)
- **ÉPOCAS** — os 10 estilos do banco de riddims, de ROCKSTEADY (1966-68) a NEW ROOTS, cada um com o organista, a batida e o BPM da época
- **RIDDIMS** — as 356 músicas do banco único da Dub Station (o mesmo das machines): harmonia, BPM e batida da gravação, tocadas pelo organista da época

**Ajustes do groove**
- **GRADE** de 16 semicolcheias (1 e & a), mão direita e mão esquerda: vazio, fantasma ou acento em cada uma, com a bateria de referência (caixa no 2 e no 4; bumbo do one drop, rockers ou steppers)
- Atalhos de **FEEL**: SKA "ki-koo-ki", ROCKSTEADY "koo-ki-koo", REGGAE (bubble clássico); **MÃO ESQUERDA NO** "e", no "a" ou nos dois
- **BATIDA** one drop / rockers / steppers, **DURAÇÃO DA NOTA** 1/32, 1/16 ou 1/8, **SWING**, **HUMAN**, **GHOST** e 64 **VARIAÇÕES** com o botão **JACKPOT**
- **FILLS** no estilo de Mittoo (licks), Wright (melodia em oitavas), Downie (acordes) ou Gladdy (fragmentos graves), a cada 2, 4 ou 8 compassos
- Acordes **do plugin** (4 acordes, maior ou menor, de 1/2 a 8 compassos) ou **do teclado** (segure o acorde e o groove segue)

**MIDI**
- Visor do loop inteiro com as três camadas em cores, **editável**: ponha, mova, estique e apague notas; o plugin toca o MIDI editado (e ele fica salvo no projeto) até você apertar **VOLTAR AO GROOVE**
- **Arraste para a DAW** o loop inteiro (3 trilhas: bubble no canal 1, chop no 2, fills no 3) ou só uma camada, x1, x2 ou x4; clique para salvar em Documentos

**Som**
- Dois registros de drawbar (mão esquerda seca e grave, mão direita cheia), Leslie, percussão e eco de fita só na mão direita, drive, click, reverb, master e maximizer
- Teclado que acende com as notas do groove

**Mac:** se o Mac bloquear, libere com
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/"Mastor Reggae Organ MRO-68".*`

## Como instalar
- **Windows:** baixe o `…-Windows-Instalador.exe` e siga o instalador (se o Windows avisar, clique em *Mais informações → Executar assim mesmo*).
- **Mac:** baixe o `…-macOS-Instalador.pkg` (VST3 + AU, Intel e Apple Silicon). Se o Mac bloquear, vá em *Ajustes do Sistema → Privacidade e Segurança → Abrir Mesmo Assim*.
- Os zips são para instalação manual. Passo a passo para cada DAW: https://dubmastor.github.io/DubMastor-DubStation-/instalacao.html
