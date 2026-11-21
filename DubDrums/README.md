# DubMastor Dub Drums 1.2

← [Voltar para a página inicial do repositório](../README.md)

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubDrums-1.2/DubMastorDubDrums-Windows.zip)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubDrums-1.2/DubMastorDubDrums-macOS.zip)**

![Interface do Dub Drums](docs/interface.png)

Gerador do esqueleto da bateria de reggae. Você escolhe o estilo (Rockers, One Drop ou Boom Bap), quantos
compassos, o baterista de referência e o balanço, e o plugin monta o groove com viradas, sincronizado com
o andamento da DAW. Sai como MIDI (arrastar para a timeline, exportar ou MIDI ao vivo) e tem uma bateria
interna de timbre reggae limpo para ouvir na hora.
Formatos: **VST3** (Windows/macOS), **AU** (macOS) e **Standalone**. É um **instrumento**: no Cubase,
carregue numa *Instrument Track*.

## ESTILO
Contagem em 4 tempos, sempre com a **caixa no 2 e no 4**.

| Botão | Groove |
|---|---|
| **ROCKERS** | Bumbo no 1 e no 3, caixa no 2 e no 4 (o rockers dos Roots Radics / Style Scott) |
| **ONE DROP** | Bumbo junto com a caixa no 2 e no 4; o 1 e o 3 ficam vazios |
| **BOOM BAP** | Hip-hop dos anos 90: bumbo no 1, no "e" do 2 e no "e" do 3, caixa no 2 e no 4, ghost notes de caixa |

**BATIDA PRINCIPAL:** grade com as 8 colcheias do compasso (1 e 2 e 3 e 4 e) para o **BUMBO** e a **CAIXA**.
Ao escolher um estilo, a grade carrega o padrão dele; depois é só clicar para pôr ou tirar (arrastando, marca ou
apaga várias de uma vez). O groove, o preview e o MIDI seguem a grade.

**SOM DA CAIXA:** **CAIXA** (padrão), **ARO** (cross-stick) ou **AMBOS** (aro e caixa juntos).

## LOOP
| Controle | O que faz |
|---|---|
| **COMPASSOS** | Contador de 1 a 4 compassos. Clique à direita para aumentar, à esquerda para diminuir |
| **VIRADA NO FIM** | Virada no último compasso e prato no "1" quando o loop volta (desligada por padrão) |
| **VIRADA DE INTRO** | Um compasso de introdução antes do compasso 1: contagem no hi-hat e a virada do baterista. Toca uma vez só; depois o loop se repete (desligada por padrão) |
| **SEMENTE** | A mesma semente sempre gera o mesmo groove. Arraste para cima/baixo ou use a rodinha |
| **NOVA VARIAÇÃO** (botão vermelho) | Sorteia outra semente |

## BATERISTA
Chave seletora de três posições (knob "cabeça de galinha" com lâmpada e nome em cada posição): clique no nome,
arraste o knob ou use a rodinha. O display mostra o baterista e embaixo vêm a banda e a assinatura.

| | Hi-hat (assinatura) | Sensação | Viradas características |
|---|---|---|---|
| **STYLE SCOTT** (Roots Radics, Volcano) | Acento forte no contratempo, tempos leves, quase sem enfeite, "latido" curto do aberto no "e" do 4 | Reto e em cima do tempo | Estalos de caixa em semicolcheia, estalos sincopados, tons descendo |
| **SANTA DAVIS** (Soul Syndicate, Aggrovators) | **Flying cymbal**: hi-hat abre no contratempo de cada tempo e o pé fecha no tempo seguinte, com semicolcheias leves no meio | Balanço médio, muitas ghost notes | Rufo de fusas em crescendo, caixa e tons em semicolcheia, flams |
| **CARLTON BARRETT** (The Wailers) | **Tercina quebrada**: alguns tempos viram tercina (cheia ou só 1ª e 3ª nota); no one drop o hi-hat **cai do 1** junto com o bumbo; hi-hat aberto na última colcheia puxando o compasso seguinte | Atrás do tempo, balanço puxado para a tercina | Rufo em tercina no 4 com flam, tercinas nos tons, entrada da caixa até o surdo |

## HI-HAT
| Controle | O que faz |
|---|---|
| **FIGURA** | 1/8 (colcheias), 1/16 (semicolcheias), CONTRA (contratempo) ou TERCINA. A assinatura do baterista entra por cima |
| **SWING** | Reto, leve, médio, pesado ou shuffle (tercina) |
| **SWING EM TUDO** | Aplica o balanço também no bumbo, caixa e tons |

