# -*- coding: utf-8 -*-
"""Banco de RIDDIMS CLÁSSICOS do MRBM-69 (20 por estilo de reggae).

Gera:
  ReggaeBassMachine/Source/RiddimPresets.h   (plugin)
  site/bassmachine/riddims.js                (versão para navegador)
  ReggaeBassMachine/docs/RIDDIMS.md          (lista para ler)

Cada preset traz o riddim, a gravação de origem e o ano, o produtor/estúdio e quem tocou
(ou programou) o baixo, o estilo do gerador mais próximo do baixista, a harmonia de referência,
o BPM, a bateria e o timbre. As notas do baixo são GERADAS pelo motor do MRBM-69 no estilo do
baixista em cima dessa harmonia: não são transcrições das linhas originais.

Fontes dos créditos: riddim-id.com (riddims mais usados por década), Wikipedia (Leroy Sibbles,
Sleng Teng, Steely & Clevie, Roots Radics), Jamaica Observer (Flabba Holt, Channel One), LargeUp
(Boris Gardiner), Rolling Stone (Real Rock), roots-archives (Revolutionaries), electronicbeats
(Bookshelf, Showtime, Tempo), DancehallMag (Playground).
Harmonia marcada com "~" = de ouvido/aproximada (confira o tom no disco e use a TÔNICA para transpor).

Uso: python3 ReggaeBassMachine/tools/riddims.py
"""
import json, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# estilos do gerador (mbm::Style)
FAMILY, FLABBA, DERRICK, ROBBIE, SIBBLES, FULLY, JAMMYS = range(7)
STYLE_NAMES = ["FAMILY MAN", "FLABBA", "DERRICK BARNETT", "ROBBIE", "LEROY SIBBLES", "FULLY FULLWOOD", "JAMMYS"]
# bateria (mbm::DrumStyle) e timbre (mbm::TimbreId)
ROOTS_D, ONEDROP, DIGITAL, DANCEHALL = range(4)
SINE, SUPER, E808 = range(3)

FOLDERS = [
    # nome, década padrão, padrões do preset
    ("ROCKSTEADY - STUDIO ONE E TREASURE ISLE 1966 A 69", dict(style=SIBBLES, drum=ONEDROP, timbre=SINE, bpm=78, dens=.55, var=.45, swing=.20)),
    ("EARLY REGGAE E ROOTS - 1969 A 75",                  dict(style=FAMILY,  drum=ONEDROP, timbre=SINE, bpm=76, dens=.55, var=.50, swing=.15)),
    ("ROOTS E ROCKERS - CHANNEL ONE, JOE GIBBS E BLACK ARK 1975 A 79", dict(style=ROBBIE, drum=ROOTS_D, timbre=SINE, bpm=76, dens=.60, var=.45, swing=.10)),
    ("RUB A DUB - ROOTS RADICS E TAXI 1979 A 84",         dict(style=FLABBA,  drum=ROOTS_D, timbre=SINE, bpm=80, dens=.60, var=.45, swing=.10)),
    ("DIGITAL - KING JAMMYS 1985 A 89",                   dict(style=JAMMYS,  drum=DIGITAL, timbre=SUPER, bpm=90, dens=.55, var=.40, swing=.05)),
    ("RAGGA - STEELY & CLEVIE, BOBBY DIGITAL E SLY & ROBBIE 1990 A 95", dict(style=JAMMYS, drum=DANCEHALL, timbre=SUPER, bpm=94, dens=.55, var=.45, swing=.05)),
    ("DANCEHALL - 1995 A 2001",                           dict(style=JAMMYS,  drum=DANCEHALL, timbre=E808, bpm=96, dens=.50, var=.45, swing=0.0)),
    ("DANCEHALL - 2002 A 2007",                           dict(style=JAMMYS,  drum=DANCEHALL, timbre=E808, bpm=100, dens=.50, var=.45, swing=0.0)),
]

SHORT = ["ROCKSTEADY 66-69", "EARLY ROOTS 69-75", "ROOTS ROCKERS 75-79", "RUB A DUB 79-84", "DIGITAL 85-89",
         "RAGGA 90-95", "DANCEHALL 95-01", "DANCEHALL 02-07"]   # nome curto para o visor

