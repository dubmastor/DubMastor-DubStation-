## Mastor Reggae Chord Machine MRCM-1240 1.6 🟢🟡⚫

- **Bubble de Hammond com as duas mãos (MIDI melhorado):** em cada tempo, mão esquerda no "e" e no "a" (grave e curta) e mão direita no contratempo, junto do skank, como os organistas de reggae tocam. Os comprimentos do órgão são próprios (curtos e marcados), não dependem mais do TAMANHO DA NOTA do skank
- **Corrigido:** mexer nos controles com o PREVIEW tocando deixava notas do loop antigo soando por cima do novo (no teste, uma nota ficou presa por 29 s). Agora o loop novo é montado fora da thread de áudio, as notas antigas são soltas na troca e os volumes PIANO M1 / HAMMOND mudam com rampa, sem estalo
- **Som interno pelo próprio motor do Roots Keys**, com o timbre **inicial** de piano do Roots Keys (M1 Classic) e o Hammond Roots Percussion. O CI confere a cada build se o timbre continua igual ao do Roots Keys
- **Versão para celular** no site: monte os acordes, ouça e baixe o MIDI direto no navegador (mesmo motor de acordes do plugin, testado nota a nota)
- **Padrão da família:** versão no título ("MASTOR REGGAE CHORD MACHINE 1.6") e placa de latão só com "DUB STATION"
- Validador oficial da Steinberg (VST3) no build do Windows

É um instrumento: no Cubase, carregue numa *Instrument Track*.

No Mac, depois de copiar o `.vst3`:
```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Mastor Reggae Chord Machine MRCM-1240.vst3"
```
