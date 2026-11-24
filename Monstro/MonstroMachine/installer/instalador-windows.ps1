# Gera o instalador .exe (Inno Setup) da Monstro Machine MM-2500.
# Uso (a partir da pasta do plugin, depois de juntar os arquivos em out/):
#   installer/instalador-windows.ps1 -Origem out -Produto "Monstro Machine MM-2500" -Sigla MM-2500 -Saida ../MM-2500-Windows-Instalador.exe
# A versão vem do build/CMakeCache.txt (CMAKE_PROJECT_VERSION).
param(
  [Parameter(Mandatory)] [string] $Origem,
  [Parameter(Mandatory)] [string] $Produto,
  [Parameter(Mandatory)] [string] $Sigla,
  [Parameter(Mandatory)] [string] $Saida
)
$ErrorActionPreference = 'Stop'
$raiz = (Resolve-Path "$PSScriptRoot/../../..").Path

$versao = (Select-String -Path build/CMakeCache.txt -Pattern '^CMAKE_PROJECT_VERSION:STATIC=(.+)$').Matches[0].Groups[1].Value
if (-not $versao) { throw "Versão não encontrada no build/CMakeCache.txt" }

$antigos = ''
foreach ($l in Get-Content "$raiz/.github/installer/nomes-antigos.txt") {
  if ($l.StartsWith('#')) { continue }
  $p = $l.Split('|')
  if ($p[0] -eq $Sigla) { $antigos = ($p | Select-Object -Skip 1 | Where-Object { $_ -ne '' }) -join '|' }
}

$iscc = Get-Command iscc.exe -ErrorAction SilentlyContinue
if ($iscc) { $iscc = $iscc.Source }
elseif (Test-Path "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe") { $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" }
else {
  choco install innosetup -y --no-progress | Out-Null
  $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
}

$origemAbs = (Resolve-Path $Origem).Path
$saidaAbs  = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $Saida))
$saidaDir  = [System.IO.Path]::GetDirectoryName($saidaAbs)
$saidaNome = [System.IO.Path]::GetFileNameWithoutExtension($saidaAbs)

& $iscc /Q "/DProduto=$Produto" "/DSigla=$Sigla" "/DVersao=$versao" "/DOrigem=$origemAbs" `
  "/DSaidaDir=$saidaDir" "/DSaidaNome=$saidaNome" "/DAntigos=$antigos" "$PSScriptRoot/monstro.iss"
if ($LASTEXITCODE -ne 0) { throw "Inno Setup falhou ($LASTEXITCODE)" }
if (-not (Test-Path $saidaAbs)) { throw "Instalador não foi gerado: $saidaAbs" }
Write-Host "Instalador: $saidaAbs ($versao)"
