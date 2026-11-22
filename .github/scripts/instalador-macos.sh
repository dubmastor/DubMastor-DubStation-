#!/bin/bash
# Gera o instalador .pkg de um plugin DubMastor para macOS.
# Uso (a partir da pasta do plugin, depois de juntar os arquivos assinados em out/):
#   instalador-macos.sh out "Mastor Big Knob MBK-76" MBK-76 ../MBK-76-macOS-Instalador.pkg
# Instala em /Library/Audio/Plug-Ins/VST3 e /Components (todas as DAWs e usuários veem)
# e o standalone em /Applications/DubMastor. Arquivos instalados pelo Instalador não ficam
# em quarentena, então não precisa do comando xattr.
set -euo pipefail
ORIGEM="$1"; PRODUTO="$2"; SIGLA="$3"; SAIDA="$4"
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"

VERSAO=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' build/CMakeCache.txt)
test -n "$VERSAO"

ANTIGOS=$(grep -v '^#' "$RAIZ/.github/installer/nomes-antigos.txt" | awk -F'|' -v s="$SIGLA" '$1==s { for (i=2;i<=NF;i++) if ($i!="") print $i }')

T=$(mktemp -d)
mkdir -p "$T/root/Library/Audio/Plug-Ins/VST3" "$T/root/Library/Audio/Plug-Ins/Components" "$T/root/Applications/DubMastor" "$T/scripts"
cp -R "$ORIGEM/$PRODUTO.vst3" "$T/root/Library/Audio/Plug-Ins/VST3/"
[ -d "$ORIGEM/$PRODUTO.component" ] && cp -R "$ORIGEM/$PRODUTO.component" "$T/root/Library/Audio/Plug-Ins/Components/"
[ -d "$ORIGEM/$PRODUTO.app" ] && cp -R "$ORIGEM/$PRODUTO.app" "$T/root/Applications/DubMastor/"

# sem isto o Instalador "realoca" o plugin para onde achar uma cópia antiga com o mesmo ID
pkgbuild --analyze --root "$T/root" "$T/comp.plist"
i=0
while /usr/libexec/PlistBuddy -c "Print :$i" "$T/comp.plist" >/dev/null 2>&1; do
  /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$T/comp.plist"
  i=$((i+1))
done

# pós-instalação: remove nomes antigos (sem duplicar na DAW), tira quarentena e recarrega a lista de AUs
{
  echo '#!/bin/bash'
  while IFS= read -r a; do
    [ -z "$a" ] && continue
    for d in "/Library/Audio/Plug-Ins/VST3/$a.vst3" "/Library/Audio/Plug-Ins/Components/$a.component" \
             "/Users/*/Library/Audio/Plug-Ins/VST3/$a.vst3" "/Users/*/Library/Audio/Plug-Ins/Components/$a.component"; do
      printf 'rm -rf %q 2>/dev/null\n' "$d" | sed 's/\\\*/*/g'
    done
  done <<< "$ANTIGOS"
  # cópia manual antiga do mesmo plugin na pasta do usuário (instalação pelo zip): fica só a do sistema
  for d in "/Users/*/Library/Audio/Plug-Ins/VST3/$PRODUTO.vst3" "/Users/*/Library/Audio/Plug-Ins/Components/$PRODUTO.component"; do
    printf 'rm -rf %q 2>/dev/null\n' "$d" | sed 's/\\\*/*/g'
  done
  printf 'xattr -dr com.apple.quarantine %q %q 2>/dev/null\n' "/Library/Audio/Plug-Ins/VST3/$PRODUTO.vst3" "/Library/Audio/Plug-Ins/Components/$PRODUTO.component"
  echo 'killall -9 AudioComponentRegistrar 2>/dev/null'
  echo 'exit 0'
} > "$T/scripts/postinstall"
chmod +x "$T/scripts/postinstall"

pkgbuild --root "$T/root" --component-plist "$T/comp.plist" --scripts "$T/scripts" \
  --identifier "com.dubmastor.$(echo "$SIGLA" | tr 'A-Z' 'a-z')" --version "$VERSAO" \
  --install-location / "$T/plugin.pkg"

cat > "$T/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>$PRODUTO $VERSAO</title>
  <options customize="never" require-scripts="false" hostArchitectures="x86_64,arm64"/>
  <domains enable_localSystem="true"/>
  <choices-outline><line choice="plugin"/></choices-outline>
  <choice id="plugin" visible="false"><pkg-ref id="com.dubmastor.plugin"/></choice>
  <pkg-ref id="com.dubmastor.plugin" version="$VERSAO">plugin.pkg</pkg-ref>
</installer-gui-script>
EOF
productbuild --distribution "$T/distribution.xml" --package-path "$T" "$SAIDA"
ls -la "$SAIDA"