O swing escolhido no knob é a base; cada baterista soma o seu jeito (Scott deixa mais reto, Santa um
pouco mais balançado, Carlton bem mais). Ele também atrasa ou adianta cada peça alguns milissegundos.

## DINÂMICA e ALEATORIEDADE
- **DINÂMICA:** suave, média, forte, acentuada, crescendo (sobe ao longo do loop) ou respirando (um compasso forte, outro fraco).
- **ALEATORIEDADE** (bumbo, caixa, hi-hat, tons): notas de passagem, ghost notes, hi-hats abertos, tons entrando no 3, variação das viradas e pequenas variações de força e de tempo em cada peça.

## SOM
| Controle | O que faz |
|---|---|
| **PREVIEW** | Toca o groove sem a DAW rodando, no **BPM** do knob, começando pela intro |
| **SOM INTERNO / VOLUME** | Bateria interna, toca junto com a DAW. Desligue quando mandar o MIDI para a sua bateria |
| **KIT: ANOS 70** | Estúdio jamaicano dos anos 70 / começo dos 80 (Channel One, Volcano): bumbo grande e abafado com batedor de feltro e um pouco de fita (sem a queda de afinação do 808), caixa afinada alta e cantando com estalo de aro, aro seco, hi-hats, tons e prato |
| **KIT: DIGITAL** | Estilo King Jammy / **Sleng Teng** (1985): sons de teclado Casio dos anos 80, curtos e lo-fi (bumbo "bip", caixa de chiado, hi-hat em tique, tons em "blip"), em baixa resolução. Também muda o groove para **tempo de máquina**: grade exata, sem atraso humano e só dois níveis de força, no preview e no MIDI |

## MIDI
- **ARRASTE O MIDI:** clique e arraste a área âmbar para a timeline. O clipe vem com a intro (se ligada) e os compassos do loop, com marcadores "Intro", "Compasso 1", "Compasso 2"...
- **EXPORTAR:** salva o `.mid` (padrão: Documentos/Dub Drums).
- **CANAL:** canal MIDI da saída ao vivo (padrão 10) e do arquivo quando o MIDI COLORIDO está desligado.
- **MIDI COLORIDO:** cada compasso vai num canal MIDI no arquivo (compasso 1 no canal 1 ... compasso 4 no canal 4, intro no canal 5), como no Dub Chords. Na DAW, peça para colorir as notas por canal: no Cubase, no Key Editor, **Cores › Canal**; no REAPER, *Color notes by channel*. Desligue para mandar tudo no CANAL.
- **NOTAS (opção avançada):** abre o mapa de notas: qual nota MIDI cada peça envia (bumbo, aro, caixa, HH fechado, HH aberto, tom 1, tom 2, surdo, prato). Use as setas, arraste ou a rodinha. **PADRÃO GM** volta para o General MIDI (C1 = 36). Vale para a saída ao vivo, o arrastar e o exportar.
- **MIDI ao vivo:** as notas saem como MIDI do plugin acompanhando o play da DAW, para outros instrumentos onde a DAW permite rotear a saída MIDI.

## PADRÃO
A grade mostra o groove inteiro, com uma cor para cada compasso: **intro em azul**, **compasso 1 âmbar**,
**2 verde**, **3 dourado** e **4 vermelho**, e o cursor de reprodução. Na DAW, as cores aparecem com o
**MIDI COLORIDO** ligado (um canal por compasso, colorir por canal).

![Mapa de notas](docs/notas.png)

## Para desenvolvedores
- `Source/PatternGenerator.*`: estilos, bateristas, viradas, intro, swing, dinâmica e aleatoriedade (C++ puro).
- `Source/DrumSynth.h`: bateria interna do preview (sintetizada, sem samples, sem alocação no áudio).
- `Source/PluginProcessor.*`: parâmetros, linha do tempo intro + loop, saída MIDI, mapa de notas, exportação.
- `Source/PluginEditor.*`: interface no visual do Dub Chords.
- `tests/test_generator.cpp`: teste do gerador (roda no CI antes do build).
- `tests/render_preview.cpp`: gera os PNGs da interface e um WAV do preview sem DAW (`-DDD_RENDER_PREVIEW=ON`).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
