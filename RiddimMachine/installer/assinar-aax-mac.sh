#!/bin/sh
# Assina o AAX do MRM-0420 para o Pro Tools "normal" (o Pro Tools só carrega AAX assinado pela PACE).
# Precisa: conta de desenvolvedor da Avid + PACE Eden Tools (wraptool), um iLok e um certificado
# "Developer ID Application" da Apple no Keychain. O build sem assinatura funciona no Pro Tools Developer.
#
# Uso: PACE_ACCOUNT=... PACE_PASSWORD=... PACE_WCGUID=... APPLE_SIGN_ID="Developer ID Application: Nome (TEAMID)" \
#      ./assinar-aax-mac.sh "/Library/Application Support/Avid/Audio/Plug-Ins/Mastor Riddim Machine MRM-0420.aaxplugin"
set -e
AAX="${1:-../build/MastorRiddimMachine_artefacts/Release/AAX/Mastor Riddim Machine MRM-0420.aaxplugin}"
: "${PACE_ACCOUNT:?defina PACE_ACCOUNT}"; : "${PACE_PASSWORD:?defina PACE_PASSWORD}"
: "${PACE_WCGUID:?defina PACE_WCGUID}";   : "${APPLE_SIGN_ID:?defina APPLE_SIGN_ID}"
wraptool sign --verbose --account "$PACE_ACCOUNT" --password "$PACE_PASSWORD" --wcguid "$PACE_WCGUID" \
              --signid "$APPLE_SIGN_ID" --in "$AAX" --out "$AAX"
wraptool verify --verbose --in "$AAX"
