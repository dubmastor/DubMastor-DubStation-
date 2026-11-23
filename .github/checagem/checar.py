#!/usr/bin/env python3
"""Checagem de coordenação dos plugins DubMastor (roda em todo push e pull request).

Confere, para cada plugin do plugins.json:
  - nome no Cubase (PRODUCT_NAME) igual ao registro
  - versão do CMakeLists (ou Source/Version.h) igual à do site, do README, das notas de release
    e da versão para celular
  - links de download do site no padrão SIGLA-versão (instaladores e zips)
  - workflow build-<pasta>.yml existe
  - versão para celular/navegador existe em site/<web>/index.html e o site tem o botão para ela
E falha se aparecer uma pasta de plugin que não está no plugins.json.

Uso: python3 .github/checagem/checar.py   (sai com erro se algo estiver fora do padrão)
"""
import json, pathlib, re, sys

RAIZ = pathlib.Path(__file__).resolve().parents[2]
REL = "https://github.com/dubmastor/DubMastor-DubStation-/releases/download"
erros, avisos = [], []

def err(sig, msg): erros.append(f"{sig}: {msg}")
def warn(sig, msg): avisos.append(f"{sig}: {msg}")

def versao(pasta):
    cm = (RAIZ / pasta / "CMakeLists.txt").read_text()
    m = re.search(r"^project\(\w+ VERSION (\d+)\.(\d+)", cm, re.M)
    if m:
        return f"{m.group(1)}.{m.group(2)}"
    vh = RAIZ / pasta / "Source/Version.h"
    if vh.exists():
        m = re.search(r'"(\d+)\.(\d+)', vh.read_text())
        if m:
            return f"{m.group(1)}.{m.group(2)}"
    return None

reg = json.loads((RAIZ / "plugins.json").read_text())["plugins"]
# o site tem uma página por categoria (efeitos, instrumentos, machines), a home e a instalação
site = "\n".join(f.read_text() for f in sorted((RAIZ / "site").glob("*.html")))
readme = (RAIZ / "README.md").read_text()

for p in reg:
    sig, pasta, nome = p["sigla"], p["pasta"], p["nome"]
    if p.get("legado"):
        # machine que virou aba de um instrumento: o código continua (outros plugins compilam a pasta),
        # mas o plugin separado saiu do site e do pacote; a última release fica como legado
        warn(sig, f"legado: virou parte do {p['legado']} (sem card no site, fora do pacote)")
        continue
    cm_path = RAIZ / pasta / "CMakeLists.txt"
    if not cm_path.exists():
        err(sig, f"pasta {pasta}/ sem CMakeLists.txt"); continue
    cm = cm_path.read_text()

    m = re.search(r'PRODUCT_NAME\s+"([^"]+)"', cm)
    if not m or m.group(1) != nome:
        err(sig, f"PRODUCT_NAME no CMakeLists é {m.group(1) if m else '(nenhum)'!r}, o registro diz {nome!r}")
    if p.get("padrao") is not False and (not nome.startswith("Mastor ") or not nome.endswith(" " + sig)):
        err(sig, f"nome fora do padrão 'Mastor <Nome> <SIGLA>': {nome!r}")

    v = versao(pasta)
    if not v:
        err(sig, "versão não encontrada no CMakeLists nem em Source/Version.h"); continue

    if p.get("padrao") is False:
        warn(sig, "plugin ainda fora do padrão de nomes e releases ('padrao': false no plugins.json)")
        if not re.search(r'<h3>' + re.escape(nome) + r' <small>' + re.escape(v) + r'</small></h3>', site):
            err(sig, f"o site não mostra '{nome}' na versão {v}")
        continue

    if not (RAIZ / f".github/workflows/build-{pasta.lower()}.yml").exists():
        err(sig, f"falta o workflow .github/workflows/build-{pasta.lower()}.yml")

    # site: card com nome + versão e links da versão atual ("oculto": true = fora do site por enquanto)
    if p.get("oculto"):
        warn(sig, "oculto: fora do site por enquanto (sem card)")
        if f"<h3>{nome} <small>" in site:
            err(sig, "está marcado como oculto, mas o site ainda tem o card")
    card = None if p.get("oculto") else re.search(r'(?s)<article class="card[^"]*"[^>]*>(?:(?!</article>).)*?<h3>' + re.escape(nome) + r' <small>([^<]+)</small></h3>.*?</article>', site)
    if not card and not p.get("oculto"):
        err(sig, "o site não tem o card deste plugin (título '<h3>nome <small>versão</small></h3>')")
    elif card:
        c = card.group(0)
        if card.group(1) != v:
            err(sig, f"site mostra a versão {card.group(1)}, o código está na {v}")
        for f in (f"{sig}-Windows-Instalador.exe", f"{sig}-macOS-Instalador.pkg", f"{sig}-Windows.zip", f"{sig}-macOS.zip"):
            if f"{REL}/{sig}-{v}/{f}" not in c:
                err(sig, f"site sem o link {sig}-{v}/{f}")
        web = p.get("web")
        if web and web != "pendente":
            if f'href="{web}/"' not in c:
                err(sig, f"site sem o botão da versão para celular ({web}/)")
    # README
    if f"{nome} {v}" not in readme:
        err(sig, f"README.md não cita '{nome} {v}'")
    # notas de release
    rn = RAIZ / pasta / "RELEASE_NOTES.md"
    if not rn.exists():
        err(sig, "falta RELEASE_NOTES.md")
    elif v not in "\n".join(rn.read_text().splitlines()[:3]):
        err(sig, f"RELEASE_NOTES.md não começa com a versão {v}")
    # versão para celular / navegador
    web = p.get("web")
    if not web:
        err(sig, "sem versão para celular: todo plugin novo precisa de site/web/<nome>/index.html (campo 'web' no plugins.json)")
    elif web == "pendente":
        warn(sig, "versão para celular/navegador ainda pendente")
    else:
        idx = RAIZ / "site" / web / "index.html"
        if not idx.exists():
            err(sig, f"versão para celular não encontrada em site/{web}/index.html")
        else:
            h = idx.read_text()
            mv = re.search(r"VERSION\s*=\s*['\"](\d+\.\d+)", h) or re.search(r"<title>[^<]*" + re.escape(sig) + r"\s+(\d+\.\d+)", h)
            if not mv:
                err(sig, f"site/{web}/index.html sem 'const VERSION = \"x.y\"' (ou a versão no <title>)")
            elif mv.group(1) != v:
                err(sig, f"versão para celular está na {mv.group(1)}, o plugin está na {v}")
            if sig not in h:
                err(sig, f"site/{web}/index.html não mostra a sigla {sig}")

