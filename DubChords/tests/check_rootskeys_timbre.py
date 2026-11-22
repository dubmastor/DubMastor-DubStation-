#!/usr/bin/env python3
"""Confere se o som interno do MRCM-1240 usa o timbre ATUAL do Roots Keys.

Lê RootsKeys/Source/PluginProcessor.cpp (valores de abertura dos parâmetros + presets
"Piano: M1 Classic" e "Organ: Roots Percussion") e compara com as constantes de
DubChords/Source/RootsKeysSound.h (rkdefaults::RootsKeysPiano / RootsKeysOrgan).
Se alguém mudar o timbre do Roots Keys e esquecer o MRCM-1240, o CI falha aqui.

Uso (da raiz do repositório):  python3 DubChords/tests/check_rootskeys_timbre.py
"""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parents[2]
rk = (root / "RootsKeys/Source/PluginProcessor.cpp").read_text(encoding="utf-8")
syn = (root / "DubChords/Source/RootsKeysSound.h").read_text(encoding="utf-8")

def num(x):
    x = x.strip().rstrip("f").strip()
    if x in ("true", "false"):
        return x == "true"
    return float(x)

# ---- Roots Keys: valores de abertura dos parâmetros
def pct(pid):
    m = re.search(r'pct\s*\(\s*"%s"\s*,\s*"[^"]*"\s*,\s*([-0-9.]+f?)\s*\)' % pid, rk)
    if not m:
        sys.exit(f"não achei o parâmetro {pid} no Roots Keys")
    return num(m.group(1))

def param_default(pid):
    m = re.search(r'ParameterID\s*\{\s*"%s"\s*,\s*1\s*\}.*?\)\s*,\s*([-0-9.]+f?|true|false)\s*[,)]' % pid, rk, re.S)
    if not m:
        sys.exit(f"não achei o padrão de {pid} no Roots Keys")
    return num(m.group(1))

def bool_default(pid):
    m = re.search(r'AudioParameterBool>\s*\(\s*ParameterID\s*\{\s*"%s"\s*,\s*1\s*\}\s*,\s*"[^"]*"\s*,\s*(true|false)' % pid, rk)
    if not m:
        sys.exit(f"não achei o padrão de {pid} no Roots Keys")
    return m.group(1) == "true"

def preset(name):
    m = re.search(r'\{\s*"%s"\s*,(.*?)\},?\s*\n' % re.escape(name), rk)
    if not m:
        sys.exit(f'não achei o preset "{name}" no Roots Keys')
    body = m.group(1)
    dbm = re.search(r"\{([^}]*)\}", body)
    drawbars = [num(v) for v in dbm.group(1).split(",")]
    before = [v for v in body[: dbm.start()].split(",") if v.strip()]
    after = [v for v in body[dbm.end():].split(",") if v.strip()]
    mode, bright, decay, chorus, levelP = (num(v) for v in before)
    perc, p3, fast, click, drive, lesMix, levelO, lesFast, reverb = (num(v) for v in after)
    return dict(mode=mode, bright=bright, decay=decay, chorus=chorus, levelP=levelP, drawbar=drawbars,
                perc=perc, perc3rd=p3, fast=fast, click=click, drive=drive, lesMix=lesMix,
                levelO=levelO, lesFast=lesFast, reverb=reverb)

# ---- MRCM-1240: constantes do RootsKeysSound.h
def block(struct):
    m = re.search(r"struct %s\s*\{(.*?)\n\s*\};" % struct, syn, re.S)
    if not m:
        sys.exit(f"não achei {struct} no RootsKeysSound.h")
    out = {}
    for name, val in re.findall(r"static constexpr \w+\s+(\w+)(?:\[\d+\])?\s*=\s*([^;]+);", m.group(1)):
        val = val.strip()
        out[name] = [num(v) for v in val.strip("{} ").split(",")] if val.startswith("{") else num(val)
    return out

P = block("RootsKeysPiano")
O = block("RootsKeysOrgan")
classic = preset("Piano: M1 Classic")
roots = preset("Organ: Roots Percussion")

expected_piano = {
    "bright": pct("pianoBright"), "decay": pct("pianoDecay"), "chorus": pct("pianoChorus"),
    "level": pct("pianoLevel"), "reverb": pct("reverb"), "master": pct("master"),
    "maxOn": bool_default("maxOn"), "maxGainDb": param_default("maxGain"), "maxCeilDb": param_default("maxCeiling"),
}
expected_organ = {
    "drawbar": roots["drawbar"], "percOn": roots["perc"], "perc3rd": roots["perc3rd"], "percFast": roots["fast"],
    "click": roots["click"], "drive": roots["drive"], "leslieMix": roots["lesMix"], "level": roots["levelO"],
    "leslieFast": roots["lesFast"], "reverb": roots["reverb"],
}

errors = []
def same(a, b):
    if a is None or b is None:
        return False
    if isinstance(a, list) or isinstance(b, list):
        return len(a) == len(b) and all(abs(x - y) < 1e-6 for x, y in zip(a, b))
    if isinstance(a, bool) or isinstance(b, bool):
        return bool(a) == bool(b)
    return abs(a - b) < 1e-6

for k, v in expected_piano.items():
    if not same(P.get(k), v):
        errors.append(f"piano.{k}: MRCM-1240 = {P.get(k)}, Roots Keys = {v}")
for k, v in expected_organ.items():
    if not same(O.get(k), v):
        errors.append(f"orgao.{k}: MRCM-1240 = {O.get(k)}, Roots Keys = {v}")

# o timbre inicial do piano do Roots Keys tem que ser o próprio preset "Piano: M1 Classic"
for k, pk in (("bright", "bright"), ("decay", "decay"), ("chorus", "chorus"), ("level", "levelP"), ("reverb", "reverb")):
    if not same(expected_piano[k], classic[pk]):
        print(f"aviso: no Roots Keys, o padrão de {k} ({expected_piano[k]}) difere do preset M1 Classic ({classic[pk]}); vale o padrão")

print("Roots Keys -> piano inicial:", expected_piano)
print("Roots Keys -> orgao Roots Percussion:", expected_organ)
if errors:
    print("\nO TIMBRE DO MRCM-1240 ESTÁ DESATUALIZADO EM RELAÇÃO AO ROOTS KEYS:")
    for e in errors:
        print("  -", e)
    print("Atualize DubChords/Source/RootsKeysSound.h (rkdefaults).")
    sys.exit(1)
print("OK: o MRCM-1240 usa o timbre atual do Roots Keys")
