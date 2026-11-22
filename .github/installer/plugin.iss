; Instalador Windows dos plugins DubMastor (Inno Setup 6).
; Gerado pelo .github/scripts/instalador-windows.ps1, que passa estes valores com /D:
;   Produto   nome do plugin no Cubase (ex.: Mastor Big Knob MBK-76)
;   Sigla     sigla do plugin (ex.: MBK-76)
;   Versao    versão (ex.: 3.2.0)
;   Origem    pasta com o .vst3, o .aaxplugin (Pro Tools, se houver) e o .exe standalone
;   SaidaDir  pasta onde o instalador é gravado
;   SaidaNome nome do arquivo sem .exe (ex.: MBK-76-Windows-Instalador)
;   Antigos   nomes antigos do plugin separados por "|" (os .vst3 antigos são apagados)

#ifndef Antigos
  #define Antigos ""
#endif

[Setup]
AppId=DubMastor.{#Sigla}
AppName={#Produto}
AppVersion={#Versao}
AppVerName={#Produto} {#Versao}
AppPublisher=DubMastor
AppPublisherURL=https://dubmastor.github.io/DubMastor-DubStation-/
VersionInfoVersion={#Versao}
DefaultDirName={commonpf64}\DubMastor\{#Produto}
DefaultGroupName=DubMastor
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir={#SaidaDir}
OutputBaseFilename={#SaidaNome}
UninstallDisplayName={#Produto}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "pt"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
pt.WelcomeLabel2=Isto vai instalar o [name/ver] na pasta padrão de VST3 do Windows (C:\Program Files\Common Files\VST3), onde o Cubase, Ableton Live, FL Studio, Reaper, Studio One, Bitwig e as outras DAWs procuram plugins.%n%nFeche a sua DAW antes de continuar.
en.WelcomeLabel2=This will install [name/ver] into the standard Windows VST3 folder (C:\Program Files\Common Files\VST3), where Cubase, Ableton Live, FL Studio, Reaper, Studio One, Bitwig and other DAWs look for plugins.%n%nClose your DAW before continuing.

[Files]
Source: "{#Origem}\{#Produto}.vst3\*"; DestDir: "{commoncf64}\VST3\{#Produto}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Origem}\{#Produto}.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#Produto}.aaxplugin"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#Origem}\{#Produto}.exe"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{autoprograms}\DubMastor\{#Produto}"; Filename: "{app}\{#Produto}.exe"; Check: StandaloneExiste

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Produto}.vst3"
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#Produto}.aaxplugin"

[Code]
function StandaloneExiste: Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\{#Produto}.exe'));
end;

// Apaga os .vst3 com nomes antigos do plugin, para não aparecer duplicado na DAW.
procedure CurStepChanged(CurStep: TSetupStep);
var
  Lista, Nome, Pasta: String;
  P: Integer;
begin
  if CurStep <> ssInstall then Exit;
  Lista := '{#Antigos}';
  while Lista <> '' do
  begin
    P := Pos('|', Lista);
    if P = 0 then begin Nome := Lista; Lista := ''; end
    else begin Nome := Copy(Lista, 1, P - 1); Delete(Lista, 1, P); end;
    if Nome <> '' then
    begin
      Pasta := ExpandConstant('{commoncf64}\VST3\') + Nome + '.vst3';
      if DirExists(Pasta) then DelTree(Pasta, True, True, True)
      else if FileExists(Pasta) then DeleteFile(Pasta);
    end;
  end;
end;
