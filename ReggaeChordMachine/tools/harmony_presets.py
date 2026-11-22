#!/usr/bin/env python3
"""Banco de PRESETS de harmonia da DubStation (MRCM-1240 e MRM-0420), separado por estilo (pastas) e década.

Fonte única: gera Source/HarmonyPresets.h (plugins) e ../site/chords/presets.js (versões do navegador).
Uso: python3 tools/harmony_presets.py   (rodar na pasta DubChords)

Cada preset tem de 1 a 4 acordes (tônica, maior/menor, duração). Os acordes com sétima viram maior ou menor
(o MRCM-1240 toca tríades). As progressões são as típicas de cada estilo; quando o nome cita um riddim ou uma
música, a harmonia é a base dele simplificada (tom e duração podem ter sido ajustados para o loop).

Pesquisa (nov/2026): progressões comuns por subgênero (I-IV roots/one drop, I-vi-IV-V rocksteady e lovers
rock, i-VII-VI-V dub, I-V-vi-IV crossover), cifras de Satta Massagana (Am Dm Em), Marcus Garvey (Am G, Dm C Am),
Night Nurse (Am G Dm), Stalag/Arleen (Cm Bb), Real Rock / Armagideon Time (Bm F#m), Cuss Cuss (Dm F G).
"""
import json, os

ROOTS = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'Ab': 8, 'A': 9, 'Bb': 10, 'B': 11}
DURS = {'1/2': 0, '1': 1, '2': 2, '3': 3, '4': 4, '8': 5}

# (pasta, década, nome, "acorde:duração ...")
BANK = [
    # ROOTS — anos 70 (Studio One, Black Ark, Burning Spear, Abyssinians, Culture)
    ('ROOTS', '70s', 'Satta (Abyssinians)',          'Am:1 Dm:1 Am:1/2 Em:1/2'),
    ('ROOTS', '70s', 'Marcus Garvey (Burning Spear)', 'Am:1 G:1'),
    ('ROOTS', '70s', 'Garvey coro',                   'Dm:1/2 C:1/2 Am:1'),
    ('ROOTS', '70s', 'Nyabinghi (um acorde)',         'Em:2'),
    ('ROOTS', '70s', 'Menor i-iv',                    'Gm:1 Cm:1'),
    ('ROOTS', '70s', 'Studio One I-IV',               'C:1 F:1'),
    ('ROOTS', '70s', 'Wailers i-VII-VI',              'Am:1 G:1/2 F:1/2'),
    ('ROOTS', '70s', 'Real Rock (Bm-F#m)',            'Bm:1 F#m:1'),
    # ROCKERS — 1975-79 (Channel One, Revolutionaries, Sly & Robbie, Augustus Pablo)
    ('ROCKERS', '70s', 'Stalag (Cm-Bb)',              'Cm:1 Bb:1'),
    ('ROCKERS', '70s', 'Channel One i-VII',           'Gm:1/2 F:1/2'),
    ('ROCKERS', '70s', 'Rockers menor (Em-Am)',       'Em:1 Am:1'),
    ('ROCKERS', '70s', 'Revolutionaries I-IV-V',      'G:1 C:1/2 D:1/2'),
    ('ROCKERS', '70s', 'Dorico (Dm-G)',               'Dm:1 G:1'),
    ('ROCKERS', '70s', 'Melodica (Am-Em)',            'Am:1 Em:1'),
    # RUB A DUB — 1979-84 (Roots Radics, Junjo Lawes, Volcano, Jah Thomas)
    ('RUB A DUB', '80s', 'Night Nurse (Am-G)',        'Am:1 G:1'),
    ('RUB A DUB', '80s', 'Night Nurse ponte',         'Dm:1/2 Am:1/2 G:1'),
    ('RUB A DUB', '80s', 'Cuss Cuss (Dm-F-G)',        'Dm:1 F:1/2 G:1/2'),
    ('RUB A DUB', '80s', 'Roots Radics (Fm-Eb)',      'Fm:1 Eb:1'),
    ('RUB A DUB', '80s', 'Rub a dub i-iv',            'Dm:1 Gm:1'),
    ('RUB A DUB', '80s', 'Dancehall 82 (Em-D)',       'Em:1 D:1'),
    ('RUB A DUB', '80s', 'Volcano (Cm-Fm)',           'Cm:1 Fm:1/2 Bb:1/2'),
    # LOVERS ROCK — UK 1975-90 (Dennis Bovell, Janet Kay, Carroll Thompson, Maxi Priest)
    ('LOVERS ROCK', '70s', 'I-vi-IV-V (G-Em-C-D)',    'G:1 Em:1 C:1 D:1'),
    ('LOVERS ROCK', '80s', 'Lovers I-IV-V (A-D-E)',   'A:1 D:1/2 E:1/2'),
    ('LOVERS ROCK', '80s', 'Soul ii-V-I (Dm-G-C)',    'Dm:1 G:1 C:2'),
    ('LOVERS ROCK', '80s', 'vi-IV-I-V (Am-F-C-G)',    'Am:1 F:1 C:1 G:1'),
    ('LOVERS ROCK', '80s', 'IV-V-iii-vi (C-D-Bm-Em)', 'C:1/2 D:1/2 Bm:1/2 Em:1/2'),
    ('LOVERS ROCK', '90s', 'Crossover I-V-vi-IV (Bb)', 'Bb:1 F:1 Gm:1 Eb:1'),
    # DANCEHALL DIGITAL — 1985-99 (Jammy's, Techniques, Steely & Clevie, Sly & Robbie)
    ('DANCEHALL DIGITAL', '80s', 'Digital 85 (um acorde)', 'Gm:2'),
    ('DANCEHALL DIGITAL', '80s', 'Ring the Alarm (Stalag 85)', 'Cm:1 Bb:1'),
    ('DANCEHALL DIGITAL', '90s', 'Ragga i-VII (Dm-C)',      'Dm:1 C:1'),
    ('DANCEHALL DIGITAL', '90s', 'Dancehall 92 (Dm-Gm-C)',  'Dm:1 Gm:1/2 C:1/2'),
    ('DANCEHALL DIGITAL', '90s', 'Real Rock 94 (Bm-F#m)',   'Bm:1 F#m:1'),
    ('DANCEHALL DIGITAL', '90s', 'Ragga menor (Am-Em-F-G)', 'Am:1/2 Em:1/2 F:1/2 G:1/2'),
    # NEW ROOTS / ONE DROP — 2000s (Xterminator, Lion Paw, Tarrus Riley, Gentleman, Alborosie)
    ('NEW ROOTS 2000', '00s', 'Full Up 2000 (Am-Dm)',    'Am:1 Dm:1'),
    ('NEW ROOTS 2000', '00s', 'One drop i-VI-VII (Em)',  'Em:1 C:1/2 D:1/2'),
    ('NEW ROOTS 2000', '00s', 'Juggling (Gm-Eb-F)',      'Gm:1 Eb:1/2 F:1/2'),
    ('NEW ROOTS 2000', '00s', 'Conscious (Bm-G-A)',      'Bm:1 G:1/2 A:1/2'),
    ('NEW ROOTS 2000', '00s', 'Pop reggae I-V-vi-IV (C)', 'C:1 G:1 Am:1 F:1'),
    ('NEW ROOTS 2000', '00s', 'Neo roots i-iv-v (Am)',   'Am:1 Dm:1/2 Em:1/2'),
]

