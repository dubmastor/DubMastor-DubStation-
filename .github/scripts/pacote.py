#!/usr/bin/env python3
"""Instalador completo da coleção Dub Station (todos os plugins num instalador só).

Subcomandos:
  pacote.py lista
      Imprime a matriz do workflow (sigla, pasta, nome) em JSON, a partir do plugins.json.
  pacote.py coletar <pasta do plugin> <sigla> <destino>
      Depois do build: copia VST3, AU, AAX e standalone de build/*_artefacts/Release para
      <destino>/<SIGLA>/ e grava a versão em <destino>/<SIGLA>/versao.txt.
  pacote.py windows <dist> <versao> <saida.exe>
      Gera o .iss (Inno Setup) com um componente por plugin e compila o instalador.
  pacote.py macos <dist> <versao> <saida.pkg>
      Gera um .pkg por plugin e junta tudo num .pkg com a lista de plugins para marcar.
  pacote.py notas <versao>
      Imprime o texto da release (lista de plugins e versões).

<dist> tem uma subpasta por sigla, como o 'coletar' deixa.
"""
import json, os, pathlib, plistlib, re, shutil, subprocess, sys, tempfile

RAIZ = pathlib.Path(__file__).resolve().parents[2]
REG = json.loads((RAIZ / "plugins.json").read_text())
PLUGINS = REG["plugins"]
SITE = "https://dubmastor.github.io/DubMastor-DubStation-/"

# pastas padrão de cada formato
WIN_VST3 = r"{commoncf64}\VST3"
WIN_AAX = r"{commoncf64}\Avid\Audio\Plug-Ins"
MAC_VST3 = "Library/Audio/Plug-Ins/VST3"
MAC_AU = "Library/Audio/Plug-Ins/Components"
MAC_AAX = "Library/Application Support/Avid/Audio/Plug-Ins"
MAC_APP = "Applications/DubMastor"


def antigos(sigla):
    """Nomes antigos do plugin (os instaladores apagam, para não duplicar na DAW)."""
    for l in (RAIZ / ".github/installer/nomes-antigos.txt").read_text().splitlines():
        if l.startswith("#") or not l.strip():
            continue
        p = l.split("|")
        if p[0] == sigla:
            return [x for x in p[1:] if x]
    return []


def versao_plugin(pasta):
    cm = (RAIZ / pasta / "CMakeLists.txt").read_text()
    m = re.search(r"^project\(\w+ VERSION (\d+\.\d+(?:\.\d+)?)", cm, re.M)
    if m:
        return m.group(1)
    vh = RAIZ / pasta / "Source/Version.h"
    m = re.search(r'"(\d+\.\d+(?:\.\d+)?)', vh.read_text()) if vh.exists() else None
    return m.group(1) if m else "?"


def numerica(v):
    """'1.2' -> '1.2.0.0' (o Windows só aceita números no VersionInfo)."""
    n = [int(x) for x in re.findall(r"\d+", v)][:4] or [0]
    return ".".join(str(x) for x in n + [0] * (4 - len(n)))


def curta(v):
    """'3.2.0' -> '3.2' (como aparece no site e nas releases)."""
    return ".".join(v.split(".")[:2])


def ident(sigla):
    return re.sub(r"[^a-z0-9]", "_", sigla.lower())


def tipo_txt(p):
    return "efeito" if p["tipo"] == "efeito" else "instrumento"


# ---------------------------------------------------------------- lista / coletar
def cmd_lista():
    m = [{"sigla": p["sigla"], "pasta": p["pasta"], "nome": p["nome"], "padrao": p.get("padrao", True)} for p in PLUGINS]
    print(json.dumps(m))


