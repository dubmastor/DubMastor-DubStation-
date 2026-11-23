#!/usr/bin/env python3
"""Gera os presets de groove do MRDM-12 a partir de tools/grooves.json:
  - Source/GroovePresets.h  (plugin MRDM-12, aba DUB DRUMS do Battery e aba BATERIA do Riddim Machine)
  - ../site/drums/grooves.js (versões no navegador: site/drums e site/battery)
Uso: python3 tools/make_grooves.py   (na pasta ReggaeDrumMachine)"""
import json, os

data = json.load(open(os.path.join("tools", "grooves.json"), encoding="utf-8"))["grooves"]
keys = ["drummer", "song", "artist", "year", "bpm", "approx", "style", "kick", "snare", "backbeat", "hat", "swing",
        "swingAll", "dyn", "rnd", "fill", "intro", "kit", "bars", "seed", "note"]
for i, g in enumerate(data):
    missing = [k for k in keys if k not in g]
    assert not missing, (i, missing)
    assert 0 <= g["drummer"] <= 4 and 50 <= g["bpm"] <= 100 and 1 <= g["seed"] <= 9999 and 1 <= g["bars"] <= 4, g["song"]
    assert len(g["note"]) <= 34, ("nota longa demais para o visor", g["song"])
    assert 0 <= g["style"] <= 3 and 0 <= g["hat"] <= 5 and 0 <= g["kick"] <= 255 and 0 <= g["snare"] <= 255, g["song"]
    if g["drummer"] == 4:
        assert g["kit"] == 1, g["song"]
# ordem: por baterista, mantendo a ordem do arquivo dentro de cada um
data.sort(key=lambda g: g["drummer"])

def c(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

b = lambda v: "true" if v else "false"
h = ["// Gerado por tools/make_grooves.py a partir de tools/grooves.json — não editar à mão.",
     "// Presets de groove do MRDM-12: músicas de cada baterista (visor azul do módulo BATERISTA).",
     "#pragma once", "", "namespace dubdrums", "{",
     "struct GroovePreset",
     "{",
     "    int drummer; const char* song; const char* artist; int year; int bpm; bool approx;",
     "    int style, kick, snare, backbeat, hat, swing; bool swingAll; int dyn;",
     "    int rndKick, rndSnare, rndHat, rndPerc; bool fill, intro; int kit, bars, seed;",
     "    const char* note;   // como é o groove (aparece gravado embaixo do visor)",
     "};", "",
     f"inline constexpr int numGroovePresets = {len(data)};",
     "inline const GroovePreset groovePresets[numGroovePresets] = {"]
for g in data:
    r = g["rnd"]
    h.append(f"    {{ {g['drummer']}, {c(g['song'])}, {c(g['artist'])}, {g['year']}, {g['bpm']}, {b(g['approx'])}, "
             f"{g['style']}, {g['kick']}, {g['snare']}, {g['backbeat']}, {g['hat']}, {g['swing']}, {b(g['swingAll'])}, {g['dyn']}, "
             f"{r[0]}, {r[1]}, {r[2]}, {r[3]}, {b(g['fill'])}, {b(g['intro'])}, {g['kit']}, {g['bars']}, {g['seed']}, {c(g['note'])} }},")
h += ["};", "} // namespace dubdrums", ""]
open(os.path.join("Source", "GroovePresets.h"), "w", encoding="utf-8").write("\n".join(h))

js = ["// Gerado por ReggaeDrumMachine/tools/make_grooves.py a partir de tools/grooves.json — não editar à mão.",
      "// Presets de groove do MRDM-12 (os mesmos do plugin): músicas de cada baterista.",
      "window.MDDM_GROOVES = " + json.dumps([{k: g[k] for k in keys} for g in data], ensure_ascii=False, indent=0) + ";", ""]
open(os.path.join("..", "site", "drums", "grooves.js"), "w", encoding="utf-8").write("\n".join(js))
# lista para ler: docs/GROOVES.md
names = ["Style Scott", "Santa Davis", "Carlton Barrett", "Sly Dunbar", "DIGIKAL"]
styles = ["Rockers", "One Drop", "Boom Bap", "Dembow"]
hats = ["colcheias", "semicolcheias", "contratempo", "tercinas", "flying cymbal", "abre no 1"]
md = ["# Grooves do MRDM-12", "", f"{len(data)} presets de músicas de cada baterista (visor azul do módulo BATERISTA).",
      "Gerado por `tools/make_grooves.py` a partir de `tools/grooves.json`. BPM com \"~\" é estimado.", ""]
for d, n in enumerate(names):
    rows = [g for g in data if g["drummer"] == d]
    md += [f"## {n} ({len(rows)})", "", "| Música | Artista | Ano | BPM | Groove | Crédito | Fonte |", "| --- | --- | --- | --- | --- | --- | --- |"]
    for g in rows:
        md.append(f"| {g['song']} | {g['artist']} | {g['year']} | {'~' if g['approx'] else ''}{g['bpm']} | "
                  f"{styles[g['style']]}, hi-hat {hats[g['hat']]}: {g['note'].lower()} | {g.get('credito', '')} | {g.get('fonte', '')} |")
    md.append("")
open(os.path.join("docs", "GROOVES.md"), "w", encoding="utf-8").write("\n".join(md))
print(len(data), "grooves")
