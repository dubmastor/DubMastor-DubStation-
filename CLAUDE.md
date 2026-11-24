# Regras do repositório DubMastor-DubStation-

Plugins VST3/AU de dub e reggae (coleção **Dub Station**, fabricante **DubMastor**), em C++ com JUCE 8.0.4,
publicados em releases do GitHub e no site https://dubmastor.github.io/DubMastor-DubStation-/ (pasta `site/`).
Escreva tudo para o usuário em português do Brasil.

## Sempre que um plugin for para o GitHub e o site

Toda mudança num plugin segue esta lista, na mesma pull request:

1. **Versão.** Subir a versão no `CMakeLists.txt` (`project(... VERSION x.y.0)`; no Dub Horns, em `Source/Version.h`).
   Toda alteração sobe o número (1.0 → 1.1 → 1.2).
2. **Mesma versão em todos os lugares:**
   - `RELEASE_NOTES.md` do plugin começa com `Nome SIGLA x.y`
   - card do plugin na página da categoria (`site/efeitos.html`, `site/instrumentos.html` ou `site/machines.html`, campo `categoria` do `plugins.json`): título `<h3>Nome SIGLA <small>x.y</small></h3>` e os links
     `releases/download/SIGLA-x.y/SIGLA-Windows-Instalador.exe`, `SIGLA-macOS-Instalador.pkg`, `SIGLA-Windows.zip`, `SIGLA-macOS.zip`
   - `README.md` cita `Nome SIGLA x.y`
   - versão para celular (pasta do campo `web` no `plugins.json`) com `const VERSION = 'x.y'` ou a versão no `<title>`
3. **Versão para celular / navegador.** Todo plugin tem uma versão web em `site/<pasta>/index.html` (plugin novo: `site/web/<pasta em minúsculas>/`),
   no estilo da do Dub Horns (`site/web/dubhorns/`): um arquivo só, Web Audio, botão para liberar o áudio,
   teclado na tela que funciona no toque, layout de celular, link "← Dub Station" e a mesma versão do plugin.
   No card do site, o botão verde "▶ Tocar no celular / navegador". Plugin novo sem versão web não passa na checagem.
   Os plugins antigos marcados como `"web": "pendente"` no `plugins.json` devem ganhar a sua quando forem atualizados.
   Plugin com `"oculto": true` no `plugins.json` fica fora do site por enquanto (sem card); a checagem só avisa e falha se o card continuar lá.
   Plugin com `"so_celular": true` está **em teste**: o card fica no site só com o botão da versão para celular, sem download
   (a checagem falha se houver link de release no card) e o README não leva link de download. Regra do dono: todo VST novo
   começa assim; só depois de aprovado ganha os downloads (tirar `so_celular` e devolver os links).
4. **Registro.** Plugin novo entra no `plugins.json` (sigla, pasta, nome, tipo, web) e ganha um workflow
   `build-<pasta em minúsculas>.yml` copiado de um existente (com instaladores, pluginval e auval).
5. **Checagem.** Rodar `python3 .github/checagem/checar.py` antes de enviar. O workflow `checagem.yml`
   roda em todo push e PR e falha se nome, versão, links ou versão para celular estiverem fora do padrão.
6. **Release.** Depois do merge na `main`, o workflow do plugin publica a release `SIGLA-x.y` com os
   instaladores e os zips. Conferir que a release existe antes de dizer que o site está atualizado.
   Se o Actions não puder criar a release (erro 403), o deploy do site (`pages.yml` + `.github/scripts/links-site.py`)
   serve os instaladores do último build em `site/downloads/` e esconde os zips que faltarem — o link nunca fica quebrado.

## Padrão de nomes

- Nome no Cubase (`PRODUCT_NAME`): `Mastor <Nome> <SIGLA>`. Pasta: o nome em CamelCase, sem "Mastor" e sem sigla.
- Release `SIGLA-x.y`; arquivos `SIGLA-Windows-Instalador.exe`, `SIGLA-macOS-Instalador.pkg`, `SIGLA-Windows.zip`, `SIGLA-macOS.zip`.
- Imagem do card: `site/img/<pasta em minúsculas>.jpg`.
- **Nunca mudar** `PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE` (`Dblb`), `BUNDLE_ID` nem o ID de estado do APVTS:
  é isso que mantém os projetos antigos abrindo. Ao renomear, acrescentar o nome antigo em
  `.github/installer/nomes-antigos.txt` (os instaladores apagam a cópia antiga).

## Compatibilidade com as DAWs

- Formatos: VST3 (Windows e Mac), AU (Mac, para Logic e GarageBand), AAX (Pro Tools, com o SDK do JUCE 8; só abre no Pro Tools normal depois da assinatura PACE, veja `PROTOOLS.md`) e standalone.
- Windows: runtime do C++ embutido (`CMAKE_MSVC_RUNTIME_LIBRARY` MultiThreaded) — o plugin não pode depender
  do Visual C++ Redistributable. A release confere isso.
- Mac: binário universal (arm64 + x86_64), macOS 10.13+, assinatura ad-hoc; o `.pkg` instala em `/Library/Audio/Plug-Ins`.
- Todo build passa no pluginval (Windows e Mac) e no auval (Mac) antes de virar release.
- Instruções por DAW ficam na seção "Instalação" do site; manter atualizadas quando mudar algo na instalação.

## Mastor Riddim Machine MRM-0420 (RiddimMachine/)
O MRM-0420 compila o código destas pastas direto, sem cópia:
- `ReggaeDrumMachine/` — Mastor Reggae Drum Machine MRDM-12 (samples DM em `ReggaeDrumMachine/Samples`)
- `ReggaeChordMachine/` — Mastor Reggae Chord Machine MRCM-1240
- `ReggaeBassMachine/` — Mastor Reggae Bass Machine MRBM-69 (timbres do MRDBI-07 em `ReggaeBass/Source`)
- `BigKnob/` — Mastor Big Knob (aba CONTROL ROOM)
- `ReggaeKeys/Source/` — timbres do piano M1 / Hammond

**Sempre que mudar uma dessas máquinas, atualize o MRM-0420 também:**
1. Compile e rode o teste offline do MRM-0420 (`RiddimMachine/tests/render_test.cpp`, alvo `mrm_render`); todos os modos precisam passar.
2. Se a mudança criar uma classe de interface ou um namespace com nome já usado por outra máquina, renomeie no lado do MRM com os headers `RiddimMachine/Source/*Names.h` (não mexa no código da máquina só por isso).
3. Se mudar parâmetros de harmonia (`root`, `qual`, `dur`, `inv`, `on`), BPM, `seed`, músicos (`drummer`, `pattern`, `style`) ou o tamanho do painel, ajuste `MrmProcessor::buildLinks`, a aba RIDDIM (`MrmMain.cpp`) ou o corte do título (`kCrop*` em `MrmEditor.cpp`).
4. Atualize as imagens em `RiddimMachine/docs/`, o `RiddimMachine/RELEASE_NOTES.md` e, se for o caso, a versão para celular `site/riddim/`.

O workflow `.github/workflows/build-riddimmachine.yml` já roda quando qualquer uma dessas pastas muda.

## Instalador completo (Dub Station, todos os plugins)
- Workflow `.github/workflows/build-pacote.yml`, script `.github/scripts/pacote.py`. Lê a lista do `plugins.json`: plugin novo entra sozinho.
- Release `DubStation-<versão>` com `DubStation-Windows-Instalador.exe` e `DubStation-macOS-Instalador.pkg` (nunca `latest`).
- `"pausado": true` no `pacote` do `plugins.json`: o workflow não compila nem publica (só manual), o site fica sem o bloco do
  instalador completo e a checagem exige que não haja link. Para liberar: tirar `pausado`, devolver o bloco `completo` em
  `site/index.html` e os links no README.
- **Sempre que a versão de um plugin subir**, atualize `plugins.json` → `pacote` → `plugins` com a nova versão, suba `pacote` → `versao` e troque `DubStation-x.y` nos links do site e do README. A checagem falha se esquecer.
- Pro Tools (AAX): todo CMakeLists inclui `.github/cmake/aax.cmake` e põe `${DM_AAX}` no fim de `FORMATS`. Plugin novo também. O AAX é compilado sempre (SDK do JUCE 8) e só entra no instalador público quando assinado pela PACE (passo a passo em `PROTOOLS.md`).


## Instrumentos com machine embutida (unificação de nov/2026)
- **Mastor Reggae Drums MRD-12** (`ReggaeDrums/`, antigo Dub Battery Reggae DBR) tem o MRDM-12 inteiro na aba DUB DRUMS.
- **Mastor Reggae Bass MRB-69** (`ReggaeBass/`, antigo Dub Bass MRDBI-07) tem o MRBM-69 inteiro na aba BASS MACHINE
  (`Source/MachineProcessorUnit.cpp` e `MachineEditorUnit.cpp`; nomes repetidos trocados em `MachineNames.h`).
