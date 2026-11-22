#!/usr/bin/env python3
"""Banco de PRESETS de harmonia da DubStation (MRCM-1240 e MRM-0420): 6 pastas por estilo e década, 10 músicas cada.

Fonte única: gera Source/HarmonyPresets.h (plugins) e ../site/chords/presets.js (versões do navegador).
Uso: python3 tools/harmony_presets.py   (rodar na pasta ReggaeChordMachine)

Cada preset é a base harmônica de uma música real, no tom da gravação original, com 1 a 4 acordes (tônica,
maior/menor, duração em compassos). Sétimas viram tríades (o MRCM-1240 toca tríades); quando a música tem
mais partes, fica o groove principal (verso ou riff). Conferido em nov/2026 em cifras públicas (Ultimate
Guitar, E-Chords, GuitarTabs.cc, GuitarTabsExplorer, Ukulele-Tabs); músicas sem cifra confiável ficaram de fora.
"""
import json, os

ROOTS = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'Ab': 8, 'A': 9, 'Bb': 10, 'B': 11}
DURS = {'1/2': 0, '1': 1, '2': 2, '3': 3, '4': 4, '8': 5}

R70, RK, RD, LR, DH, NR = ('ROOTS - ANOS 70', 'ROCKERS - FIM DOS 70', 'RUB A DUB - ANOS 80',
                           'LOVERS ROCK - 70 A 90', 'DANCEHALL - 80 E 90', 'NEW ROOTS - ANOS 2000')

# (pasta, década, nome, "acorde:duração ...")
BANK = [
    (R70, '70s', 'Satta Massagana (Abyssinians 1969)', 'Am:1 Dm:1 Am:1 Em:1'),
    (R70, '70s', 'Pressure Drop (Toots 1969)',         'Ab:1 Eb:1 Db:1 Ab:1'),
    (R70, '70s', 'Marcus Garvey (Burning Spear 1975)', 'Am:1 G:1'),
    (R70, '70s', 'Natural Mystic (Bob Marley 1977)',   'Dm:1 Em:1 Am:2'),
    (R70, '70s', 'Exodus (Bob Marley 1977)',           'Am:4 C:1/2 Dm:1/2 Am:1'),
    (R70, '70s', 'Jamming (Bob Marley 1977)',          'Bm:1 E:1 G:1 F#m:1'),
    (R70, '70s', 'One Love (Bob Marley 1977)',         'Bb:1 F:1 Eb:1 Bb:1'),
    (R70, '70s', 'Smile Jamaica (Bob Marley 1976)',    'Bm:1 G:1 A:1'),
    (R70, '70s', 'Police and Thieves (Junior Murvin 1976)', 'G:1 Am:1 D:1 Am:1'),
    (R70, '70s', 'Roots (Bob Marley 1977)',            'F#m:1 Bm:1 D:1 C#m:1'),

    (RK, '70s', 'Legalize It (Peter Tosh 1976)',       'G:1 F:1'),
    (RK, '70s', 'Burial (Peter Tosh 1976)',            'Bb:1 Gm:1 Eb:1 F:1'),
    (RK, '70s', 'Uptown Top Ranking (Althea & Donna 1977)', 'Am:1 G:1'),
    (RK, '70s', 'Money in My Pocket (Dennis Brown 1979)', 'F:1 Gm:1 Dm:1/2 C:1/2'),
    (RK, '70s', 'Now That We Found Love (Third World 1978)', 'Cm:1 Fm:1 Gm:1'),
    (RK, '70s', 'Ku Klux Klan (Steel Pulse 1978)',     'Am:1 Em:1'),
    (RK, '70s', 'Soldiers (Steel Pulse 1978)',         'Bm:1 F#:1 G:1 A:1'),
    (RK, '70s', 'One Drop (Bob Marley 1979)',          'C:1 Am:1 Dm:1 G:1'),
    (RK, '80s', 'Could You Be Loved (Bob Marley 1980)', 'Bm:1 Em:1 D:1 Bm:1'),
    (RK, '80s', 'Buffalo Soldier (Bob Marley 1983)',   'A:1 F#m:1 D:1 A:1'),

    (RD, '80s', 'Night Nurse (Gregory Isaacs 1982)',   'Am:1 G:1 Dm:1'),
    (RD, '80s', 'Here I Come (Barrington Levy 1984)',  'Em:1 Am:1'),
    (RD, '80s', 'Under Mi Sensi (Barrington Levy 1985)', 'C:1 F:1 G:1'),
    (RD, '80s', 'Sensimilla (Barrington Levy 1980)',   'Fm:1 Cm:1'),
    (RD, '80s', 'Pass the Kouchie (Mighty Diamonds 1981)', 'C:1 Dm:1'),
    (RD, '80s', 'Bam Bam (Sister Nancy 1982)',         'C:1 Bb:1'),
    (RD, '80s', 'Cool Down the Pace (Gregory Isaacs 1982)', 'Bb:1 Cm:1'),
    (RD, '80s', 'Truths and Rights (Johnny Osbourne 1980)', 'C#m:1 E:1 Ab:1'),
    (RD, '80s', 'Sinsemilla (Black Uhuru 1980)',       'Am:1 Em:1 Am:1 G:1'),
    (RD, '70s', 'Shine Eye Gal (Black Uhuru 1979)',    'Cm:2 Gm:2'),

    (LR, '70s', 'Help Me Make It Through the Night (John Holt 1974)', 'E:1 A:1 F#m:1 B:1'),
    (LR, '70s', 'Is This Love (Bob Marley 1978)',      'F#m:2 D:1 A:1'),
    (LR, '70s', 'Waiting in Vain (Bob Marley 1977)',   'Ab:1 Db:1'),
    (LR, '80s', 'Red Red Wine (UB40 1983)',            'Db:1 Gb:1 Ab:1 Gb:1'),
    (LR, '80s', 'Kingston Town (UB40 1989)',           'Ab:1 Cm:1 Db:1 Eb:1'),
    (LR, '90s', 'Close to You (Maxi Priest 1990)',     'Ebm:2 Abm:1 Bbm:1'),
    (LR, '90s', 'Rockaway (Beres Hammond 1994)',       'Ab:1 Eb:1 Fm:1 Db:1'),
    (LR, '90s', 'Tempted to Touch (Beres Hammond 1990)',       'C:1 Am:1 Dm:1 G:1'),
    (LR, '90s', 'My Number One (Gregory Isaacs 1990)', 'G:1 Am:1 D:1'),
    (LR, '80s', 'Wild World (Maxi Priest 1988)',       'Am:1 D:1 G:1 C:1'),

    (DH, '80s', 'Ring the Alarm (Tenor Saw 1985)',     'C:1 Bb:1'),
    (DH, '90s', 'Mr. Loverman (Shabba Ranks 1992)',    'Am:1 Dm:1 G:1 Am:1'),
    (DH, '90s', 'Informer (Snow 1992)',                'F#:1 E:1'),
    (DH, '90s', 'Sweat (Inner Circle 1992)',           'Bb:1 Gm:1 Cm:1 F:1'),
    (DH, '90s', 'Here Comes the Hotstepper (Ini Kamoze 1994)', 'Am:1 G:1'),
    (DH, '90s', "You Don't Love Me (Dawn Penn 1994)",  'Am:1 D:1'),
    (DH, '90s', 'Boombastic (Shaggy 1995)',            'D:1 G:1 A:1'),
    (DH, '00s', "It Wasn't Me (Shaggy 2000)",          'C:1 G:1 F:1'),
    (DH, '00s', 'Get Busy (Sean Paul 2003)',           'Fm:1 Gb:1'),
    (DH, '00s', 'Temperature (Sean Paul 2005)',        'Bbm:1 Ab:1 Bbm:1 C:1'),

    (NR, '00s', "She's Royal (Tarrus Riley 2006)",     'Eb:1 Ab:1 Eb:1 Bb:1'),
    (NR, '00s', 'Welcome to Jamrock (Damian Marley 2005)', 'Gm:1 Dm:1'),
    (NR, '00s', 'Herbalist (Alborosie 2008)',          'Fm:1 Bb:1'),
    (NR, '00s', 'Kingston Town (Alborosie 2008)',      'Em:1 Am:1'),
    (NR, '00s', 'Mystical Reggae (Alborosie 2009)',    'Bm:1 G:1 A:1'),
    (NR, '10s', 'Callin (Alborosie 2011)',                 'Am:1 Dm:1'),
    (NR, '00s', 'Ganja Farmer (Marlon Asher 2005)',    'C:1 Em:1 Am:1 F:1'),
    (NR, '10s', 'Dem Gone (Kabaka Pyramid 2018)',      'Am:1 G:1 F:1'),
    (NR, '10s', 'Access Granted (Protoje 2013)',       'Gm:1 Cm:1 Gm:1 F:1'),
    (NR, '10s', 'Smile Jamaica (Chronixx 2014)',       'G:1 C:1 Em:1 D:1'),
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

for f in (R70, RK, RD, LR, DH, NR):
    assert sum(1 for b in BANK if b[0] == f) == 10, f

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
