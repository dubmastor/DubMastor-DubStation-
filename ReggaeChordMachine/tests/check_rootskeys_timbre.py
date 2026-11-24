#!/usr/bin/env python3
"""Confere se o som interno do MRCM-1240 usa o timbre ATUAL do Roots Keys.

Lê ReggaePiano/Source/PluginProcessor.cpp (valores de abertura dos parâmetros + preset "Piano: M1 Classic")
e compara com as constantes de ReggaeChordMachine/Source/RootsKeysSound.h (rkdefaults::RootsKeysPiano).
O órgão saiu do piano na 3.1 (Mastor Reggae Piano MRP-80): o Hammond do MRCM-1240 (rkdefaults::RootsKeysOrgan)
fica com os valores do preset "Roots Percussion" e não é mais conferido aqui.
Se alguém mudar o timbre do Roots Keys e esquecer o MRCM-1240, o CI falha aqui.

Uso (da raiz do repositório):  python3 ReggaeChordMachine/tests/check_rootskeys_timbre.py
"""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parents[2]
rk = (root / "ReggaePiano/Source/PluginProcessor.cpp").read_text(encoding="utf-8")
syn = (root / "ReggaeChordMachine/Source/RootsKeysSound.h").read_text(encoding="utf-8")

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

def choice_default(pid):
    m = re.search(r'ParameterID\s*\{\s*"%s"\s*,\s*1\s*\}\s*,\s*"[^"]*"\s*,\s*StringArray\s*\{[^}]*\}\s*,\s*(\d+)' % pid, rk, re.S)
    if not m:
        return 0   # Roots Keys antigo, sem esse parâmetro: só existia o M1
    return int(m.group(1))

def preset(name, required=True):
    m = re.search(r'\{\s*"%s"\s*,(.*?)\},?\s*\n' % re.escape(name), rk)
    if not m:
        sys.exit(f'não achei o preset "{name}" no piano')
    bright, decay, chorus, levelP, reverb = (num(v) for v in m.group(1).split(",") if v.strip())
    return dict(bright=bright, decay=decay, chorus=chorus, levelP=levelP, reverb=reverb)

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
classic = preset("Piano: M1 Classic")

expected_piano = {
    "type": choice_default("pianoType"),
    "bright": pct("pianoBright"), "decay": pct("pianoDecay"), "chorus": pct("pianoChorus"),
    "level": pct("pianoLevel"), "reverb": pct("reverb"), "master": pct("master"),
    "maxOn": bool_default("maxOn"), "maxGainDb": param_default("maxGain"), "maxCeilDb": param_default("maxCeiling"),
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

# o timbre inicial do piano do Roots Keys tem que ser o próprio preset "Piano: M1 Classic"
for k, pk in (("bright", "bright"), ("decay", "decay"), ("chorus", "chorus"), ("level", "levelP"), ("reverb", "reverb")):
    if not same(expected_piano[k], classic[pk]):
        print(f"aviso: no Roots Keys, o padrão de {k} ({expected_piano[k]}) difere do preset M1 Classic ({classic[pk]}); vale o padrão")

print("Roots Keys -> piano inicial:", expected_piano)
if errors:
    print("\nO TIMBRE DO MRCM-1240 ESTÁ DESATUALIZADO EM RELAÇÃO AO ROOTS KEYS:")
    for e in errors:
        print("  -", e)
    print("Atualize ReggaeChordMachine/Source/RootsKeysSound.h (rkdefaults).")
    sys.exit(1)
print("OK: o MRCM-1240 usa o timbre atual do Roots Keys")
