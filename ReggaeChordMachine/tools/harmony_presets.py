#!/usr/bin/env python3
"""Banco de RIDDIMS (PRESETS) da DubStation (MRCM-1240 e MRM-0420): 9 pastas por estilo e época, 10 músicas cada.

Fonte única: gera Source/HarmonyPresets.h (plugins) e ../site/chords/presets.js (versões do navegador).
Uso: python3 tools/harmony_presets.py   (rodar na pasta ReggaeChordMachine)

Cada preset é a base de uma música real, no tom da gravação original: 1 a 4 acordes (tônica, maior/menor,
duração em compassos), o BPM sugerido (contado como a bateria do MRDM-12 conta: caixa no 2 e no 4), o estilo de
bateria (ROCKERS ou ONE DROP) e os músicos das máquinas mais próximos de quem tocou (baterista do MRDM-12 e
baixista do MRBM-69). O MRCM-1240 usa só os acordes; o MRM-0420 usa tudo.
Sétimas viram tríades (o MRCM-1240 toca tríades); quando a música tem mais partes, fica o groove principal.

Fontes (nov/2026): cifras feitas por pessoas (Ultimate Guitar, E-Chords, GuitarTabs.cc, GuitarTabsExplorer,
Ukulele-Tabs, Recifra) e, onde marcado [auto], detecção automática (ChordU/ChordIntel), que vale conferir de ouvido.
O BPM é o andamento típico do estilo e da época, não uma medição de cada gravação.
"""
import json, os

ROOTS = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'Ab': 8, 'A': 9, 'Bb': 10, 'B': 11}
DURS = {'1/2': 0, '1': 1, '2': 2, '3': 3, '4': 4, '8': 5}
KIT = {'RK': 0, 'OD': 1}                                                     # estilo do MRDM-12
DRUM = {'STYLE': 0, 'SANTA': 1, 'CARLTON': 2, 'SLY': 3, 'DIGI': 4}            # bateristas do MRDM-12
BASS = {'FAMILY': 0, 'FLABBA': 1, 'BARNETT': 2, 'ROBBIE': 3, 'SIBBLES': 4, 'FULLWOOD': 5, 'JAMMYS': 6}  # MRBM-69

EAR, R70, RK, RD, GI, DG, LR, DH, NR = (
    'EARLY REGGAE - 1968 A 72', 'ROOTS - ANOS 70', 'ROCKERS - FIM DOS 70', 'RUB A DUB - VOLCANO E ANOS 80',
    'GREGORY ISAACS - COOL RULER', 'DIGITAL - KING JAMMY E 85-89', 'LOVERS ROCK - 69 A 94',
    'DANCEHALL - ANOS 90', 'NEW ROOTS - ANOS 2000')

# padrão de cada pasta: "bpm kit baterista baixista" (cada música pode trocar qualquer um)
FOLDER = {
    EAR: '82 OD CARLTON SIBBLES',   R70: '72 OD CARLTON FAMILY',    RK: '80 RK SLY ROBBIE',
    RD: '84 RK STYLE FLABBA',       GI: '76 RK STYLE FLABBA',       DG: '88 RK DIGI JAMMYS',
    LR: '74 OD SANTA SIBBLES',      DH: '96 RK DIGI BARNETT',       NR: '74 OD SANTA FAMILY',
}

# (pasta, década, nome, "acorde:duração ...", "trocas": bpm=, kit=, dr=, bx=)
BANK = [
    (EAR, '60s', 'Israelites (Desmond Dekker 1968)',     'Bb:2 Eb:1/2 F:1/2 Bb:1', 'bpm=86'),
    (EAR, '60s', '54-46 That\'s My Number (Toots 1968)',  'G:1 C:1', 'bpm=84'),
    (EAR, '60s', 'Monkey Man (Toots 1969)',               'Bb:2 Eb:1/2 F:1/2 Bb:1', 'bpm=88'),
    (EAR, '60s', 'Wonderful World (Jimmy Cliff 1969)',    'D:1 G:1', 'bpm=84'),
    (EAR, '60s', 'Long Shot Kick de Bucket (Pioneers 1969) [auto]', 'C:1 G:1', 'bpm=86'),
    (EAR, '70s', 'Rivers of Babylon (Melodians 1970)',    'G:2 D:1 G:1', 'bpm=80'),
    (EAR, '70s', 'Duppy Conqueror (Wailers 1970)',        'C:1/2 F:1/2 G:1/2 F:1/2', 'bpm=78 bx=FAMILY'),
    (EAR, '70s', 'Small Axe (Wailers 1970)',              'Eb:1/2 Bb:1/2 F:1', 'bpm=78 bx=FAMILY'),
    (EAR, '70s', 'You Can Get It (Jimmy Cliff 1970)',     'C:1 F:1', 'bpm=84'),
    (EAR, '70s', 'The Harder They Come (Jimmy Cliff 1972)', 'Db:1 Ab:1 Gb:1 Ab:1', 'bpm=80'),

    (R70, '70s', 'Satta Massagana (Abyssinians 1969)',    'Am:1 Dm:1 Am:1 Em:1', 'bpm=70 dr=SANTA bx=SIBBLES'),
    (R70, '70s', 'Pressure Drop (Toots 1969)',            'Ab:1 Eb:1 Db:1 Ab:1', 'bpm=80 bx=SIBBLES'),
    (R70, '70s', 'Marcus Garvey (Burning Spear 1975)',    'Am:1 G:1', 'bpm=72 dr=SANTA bx=ROBBIE'),
    (R70, '70s', 'Smile Jamaica (Bob Marley 1976)',       'Bm:1 G:1 A:1', 'bpm=76'),
    (R70, '70s', 'Police and Thieves (Junior Murvin 1976)', 'G:1 Am:1 D:1 Am:1', 'bpm=72 dr=SANTA bx=FULLWOOD'),
    (R70, '70s', 'Natural Mystic (Bob Marley 1977)',      'Dm:1 Em:1 Am:2', 'bpm=72'),
    (R70, '70s', 'Exodus (Bob Marley 1977)',              'Am:4 C:1/2 Dm:1/2 Am:1', 'bpm=78 kit=RK'),
    (R70, '70s', 'Jamming (Bob Marley 1977)',             'Bm:1 E:1 G:1 F#m:1', 'bpm=74'),
    (R70, '70s', 'One Love (Bob Marley 1977)',            'Bb:1 F:1 Eb:1 Bb:1', 'bpm=76'),
    (R70, '70s', 'Roots (Bob Marley 1977)',               'F#m:1 Bm:1 D:1 C#m:1', 'bpm=72'),

    (RK, '70s', 'Legalize It (Peter Tosh 1976)',          'G:1 F:1', 'bpm=76'),
    (RK, '70s', 'Burial (Peter Tosh 1976)',               'Bb:1 Gm:1 Eb:1 F:1', 'bpm=78'),
    (RK, '70s', 'Uptown Top Ranking (Althea & Donna 1977)', 'Am:1 G:1', 'bpm=80'),
    (RK, '70s', 'Now That We Found Love (Third World 1978)', 'Cm:1 Fm:1 Gm:1', 'bpm=82'),
    (RK, '70s', 'Ku Klux Klan (Steel Pulse 1978)',        'Am:1 Em:1', 'bpm=78'),
    (RK, '70s', 'Soldiers (Steel Pulse 1978)',            'Bm:1 F#:1 G:1 A:1', 'bpm=80'),
    (RK, '70s', 'Money in My Pocket (Dennis Brown 1979)', 'F:1 Gm:1 Dm:1/2 C:1/2', 'bpm=78'),
    (RK, '70s', 'One Drop (Bob Marley 1979)',             'C:1 Am:1 Dm:1 G:1', 'bpm=76 kit=OD dr=CARLTON bx=FAMILY'),
    (RK, '80s', 'Could You Be Loved (Bob Marley 1980)',   'Bm:1 Em:1 D:1 Bm:1', 'bpm=84 dr=CARLTON bx=FAMILY'),
    (RK, '80s', 'Buffalo Soldier (Bob Marley 1983)',      'A:1 F#m:1 D:1 A:1', 'bpm=82 kit=OD dr=CARLTON bx=FAMILY'),

    (RD, '70s', 'Shine Eye Gal (Black Uhuru 1979)',       'Cm:2 Gm:2', 'bpm=80 dr=SLY bx=ROBBIE'),
    (RD, '80s', 'Truths and Rights (Johnny Osbourne 1980)', 'C#m:1 E:1 Ab:1', 'bpm=78 dr=SANTA bx=SIBBLES'),
    (RD, '80s', 'Sensimilla (Barrington Levy 1980)',      'Fm:1 Cm:1', 'bpm=82'),
    (RD, '80s', 'Sinsemilla (Black Uhuru 1980)',          'Am:1 Em:1 Am:1 G:1', 'bpm=80 dr=SLY bx=ROBBIE'),
    (RD, '80s', 'Fire House Rock (Wailing Souls 1981)',   'A:1 Bm:1', 'bpm=84'),
    (RD, '80s', 'Ice Cream Love (Johnny Osbourne 1981) [auto]', 'Gm:1 F:1', 'bpm=84'),
    (RD, '80s', 'Pass the Kouchie (Mighty Diamonds 1981)', 'C:1 Dm:1', 'bpm=84'),
    (RD, '80s', 'Bam Bam (Sister Nancy 1982)',            'C:1 Bb:1', 'bpm=86'),
    (RD, '80s', 'Here I Come (Barrington Levy 1984)',     'Em:1 Am:1', 'bpm=86'),
    (RD, '80s', 'Peeni Walli (Eek-A-Mouse 1984) [auto]',  'Ab:1 Ebm:1 Db:1 Eb:1', 'bpm=74 kit=OD'),

    (GI, '70s', 'My Number One (Gregory Isaacs)',    'G:1 Am:1 D:1', 'bpm=74 dr=SANTA bx=FULLWOOD'),
    (GI, '70s', 'Soon Forward (Gregory Isaacs 1979)',     'C:1 Dm:1', 'bpm=74 dr=SLY bx=ROBBIE'),
    (GI, '70s', 'Not the Way (Gregory Isaacs)',           'D:1 Bm:1 G:1/2 A:1/2', 'bpm=74 kit=OD dr=SANTA bx=FULLWOOD'),
    (GI, '80s', 'Front Door (Gregory Isaacs 1981) [auto]', 'D:1 Em:1', 'bpm=76'),
    (GI, '80s', 'Night Nurse (Gregory Isaacs 1982)',      'Am:1 G:1 Dm:1', 'bpm=72'),
    (GI, '80s', 'Cool Down the Pace (Gregory Isaacs 1982)', 'Bb:1 Cm:1', 'bpm=76'),
    (GI, '80s', 'Private Secretary (Gregory Isaacs) [auto]', 'Am:1 G:1', 'bpm=74'),
    (GI, '80s', 'If I Don\'t Have You (Gregory Isaacs) [auto]', 'Am:1 G:1 Am:1 Dm:1', 'bpm=76'),
    (GI, '80s', 'Hard Drugs (Gregory Isaacs) [auto]',     'C:1 G:1 C:1 D:1', 'bpm=82'),
    (GI, '80s', 'Good Morning (Gregory Isaacs) [auto]',   'A:1 D:1', 'bpm=82'),

    (DG, '80s', 'Under Mi Sleng Teng (Wayne Smith 1985) [auto]', 'D:2', 'bpm=82'),
    (DG, '80s', 'Ring the Alarm (Tenor Saw 1985)',        'C:1 Bb:1', 'bpm=90'),
    (DG, '80s', 'Lots of Sign (Tenor Saw 1985) [auto]',   'C:1 F:1 Gm:1 C:1', 'bpm=88'),
    (DG, '80s', 'Rub a Dub Market (Tenor Saw) [auto]',    'Bb:1 Cm:1', 'bpm=86'),
    (DG, '80s', 'Run Come Call Me (Tenor Saw) [auto]',    'Gm:1 D:1', 'bpm=88'),
    (DG, '80s', 'Budy Bye (Johnny Osbourne 1985) [auto]', 'D:1 G:1', 'bpm=90'),
    (DG, '80s', 'Greetings (Half Pint 1986) [auto]',      'F:1 C:1 Dm:1 C:1', 'bpm=86'),
    (DG, '80s', 'Winsome (Half Pint) [auto]',             'C:1 D:1 G:2', 'bpm=84'),
    (DG, '80s', 'Sonia Come Back (Cocoa Tea, Punaany) [auto]', 'Ab:1 Db:1', 'bpm=94'),
    (DG, '80s', 'She Loves Me Now (Cocoa Tea, Real Rock) [auto]', 'Bb:1 Fm:1', 'bpm=80'),

    (LR, '60s', 'Many Rivers to Cross (Jimmy Cliff 1969)', 'G:1 Bm:1/2 C:1/2 D:1', 'bpm=70'),
    (LR, '70s', 'Help Me Make It Through the Night (John Holt 1974)', 'E:1 A:1 F#m:1 B:1', 'bpm=72'),
    (LR, '70s', 'Waiting in Vain (Bob Marley 1977)',      'Ab:1 Db:1', 'bpm=76 dr=CARLTON bx=FAMILY'),
    (LR, '70s', 'Is This Love (Bob Marley 1978)',         'F#m:2 D:1 A:1', 'bpm=76 dr=CARLTON bx=FAMILY'),
    (LR, '80s', 'Red Red Wine (UB40 1983)',               'Db:1 Gb:1 Ab:1 Gb:1', 'bpm=82 kit=RK'),
    (LR, '80s', 'Wild World (Maxi Priest 1988)',          'Am:1 D:1 G:1 C:1', 'bpm=80 kit=RK'),
    (LR, '80s', 'Kingston Town (UB40 1989)',              'Ab:1 Cm:1 Db:1 Eb:1', 'bpm=70'),
    (LR, '90s', 'Tempted to Touch (Beres Hammond 1990)',  'C:1 Am:1 Dm:1 G:1', 'bpm=74'),
    (LR, '90s', 'Close to You (Maxi Priest 1990)',        'Ebm:2 Abm:1 Bbm:1', 'bpm=78 kit=RK'),
    (LR, '90s', 'Rockaway (Beres Hammond 1994)',          'Ab:1 Eb:1 Fm:1 Db:1', 'bpm=72'),

    (DH, '90s', 'Mr. Loverman (Shabba Ranks 1992)',       'Am:1 Dm:1 G:1 Am:1', 'bpm=96'),
    (DH, '90s', 'Informer (Snow 1992)',                   'F#:1 E:1', 'bpm=98'),
    (DH, '90s', 'Sweat (Inner Circle 1992)',              'Bb:1 Gm:1 Cm:1 F:1', 'bpm=96 dr=SLY bx=ROBBIE'),
    (DH, '90s', 'Here Comes the Hotstepper (Ini Kamoze 1994)', 'Am:1 G:1', 'bpm=100 dr=SLY bx=ROBBIE'),
    (DH, '90s', "You Don't Love Me (Dawn Penn 1994)",     'Am:1 D:1', 'bpm=92 dr=SLY bx=ROBBIE'),
    (DH, '90s', 'Boombastic (Shaggy 1995)',               'D:1 G:1 A:1', 'bpm=82'),
    (DH, '80s', 'Under Mi Sensi (Barrington Levy 1985)',  'C:1 F:1 G:1', 'bpm=90'),
    (DH, '00s', "It Wasn't Me (Shaggy 2000)",             'C:1 G:1 F:1', 'bpm=94'),
    (DH, '00s', 'Get Busy (Sean Paul 2003)',              'Fm:1 Gb:1', 'bpm=100'),
    (DH, '00s', 'Temperature (Sean Paul 2005)',           'Bbm:1 Ab:1 Bbm:1 C:1', 'bpm=104'),

    (NR, '00s', 'Welcome to Jamrock (Damian Marley 2005)', 'Gm:1 Dm:1', 'bpm=82 kit=RK dr=DIGI'),
    (NR, '00s', 'Ganja Farmer (Marlon Asher 2005)',       'C:1 Em:1 Am:1 F:1', 'bpm=76'),
    (NR, '00s', "She's Royal (Tarrus Riley 2006)",        'Eb:1 Ab:1 Eb:1 Bb:1', 'bpm=72'),
    (NR, '00s', 'Herbalist (Alborosie 2008)',             'Fm:1 Bb:1', 'bpm=78 kit=RK dr=STYLE bx=FLABBA'),
    (NR, '00s', 'Kingston Town (Alborosie 2008)',         'Em:1 Am:1', 'bpm=76'),
    (NR, '00s', 'Mystical Reggae (Alborosie 2009)',       'Bm:1 G:1 A:1', 'bpm=74'),
    (NR, '10s', 'Callin (Alborosie 2011)',                'Am:1 Dm:1', 'bpm=74'),
    (NR, '10s', 'Access Granted (Protoje 2013)',          'Gm:1 Cm:1 Gm:1 F:1', 'bpm=72'),
    (NR, '10s', 'Smile Jamaica (Chronixx 2014)',          'G:1 C:1 Em:1 D:1', 'bpm=76'),
    (NR, '10s', 'Dem Gone (Kabaka Pyramid 2018)',         'Am:1 G:1 F:1', 'bpm=78 kit=RK'),
]

