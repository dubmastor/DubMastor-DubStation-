; Instalador Windows do Mastor Reggae Drum Machine MRDM-12 (Inno Setup 6)
; Coloca o VST3 na pasta padrão (C:\Program Files\Common Files\VST3), onde Cubase, FL Studio,
; Ableton Live, Reaper, Studio One, Bitwig e as outras DAWs VST3 procuram plugins, e apaga as versões
; antigas (MDDM-12 / Dub Drums), que têm o mesmo código de plugin e apareceriam duplicadas.
; Compilar: ISCC /DAppVersion=2.3 MRDM.iss   (a partir da pasta ReggaeDrumMachine/installer)

#define AppName "Mastor Reggae Drum Machine MRDM-12"
#ifndef AppVersion
  #define AppVersion "0.0"
#endif
#define Build "..\build\DubDrums_artefacts\Release"

[Setup]
AppId={{8E3A2C51-7B4D-4F1A-9C62-3D5B7E9F1A24}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=DubMastor
AppPublisherURL=https://dubmastor.github.io/DubMastor-DubStation-/
DefaultDirName={commonpf64}\DubMastor\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..
OutputBaseFilename=MastorReggaeDrumMachine-Windows-Instalador
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
Type: filesandordirs; Name: "{commoncf64}\VST3\Mastor Dub Drums Machine MDDM-12.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\DubMastor Dub Drums.vst3"

[Files]
Source: "{#Build}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#Build}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\DubMastor\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "Abrir o {#AppName} agora"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone
