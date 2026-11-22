#!/usr/bin/env python3
"""Gera Source/BatterySamples.h com os WAVs de Samples/ embutidos (um por pad, na ordem das notas 36..51).
Para trocar um timbre: substitua o WAV em Samples/ (ou mude o nome aqui) e rode de novo.
Uso: python3 tools/make_samples_header.py  (rodar na pasta DubBattery)"""
import os

# (arquivo, nome curto no visor) — os 16 primeiros são os timbres de fábrica dos pads, na ordem das notas
# (pad 1 = nota 36 ... pad 16 = nota 51); os seguintes formam a biblioteca do < SAMPLE > e do TIMBRE 2
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
    ("DM-PRATOS1.wav",  "DM PRATO 1"),
    ("SYN-TOM1.wav",    "SINT TOM 1"),
    ("SYN-BLOCO.wav",   "SINT BLOCO"),
    # --- biblioteca: mais timbres para o < SAMPLE > e o TIMBRE 2 (layer) de qualquer pad
    ("DM-HATs1.wav",   "DM HAT 1"),
    ("DM-HATs4.wav",   "DM HAT 4"),
    ("DM-HATs5.wav",   "DM HAT 5"),
    ("DM-HATs7.wav",   "DM HAT 7"),
    ("DM-HATs8.wav",   "DM HAT 8"),
    ("DM-HATs9.wav",   "DM HAT 9"),
    ("DM-HATs10.wav",  "DM HAT 10"),
    ("DM-HATs11.wav",  "DM HAT 11"),
    ("DM-HATs12.wav",  "DM HAT 12"),
    ("DM-HATs13.wav",  "DM HAT 13"),
    ("DM-HATs14.wav",  "DM HAT 14"),
    ("DM-HATs15.wav",  "DM HAT 15"),
    ("DM-HATs16.wav",  "DM HAT 16"),
    ("DM-PRATOS2.wav", "DM PRATO 2"),
    ("DM-PRATOS3.wav", "DM PRATO 3"),
    ("DM-PRATOS4.wav", "DM PRATO 4"),
    ("DM-PRATOS5.wav", "DM PRATO 5"),
    ("SYN-PRATO.wav",   "SINT PRATO"),
]
out = ["// Gerado por tools/make_samples_header.py a partir de Samples/*.wav — não editar à mão.",
       "// Timbres de fábrica do DUB BATTERY REGGAE: 0..15 = um por pad (notas 36..51), depois a biblioteca.",
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