def cmd_coletar(pasta, sigla, destino):
    p = next(x for x in PLUGINS if x["sigla"] == sigla)
    nome = p["nome"]
    build = RAIZ / pasta / "build"
    rel = [d for d in build.glob("*_artefacts/Release") if (d / "VST3" / f"{nome}.vst3").exists()]
    if not rel:
        sys.exit(f"{sigla}: não achei build/*_artefacts/Release/VST3/{nome}.vst3")
    rel = rel[0]
    out = pathlib.Path(destino) / sigla
    out.mkdir(parents=True, exist_ok=True)
    achou = []
    for sub, ext in (("VST3", ".vst3"), ("AU", ".component"), ("AAX", ".aaxplugin"),
                     ("Standalone", ".app"), ("Standalone", ".exe")):
        src = rel / sub / f"{nome}{ext}"
        if not src.exists():
            continue
        dst = out / src.name
        if src.is_dir():
            shutil.copytree(src, dst, symlinks=True, dirs_exist_ok=True)
        else:
            shutil.copy2(src, dst)
        achou.append(src.name)
    v = None
    cache = build / "CMakeCache.txt"
    if cache.exists():
        m = re.search(r"^CMAKE_PROJECT_VERSION:STATIC=(.+)$", cache.read_text(), re.M)
        v = m.group(1).strip() if m else None
    (out / "versao.txt").write_text((v or versao_plugin(pasta)) + "\n")
    print(f"{sigla} {v}: " + ", ".join(achou))


def presentes(dist):
    """Plugins que chegaram do build, na ordem do plugins.json."""
    r = []
    for p in PLUGINS:
        d = pathlib.Path(dist) / p["sigla"]
        if (d / f"{p['nome']}.vst3").exists():
            v = (d / "versao.txt").read_text().strip() if (d / "versao.txt").exists() else versao_plugin(p["pasta"])
            r.append((p, d, v))
        else:
            print(f"::warning::{p['sigla']} não chegou do build; fica fora do instalador completo")
    if not r:
        sys.exit("nenhum plugin em " + str(dist))
    return r


