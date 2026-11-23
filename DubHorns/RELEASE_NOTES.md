## Mastor Dub Horns MDH-70 1.3 🟢🟡⚫

**Timbre de sessão jamaicana dos anos 70 e 80, visor de presets sempre à vista e o plugin abrindo no dub.**

- **Dinâmica de metal de verdade:** tocando fraco (pp) o som fica escuro; tocando forte (ff) os harmônicos agudos crescem muito, como medido em trompete e trombone acústicos. O teclado passa a mudar o *timbre*, não só o volume.
- **Saxes com todos os harmônicos:** antes eles usavam onda quadrada (só harmônicos ímpares), o que dava um som oco de clarinete. Agora têm a série completa, como o tubo cônico do saxofone.
- **Zumbido do lábio e da palheta** no ritmo de cada ciclo da nota, **3º formante** (o corpo do instrumento) e a **campana**, que irradia pouco grave.
- **Naipe de sessão:** um músico lidera e entra primeiro, os outros chegam alguns milissegundos depois, cada um com a sua afinação e a sua força, como numa gravação ao vivo no estúdio.
- **Fita e sala:** "head bump" da fita no grave (cresce com a Saturação) e a Sala com cara de plate de estúdio.
- **Visor azul de presets sempre à vista**, entre o cabeçalho e as abas, como na Riddim Machine. A lista começa pelo **DUB** e o plugin **abre no "Tubby · naipe no eco"**, com o Tape Echo na frente (1/8 pontuada, intensidade 62%).
- A janela ficou um pouco mais alta (1200 × 880) para caber o visor.

### O que veio na 1.2

**Metais mais naturais e a aba MÚSICOS.**

- **Fim de nota de músico de verdade:** o *fall* e o *doit* seguram a nota um instante e depois caem (ou sobem) acelerando, com o volume sumindo junto. A parte de baixo do fall quase não se ouve, como num naipe real. Cada nota cai um pouco diferente; o sax cai "pelas chaves" e o trombone desliza liso pela vara. Acabou a sirene robótica no final.
- **Fim normal:** o lábio relaxa e a afinação cede só uns poucos cents enquanto o som some; o brilho escurece junto com o volume.
- **Vibrato humano:** começa depois do ataque, varia de velocidade e de fundura, mexe também no volume e no brilho e **para** quando a nota é solta.
- **Afinação viva** (cada músico oscila uns cents devagar), **golpe de língua** no ataque e scoop que muda a cada nota.
- **Knob Rouco:** a aspereza da palheta, o tenor estilo Dean Fraser. **Knob Sala:** a sala de gravação do estúdio.
- **Aba MÚSICOS** no padrão da Riddim Machine: visor azul de MPC, ESTILO ◀ ▶, MÚSICO ◀ ▶ e LISTA por estilo. São 24 presets em 7 estilos (Ska/Skatalites, Rocksteady, Studio One, Roots/Channel One, Wailers/Island, anos 80 e Dub) com Dean Fraser, Vin Gordon "Don D Junior", Tommy McCook, Bobby Ellis, Roland Alphonso, Don Drummond, Johnny "Dizzy" Moore, Cedric "Im" Brooks, Herman Marquis, Rico Rodriguez, Glen DaCosta, David Madden, Nambo Robinson e os naipes clássicos. Cada preset carrega naipe, timbre, rouco, sala, eco e EQ; o tom e a escala continuam os seus.

Os projetos feitos na 1.1 continuam abrindo. Eles ganham a sala do estúdio em 25%; para o som antigo, ponha Sala em 0%.

Também na versão do navegador: https://dubmastor.github.io/DubMastor-DubStation-/web/dubhorns/

### O que veio na 1.1

**Nome novo:** no Cubase ele agora aparece como **Mastor Dub Horns MDH-70**. Os projetos antigos continuam abrindo o plugin; apague o `Mastor Dub Horns.vst3` antigo da pasta VST3 para não aparecer duplicado.

Naipe de metais anos 70 para sessão de dub. Você escolhe os instrumentos e cada nota vira um acorde de metais, com o som de estúdio jamaicano.

- **Naipe:** trompete, flugelhorn, sax alto, sax tenor, trombone e sax barítono. Cada um tem volume, pan, oitava e harmonia (uníssono, 3ª, 4ª, 5ª, 6ª) dentro do tom e da escala escolhidos
- **Timbre '70s:** 6 presets de estúdio com brilho, sopro, scoop, vibrato atrasado, saturação de fita, banda de console, chiado e humanizar
- **Mastor Tape Echo:** 3 cabeças, sync com o andamento da DAW, intensidade, agudos e graves, wow & flutter, saturação, **Dub Throw** (o pedal de sustain do teclado também faz) e auto-oscilação
- **EQ gráfico de 10 bandas** com curva na tela e **Filtro Tubby** (passa-altas ressonante)
- **Frases MIDI:** 8 ataques curtos estilo Dean Fraser e 6 introduções. As notas são suas: escolha na lista ou toque no teclado. A frase segue o transporte da DAW e dá para **arrastar o MIDI** para a timeline, com uma trilha por instrumento já harmonizada
- Teclado na tela e entrada MIDI

**Correções em relação à 1.0 (versão do navegador):** a realimentação do Tape Echo não dispara mais sozinha; a 5ª abaixo agora é mesmo uma 5ª; a saturação não aumenta mais o volume.

É um instrumento: no Cubase, carregue numa *Instrument Track*.

No Mac, depois de copiar o `.vst3`:
```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Mastor Dub Horns.vst3"
```

Também toca no navegador: https://dubmastor.github.io/DubMastor-DubStation-/horns/

## Como instalar
- **Windows:** baixe o `…-Windows-Instalador.exe` e siga o instalador (se o Windows avisar, clique em *Mais informações → Executar assim mesmo*).
- **Mac:** baixe o `…-macOS-Instalador.pkg` (VST3 + AU, Intel e Apple Silicon). Se o Mac bloquear, vá em *Ajustes do Sistema → Privacidade e Segurança → Abrir Mesmo Assim*.
- Os zips são para instalação manual. Passo a passo para cada DAW: https://dubmastor.github.io/DubMastor-DubStation-/#instalacao
