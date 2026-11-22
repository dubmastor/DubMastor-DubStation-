; Instalador Windows do Mastor Riddim Machine MRM-0420 (Inno Setup 6)
;  - VST3 em C:\Program Files\Common Files\VST3: Cubase, FL Studio, Ableton Live, Reaper, Studio One, Bitwig...
;  - AAX em C:\Program Files\Common Files\Avid\Audio\Plug-Ins: Pro Tools
;  - programa standalone (toca sem DAW)
; Compilar: ISCC /DAppVersion=1.0 MRM0420.iss   (a partir da pasta RiddimMachine/installer)

#define AppName "Mastor Riddim Machine MRM-0420"
#ifndef AppVersion
  #define AppVersion "0.0"
#endif
#define Build "..\build\MastorRiddimMachine_artefacts\Release"

[Setup]
AppId={{9A4D2E61-0420-4C3B-B7E5-6F1D2A8C0420}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=DubStation
AppPublisherURL=https://dubmastor.github.io/DubMastor-DubStation-/
DefaultDirName={commonpf64}\DubStation\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..
OutputBaseFilename=MRM-0420-Windows-Instalador
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
Name: "full"; Description: "Completa (VST3 + AAX + programa standalone)"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 (Cubase, FL Studio, Ableton Live, Reaper, Studio One, Bitwig...)"; Types: full custom
Name: "aax"; Description: "AAX (Pro Tools)"; Types: full custom
Name: "standalone"; Description: "Programa standalone (toca sem DAW)"; Types: full

[InstallDelete]
; versão antiga instalada à mão pelo zip
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#AppName}.aaxplugin"; Components: aax

[Files]
Source: "{#Build}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#Build}\AAX\{#AppName}.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#AppName}.aaxplugin"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: aax
Source: "{#Build}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\DubStation\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "Abrir o {#AppName} agora"; Flags: postinstall nowait skipifsilent unchecked; Components: standalone