# ---------------------------------------------------------------- Windows (Inno Setup)
def gerar_iss(dist, versao, saida):
    itens = presentes(dist)
    tem_aax = any((d / f"{p['nome']}.aaxplugin").exists() for p, d, _ in itens)
    saida = pathlib.Path(saida).resolve()
    L = []
    a = L.append
    a("; Gerado por .github/scripts/pacote.py — não edite à mão.")
    a("[Setup]")
    a("AppId=DubMastor.DubStation")
    a("AppName=DubMastor Dub Station (todos os plugins)")
    a(f"AppVersion={versao}")
    a(f"AppVerName=DubMastor Dub Station {versao}")
    a("AppPublisher=DubMastor")
    a(f"AppPublisherURL={SITE}")
    a(f"VersionInfoVersion={numerica(versao)}")
    a(r"DefaultDirName={commonpf64}\DubMastor\Dub Station")
    a("DefaultGroupName=DubMastor")
    a("DisableDirPage=yes")
    a("DisableProgramGroupPage=yes")
    a("ArchitecturesAllowed=x64compatible")
    a("ArchitecturesInstallIn64BitMode=x64compatible")
    a("PrivilegesRequired=admin")
    a(f"OutputDir={saida.parent}")
    a(f"OutputBaseFilename={saida.stem}")
    a("UninstallDisplayName=DubMastor Dub Station (todos os plugins)")
    a("Compression=lzma2/max")
    a("SolidCompression=yes")
    a("WizardStyle=modern")
    a("")
    a("[Languages]")
    a('Name: "pt"; MessagesFile: "compiler:Languages\\BrazilianPortuguese.isl"')
    a('Name: "en"; MessagesFile: "compiler:Default.isl"')
    a("")
    daws = "Cubase, Ableton Live, FL Studio, Reaper, Studio One e Bitwig"
    a("[Messages]")
    a(f"pt.WelcomeLabel2=Isto vai instalar os plugins da coleção Dub Station de uma vez, na pasta padrão de VST3 do Windows (C:\\Program Files\\Common Files\\VST3), onde {daws} procuram plugins."
      + (" A versão AAX vai para a pasta de plugins do Pro Tools." if tem_aax else "")
      + "%n%nNa próxima tela você pode desmarcar os que não quiser.%n%nFeche a sua DAW antes de continuar.")
    a(f"en.WelcomeLabel2=This will install every Dub Station plugin at once into the standard Windows VST3 folder (C:\\Program Files\\Common Files\\VST3)."
      + (" The AAX version goes to the Pro Tools plug-in folder." if tem_aax else "")
      + "%n%nYou can untick the ones you don't want on the next page.%n%nClose your DAW before continuing.")
    a("")
    a("[Types]")
    a('Name: "tudo"; Description: "Todos os plugins"')
    a('Name: "escolher"; Description: "Escolher os plugins"; Flags: iscustom')
    a("")
    a("[Components]")
    for grupo, titulo in (("efeito", "Efeitos"), ("instrumento", "Instrumentos e machines")):
        a(f'Name: "{grupo}"; Description: "{titulo}"; Types: tudo escolher')
        for p, d, v in itens:
            if tipo_txt(p) == grupo:
                a(f'Name: "{grupo}\\{ident(p["sigla"])}"; Description: "{p["nome"]} {curta(v)}"; Types: tudo escolher')
    if tem_aax:
        a('Name: "aax"; Description: "Pro Tools (AAX) para os plugins marcados"; Types: tudo escolher')
    a("")
    a("[InstallDelete]")
    for p, d, v in itens:
        c = f'{tipo_txt(p)}\\{ident(p["sigla"])}'
        for velho in antigos(p["sigla"]):
            a(f'Type: filesandordirs; Name: "{WIN_VST3}\\{velho}.vst3"; Components: {c}')
            a(f'Type: filesandordirs; Name: "{WIN_AAX}\\{velho}.aaxplugin"; Components: {c}')
        # cópia antiga do próprio plugin: troca inteira (não mistura arquivos de versões diferentes)
        a(f'Type: filesandordirs; Name: "{WIN_VST3}\\{p["nome"]}.vst3"; Components: {c}')
    a("")
    a("[Files]")
    for p, d, v in itens:
        c = f'{tipo_txt(p)}\\{ident(p["sigla"])}'
        n = p["nome"]
        a(f'Source: "{d.resolve()}\\{n}.vst3\\*"; DestDir: "{WIN_VST3}\\{n}.vst3"; Components: {c}; Flags: ignoreversion recursesubdirs createallsubdirs')
        if (d / f"{n}.aaxplugin").exists():
            a(f'Source: "{d.resolve()}\\{n}.aaxplugin\\*"; DestDir: "{WIN_AAX}\\{n}.aaxplugin"; Components: {c} and aax; Flags: ignoreversion recursesubdirs createallsubdirs')
        if (d / f"{n}.exe").exists():
            a(f'Source: "{d.resolve()}\\{n}.exe"; DestDir: "{{app}}"; Components: {c}; Flags: ignoreversion')
    a("")
    a("[Icons]")
    for p, d, v in itens:
        if (d / f"{p['nome']}.exe").exists():
            a(f'Name: "{{autoprograms}}\\DubMastor\\{p["nome"]}"; Filename: "{{app}}\\{p["nome"]}.exe"; Components: {tipo_txt(p)}\\{ident(p["sigla"])}')
    a("")
    a("[UninstallDelete]")
    for p, d, v in itens:
        a(f'Type: filesandordirs; Name: "{WIN_VST3}\\{p["nome"]}.vst3"')
        a(f'Type: filesandordirs; Name: "{WIN_AAX}\\{p["nome"]}.aaxplugin"')
    return "\r\n".join(L) + "\r\n"


def achar_iscc():
    c = shutil.which("iscc") or shutil.which("ISCC.exe")
    if c:
        return c
    for base in (os.environ.get("ProgramFiles(x86)", ""), os.environ.get("ProgramFiles", "")):
        f = pathlib.Path(base) / "Inno Setup 6" / "ISCC.exe"
        if f.exists():
            return str(f)
    subprocess.run(["choco", "install", "innosetup", "-y", "--no-progress"], check=True)
    return str(pathlib.Path(os.environ["ProgramFiles(x86)"]) / "Inno Setup 6" / "ISCC.exe")