FOLDERS = [EAR, R70, RK, RD, GI, DG, LR, DH, NR]

def riddim(folder, over):
    bpm, kit, dr, bx = FOLDER[folder].split()
    r = {'bpm': int(bpm), 'kit': KIT[kit], 'drummer': DRUM[dr], 'bassist': BASS[bx]}
    for kv in over.split():
        k, v = kv.split('=')
        if k == 'bpm': r['bpm'] = int(v)
        elif k == 'kit': r['kit'] = KIT[v]
        elif k == 'dr': r['drummer'] = DRUM[v]
        elif k == 'bx': r['bassist'] = BASS[v]
        else: raise ValueError(kv)
    assert 60 <= r['bpm'] <= 180
    return r

def parse(spec):
    out = []
    for tok in spec.split():
        name, dur = tok.split(':')
        minor = name.endswith('m')
        root = ROOTS[name[:-1] if minor else name]
        out.append((root, 1 if minor else 0, DURS[dur]))
    assert 1 <= len(out) <= 4, spec
    return out

for f in FOLDERS:
    assert sum(1 for b in BANK if b[0] == f) == 10, f
assert len({b[2] for b in BANK}) == len(BANK), 'nome repetido'

presets = [dict(folder=f, era=e, name=n, chords=parse(s), **riddim(f, o)) for f, e, n, s, o in BANK]
folders = FOLDERS

