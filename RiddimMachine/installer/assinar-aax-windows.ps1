# Assina o AAX do MRM-0420 para o Pro Tools "normal" (o Pro Tools só carrega AAX assinado pela PACE).
# Precisa: conta de desenvolvedor da Avid + PACE Eden Tools (wraptool), um iLok e um certificado de
# assinatura de código (.p12/.pfx). O build sem assinatura funciona no Pro Tools Developer.
#
# Uso: $env:PACE_ACCOUNT="..."; $env:PACE_PASSWORD="..."; $env:PACE_WCGUID="..."
#      $env:SIGN_PFX="C:\certs\codesign.pfx"; $env:SIGN_PFX_PASSWORD="..."
#      .\assinar-aax-windows.ps1 "C:\Program Files\Common Files\Avid\Audio\Plug-Ins\Mastor Riddim Machine MRM-0420.aaxplugin"
param([string]$Aax = "..\build\MastorRiddimMachine_artefacts\Release\AAX\Mastor Riddim Machine MRM-0420.aaxplugin")
$ErrorActionPreference = "Stop"
foreach ($v in "PACE_ACCOUNT", "PACE_PASSWORD", "PACE_WCGUID", "SIGN_PFX", "SIGN_PFX_PASSWORD") {
  if (-not (Get-Item "env:$v" -ErrorAction SilentlyContinue)) { throw "defina $v" }
}
wraptool sign --verbose --account $env:PACE_ACCOUNT --password $env:PACE_PASSWORD --wcguid $env:PACE_WCGUID `
              --keyfile $env:SIGN_PFX --keypassword $env:SIGN_PFX_PASSWORD --in $Aax --out $Aax
wraptool verify --verbose --in $Aax
