## Mastor Reggae Drum Machine MRDM-12 2.3 🟢🟡⚫

- **Instaladores**: `.pkg` para Mac (VST3 + AU em /Library/Audio/Plug-Ins, app em /Applications) e `.exe` para Windows (VST3 na pasta comum de todas as DAWs)
- O instalador do Mac **tira a quarentena** (o aviso de "danificado") e relê os Audio Units para o Logic/GarageBand
- Os dois instaladores **apagam as versões antigas** (DubMastor Dub Drums / MDDM-12), que têm o mesmo código de plugin e confundiam as DAWs
- Antes de publicar, o GitHub instala o .pkg num Mac de verdade e confere o AU com o validador da Apple
- Downloads: `MastorReggaeDrumMachine-macOS-Instalador.pkg`, `MastorReggaeDrumMachine-Windows-Instalador.exe` (e os zips)
- No Mac, se o instalador não abrir: botão direito › Abrir, ou Ajustes do Sistema › Privacidade e Segurança › Abrir Mesmo Assim
- **Groove mais limpo**: quando a aleatoriedade joga uma ghost em cima da mesma peça, fica só a nota principal, no tempo certo. Colada (menos de 0,09 tempo), duplicada ou flam (também nas viradas): a fraca sai. Próxima (até 0,2 tempo) no bumbo, caixa e tons, a fraca cai para 35% da principal. Os rufos das viradas ficam como estão

### Da 2.2

- **Versão do navegador/celular com layout novo**: na ordem do plugin, em 3 colunas no computador, 2 no tablet e 1 no celular; módulos alinhados, alto-falante e PREVIEW lado a lado sem invadir o MIDI, botões que completam as linhas e nada mais largo que a tela
- O plugin é o mesmo da 2.1

### Da 2.1

O **Mastor Dub Drums Machine MDDM-12** agora se chama **Mastor Reggae Drum Machine MRDM-12**.

- **Alto-falante vintage** no meio da seção SOM, no estilo dos gabinetes de bateria eletrônica valvulada de 1959: madeira, tweed dourado, aro de latão e tweeter em cima. O cone mexe com o som que sai do plugin
- **ALEATORIEDADE** mais compacta para abrir espaço para o alto-falante
- **Preset inicial**: hi-hat em **1/16** e baterista **Style Scott**
- Tudo da 2.0: Sly Dunbar e DIGIKAL (Casio MT-40 / Oberheim DMX), kit ANOS 70 com os samples DM, grade editável, arraste de um compasso só, correção do "embolado"

Atenção: como o nome mudou, a DAW mostra o MRDM-12 como um plugin novo. Apague o antigo `Mastor Dub Drums Machine MDDM-12.vst3` / `.component` e troque nos projetos.

**📱 Versão para celular:** https://dubmastor.github.io/DubMastor-DubStation-/drums/

**Testado em todas as DAWs antes de publicar:** validador oficial da Steinberg (VST3), pluginval nível 10 (VST3 e AU) e auval (Logic/GarageBand), em Windows e macOS.

É um instrumento: no Cubase, carregue numa *Instrument Track*.
Downloads: `MastorReggaeDrumMachine-Windows.zip` e `MastorReggaeDrumMachine-macOS.zip`.

No Mac, depois de copiar o `.vst3`:
```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Mastor Reggae Drum Machine MRDM-12.vst3"
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Mastor Reggae Drum Machine MRDM-12.component"
```
