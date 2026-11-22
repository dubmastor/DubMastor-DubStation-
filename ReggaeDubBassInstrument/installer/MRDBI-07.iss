; Instalador Windows do Mastor Reggae Dub Bass Instrument MRDBI-07 (Inno Setup 6)
; Coloca o VST3 na pasta padrão (C:\Program Files\Common Files\VST3), onde Cubase, FL Studio,
; Ableton Live, Reaper, Studio One, Bitwig e as outras DAWs VST3 procuram plugins.
; Compilar: ISCC /DAppVersion=1.6 MRDBI-07.iss   (a partir da pasta ReggaeDubBassInstrument/installer)

#define AppName "Mastor Reggae Dub Bass Instrument MRDBI-07"
#ifndef AppVersion
  #define AppVersion "0.0"
#endif
#define Build "..\build\DubBass_artefacts\Release"

[Setup]
AppId={{68E71A37-07BF-4F43-898A-AEEE3DBC9DB4}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=DubMastor
AppPublisherURL=https://dubmastor.github.io/DubMastor-DubStation-/
DefaultDirName={commonpf64}\DubMastor\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..
OutputBaseFilename=MastorReggaeDubBassInstrument-Windows-Instalador
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName}

[Languages]
Name: "pt"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Completa (VST3 + programa standalone)"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3 (todas as DAWs VST3)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Programa standalone (toca sem DAW)"; Types: full

[InstallDelete]
; versões antigas com o nome Dub Bass: são o mesmo plugin e apareceriam duplicadas na DAW
Type: filesandordirs; Name: "{commoncf64}\VST3\Dub Bass - Dub Station.vst3"

[Files]
Source: "{#Build}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#Build}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\DubMastor\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "Abrir o {#AppName} agora"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone
