# Regras do repositório DubStation

## Mastor Riddim Machine MRM-0420 (RiddimMachine/)
O MRM-0420 compila o código destas pastas direto, sem cópia:
- `DubDrums/` — Mastor Reggae Drum Machine MRDM-12 (samples DM em `DubDrums/Samples`)
- `DubChords/` — Mastor Reggae Chord Machine MRCM-1240
- `BassMachine/` — Mastor Reggae Bass Machine MRBM-69 (timbres do MRDBI-07 em `DubBass/Source`)
- `BigKnob/` — Mastor Big Knob (aba CONTROL ROOM)
- `RootsKeys/Source/` — timbres do piano M1 / Hammond

**Sempre que mudar uma dessas máquinas, atualize o MRM-0420 também:**
1. Compile e rode o teste offline do MRM-0420 (`RiddimMachine/tests/render_test.cpp`, alvo `mrm_render`); todos os modos precisam passar.
2. Se a mudança criar uma classe de interface ou um namespace com nome já usado por outra máquina, renomeie no lado do MRM com os headers `RiddimMachine/Source/*Names.h` (não mexa no código da máquina só por isso).
3. Se mudar parâmetros de harmonia (`root`, `qual`, `dur`, `inv`, `on`), BPM, `seed`, músicos (`drummer`, `pattern`, `style`) ou o tamanho do painel, ajuste `MrmProcessor::buildLinks`, a aba RIDDIM (`MrmMain.cpp`) ou o corte do título (`kCrop*` em `MrmEditor.cpp`).
4. Atualize as imagens em `RiddimMachine/docs/`, o `RiddimMachine/RELEASE_NOTES.md` e, se for o caso, a versão para celular `site/riddim/`.

O workflow `.github/workflows/build-mrm0420.yml` já roda quando qualquer uma dessas pastas muda.
