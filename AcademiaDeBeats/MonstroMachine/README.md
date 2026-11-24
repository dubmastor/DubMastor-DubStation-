# Monstro Machine MM-2500 1.0

Projeto da linha **Academia de Beats**, separado da coleção Dub Station.

Drum machine no estilo MPC2500 com os kits dos produtores do **Monstro.zip** (Vol. 1 a 4) da Academia de Beats.

**[⬇️ Windows](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MM-2500-1.0/MM-2500-Windows-Instalador.exe)** · **[⬇️ macOS](https://github.com/dubmastor/DubMastor-DubStation-/releases/download/MM-2500-1.0/MM-2500-macOS-Instalador.pkg)** · **📱 [Testar no navegador](https://dubmastor.github.io/DubMastor-DubStation-/academiadebeats/monstromachine/)**

- 33 kits com a ficha de cada produtor (volume, bio, com quem trabalhou, conteúdo do kit)
- 16 pads (notas 36–51, canal 10) e 4 sequências (A–D) de 16 passos com acento, swing, REC, TAP e UNDO
- Grooves por estilo do produtor: Trap, Funk, Boom Bap, Drill e Reggae (na contagem com a caixa no 2 e no 4)
- Segue o andamento e o play da DAW
- **CARREGAR PASTA**: toca os samples reais do kit, distribuídos nos pads pelo nome do arquivo
- Saturador e Tape Echo começam em bypass
- Arraste do MIDI para a DAW e SALVAR MIDI
- Formatos: VST3 (Windows e Mac), AU (Logic, GarageBand), standalone; AAX compilado (Pro Tools depois da assinatura PACE)

## Código

- `Source/MonstroEngine.h`: síntese dos 16 pads, kits, grooves e MIDI (sem JUCE). A versão web
  (`site/academiadebeats/monstromachine/index.html`) tem a mesma síntese e os mesmos grooves em JavaScript: ao mudar um, mude o outro.
- `Source/PluginProcessor.*` e `Source/PluginEditor.*`: plugin JUCE 8.0.4.
- `tests/render_test.cpp`: teste offline (`g++ -std=c++17 -I Source tests/render_test.cpp && ./a.out`).
- Build e instaladores: `.github/workflows/build-monstromachine.yml` (release `MM-2500-x.y`).
- Fora do `plugins.json` de propósito: não entra no site nem no pacote da Dub Station.
