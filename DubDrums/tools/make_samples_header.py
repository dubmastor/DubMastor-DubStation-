#!/usr/bin/env python3
"""Gera Source/DMSamples.h com os WAVs de Samples/ embutidos (kit SAMPLES DM do preview).
Uso: python3 tools/make_samples_header.py  (rodar na pasta DubDrums)"""
import os
pieces = [("kick", "DM-KICKS1.wav"), ("snare", "DM-SNARES6.wav"), ("rim", "DM-SNARES28.wav"),
          ("hat", "DM-HATs6.wav"), ("openHat", "DM-HATs39.wav")]
out = ["// Gerado por tools/make_samples_header.py a partir de Samples/*.wav — não editar à mão.",
       "// Kit SAMPLES DM do preview do Mastor Dub Drums Machine MDDM-12.",
       "#pragma once", "#include <cstddef>", "", "namespace dubdrums::dmsamples", "{"]
for name, f in pieces:
    data = open(os.path.join("Samples", f), "rb").read()
    out.append(f"    // {f}")
    out.append(f"    inline const unsigned char {name}[] = {{")
    for i in range(0, len(data), 32):
        out.append("        " + ",".join(str(b) for b in data[i:i+32]) + ",")
    out.append("    };")
    out.append(f"    inline constexpr std::size_t {name}Size = {len(data)};")
out.append("}")
open("Source/DMSamples.h", "w").write("\n".join(out) + "\n")
print("ok", os.path.getsize("Source/DMSamples.h"))
