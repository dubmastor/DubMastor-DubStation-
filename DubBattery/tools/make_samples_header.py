#!/usr/bin/env python3
"""Gera Source/BatterySamples.h com os WAVs de Samples/ embutidos (um por pad, na ordem das notas 36..51).
Para trocar um timbre: substitua o WAV em Samples/ (ou mude o nome aqui) e rode de novo.
Uso: python3 tools/make_samples_header.py  (rodar na pasta DubBattery)"""
import os

# (arquivo, nome curto no visor) — a posição na lista é o pad (pad 1 = nota 36 ... pad 16 = nota 51)
samples = [
    ("DM-KICKS1.wav",   "DM KICK 1"),
    ("DM-SNARES28.wav", "DM ARO 28"),
    ("DM-SNARES6.wav",  "DM CAIXA 6"),
    ("SYN-SNARE2.wav",  "SINT CAIXA 2"),
    ("SYN-SNARE3.wav",  "SINT CAIXA 3"),
    ("SYN-KICK2.wav",   "SINT BUMBO 2"),
    ("DM-HATs6.wav",    "DM HAT 6"),
    ("SYN-SURDO.wav",   "SINT SURDO"),
    ("DM-HATs39.wav",   "DM HAT 39"),
    ("SYN-KICK3.wav",   "SINT BUMBO 3"),
    ("DM-HATs17.wav",   "DM HAT 17 ABERTO"),
    ("SYN-TOM2.wav",    "SINT TOM 2"),
    ("SYN-SHAKER.wav",  "SINT SHAKER"),
    ("SYN-PRATO.wav",   "SINT PRATO"),
    ("SYN-TOM1.wav",    "SINT TOM 1"),
    ("SYN-BLOCO.wav",   "SINT BLOCO"),
]
out = ["// Gerado por tools/make_samples_header.py a partir de Samples/*.wav — não editar à mão.",
       "// Timbres de fábrica do DUB BATTERY REGGAE (um por pad, notas 36..51).",
       "#pragma once", "#include <cstddef>", "", "namespace dbr::factory", "{"]
for i, (f, _) in enumerate(samples):
    data = open(os.path.join("Samples", f), "rb").read()
    out.append(f"    // {f}")
    out.append(f"    inline const unsigned char wav{i}[] = {{")
    for k in range(0, len(data), 40):
        out.append("        " + ",".join(str(b) for b in data[k:k + 40]) + ",")
    out.append("    };")
out.append("")
out.append("    struct Entry { const unsigned char* data; std::size_t size; const char* file; const char* name; };")
out.append(f"    inline constexpr int numSamples = {len(samples)};")
out.append("    inline const Entry samples[numSamples] = {")
for i, (f, n) in enumerate(samples):
    out.append(f"        {{ wav{i}, sizeof (wav{i}), \"{f}\", \"{n}\" }},")
out.append("    };")
out.append("}")
open("Source/BatterySamples.h", "w").write("\n".join(out) + "\n")
print("ok", os.path.getsize("Source/BatterySamples.h"))
