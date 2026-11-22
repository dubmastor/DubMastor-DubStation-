; Instalador Windows do Dub Battery Reggae (Inno Setup 6)
; Coloca o VST3 na pasta padrão (C:\Program Files\Common Files\VST3), onde Cubase, FL Studio,
; Ableton Live, Reaper, Studio One, Bitwig e as outras DAWs VST3 procuram plugins.
; Compilar: ISCC /DAppVersion=1.0 DBR.iss   (a partir da pasta DubBattery/installer)

#define AppName "Dub Battery Reggae"
#ifndef AppVersion
  #define AppVersion "0.0"
#endif
#define Build "..\build\DubBatteryReggae_artefacts\Release"

[Setup]
AppId={{5C1E8B7A-2D4F-4B9E-9A31-D7B2A0C4E816}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=DubMastor
AppPublisherURL=https://dubmastor.github.io/DubMastor-DubStation-/
DefaultDirName={commonpf64}\DubMastor\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..
OutputBaseFilename=DubBatteryReggae-Windows-Instalador
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

[Files]
Source: "{#Build}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#Build}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\DubMastor\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "Abrir o {#AppName} agora"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone
