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
   - card do plugin em `site/index.html`: título `<h3>Nome SIGLA <small>x.y</small></h3>` e os links
     `releases/download/SIGLA-x.y/SIGLA-Windows-Instalador.exe`, `SIGLA-macOS-Instalador.pkg`, `SIGLA-Windows.zip`, `SIGLA-macOS.zip`
   - `README.md` cita `Nome SIGLA x.y`
   - versão para celular (`site/web/<nome>/index.html`) com `const VERSION = 'x.y'`
3. **Versão para celular / navegador.** Todo plugin tem uma versão web em `site/web/<pasta em minúsculas>/index.html`,
   no estilo da do Dub Horns (`site/web/dubhorns/`): um arquivo só, Web Audio, botão para liberar o áudio,
   teclado na tela que funciona no toque, layout de celular, link "← Dub Station" e a mesma versão do plugin.
   No card do site, o botão verde "▶ Tocar no celular / navegador". Plugin novo sem versão web não passa na checagem.
   Os plugins antigos marcados como `"web": "pendente"` no `plugins.json` devem ganhar a sua quando forem atualizados.
4. **Registro.** Plugin novo entra no `plugins.json` (sigla, pasta, nome, tipo, web) e ganha um workflow
   `build-<pasta em minúsculas>.yml` copiado de um existente (com instaladores, pluginval e auval).
5. **Checagem.** Rodar `python3 .github/checagem/checar.py` antes de enviar. O workflow `checagem.yml`
   roda em todo push e PR e falha se nome, versão, links ou versão para celular estiverem fora do padrão.
6. **Release.** Depois do merge na `main`, o workflow do plugin publica a release `SIGLA-x.y` com os
   instaladores e os zips. Conferir que a release existe antes de dizer que o site está atualizado.

## Padrão de nomes

- Nome no Cubase (`PRODUCT_NAME`): `Mastor <Nome> <SIGLA>`. Pasta: o nome em CamelCase, sem "Mastor" e sem sigla.
- Release `SIGLA-x.y`; arquivos `SIGLA-Windows-Instalador.exe`, `SIGLA-macOS-Instalador.pkg`, `SIGLA-Windows.zip`, `SIGLA-macOS.zip`.
- Imagem do card: `site/img/<pasta em minúsculas>.jpg`.
- **Nunca mudar** `PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE` (`Dblb`), `BUNDLE_ID` nem o ID de estado do APVTS:
  é isso que mantém os projetos antigos abrindo. Ao renomear, acrescentar o nome antigo em
  `.github/installer/nomes-antigos.txt` (os instaladores apagam a cópia antiga).

## Compatibilidade com as DAWs

- Formatos: VST3 (Windows e Mac), AU (Mac, para Logic e GarageBand) e standalone. Sem AAX (Pro Tools).
- Windows: runtime do C++ embutido (`CMAKE_MSVC_RUNTIME_LIBRARY` MultiThreaded) — o plugin não pode depender
  do Visual C++ Redistributable. A release confere isso.
- Mac: binário universal (arm64 + x86_64), macOS 10.13+, assinatura ad-hoc; o `.pkg` instala em `/Library/Audio/Plug-Ins`.
- Todo build passa no pluginval (Windows e Mac) e no auval (Mac) antes de virar release.
- Instruções por DAW ficam na seção "Instalação" do site; manter atualizadas quando mudar algo na instalação.

## Instalador completo (Dub Station, todos os plugins)
- Workflow `.github/workflows/build-pacote.yml`, script `.github/scripts/pacote.py`. Lê a lista do `plugins.json`: plugin novo entra sozinho.
- Release `DubStation-<versão>` com `DubStation-Windows-Instalador.exe` e `DubStation-macOS-Instalador.pkg` (nunca `latest`).
- **Sempre que a versão de um plugin subir**, atualize `plugins.json` → `pacote` → `plugins` com a nova versão, suba `pacote` → `versao` e troque `DubStation-x.y` nos links do site e do README. A checagem falha se esquecer.
- Pro Tools (AAX): todo CMakeLists inclui `.github/cmake/aax.cmake` e põe `${DM_AAX}` no fim de `FORMATS`. Plugin novo também. O AAX é compilado sempre (SDK do JUCE 8) e só entra no instalador público quando assinado pela PACE (passo a passo em `PROTOOLS.md`).