here = os.path.dirname(os.path.abspath(__file__))
h = ['// Gerado por tools/harmony_presets.py — não editar à mão.',
     '// Banco de RIDDIMS da DubStation (MRCM-1240 e MRM-0420): harmonia, BPM e músicos, por estilo e época.',
     '#pragma once', '', 'namespace dubpresets', '{',
     'struct Chord { int root; int minor; int dur; };   // tônica 0..11 (C = 0), 1 = menor, duração 0..5 (1/2, 1, 2, 3, 4, 8 comp.)',
     '// bpm sugerido, kit (0 ROCKERS, 1 ONE DROP), baterista do MRDM-12 (0 STYLE SCOTT, 1 SANTA DAVIS, 2 CARLTON BARRETT, 3 SLY DUNBAR, 4 DIGIKAL),',
     '// baixista do MRBM-69 (0 FAMILY MAN, 1 FLABBA, 2 DERRICK BARNETT, 3 ROBBIE, 4 LEROY SIBBLES, 5 FULLY FULLWOOD, 6 JAMMYS)',
     'struct Preset { const char* folder; const char* era; const char* name; int numChords; Chord chords[4]; int bpm, kit, drummer, bassist; };', '',
     f'inline constexpr int numFolders = {len(folders)};',
     'inline const char* const folders[numFolders] = { ' + ', '.join(json.dumps(f) for f in folders) + ' };', '',
     f'inline constexpr int numPresets = {len(presets)};',
     'inline const Preset presets[numPresets] = {']
