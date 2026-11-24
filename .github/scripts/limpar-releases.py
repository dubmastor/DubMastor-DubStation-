#!/usr/bin/env python3
"""Deixa só a ÚLTIMA release de cada plugin / machine (e do instalador completo) e apaga o resto, com a tag.

Fica:  a maior versão de cada sigla do plugins.json, do MM-2500 e do DubStation, e qualquer release que o site,
       os README ou as notas ainda linkam (releases/download/<tag>/...). Sai todo o resto, inclusive as releases
       com os nomes antigos (DBR, DubBass, RootsKeys, BassMachine...).
Uso:   python3 .github/scripts/limpar-releases.py            -> só mostra o que sairia
       python3 .github/scripts/limpar-releases.py --apagar   -> apaga (precisa do gh com GH_TOKEN e contents: write)
"""
import json, pathlib, re, subprocess, sys

RAIZ = pathlib.Path(__file__).resolve().parents[2]
REPO = "dubmastor/DubMastor-DubStation-"


def gh(*args):
    return subprocess.run(["gh", *args], check=True, capture_output=True, text=True).stdout


def versao(tag):
    m = re.match(r"^(.*?)-v?(\d+(?:\.\d+)*)$", tag)
    return (m.group(1), tuple(int(x) for x in m.group(2).split("."))) if m else (tag, ())


def main():
    apagar = "--apagar" in sys.argv
    reg = json.loads((RAIZ / "plugins.json").read_text())
    siglas = {p["sigla"] for p in reg["plugins"]} | {"MM-2500", "DubStation"}

    tags = []
    pagina = 1
    while True:
        lote = json.loads(gh("api", f"repos/{REPO}/releases?per_page=100&page={pagina}"))
        if not lote:
            break
        tags += [r["tag_name"] for r in lote]
        pagina += 1

    linkadas = set()
    for f in list(RAIZ.glob("site/**/*.html")) + list(RAIZ.glob("**/*.md")):
        if ".git" in f.parts:
            continue
        linkadas |= set(re.findall(r"releases/download/([^/\"'\s)]+)/", f.read_text(errors="ignore")))

    ultima = {}
    for t in tags:
        nome, v = versao(t)
        if nome in siglas and (nome not in ultima or v > ultima[nome][1]):
            ultima[nome] = (t, v)
    fica = {t for t, _ in ultima.values()} | (linkadas & set(tags))
    sai = [t for t in tags if t not in fica]

    print(f"{len(tags)} releases · ficam {len(fica)} · saem {len(sai)}")
    print("FICAM:", " ".join(sorted(fica)))
    print("SAEM: ", " ".join(sai) or "(nenhuma)")
    if not apagar:
        print("\n(só mostrando: rode com --apagar para apagar)")
        return
    for t in sai:
        subprocess.run(["gh", "release", "delete", t, "--repo", REPO, "--yes", "--cleanup-tag"], check=False)
        print("apagada", t)


if __name__ == "__main__":
    main()