def cmd_windows(dist, versao, saida):
    iss = pathlib.Path(dist) / "DubStation.iss"
    iss.write_text(gerar_iss(dist, versao, saida), encoding="utf-8-sig")
    print(iss.read_text(encoding="utf-8-sig"))
    if os.name != "nt":
        print("(fora do Windows: só gerei o .iss)")
        return
    subprocess.run([achar_iscc(), "/Q", str(iss)], check=True)
    if not pathlib.Path(saida).exists():
        sys.exit("instalador não foi gerado: " + saida)
    print("Instalador:", saida)


# ---------------------------------------------------------------- macOS (pkgbuild + productbuild)
def postinstall(p):
    n = p["nome"]
    linhas = ["#!/bin/bash", "# gerado por pacote.py: tira nomes antigos, cópias soltas na pasta do usuário e a quarentena"]
    q = lambda s: "'" + s.replace("'", "'\\''") + "'"
    for velho in antigos(p["sigla"]):
        for pasta, ext in ((MAC_VST3, ".vst3"), (MAC_AU, ".component"), (MAC_AAX, ".aaxplugin")):
            linhas.append(f"rm -rf {q('/' + pasta + '/' + velho + ext)} 2>/dev/null")
            linhas.append(f"rm -rf /Users/*/{q(pasta + '/' + velho + ext)} 2>/dev/null")
    for pasta, ext in ((MAC_VST3, ".vst3"), (MAC_AU, ".component")):
        linhas.append(f"rm -rf /Users/*/{q(pasta + '/' + n + ext)} 2>/dev/null")
    for pasta, ext in ((MAC_VST3, ".vst3"), (MAC_AU, ".component"), (MAC_AAX, ".aaxplugin"), (MAC_APP, ".app")):
        linhas.append(f"xattr -dr com.apple.quarantine {q('/' + pasta + '/' + n + ext)} 2>/dev/null")
    linhas += ["killall -9 AudioComponentRegistrar 2>/dev/null", "exit 0", ""]
    return "\n".join(linhas)