def parse(spec):
    out = []
    for tok in spec.split():
        name, dur = tok.split(':')
        minor = name.endswith('m')
        root = ROOTS[name[:-1] if minor else name]
        out.append((root, 1 if minor else 0, DURS[dur]))
    assert 1 <= len(out) <= 4, spec
    return out

presets = [{'folder': f, 'era': e, 'name': n, 'chords': parse(s)} for f, e, n, s in BANK]
folders = []
for p in presets:
    if p['folder'] not in folders: folders.append(p['folder'])

here = os.path.dirname(os.path.abspath(__file__))
h = ['// Gerado por tools/harmony_presets.py — não editar à mão.',
     '// Banco de PRESETS de harmonia da DubStation (MRCM-1240 e MRM-0420), por estilo e década.',
     '#pragma once', '', 'namespace dubpresets', '{',
     'struct Chord { int root; int minor; int dur; };   // tônica 0..11 (C = 0), 1 = menor, duração 0..5 (1/2, 1, 2, 3, 4, 8 comp.)',
     'struct Preset { const char* folder; const char* era; const char* name; int numChords; Chord chords[4]; };', '',
     f'inline constexpr int numFolders = {len(folders)};',
     'inline const char* const folders[numFolders] = { ' + ', '.join(json.dumps(f) for f in folders) + ' };', '',
     f'inline constexpr int numPresets = {len(presets)};',
     'inline const Preset presets[numPresets] = {']
for p in presets:
    ch = ', '.join(f'{{ {r}, {m}, {d} }}' for r, m, d in p['chords'])
    h.append(f'    {{ {json.dumps(p["folder"])}, {json.dumps(p["era"])}, {json.dumps(p["name"], ensure_ascii=False)}, {len(p["chords"])}, {{ {ch} }} }},')
h += ['};', '}', '']
open(os.path.join(here, '..', 'Source', 'HarmonyPresets.h'), 'w', encoding='utf-8').write('\n'.join(h))

js = ('// Gerado por ReggaeChordMachine/tools/harmony_presets.py — não editar à mão.\n'
      '// Banco de PRESETS de harmonia da DubStation (MRCM-1240 e MRM-0420), por estilo e década.\n'
      '(function (root) {\n  root.DUB_PRESETS = ' +
      json.dumps({'folders': folders, 'presets': [{'folder': p['folder'], 'era': p['era'], 'name': p['name'],
                  'chords': [{'root': r, 'quality': m, 'dur': d} for r, m, d in p['chords']]} for p in presets]},
                 ensure_ascii=False, indent=1) + ';\n})(typeof window !== "undefined" ? window : globalThis);\n')
open(os.path.join(here, '..', '..', 'site', 'chords', 'presets.js'), 'w', encoding='utf-8').write(js)
print('ok', len(presets), 'presets em', len(folders), 'pastas')