- **Mastor Reggae Keys MRK-80** (`ReggaeKeys/`, antigo Roots Keys) tem o MRCM-1240 inteiro na aba CHORD MACHINE (só maior e menor;
  `Source/MachineProcessorUnit.cpp` e `MachineEditorUnit.cpp`). Projetos da antiga aba DUB CHORDS migram em `migrateOldDubChords`.
  Versão para celular em `site/keys/` (usa `site/chords/engine.js`, que continua sendo o motor web da machine).
  Desde a 3.1 o MRK-80 é **só piano** (o órgão foi para o MRO-68): `mode` fica só para projetos antigos, skank e bubble tocam no piano,
  e só ele mostra o padrão do skank (parâmetro `pattern` do MRCM: CONTRATEMPO ou BANG BANG; no MRCM-1240 e no MRM-0420 fica 0).
- As machines separadas (MRDM-12, MRBM-69, MRCM-1240) ficam como **legado** no `plugins.json` (`"legado": "<instrumento>"`):
  saem do site e do pacote, a última release fica congelada (o workflow só publica por tag manual), mas a pasta continua
  porque o instrumento e o MRM-0420 compilam o código dela. Mudou uma machine? Teste o instrumento dela e o MRM-0420.
- Ao renomear, o alvo do CMake e os IDs internos (`DubBatteryReggae`, `DubBatteryReggaeState`, `DubBassState`, `RootsKeys`, `PARAMS`) ficam:
  são eles que mantêm os projetos antigos abrindo.
- Toda alteração de bateria vale para o MRD-12, o MRM-0420 e o código do MRDM-12.

## GUITAR SKANK do MRK-80 (ReggaeKeys/Source/GuitarSkank.h)
O botão GUITAR SKANK do Mastor Reggae Keys MRK-80 toca a guitarra do skank junto com o piano usando o **motor do
Mastor Reggae Guitar MRG-68** direto, sem cópia (`ReggaeGuitar/Source/GuitarEngine.h`: `mrg::StringVoice` e `mrg::Chain`).
Mudou o motor do MRG-68? Rode também o teste do MRK-80 (`ReggaeKeys/tests/render_demo.cpp`, item GTR SKANK) e o do MRO-68.
O botão sempre começa desligado. Só o MRK-80 aloca as cordas (`engine.enableGuitar()` antes do `prepare`); o MRO-68, o MRM-0420
e as machines que usam o `RootsKeysEngine.h` não gastam memória com a guitarra. Os workflows desses plugins rodam quando
`ReggaeGuitar/Source/**` muda.

## Mastor Reggae Organ MRO-68 (ReggaeOrgan/)
Usa direto, sem cópia, o órgão (`RootsKeysEngine.h`, `TapeEchoDSP.h`), o visual (`VintageLookAndFeel.h`, `SkinTextures.h`, `VUMeter.h`)
e as fontes de `ReggaeKeys/`, e o banco de riddims de `RiddimBank/Source`. Mudou o motor do órgão no MRK-80 ou o banco?
Rode também o teste do MRO-68 (`ReggaeOrgan/tests/render_test.cpp`). Tudo nele é na contagem com a caixa no 2 e no 4.
O motor de grooves (`ReggaeOrgan/Source/GrooveEngine.h`) tem uma cópia em JavaScript na versão para celular (`site/web/reggaeorgan/`):
ao mudar um, mude o outro (mesmo gerador aleatório: o mesmo compasso tem que sair igual).

## Mastor Reggae Clavinet MRCL-71 (ReggaeClavinet/)
O som (`Source/ClavEngine.h`, corda por modelo físico) e o gerador das linhas (`Source/LineEngine.h`) têm uma cópia em JavaScript
na versão para celular (`site/web/reggaeclavinet/`, funções `clavBuf` e `generate`): ao mudar um, mude o outro.
Tudo na contagem com a caixa no 2 e no 4; nomes das notas como no Cubase (C3 = dó central). Timbre inicial: M1. Tape Echo começa desligado.
A machine não escuta outros plugins: segue os acordes do plugin (banco de riddims) ou o MIDI que chega na trilha (o do MRP-80 ou do MRM-0420).
Testes: `tests/render_test.cpp` (sem JUCE) e `tests/snapshot.cpp` (plugin inteiro + imagens de `docs/`).

## Mastor Intro Fills MIF (IntroFills/)
As viradas, os grooves e o feel (`Source/FillEngine.h`) e a dinâmica do kit (`Source/FillSampler.h`) têm uma cópia em JavaScript
na versão para celular (`site/web/introfills/`, funções `buildSequence`, `perform` e `hit`): ao mudar um, mude o outro
(mesmo gerador aleatório Park-Miller: a mesma virada tem que sair igual). Tudo na contagem com a caixa no 2 e no 4; o passo 16
da virada é o 1 do groove (a chegada). MIDI nas notas do MRD-12, canal 10. Tape Echo e saturador começam desligados.
Testes: `tests/render_test.cpp` (sem JUCE) e `tests/snapshot.cpp` (plugin inteiro + imagens de `docs/`).