# (riddim, gravação de origem, ano, produtor/estúdio, baixo, harmonia, bpm ou None, extras)
# harmonia: "Am:1 G:1" (tônica + m / 7 / m7, ":" duração h=1/2, 1, 2, 3, 4, 8 compassos); "~" no início = aproximada
R = [
 # ---------------------------------------------------------------- 0 ROCKSTEADY 1966-69
 [
  ("Real Rock", "Sound Dimension - Real Rock", 1967, "Coxsone Dodd (Studio One)", "Boris Gardiner", "~Bb:1 Fm:1", 76, {}),
  ("Full Up", "Sound Dimension - Full Up", 1968, "Coxsone Dodd (Studio One)", "Leroy Sibbles", "~Am:1 G:1", 80, {}),
  ("Satta Massagana", "The Abyssinians - Satta Massagana", 1969, "Coxsone Dodd (Studio One)", "Leroy Sibbles", "Am:1 Dm:1 Am:1 Em:1", 70, {}),
  ("Heavenless", "Don Drummond Jr & Sound Dimension - Heavenless", 1968, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Gm:1 F:1", 78, {}),
  ("Mad Mad", "Alton Ellis - Mad Mad", 1967, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Cm:1 Bb:1", 76, {}),
  ("Answer", "Slim Smith & The Uniques - I'll Never Let You Go", 1967, "Studio One", "Sound Dimension", "~C:1 F:1", 74, {}),
  ("Pretty Looks", "Sound Dimension - Pretty Looks", 1969, "Coxsone Dodd (Studio One)", "Leroy Sibbles", "~Gm:1 Cm:1", 74, {}),
  ("Hot Milk", "Jackie Mittoo & The Soul Vendors - Hot Milk", 1968, "Coxsone Dodd (Studio One)", "Soul Vendors", "~Dm:2", 82, {}),
  ("Love Me Forever", "Carlton & The Shoes - Love Me Forever", 1968, "Coxsone Dodd (Studio One)", "Sound Dimension", "~F:1 Bb:1", 72, {}),
  ("Darker Shade Of Black", "Jackie Mittoo & The Soul Vendors - Darker Shade Of Black", 1967, "Studio One", "Soul Vendors", "~Am:1 D:1", 80, {}),
  ("Drum Song", "Jackie Mittoo & Sound Dimension - Drum Song", 1967, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Em:2", 84, {}),
  ("Rockfort Rock", "Sound Dimension - Rockfort Rock", 1967, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Dm:1 C:1", 80, {}),
  ("General", "Leroy Sibbles & Anthony Ellis - Love Me Girl", 1967, "Studio One", "Leroy Sibbles", "~G:1 C:1", 80, {}),
  ("Bobby Babylon", "Jackie Mittoo & The Soul Vendors - One Step Beyond", 1967, "Studio One", "Soul Vendors", "~Cm:1 Fm:1", 76, {}),
  ("Nanny Goat", "Larry Marshall - Nanny Goat", 1968, "Coxsone Dodd (Studio One)", "Sound Dimension", "~G:1 D:1", 84, {}),
  ("Mean Girl", "Larry Marshall - Mean Girl", 1968, "Studio One", "Sound Dimension", "~F:1 C:1", 80, {}),
  ("I'm Still In Love", "Alton Ellis - I'm Still In Love With You", 1967, "Studio One", "Sound Dimension", "Am:1 G:1", 76, {}),
  ("Love Is Not A Gamble", "The Techniques - Love Is Not A Gamble", 1967, "Duke Reid (Treasure Isle)", "Supersonics", "~D:1 G:1 A:1 G:1", 82, {}),
  ("Ba Ba Boom", "The Jamaicans - Ba Ba Boom", 1967, "Duke Reid (Treasure Isle)", "Supersonics", "~C:1 F:1 G:1 F:1", 84, {}),
  ("Boops (Train To Skaville)", "The Ethiopians - Train To Skaville", 1967, "The Ethiopians", "Supersonics", "G:1 C:1", 88, {}),
 ],
 # ---------------------------------------------------------------- 1 EARLY REGGAE E ROOTS 1969-75
 [
  ("Stalag", "Ansell Collins - Stalag 17", 1973, "Winston Riley (Techniques)", "Boris Gardiner", "C:1 Bb:1", 78, dict(style=FULLY)),
  ("Duppy Conqueror", "Bob Marley & The Wailers - Duppy Conqueror", 1970, "Lee Perry (Upsetters)", "Aston \"Family Man\" Barrett", "C:h F:h G:h F:h", 78, {}),
  ("Small Axe", "Bob Marley & The Wailers - Small Axe", 1970, "Lee Perry (Upsetters)", "Aston \"Family Man\" Barrett", "Eb:h Bb:h F:1", 78, {}),
  ("Concrete Jungle", "Bob Marley & The Wailers - Concrete Jungle", 1973, "Chris Blackwell / Wailers (Island)", "Aston \"Family Man\" Barrett", "Em:1 Am:1 Em:2", 72, {}),
  ("Get Up Stand Up", "Bob Marley & The Wailers - Get Up Stand Up", 1973, "Wailers (Island)", "Aston \"Family Man\" Barrett", "Bm:4", 76, {}),
  ("Lively Up Yourself", "Bob Marley & The Wailers - Lively Up Yourself", 1973, "Wailers (Island)", "Aston \"Family Man\" Barrett", "D:1 G:1", 78, {}),
  ("Far East", "Rolando Alphonso - Jah Shakey", 1973, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Am:2", 72, dict(style=SIBBLES)),
  ("He Prayed", "Burning Spear & Morris Toughest - Joe Frazier", 1972, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Dm:1 C:1", 72, dict(style=SIBBLES)),
  ("Things And Time", "The Wailing Souls - Back Out With It", 1970, "Studio One", "Sound Dimension", "~Gm:1 F:1", 74, dict(style=SIBBLES)),
  ("A Love I Can Feel", "John Holt - A Love I Can Feel", 1970, "Studio One", "Sound Dimension", "~D:1 G:1 A:1 G:1", 74, dict(style=SIBBLES)),
  ("Freedom Blues", "Roy Richards - Freedom Blues", 1970, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Em:1 A:1", 78, dict(style=SIBBLES)),
  ("Mr. Bassie", "Horace Andy - Mr. Bassie", 1971, "Studio One", "Leroy Sibbles", "~Am:1 Em:1", 74, dict(style=SIBBLES)),
  ("College Rock", "Big Willie - College Rock", 1972, "Coxsone Dodd (Studio One)", "Sound Dimension", "~Cm:1 Bb:1", 76, dict(style=SIBBLES)),
  ("Skylarking", "Horace Andy - Skylarking", 1972, "Studio One", "Sound Dimension", "~Em:1 D:1", 72, dict(style=SIBBLES)),
  ("Java", "Augustus Pablo - Java", 1971, "Clive Chin (Randy's)", "Randy's (sessão)", "~Gm:1 Cm:1", 74, {}),
  ("Cherry Oh Baby", "Eric Donaldson - Cherry Oh Baby", 1971, "Dynamic Sounds", "Dynamic Sounds (sessão)", "~G:1 C:1 D:1 C:1", 80, {}),
  ("Talk About Love", "Pat Kelly - They Talk About Love", 1971, "Studio One", "Sound Dimension", "~F:1 Dm:1", 76, dict(style=SIBBLES)),
  ("Black Cinderella", "Errol Dunkley - Black Cinderella", 1972, "Jimmy Radway", "sessão", "~Bb:1 Eb:1", 76, {}),
  ("Fever", "Horace Andy - Fever", 1972, "Studio One", "Sound Dimension", "~Dm:1 Am:1", 74, dict(style=SIBBLES)),
  ("Westbound Train", "Dennis Brown - Westbound Train", 1973, "Niney The Observer", "Soul Syndicate (Fully Fullwood)", "~Em:1 A:1", 80, dict(style=FULLY)),
 ],
 # ---------------------------------------------------------------- 2 ROOTS E ROCKERS 1975-79
 [
  ("MPLA", "The Revolutionaries - MPLA", 1976, "Jo Jo Hookim (Channel One)", "Revolutionaries (Ranchie McLean / Robbie Shakespeare)", "~Em:2", 78, {}),
  ("Ballistic Affair", "Leroy Smart - Ballistic Affair", 1976, "Jo Jo Hookim (Channel One)", "Revolutionaries (Ranchie McLean / Robbie Shakespeare)", "~Gm:1 F:1", 76, {}),
  ("Taxi", "Little Roy - Prophecy", 1975, "Maurice & Melvin Jackson", "sessão", "~Am:1 Em:1", 76, {}),
  ("Storm", "Gregory Isaacs - Storm", 1977, "Gregory Isaacs (African Museum)", "sessão", "~Am:1 G:1", 74, {}),
  ("Two Sevens Clash", "Culture - Two Sevens Clash", 1977, "Joe Gibbs", "Joe Gibbs Professionals (Lloyd Parks / Robbie Shakespeare)", "~Ab:1 Db:1", 82, {}),
  ("Uptown Top Ranking", "Althea & Donna - Uptown Top Ranking", 1977, "Joe Gibbs", "Joe Gibbs Professionals", "Am:1 G:1", 80, {}),
  ("Police And Thieves", "Junior Murvin - Police And Thieves", 1976, "Lee Perry (Black Ark)", "Boris Gardiner", "G:1 Am:1 D:1 Am:1", 72, dict(style=FAMILY, drum=ONEDROP)),
  ("Chase The Devil", "Max Romeo - Chase The Devil", 1976, "Lee Perry (Black Ark)", "Boris Gardiner", "Am:1 Em:1 Dm:1 Am:1", 70, dict(style=FAMILY, drum=ONEDROP)),
  ("War Ina Babylon", "Max Romeo - War Ina Babylon", 1976, "Lee Perry (Black Ark)", "Boris Gardiner", "~Am:1 Em:1", 70, dict(style=FAMILY, drum=ONEDROP)),
  ("Exodus", "Bob Marley & The Wailers - Exodus", 1977, "Wailers (Tuff Gong)", "Aston \"Family Man\" Barrett", "Am:4 C:h Dm:h Am:1", 78, dict(style=FAMILY)),
  ("Natural Mystic", "Bob Marley & The Wailers - Natural Mystic", 1977, "Wailers (Tuff Gong)", "Aston \"Family Man\" Barrett", "Dm:1 Em:1 Am:2", 72, dict(style=FAMILY, drum=ONEDROP)),
  ("Heathen", "Bob Marley & The Wailers - The Heathen", 1977, "Wailers (Tuff Gong)", "Aston \"Family Man\" Barrett", "~Cm:2", 72, dict(style=FAMILY, drum=ONEDROP)),
  ("Marcus Garvey", "Burning Spear - Marcus Garvey", 1975, "Jack Ruby", "Black Disciples (Robbie Shakespeare)", "Am:1 G:1", 72, dict(drum=ONEDROP)),
  ("Legalize It", "Peter Tosh - Legalize It", 1976, "Peter Tosh", "Robbie Shakespeare", "G:1 F:1", 76, {}),
  ("Declaration Of Rights", "The Abyssinians - Declaration Of Rights", 1976, "Coxsone Dodd (Studio One)", "Leroy Sibbles", "~Em:1 D:1", 74, dict(style=SIBBLES, drum=ONEDROP)),
  ("Youthman", "Glenroy Richards - Wicked Can't Run Away", 1977, "Glen Brown", "sessão", "~Dm:1 Gm:1", 76, {}),
  ("Weather Balloon", "U Brown - Weather Balloon", 1977, "U Brown", "sessão", "~Cm:1 Fm:1", 76, {}),
  ("Operation Radication", "Ronnie Davis & The Tennors - Won't You Come Home", 1975, "Lloyd Campbell", "sessão", "~F:1 Bb:1", 76, {}),
  ("No Warrior", "Junior Delgado - The Raiders", 1979, "Junior Delgado", "sessão", "~Am:2", 78, {}),
  ("Ku Klux Klan", "Steel Pulse - Ku Klux Klan", 1978, "Karl Pitterson (Island)", "Ronnie McQueen", "Am:1 Em:1", 78, dict(style=DERRICK)),
 ],
 # ---------------------------------------------------------------- 3 RUB A DUB 1979-84
 [
  ("Gunman", "Michael Prophet - Gunman", 1980, "Henry \"Junjo\" Lawes (Volcano)", "Errol \"Flabba\" Holt (Roots Radics)", "~Cm:1 Bb:1", 78, {}),
  ("Night Nurse", "Gregory Isaacs - Night Nurse", 1982, "Gregory Isaacs (African Museum)", "Errol \"Flabba\" Holt (Roots Radics)", "~Gm:1 Cm:1", 74, {}),
  ("Bounty Hunter", "Barrington Levy - Bounty Hunter", 1979, "Henry \"Junjo\" Lawes", "Errol \"Flabba\" Holt (Roots Radics)", "~Am:1 Dm:1", 80, {}),
  ("Sensimilla", "Barrington Levy - Sensimilla", 1980, "Henry \"Junjo\" Lawes", "Errol \"Flabba\" Holt (Roots Radics)", "Fm:1 Cm:1", 82, {}),
  ("Wa Do Dem", "Eek-A-Mouse - Wa Do Dem", 1981, "Henry \"Junjo\" Lawes (Volcano)", "Errol \"Flabba\" Holt (Roots Radics)", "~Dm:1 Am:1", 80, {}),
  ("Big Ship", "Freddie McGregor - Big Ship", 1982, "Linval Thompson", "Errol \"Flabba\" Holt (Roots Radics)", "~G:1 C:1", 76, {}),
  ("Diseases (Mad Mad)", "Michigan & Smiley - Diseases", 1981, "Henry \"Junjo\" Lawes (Volcano)", "Errol \"Flabba\" Holt (Roots Radics)", "~Cm:1 Bb:1", 80, {}),
  ("Ice Cream Love", "Johnny Osbourne - Ice Cream Love", 1981, "Henry \"Junjo\" Lawes", "Errol \"Flabba\" Holt (Roots Radics)", "~Gm:1 F:1", 84, {}),
  ("Zungguzung", "Yellowman - Zungguzungguguzungguzeng", 1983, "Henry \"Junjo\" Lawes", "Errol \"Flabba\" Holt (Roots Radics)", "~G#m:1 G#:1 B:1 G#:1", 82, {}),
  ("Boxing", "Cornell Campbell - Boxing Around", 1981, "Joe Gibbs", "sessão", "~Am:1 G:1", 80, {}),
  ("Rope In", "Cornell Campbell - Rope In", 1980, "Joe Gibbs", "sessão", "~Dm:1 C:1", 80, {}),
  ("Truths And Rights", "Johnny Osbourne - Truths And Rights", 1980, "Coxsone Dodd (Studio One)", "sessão (Studio One)", "C#m:1 E:1 G#:1", 78, {}),
  ("Shine Eye Gal", "Black Uhuru - Shine Eye Gal", 1979, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "Cm:2 Gm:2", 80, dict(style=ROBBIE)),
  ("Sitting And Watching", "Dennis Brown - Sitting And Watching", 1978, "Sly & Robbie", "Robbie Shakespeare", "~Em:1 Am:1", 76, dict(style=ROBBIE)),
  ("Baltimore", "The Tamlins - Baltimore", 1980, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "~Dm:1 C:1", 80, dict(style=ROBBIE)),
  ("Old Broom", "The Wailing Souls - Old Broom", 1980, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "~Am:1 D:1", 82, dict(style=ROBBIE)),
  ("Revolution", "Dennis Brown - Revolution", 1983, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "~Am:1 G:1", 82, dict(style=ROBBIE)),
  ("World A Music", "Ini Kamoze - World A Music", 1983, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "~Em:1 D:1", 84, dict(style=ROBBIE)),
  ("Pass The Kouchie (Full Up)", "Mighty Diamonds - Pass The Kouchie", 1981, "Gussie Clarke (Music Works)", "sessão", "~Am:1 G:1", 82, dict(style=SIBBLES)),
  ("Promised Land", "Aswad - Love Fire", 1981, "Aswad", "Tony Gad", "~Gm:1 F:1", 78, dict(style=DERRICK)),
 ],
 # ---------------------------------------------------------------- 4 DIGITAL KING JAMMYS 1985-89
 [
  ("Sleng Teng", "Wayne Smith - Under Mi Sleng Teng", 1985, "King Jammy (Jammys)", "Noel Davey (Casio MT-40)", "~D:1 G:1", 90, {}),
  ("Punaany", "Admiral Bailey - Punaany", 1986, "King Jammy (Jammys)", "Steely & Clevie", "~G:1 D:1", 94, {}),
  ("Duck", "Little John - Dance Medley", 1985, "King Jammy (Jammys)", "Steely & Clevie", "~Dm:1 C:1", 94, {}),
  ("Kuff", "Frankie Paul - Cassanova", 1987, "King Jammy (Jammys)", "Steely & Clevie", "~Am:1 G:1", 92, {}),
  ("Agony", "Pinchers - Agony", 1987, "King Jammy (Jammys)", "Steely & Clevie", "~Em:1 D:1", 90, {}),
  ("Head To Toe", "Frankie Paul - Head To Toe", 1987, "King Jammy (Jammys)", "Steely & Clevie", "~Gm:1 F:1", 92, {}),
  ("Sara", "Frankie Paul - Sara", 1987, "King Jammy (Jammys)", "Steely & Clevie", "~C:1 G:1", 88, {}),
  ("Chase Vampire", "Wayne Smith - Walk Like Granny", 1986, "King Jammy (Jammys)", "Jammys (sessão)", "~Fm:1 Eb:1", 92, {}),
  ("Tonight (digital)", "King Jammy - Tonight", 1987, "King Jammy (Jammys)", "Steely & Clevie", "~F:1 C:1", 88, {}),
  ("Swing Easy (digital)", "King Everald - Bad Girls", 1986, "King Jammy (Jammys)", "Steely & Clevie", "~Bb:1 Eb:1", 90, {}),
  ("Tempo", "Anthony Red Rose - Tempo", 1985, "King Tubby (Firehouse)", "Noel Davey", "~Am:2", 88, {}),
  ("Ring The Alarm (Stalag)", "Tenor Saw - Ring The Alarm", 1985, "Winston Riley (Techniques)", "Techniques All Stars", "C:1 Bb:1", 90, {}),
  ("Boops", "Super Cat - Boops", 1986, "Winston Riley (Techniques)", "Techniques All Stars", "G:1 C:1", 90, {}),
  ("Mud Up", "Super Cat - Mud Up", 1987, "Kenneth Black", "Dave Kelly", "~Cm:1 Bb:1", 92, {}),
  ("Peanie Peanie", "Shabba Ranks - Peanie Peanie", 1987, "Bobby Digital", "Bobby Digital (programação)", "~Em:1 Am:1", 92, {}),
  ("Rumours", "Gregory Isaacs - Rumours", 1988, "Gussie Clarke (Music Works)", "Music Works (sessão)", "~Ab:1 Bbm:1", 84, {}),
  ("Hard Drugs", "Gregory Isaacs - Hard Drugs", 1988, "Sly & Robbie / Tappa Zukie", "Robbie Shakespeare", "~Am:1 Dm:1", 84, dict(style=ROBBIE)),
  ("Poco Man Jam", "Gregory Peck - Poco Man Jam", 1989, "Steely & Clevie", "Steely & Clevie", "~Gm:1 Cm:1", 96, dict(drum=DANCEHALL)),
  ("Soap", "Dennis Star / Beenie Man - Tata Fool", 1989, "Mafia & Fluxy", "Leroy \"Mafia\" Heywood", "~Dm:1 Bb:1", 92, {}),
  ("Gun In A Baggy", "Little Lenny - Gun In A Baggy", 1989, "Tony Kelly", "Tony Kelly (programação)", "~Em:2", 96, dict(drum=DANCEHALL)),
 ],
 # ---------------------------------------------------------------- 5 RAGGA 1990-95
 [
  ("Dem Bow", "Shabba Ranks - Dem Bow", 1990, "Bobby Digital", "Steely & Clevie", "~Gm:1 F:1", 96, {}),
  ("Fish Market", "Steely & Clevie - Fish Market", 1990, "Steely & Clevie", "Steely & Clevie", "~Am:2", 98, {}),
  ("Giggy", "Baby Wayne - Mama", 1991, "Bulby York / Steely & Clevie", "Steely & Clevie", "~Em:1 D:1", 94, {}),
  ("Top Ten", "Terror Fabulous - Top Ten", 1992, "Bobby Digital", "Bobby Digital (programação)", "~Cm:1 Bb:1", 92, {}),
  ("Muslim", "Papa San - Price Increase", 1992, "Sly & Robbie / Firehouse Crew", "Robbie Shakespeare", "~Am:1 Dm:1", 94, dict(style=ROBBIE)),
  ("Pitch", "Dollar Fifty - The Key", 1992, "Gitsy Willis & Sly Dunbar", "Sly & Robbie", "~Fm:2", 94, dict(style=ROBBIE)),
  ("Gangster", "Anthony Red Rose - The Gangster", 1992, "Sly Dunbar / Gitsy Willis", "Sly & Robbie", "~Dm:1 C:1", 92, dict(style=ROBBIE)),
  ("Murder She Wrote", "Chaka Demus & Pliers - Murder She Wrote", 1992, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "D:1 C:1 D:1 Am:1", 94, dict(style=ROBBIE)),
  ("Tease Me", "Chaka Demus & Pliers - Tease Me", 1993, "Sly & Robbie (Taxi)", "Robbie Shakespeare", "C:2 Ab:2", 94, dict(style=ROBBIE)),
  ("Bellevue", "Red Dragon - Sample", 1993, "Mafia & Fluxy", "Leroy \"Mafia\" Heywood", "~Gm:1 Eb:1", 96, {}),
  ("The Gum", "Captain Willy - You Have The Gum", 1992, "Danny Browne", "Danny Browne (programação)", "~Am:1 G:1", 96, {}),
  ("Dickie", "Nadine Sutherland & Buju Banton - Wicked Dickie", 1992, "Dave Kelly (Penthouse)", "Dave Kelly", "~Cm:2", 94, {}),
  ("Action", "Terror Fabulous & Nadine Sutherland - Action", 1992, "Dave Kelly (Penthouse)", "Dave Kelly", "~Am:1 Em:1", 94, {}),
  ("Pepper Seed", "Beenie Man - Miss Angella", 1993, "Dave Kelly (Penthouse)", "Dave Kelly", "~Dm:2", 96, {}),
  ("Frog", "Merciless - Matey Renk", 1994, "Computer Paul / Steely", "Steely & Clevie", "~Em:2", 96, {}),
  ("Corduroy", "Beenie Man - World Dance", 1994, "Patrick Roberts (Shocking Vibes)", "Shocking Vibes Crew", "~Gm:1 F:1", 96, {}),
  ("Hot Wax", "Merciless - Gun From Paris", 1995, "Sly & Robbie / Bulby (Fat Eyes)", "Robbie Shakespeare", "~Cm:1 Ab:1", 96, dict(style=ROBBIE)),
  ("Fed Up", "Bounty Killer - Fed Up", 1995, "Gitsy / Sly & Robbie", "Robbie Shakespeare", "~Am:2", 94, dict(style=ROBBIE)),
  ("Love Is The Answer", "Garnett Silk - Love Is The Answer", 1993, "Bobby Digital", "Bobby Digital (programação)", "~Cm:1 Gm:1", 80, dict(drum=ONEDROP, style=FAMILY)),
  ("Wanna Be Loved", "Buju Banton - Wanna Be Loved", 1995, "Donovan Germain (Penthouse)", "Dave Kelly", "~Bb:1 Gm:1", 74, dict(drum=ONEDROP, style=FAMILY, timbre=SINE)),
 ],
 # ---------------------------------------------------------------- 6 DANCEHALL 1995-2001
 [
  ("Arab Attack", "Bounty Killer & outros", 1995, "Dave Kelly (Madhouse)", "Dave Kelly", "~Em:2", 96, {}),
  ("Joy Ride", "Wayne Wonder & Baby Cham - Joy Ride", 1996, "Dave Kelly (Madhouse)", "Dave Kelly", "~Am:1 F:1", 96, {}),
  ("Stink", "Rude Boy Kelly - Girls Anthem", 1996, "Dave Kelly (Madhouse)", "Dave Kelly", "~Cm:2", 96, {}),
  ("Showtime", "Alias - Slurp", 1997, "Dave Kelly (Madhouse)", "Dave Kelly", "~Gm:2", 98, {}),
  ("Playground", "Beenie Man - Who Am I", 1997, "Jeremy Harding (2 Hard)", "Jeremy Harding (programação)", "~Fm:1 Db:1", 88, {}),
  ("African Rock", "Shabba Ranks - One Man", 1997, "Bobby Konders (Massive B)", "Bobby Konders (programação)", "~Am:2", 96, {}),
  ("Bookshelf", "Beenie Man - Bookshelf", 1998, "Tony Kelly (K-Licious)", "Tony Kelly (programação)", "~Dm:1 Bb:1", 98, {}),
  ("Bada Bada", "Ward 21 - Haters", 1998, "Ward 21", "Ward 21 (programação)", "~Em:2", 98, {}),
  ("Baddis", "Lady Saw & T.O.K. - Hardcore Lover", 1998, "Richard Browne", "Richard Browne (programação)", "~Gm:1 F:1", 98, {}),
  ("Da Joint", "Black Uhuru - It Ago Red", 1999, "Ward 21 / King Jammy", "Ward 21 (programação)", "~Cm:2", 98, {}),
  ("Bellyas", "Capleton - Who Dem", 1999, "Ward 21 / King Jammy", "Ward 21 (programação)", "~Am:2", 98, {}),
  ("Street Sweeper", "Steely & Clevie - Street Sweeper", 1999, "Steely & Clevie", "Steely & Clevie", "~Dm:2", 98, {}),
  ("Kiki", "Sean Paul - Disrespect", 1999, "Tony Kelly", "Tony Kelly (programação)", "~Bbm:1 Gb:1", 98, {}),
  ("Headache", "Elephant Man & Delly Ranks - Headache", 1999, "Ricky Myrie", "Ricky Myrie (programação)", "~F#m:2", 100, {}),
  ("Joker", "Red Rat - Chunny Mouth", 2000, "Shocking Vibes Crew", "Marlon Cooke / Donald Dennis", "~Em:1 C:1", 100, {}),
  ("Doorslam", "Capleton - Bust It", 2000, "Harvel \"Gadaffi\" Hart (Annex)", "Annex Crew", "~Am:2", 100, {}),
  ("Scarface", "Mr. Vegas - Original Trend", 2000, "Patrick Roberts (Shocking Vibes)", "Shocking Vibes Crew", "~Cm:2", 98, {}),
  ("Speed", "Madd Anju - Look Like", 2000, "Desmond & Rupert Blake", "Desmond & Rupert Blake", "~Gm:2", 102, {}),
  ("Rice And Peas", "Spragga Benz & Elephant Man - Warrior Cause", 2001, "Rohan \"Snow Cone\" Fuller", "Paul \"Teetimus\" Edmund", "~Dm:1 C:1", 100, {}),
  ("Trilogy", "Spragga Benz & Red Square - Gi Wi Dem", 2001, "King Jammy / Ward 21", "Ward 21 (programação)", "~Em:2", 100, {}),
 ],
 # ---------------------------------------------------------------- 7 DANCEHALL 2002-07
 [
  ("Diwali", "Sean Paul - Get Busy / Wayne Wonder - No Letting Go", 2002, "Steven \"Lenky\" Marsden", "Lenky (programação)", "Am:1 Em:1", 100, {}),
  ("Martial Arts", "Sizzla - Karate", 2002, "Craig \"Leftside\" Parks / Scatta", "Leftside (programação)", "~Em:2", 100, {}),
  ("Hard Drive", "Ce'Cile & Tanya Stephens - Buss Back Sketell", 2002, "Christopher Clarke / Gregory Harriot", "programação", "~Am:2", 102, {}),
  ("Famine", "Merciless - Pick A Number", 2002, "Leftside / Dia / Scatta", "Leftside (programação)", "~Cm:2", 100, {}),
  ("Coolie Dance", "Ce'Cile - Give It To Me", 2003, "Scatta & Eva Burrell", "Scatta (programação)", "~Dm:1 Bb:1", 104, {}),
  ("Chrome", "Capleton - In Her Heart", 2003, "Michael Sterling / Craig Marsh / Daniel Lewis", "programação", "~Gm:2", 100, {}),
  ("Bad Company", "Ce'Cile - More Than One Time", 2003, "Cordel \"Scatta\" Burrell", "Scatta (programação)", "~F#m:2", 102, {}),
  ("Applause", "Sean Paul - Temperature", 2004, "Rohan \"Snow Cone\" Fuller", "Snow Cone (programação)", "Bbm:1 Ab:1 Bbm:1 C:1", 104, {}),
  ("Military", "Assassin - Step Pon Dem", 2004, "Christopher Birch", "Christopher Birch (programação)", "~Em:2", 100, {}),
  ("Stepz", "Tami Chynn - Hyperventilating", 2004, "Michael Jarrett / Craig \"Serani\" Marsh", "Serani (programação)", "~Am:1 F:1", 104, {}),
  ("Kasablanca", "Jagwa - Mi Mean That", 2004, "Sly Dunbar / Computer Paul / Alberto Blackwood", "Sly & Robbie", "~Dm:2", 100, dict(style=ROBBIE)),
  ("Blackout", "Vybz Kartel - Picture This", 2004, "—", "programação", "~Cm:2", 100, {}),
  ("Sleng Teng Resurrection", "Luciano - Dancehall Style", 2005, "King Jammy (Jammys)", "King Jammy (relick)", "~D:1 G:1", 96, dict(timbre=SUPER, drum=DIGITAL)),
  ("Forty Sup'emmh", "Pinchers - Mama Mama", 2006, "Steely & Clevie", "Steely & Clevie", "~Gm:1 F:1", 96, {}),
  ("Red Bull & Guinness", "Delly Ranks & Chino - Red Bull & Guinness", 2006, "Delly Ranks / Stephen \"Di Genius\" McGregor", "Di Genius (programação)", "~Em:2", 104, {}),
  ("Ghetto Whiskey", "Mavado & Vybz Kartel - Sunrise", 2006, "Stephen \"Di Genius\" McGregor", "Di Genius (programação)", "~Cm:1 Ab:1", 100, {}),
  ("Power Cut", "Einstein - Caliber", 2006, "Stephen \"Di Genius\" McGregor", "Di Genius (programação)", "~Am:2", 102, {}),
  ("Wipe Out", "Flippa Mafia - Look Alike", 2006, "—", "programação", "~Gm:2", 102, {}),
  ("After Dark", "Sean Paul - All Alone", 2007, "Di Genius / DJ Sunshine", "Di Genius (programação)", "~Fm:1 Db:1", 96, {}),
  ("Tremor", "Craig Dennis - Actor", 2007, "Stephen \"Di Genius\" McGregor", "Di Genius (programação)", "~Bm:2", 104, {}),
 ],
]

NOTE = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6, "Gb": 6, "G": 7, "G#": 8, "Ab": 8,
        "A": 9, "A#": 10, "Bb": 10, "B": 11}
QUAL = {"": 0, "m": 1, "7": 2, "m7": 3}   # mbm::Quality
DUR = {"h": 0, "1": 1, "2": 2, "3": 3, "4": 4, "8": 5}

def parse_chords(s):
    approx = s.startswith("~")
    out = []
    for tok in s.lstrip("~").split():
        name, d = tok.split(":")
        m = re.fullmatch(r"([A-G][b#]?)(m7|m|7|)", name)
        assert m, tok
        out.append((NOTE[m.group(1)], QUAL[m.group(2)], DUR[d]))
    assert 1 <= len(out) <= 4, s
    return out, approx

def decade(y):
    return "%02ds" % ((y // 10 * 10) % 100) if y < 2000 else "%ds" % (y // 10 * 10)

def build():
    rows = []
    for fi, (folder, base) in enumerate(FOLDERS):
        assert len(R[fi]) == 20, (folder, len(R[fi]))
        for (name, origin, year, producer, bass, chords, bpm, extra) in R[fi]:
            p = dict(base); p.update(extra)
            ch, approx = parse_chords(chords)
            rows.append(dict(folder=fi, name=name, origin=origin, year=year, decade=("ANOS " + str(year // 10 * 10)[2:]) if year < 2000 else "ANOS 2000",
                             producer=producer, bass=bass, chords=ch, approx=approx, bpm=bpm or p["bpm"],
                             style=p["style"], drum=p["drum"], timbre=p["timbre"], dens=p["dens"], var=p["var"], swing=p["swing"]))
    names = [r["name"] for r in rows]
    assert len(set(names)) == len(names), "nomes repetidos"
    return rows

def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

def write_header(rows):
    L = []
    L.append("// Gerado por ReggaeBassMachine/tools/riddims.py — não editar à mão.")
    L.append("// RIDDIMS CLÁSSICOS do MRBM-69: 20 por estilo de reggae, com origem, ano, produtor e quem tocou o baixo.")
    L.append("// O baixo é gerado pelo motor no estilo do baixista em cima da harmonia de referência (não é transcrição).")
    L.append("#pragma once")
    L.append("")
    L.append("namespace mbmriddims")
    L.append("{")
    L.append("struct Chord { int root, quality, dur; };   // tônica 0..11, mbm::Quality, duração 0..5 (1/2, 1, 2, 3, 4, 8 comp.)")
    L.append("struct Riddim")
    L.append("{")
    L.append("    int folder; const char* name; const char* origin; int year; const char* decade;")
    L.append("    const char* producer; const char* bass;")
    L.append("    int numChords; Chord chords[4]; bool approx;")
    L.append("    int bpm, style, drum, timbre; float density, variation, swing;")
    L.append("};")
    L.append("")
    L.append("inline constexpr int numFolders = %d;" % len(FOLDERS))
    L.append("inline const char* const folders[numFolders] = { %s };" % ", ".join(cstr(f[0]) for f in FOLDERS))
    L.append("inline const char* const folderShort[numFolders] = { %s };" % ", ".join(cstr(f) for f in SHORT))
    L.append("")
    L.append("inline constexpr int numRiddims = %d;" % len(rows))
    L.append("inline const Riddim riddims[numRiddims] = {")
    for r in rows:
        ch = ", ".join("{ %d, %d, %d }" % c for c in r["chords"])
        L.append("    { %d, %s, %s, %d, %s, %s, %s, %d, { %s }, %s, %d, %d, %d, %d, %.2ff, %.2ff, %.2ff }," % (
            r["folder"], cstr(r["name"]), cstr(r["origin"]), r["year"], cstr(r["decade"]), cstr(r["producer"]), cstr(r["bass"]),
            len(r["chords"]), ch, "true" if r["approx"] else "false", r["bpm"], r["style"], r["drum"], r["timbre"], r["dens"], r["var"], r["swing"]))
    L.append("};")
    L.append("} // namespace mbmriddims")
    open(os.path.join(ROOT, "ReggaeBassMachine/Source/RiddimPresets.h"), "w").write("\n".join(L) + "\n")

def write_js(rows):
    data = dict(folders=[f[0] for f in FOLDERS], short=SHORT, riddims=[dict(f=r["folder"], n=r["name"], o=r["origin"], y=r["year"], d=r["decade"],
                p=r["producer"], b=r["bass"], c=[list(c) for c in r["chords"]], a=1 if r["approx"] else 0, bpm=r["bpm"], s=r["style"],
                dr=r["drum"], t=r["timbre"], den=r["dens"], v=r["var"], sw=r["swing"]) for r in rows])
    js = ("// Gerado por ReggaeBassMachine/tools/riddims.py — não editar à mão. Mesmo banco do plugin MRBM-69.\n"
          "window.MRBM_RIDDIMS = " + json.dumps(data, ensure_ascii=False, separators=(",", ":")) + ";\n")
    open(os.path.join(ROOT, "site/bassmachine/riddims.js"), "w").write(js)

ROOT_N = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]
QN = ["", "m", "7", "m7"]; DN = ["1/2", "1", "2", "3", "4", "8"]

def write_md(rows):
    L = ["# Riddims clássicos do MRBM-69", "",
         "Gerado por `tools/riddims.py`. Cada preset carrega a harmonia de referência, o BPM, a bateria, o timbre e o estilo",
         "do gerador mais próximo de quem tocou o baixo. **As notas são geradas pelo MRBM-69 no estilo do baixista; não são",
         "transcrições das linhas originais.** Harmonia com `~` é aproximada (de ouvido): confira o tom no disco e use a TÔNICA.", ""]
    for fi, (folder, _) in enumerate(FOLDERS):
        L += ["## " + folder, "", "| # | Riddim | Gravação de origem | Ano | Produtor | Baixo | Harmonia | BPM | Estilo no MRBM |",
              "|---|---|---|---|---|---|---|---|---|"]
        for i, r in enumerate([r for r in rows if r["folder"] == fi], 1):
            h = ("~ " if r["approx"] else "") + " · ".join("%s%s (%s)" % (ROOT_N[c[0]], QN[c[1]], DN[c[2]]) for c in r["chords"])
            L.append("| %d | %s | %s | %d | %s | %s | %s | %d | %s |" % (i, r["name"], r["origin"], r["year"], r["producer"], r["bass"], h, r["bpm"], STYLE_NAMES[r["style"]]))
        L.append("")
    open(os.path.join(ROOT, "ReggaeBassMachine/docs/RIDDIMS.md"), "w").write("\n".join(L))

if __name__ == "__main__":
    rows = build()
    write_header(rows); write_js(rows); write_md(rows)
    print("%d riddims em %d estilos" % (len(rows), len(FOLDERS)))