for p in presets:
    ch = ', '.join(f'{{ {r}, {m}, {d} }}' for r, m, d in p['chords'])
    h.append(f'    {{ {json.dumps(p["folder"])}, {json.dumps(p["era"])}, {json.dumps(p["name"], ensure_ascii=False)}, {len(p["chords"])}, {{ {ch} }}, {p["bpm"]}, {p["kit"]}, {p["drummer"]}, {p["bassist"]} }},')
h += ['};', '}', '']
open(os.path.join(here, '..', 'Source', 'HarmonyPresets.h'), 'w', encoding='utf-8').write('\n'.join(h))

js = ('// Gerado por ReggaeChordMachine/tools/harmony_presets.py — não editar à mão.\n'
      '// Banco de RIDDIMS da DubStation (MRCM-1240 e MRM-0420): harmonia, BPM e músicos, por estilo e época.\n'
      '(function (root) {\n  root.DUB_PRESETS = ' +
      json.dumps({'folders': folders, 'presets': [{'folder': p['folder'], 'era': p['era'], 'name': p['name'],
                  'chords': [{'root': r, 'quality': m, 'dur': d} for r, m, d in p['chords']],
                  'bpm': p['bpm'], 'kit': p['kit'], 'drummer': p['drummer'], 'bassist': p['bassist']} for p in presets]},
                 ensure_ascii=False, indent=1) + ';\n})(typeof window !== "undefined" ? window : globalThis);\n')
open(os.path.join(here, '..', '..', 'site', 'chords', 'presets.js'), 'w', encoding='utf-8').write(js)
print('ok', len(presets), 'presets em', len(folders), 'pastas')