# pasta de plugin nova sem registro
registradas = {p["pasta"] for p in reg}
for cm in RAIZ.glob("*/CMakeLists.txt"):
    if "juce_add_plugin" in cm.read_text() and cm.parent.name not in registradas:
        err(cm.parent.name, "plugin novo fora do plugins.json: registre sigla, nome, pasta e versão para celular")

# instalador completo (todos os plugins): versão de cada plugin registrada no pacote e links no site
pac = json.loads((RAIZ / "plugins.json").read_text()).get("pacote")
if not pac:
    err("DubStation", "plugins.json sem o bloco 'pacote' do instalador completo")
else:
    pv = pac["versao"]
    for p in reg:
        if p.get("legado"):
            if p["sigla"] in pac["plugins"]:
                err("DubStation", f"{p['sigla']} é legado e não pode estar no instalador completo")
            continue
        v = versao(p["pasta"])
        if pac["plugins"].get(p["sigla"]) != v:
            err("DubStation", f"{p['sigla']} está na {v}, o instalador completo registra {pac['plugins'].get(p['sigla'])}: "
                "atualize 'pacote' → 'plugins' e suba 'pacote' → 'versao' no plugins.json")
    if pac.get("pausado"):
        # instalador completo pausado: fica fora do site até ser liberado (tirar "pausado" e pôr os links de volta)
        warn("DubStation", f"instalador completo {pv} pausado (sem link no site)")
        if f"{REL}/DubStation-" in site:
            err("DubStation", "pacote pausado, mas o site ainda tem link para o instalador completo")
    for f in (() if pac.get("pausado") else ("DubStation-Windows-Instalador.exe", "DubStation-macOS-Instalador.pkg")):
        if f"{REL}/DubStation-{pv}/{f}" not in site:
            err("DubStation", f"site sem o link do instalador completo DubStation-{pv}/{f}")
    if not (RAIZ / ".github/workflows/build-pacote.yml").exists():
        err("DubStation", "falta o workflow build-pacote.yml")

for a in avisos:
    print(f"::warning::{a}")
for e in erros:
    print(f"::error::{e}")
print(f"\n{len(reg)} plugins conferidos · {len(erros)} erro(s) · {len(avisos)} aviso(s)")
sys.exit(1 if erros else 0)
