# DubMastor — Dub Station 🟢🟡⚫

🌐 **Site de downloads:** https://dubmastor.github.io/DubMastor-DubStation-/

Plugins de dub por **DubMastor**, em VST3 (Windows e Mac) e AU (Mac). Feitos e testados para o **Cubase**.

| Plugin | O que é | Mac | Windows | Detalhes |
|---|---|---|---|---|
| **DubMastor Big Knob 3.2** | Estação de dub: filtro big knob estilo King Tubby (com bypass), eco de fita com cabeçotes, presets de EQ e liga/desliga, sirene de dub, VUs e maximizer | [⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest/download/DubMastorBigKnob-macOS.zip) | [⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest/download/DubMastorBigKnob-Windows.zip) | [BigKnob/](BigKnob/) |
| **DubMastor Roots Keys 2.2** | Instrumento: piano estilo Korg M1 e órgão estilo Hammond com Leslie, eco de fita FitaEco, flanger dub, maximizer, VUs e teclado que acende com o MIDI; aba **Dub Chords** para montar o riddim e arrastar o MIDI para a DAW; visual de teclado anos 80 no piano e de madeira e marfim no órgão | [⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/RootsKeys-2.2/DubMastorRootsKeys-macOS.zip) | [⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/RootsKeys-2.2/DubMastorRootsKeys-Windows.zip) | [RootsKeys/](RootsKeys/) |
| **DubMastor Dub Chords 1.1** | Instrumento / gerador MIDI: escolha até 4 acordes (teclado mecânico, maior/menor, contador de compassos) e ele toca o skank de piano M1 com as duas mãos e o bubble de Hammond no andamento da DAW; arraste o MIDI colorido por acorde para a timeline | [⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubChords-1.1/DubMastorDubChords-macOS.zip) | [⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubChords-1.1/DubMastorDubChords-Windows.zip) | [DubChords/](DubChords/) |
| **Dub Bass - Dub Station 1.1** | Instrumento: baixo de reggae em senoide com harmônicos ou 808, saturação com válvula animada, EQ gráfico, reverb, bass boost em 60 Hz e maximizer com VU digital estilo Ozone, no visual do Dub Chords | [⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubBass-1.1/DubBass-macOS.zip) | [⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/DubBass-1.1/DubBass-Windows.zip) | [DubBass/](DubBass/) |
| **Mastor Fone 1.6** | Efeito de voz: rádio AM, telefone antigo e megafone, com o eco de fita e o maximizer do Big Knob, no visual Dub Station | [⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest/download/MastorFone-macOS.zip) | [⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest/download/MastorFone-Windows.zip) | [MastorFone/](MastorFone/) |
| **Dub Station 2.6 (FitaEco TE-12)** | Eco de fita de 3 cabeças + reverb de mola, com EQ gráfico | [⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest/download/DubStation26-macOS.zip) | [⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest/download/DubStation26-Windows.zip) | [DubStation21/](DubStation21/) |

Todos os downloads ficam na página de **[Releases](https://github.com/dubmastor/DubMastor-DubStation-/releases/latest)**. Não precisa de conta no GitHub para baixar.

## Instalação no Cubase

**Mac**
1. Baixe o zip do plugin. O Mac descompacta sozinho.
2. Feche o Cubase.
3. No Finder, aperte **Cmd+Shift+G**, cole `~/Library/Audio/Plug-Ins/VST3/` e copie para lá o arquivo **`.vst3`** do plugin.
4. No **Terminal**, libere o plugin (troque pelo nome do arquivo que você copiou):
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"DubMastor Big Knob.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"FitaEco TE-12.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"DubMastor Roots Keys.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Mastor Fone.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"DubMastor Dub Chords.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Dub Bass - Dub Station.vst3"
   ```
5. Abra o Cubase.

**Windows**
1. Descompacte o zip (botão direito → *Extrair tudo*).
2. Copie a pasta **`.vst3`** do plugin para `C:\Program Files\Common Files\VST3\`.
3. Abra o Cubase.

**Se o plugin não aparecer:** em **Studio → VST Plug-in Manager**, faça um novo scan. Se ele estiver na lista de bloqueados, clique em **Reactivate**.

O formato é **VST3** (`.vst3`). O formato antigo `.vst` (VST2) não é suportado pelo Cubase atual em Mac com Apple Silicon.

## Estrutura do repositório

```
BigKnob/                         DubMastor Big Knob (C++ / JUCE 8)
DubStation21/                    Dub Station 2.6 — FitaEco TE-12 (C++ / JUCE 8)
RootsKeys/                       DubMastor Roots Keys — piano M1 + órgão Hammond + Dub Chords (C++ / JUCE 8)
MastorFone/                      Mastor Fone — rádio AM, telefone e megafone para voz (C++ / JUCE 8)
DubChords/                       DubMastor Dub Chords — gerador de skank/harmonia para riddim (C++ / JUCE 8)
DubBass/                         Dub Bass - Dub Station — baixo de reggae senoide / 808 (C++ / JUCE 8)
.github/workflows/
  build-bigknob.yml              compila, valida e publica o Big Knob
  build-dubstation21.yml         compila, valida e publica o Dub Station 2.6
  build-rootskeys.yml            compila, valida e publica o DubMastor Roots Keys
  build-mastorfone.yml           compila, valida e publica o Mastor Fone
  build-dubchords.yml            compila, valida e publica o Dub Chords
  build-dubbass.yml              testa, compila, valida e publica o Dub Bass (tag DubBass-x.y)
```

Cada plugin é compilado e validado sozinho quando a pasta dele muda. A validação usa o **pluginval** (carrega o plugin, processa áudio, testa estado e interface) e, no Mac, o **auval** da Apple. Quando uma release é publicada, os builds anexam seus instaladores a ela.
