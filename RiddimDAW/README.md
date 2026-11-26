# Riddim Machine DAW RMD-0420 1.0

Projeto separado do VST **Mastor Riddim Machine MRM-0420**: o MRM-0420 continua sendo o plugin que abre dentro da DAW;
a Riddim Machine DAW é o estúdio no navegador, com a música inteira numa timeline, gravação de voz, mixer e exportação.

Em teste: por enquanto só a versão no site, em https://dubmastor.github.io/DubMastor-DubStation-/daw/

## O que tem na 1.0

- Aba ESTÚDIO: timeline com as partes do Arranjo, os acordes de cada compasso e todas as machines (ligar ou desligar cada machine em cada parte)
- Faixas de áudio: gravar o microfone (contagem de um compasso, metrônomo com o 2 e o 4 acentuados, ajuste de latência) ou importar arquivos
- Clipes: arrastar na grade, cortar no cursor, duplicar, volume e trocar de faixa
- Mixer das faixas: volume, pan, mute, solo e envio para o Tape Echo do master
- Exportar a música (WAV e MP3) e os stems (WAV, todos começando no compasso 1)
- Projeto salvo sozinho no aparelho (IndexedDB) e em arquivo `.rmd`

## Como é montada

`site/daw/index.html` é gerado: a versão web do MRM-0420 (`site/riddim/index.html`, com todas as machines, o Arranjo e o mixer)
mais a aba ESTÚDIO de `web/daw.js` e `web/daw.css`. Depois de mudar um deles, ou quando a versão web do MRM-0420 mudar, rode:

```
python3 RiddimDAW/build_web.py
```

O MP3 usa o `site/riddim/lame.min.js` (LAME, LGPL).
