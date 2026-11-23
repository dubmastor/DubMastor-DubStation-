#!/usr/bin/env python3
"""Roda no deploy do site (pages.yml), antes de publicar a pasta site/.

Para cada plugin do plugins.json (e o instalador completo), confere se a release SIGLA-versão
já tem os instaladores. Se não tiver (ex.: o GitHub Actions sem permissão para criar a release),
baixa os instaladores do último build bem-sucedido do plugin na main, coloca em site/downloads/
e troca os links do site para eles. Zip sem release: a linha "Instalação manual" sai do card.
Assim o site nunca fica com link quebrado. Precisa do gh com GH_TOKEN.
"""
import json, os, pathlib, re, shutil, subprocess, tempfile

RAIZ = pathlib.Path(__file__).resolve().parents[2]
REPO = os.environ.get("GITHUB_REPOSITORY", "dubmastor/DubMastor-DubStation-")
REL = "https://github.com/dubmastor/DubMastor-DubStation-/releases/download"
SITE = RAIZ / "site"
DL = SITE / "downloads"
reg = json.loads((RAIZ / "plugins.json").read_text())


def gh(*a):
    r = subprocess.run(["gh", *a], capture_output=True, text=True)
    return r.stdout if r.returncode == 0 else None


def assets(tag):
    out = gh("api", f"repos/{REPO}/releases/tags/{tag}", "--jq", "[.assets[].name]")
    return set(json.loads(out)) if out else set()


def do_build(workflow, padrao):
    """Baixa os arquivos que casam com o padrão do último build da main que os tem."""
    ids = gh("run", "list", "-R", REPO, "--workflow", workflow, "--branch", "main", "-L", "15",
             "--json", "databaseId,status", "--jq", '.[] | select(.status=="completed") | .databaseId')
    for i in (ids or "").split():
        d = pathlib.Path(tempfile.mkdtemp())
        if subprocess.run(["gh", "run", "download", i, "-R", REPO, "-p", padrao, "-D", str(d)],
                          capture_output=True).returncode == 0:
            achados = [f for f in d.rglob("*") if f.suffix in (".exe", ".pkg") and "Instalador" in f.name]
            if achados:
                return achados
    return []


trocas = {}   # link da release -> link local
sem_zip = []  # links de zip sem release
itens = [(p["sigla"], p["pasta"], p.get("padrao", True)) for p in reg["plugins"]]
for sigla, pasta, padrao in itens:
    if not padrao:
        continue
    cm = (RAIZ / pasta / "CMakeLists.txt").read_text()
    m = re.search(r"^project\(\w+ VERSION (\d+)\.(\d+)", cm, re.M)
    if not m:
        m = re.search(r'"(\d+)\.(\d+)', (RAIZ / pasta / "Source/Version.h").read_text())
    tag = f"{sigla}-{m.group(1)}.{m.group(2)}"
    tem = assets(tag)
    # a release pode existir com os arquivos no nome antigo (ex.: MastorReggaeDubBassInstrument-Windows.zip)
    def antigo(arq):
        fim = arq[len(sigla):]
        return next((a for a in sorted(tem) if a.endswith(fim)), None)
    for arq in (f"{sigla}-Windows.zip", f"{sigla}-macOS.zip"):
        if arq not in tem:
            if antigo(arq): trocas[f"{REL}/{tag}/{arq}"] = f"{REL}/{tag}/{antigo(arq)}"
            else: sem_zip.append(f"{REL}/{tag}/{arq}")
    faltam = [f for f in (f"{sigla}-Windows-Instalador.exe", f"{sigla}-macOS-Instalador.pkg") if f not in tem]
    if faltam:
        for f in do_build(f"build-{pasta.lower()}.yml", f"{sigla}-*"):
            if f.name in faltam:
                DL.mkdir(exist_ok=True); shutil.copy2(f, DL / f.name)
                trocas[f"{REL}/{tag}/{f.name}"] = f"downloads/{f.name}"
        for arq in faltam:
            if f"{REL}/{tag}/{arq}" not in trocas and antigo(arq):
                trocas[f"{REL}/{tag}/{arq}"] = f"{REL}/{tag}/{antigo(arq)}"
    print(tag, "release:", sorted(tem) or "não existe", "| servidos pelo site:", [t for t in trocas if f"/{tag}/" in t])

# instalador completo
pv = reg["pacote"]["versao"]
tag = f"DubStation-{pv}"
tem = assets(tag)
faltam = [] if reg["pacote"].get("pausado") else [f for f in ("DubStation-Windows-Instalador.exe", "DubStation-macOS-Instalador.pkg") if f not in tem]
if faltam:
    for f in do_build("build-pacote.yml", "DubStation-*"):
        if f.name in faltam:
            DL.mkdir(exist_ok=True); shutil.copy2(f, DL / f.name)
            trocas[f"{REL}/{tag}/{f.name}"] = f"downloads/{f.name}"
print(tag, "release:", sorted(tem) or "não existe")

for h in SITE.glob("*.html"):
    t = h.read_text()
    for a, b in trocas.items():
        t = t.replace(a, b)
    for z in sem_zip:
        t = re.sub(r'\n *<div class="zips">[^\n]*' + re.escape(z) + r'[^\n]*</div>', "", t)
    h.write_text(t)

# link que ainda aponta para release sem o arquivo: avisa no log do deploy
for h in SITE.glob("*.html"):
    for a in set(re.findall(re.escape(REL) + r'/[^"]+', h.read_text())):
        tag, arq = a[len(REL) + 1:].split("/", 1)
        if arq not in assets(tag):  # (links já trocados para o nome antigo existem na release)
            print(f"::warning::{h.name}: {arq} ainda não existe na release {tag} nem num build da main")
print("trocados:", len(trocas), "| zips escondidos:", len(sem_zip))