def cmd_macos(dist, versao, saida):
    itens = presentes(dist)
    T = pathlib.Path(tempfile.mkdtemp())
    escolhas, refs = [], []
    for p, d, v in itens:
        n, sid = p["nome"], ident(p["sigla"])
        root = T / sid / "root"
        destinos = ((f"{n}.vst3", MAC_VST3), (f"{n}.component", MAC_AU),
                    (f"{n}.aaxplugin", MAC_AAX), (f"{n}.app", MAC_APP))
        for arq, pasta in destinos:
            if (d / arq).exists():
                (root / pasta).mkdir(parents=True, exist_ok=True)
                subprocess.run(["ditto", str(d / arq), str(root / pasta / arq)], check=True)
        scripts = T / sid / "scripts"
        scripts.mkdir(parents=True)
        (scripts / "postinstall").write_text(postinstall(p))
        os.chmod(scripts / "postinstall", 0o755)
        # sem isto o Instalador "realoca" o plugin para onde achar uma cópia antiga com o mesmo ID
        cp = T / sid / "comp.plist"
        subprocess.run(["pkgbuild", "--analyze", "--root", str(root), str(cp)], check=True)
        comps = plistlib.loads(cp.read_bytes())
        for c in comps:
            c["BundleIsRelocatable"] = False
        cp.write_bytes(plistlib.dumps(comps))
        pid = f"com.dubmastor.dubstation.{sid}"
        subprocess.run(["pkgbuild", "--root", str(root), "--component-plist", str(cp), "--scripts", str(scripts),
                        "--identifier", pid, "--version", v, "--install-location", "/",
                        str(T / f"{sid}.pkg")], check=True)
        tem_aax = (d / f"{n}.aaxplugin").exists()
        formatos = "VST3 + AU" + (" + AAX (Pro Tools)" if tem_aax else "")
        escolhas.append(
            f'  <choice id="{sid}" title="{n} {curta(v)}" description="{tipo_txt(p).capitalize()} · {formatos} · standalone" start_selected="true">\n'
            f'    <pkg-ref id="{pid}"/>\n  </choice>')
        refs.append(f'  <pkg-ref id="{pid}" version="{v}" onConclusion="none">{sid}.pkg</pkg-ref>')
    linhas = "\n".join(f'    <line choice="{ident(p["sigla"])}"/>' for p, _, _ in itens)
    (T / "res").mkdir()
    (T / "res" / "welcome.html").write_text(
        "<html><body style='font-family:-apple-system'><h2>Dub Station · DubMastor</h2>"
        "<p>Instala todos os plugins da coleção de uma vez, em <code>/Library/Audio/Plug-Ins</code> "
        "(VST3 para Cubase, Ableton Live, FL Studio, Reaper, Studio One e Bitwig; AU para Logic e GarageBand)"
        + (" e em <code>/Library/Application Support/Avid/Audio/Plug-Ins</code> (AAX para Pro Tools)"
           if any((d / f"{p['nome']}.aaxplugin").exists() for p, d, _ in itens) else "")
        + ".</p><p>Em <b>Personalizar</b> você pode desmarcar os plugins que não quiser.</p>"
          "<p>Feche a sua DAW antes de continuar.</p></body></html>", encoding="utf-8")
    dist_xml = f"""<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>DubMastor Dub Station {versao}</title>
  <welcome file="welcome.html" mime-type="text/html"/>
  <options customize="allow" require-scripts="false" hostArchitectures="x86_64,arm64"/>
  <domains enable_localSystem="true"/>
  <choices-outline>
{linhas}
  </choices-outline>
{chr(10).join(escolhas)}
{chr(10).join(refs)}
</installer-gui-script>
"""
    (T / "distribution.xml").write_text(dist_xml, encoding="utf-8")
    print(dist_xml)
    subprocess.run(["productbuild", "--distribution", str(T / "distribution.xml"), "--resources", str(T / "res"),
                    "--package-path", str(T), saida], check=True)
    print("Instalador:", saida, os.path.getsize(saida), "bytes")


# ---------------------------------------------------------------- notas da release
def cmd_notas(versao, dist=None):
    print(f"# DubMastor Dub Station {versao} — instalador completo\n")
    print("Todos os plugins da coleção num instalador só. Na instalação dá para desmarcar os que não quiser.\n")
    print("| Plugin | Versão | Tipo |\n|---|---|---|")
    for p in PLUGINS:
        print(f"| {p['nome']} | {curta(versao_plugin(p['pasta']))} | {tipo_txt(p)} |")
    print("\n**Windows** (`DubStation-Windows-Instalador.exe`): VST3 em `C:\\Program Files\\Common Files\\VST3` "
          "(Cubase, Ableton Live, FL Studio, Reaper, Studio One, Bitwig) e os programas standalone no menu Iniciar.")
    print("\n**Mac** (`DubStation-macOS-Instalador.pkg`): VST3 e AU em `/Library/Audio/Plug-Ins` "
          "(Logic, GarageBand, Cubase, Ableton Live, FL Studio…), Intel e Apple Silicon, macOS 10.13+.")
    aax = os.environ.get("AAX_STATUS", "")
    if aax == "assinado":
        print("\n**Pro Tools:** AAX assinado incluído (componente \"Pro Tools (AAX)\").")
    else:
        print("\n**Pro Tools:** a versão AAX ainda não está nesta release (falta a assinatura PACE/Avid).")


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    c = a[0]
    if c == "lista":
        cmd_lista()
    elif c == "coletar":
        cmd_coletar(*a[1:4])
    elif c == "windows":
        cmd_windows(*a[1:4])
    elif c == "macos":
        cmd_macos(*a[1:4])
    elif c == "notas":
        cmd_notas(a[1])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
