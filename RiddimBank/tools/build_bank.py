#!/usr/bin/env python3
"""BANCO ÚNICO DE RIDDIMS da Dub Station (MRCM-1240, MRBM-69, MRDM-12 e MRM-0420).

Junta os três bancos que já existem, sem mudar nenhum deles:
  ReggaeChordMachine/Source/HarmonyPresets.h   (180 músicas, harmonia + BPM + músicos)
  ReggaeBassMachine/tools/riddims.py            (160 riddims de baixo)
  ReggaeDrumMachine/tools/grooves.json          (118 grooves de bateria)
e aplica RiddimBank/correcoes.json (pesquisa de nov/2026).

Cada entrada é UMA música (gravação), com:
  - harmonia única (e de onde ela veio),
  - BPM único (contado como a bateria do MRDM-12: caixa no 2 e no 4, nunca acima de 100),
  - bateria: o groove ORIGINAL quando o MRDM-12 tem a música; senão, um groove ESCOLHIDO do baterista
    mais próximo de quem gravou (mesmo estilo de bateria, BPM e época mais próximos),
  - baixo: o riddim ORIGINAL quando o MRBM-69 tem a música; senão, o estilo do baixista mais próximo
    de quem gravou, com os ajustes da época,
  - os índices nos bancos atuais (a ordem deles NÃO muda: o índice fica salvo nos projetos).

Gera: RiddimBank/riddims.json (fonte única), RiddimBank/CATALOGO.md (para ler) e RiddimBank/riddims.csv.
Uso: python3 RiddimBank/tools/build_bank.py   (na raiz do repositório)
"""
import csv, json, os, re, unicodedata
from collections import Counter, OrderedDict

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "RiddimBank")
NOTE = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]
ROOTS = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8, 'Ab': 8,
         'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}
DURS = {'1/2': 0, 'h': 0, '½': 0, '1': 1, '2': 2, '3': 3, '4': 4, '8': 5}
DURTXT = ['1/2', '1', '2', '3', '4', '8']
DRUMMERS = ["STYLE SCOTT", "SANTA DAVIS", "CARLTON BARRETT", "SLY DUNBAR", "DIGIKAL"]
BASSISTS = ["FAMILY MAN", "FLABBA", "DERRICK BARNETT", "ROBBIE", "LEROY SIBBLES", "FULLY FULLWOOD", "JAMMYS"]
KITS = ["ROCKERS", "ONE DROP", "BOOM BAP", "DEMBOW"]
STYLES = ["ROCKSTEADY", "EARLY REGGAE", "ROOTS", "ROCKERS", "LOVERS ROCK", "RUB A DUB", "DIGITAL", "RAGGA",
          "DANCEHALL", "NEW ROOTS"]
STYLE_YEARS = {"ROCKSTEADY": "1966-68", "EARLY REGGAE": "1968-72", "ROOTS": "1972-79", "ROCKERS": "1975-80",
               "LOVERS ROCK": "1969-94", "RUB A DUB": "1979-84", "DIGITAL": "1985-89", "RAGGA": "1990-95",
               "DANCEHALL": "1995-2007", "NEW ROOTS": "1999-hoje"}


def norm(s):
    s = unicodedata.normalize("NFKD", s.lower().replace("’", "'"))
    s = "".join(c for c in s if not unicodedata.combining(c))
    s = re.sub(r"\(.*?\)|\[.*?\]", "", s)
    s = re.sub(r"\b(the|a)\b", " ", s)
    for a, b in (("punanny", "punaany"), ("sensimilla", "sinsemilla"), ("firehouse", "fire house"),
                 ("zungguzungguguzungguzeng", "zungguzung"), ("sleng teng resurrection", "slengtengres"),
                 ("under mi sleng teng", "sleng teng"), ("coming in from cold", "cominginfromcold")):
        s = s.replace(a, b)
    s = re.sub(r"[^a-z0-9]+", "", s)
    return s.replace("cominginfromcold", "cominginfromcold")


# títulos iguais de músicas diferentes: a chave leva o artista
HOMONIMOS = {"topten", "sinsemilla", "moneyinmypocket", "bambam", "kingstontown", "smilejamaica", "equalrights", "war",
             "sunisshining"}
BASE_KEY = lambda k: k.split("|")[0]


def key(title, artist=""):
    k = norm(title)
    if k == "cominginfromcold" or k == "cominginfromthecold":
        k = "cominginfromcold"
    return k + "|" + norm(artist) if k in HOMONIMOS else k


def parse_h(spec):
    """'Am:1 G:1' (formatos do MRCM e do MRBM) -> [(tônica, menor 0/1, duração 0..5)], texto, aproximada?"""
    approx = spec.startswith("~")
    out = []
    for tok in spec.lstrip("~").split():
        name, dur = tok.split(":")
        q = ""
        m = re.match(r"^([A-G][#b]?)(.*)$", name)
        root, q = m.group(1), m.group(2)
        minor = q.startswith("m") and not q.startswith("maj")
        out.append((ROOTS[root], 1 if minor else 0, DURS[dur]))
    return out, approx


def htext(h):
    return " ".join(NOTE[r] + ("m" if m else "") + ":" + DURTXT[d] for r, m, d in h)


# ------------------------------------------------------------------ bancos atuais
def load_mrcm():
    src = open(os.path.join(ROOT, "ReggaeChordMachine/Source/HarmonyPresets.h"), encoding="utf-8").read()
    rows = re.findall(r'\{ "([^"]+)", "([^"]+)", "([^"]+)", (\d), \{((?: \{ [^}]* \},?)+) \}, (\d+), (\d), (\d), (\d), (\d) \}', src)
    out = []
    for i, (folder, era, name, n, ch, bpm, kit, dr, bx, real) in enumerate(rows):
        cs = [tuple(int(v) for v in c) for c in re.findall(r'\{ (\d+), (\d), (\d) \}', ch)[:int(n)]]
        m = re.match(r"^(.*?) \((.*?)(?: (\d{4}))?(?:, .*?)?\)(?: \[auto\])?$", name)
        title, artist, year = (m.group(1), m.group(2), int(m.group(3)) if m.group(3) else None) if m else (name, "", None)
        out.append(dict(idx=i, name=name, title=title, artist=artist, year=year, folder=folder, h=cs, bpm=int(bpm),
                        kit=int(kit), dr=int(dr), bx=int(bx), real=int(real), auto="[auto]" in name))
    return out


def load_mrbm():
    src = open(os.path.join(ROOT, "ReggaeBassMachine/tools/riddims.py"), encoding="utf-8").read().split("\ndef parse_chords")[0]
    g = {"__file__": os.path.join(ROOT, "ReggaeBassMachine/tools/riddims.py")}
    exec(src, g)
    out, i = [], 0
    for fi, fold in enumerate(g["R"]):
        dflt = g["FOLDERS"][fi][1]
        for (riddim, rec, year, prod, bass, harm, bpm, extra) in fold:
            p = dict(dflt); p.update(extra)
            artist, song = (rec.split(" - ", 1) + [""])[:2] if " - " in rec else ("", rec)
            h, approx = parse_h(harm)
            out.append(dict(idx=i, riddim=riddim, rec=rec, artist=artist, song=song, year=year, prod=prod, bassman=bass,
                            h=h, approx=approx, htxt=harm, bpm=bpm or dflt["bpm"], style=p["style"], drum=p["drum"],
                            timbre=p["timbre"], dens=p["dens"], var=p["var"], swing=p["swing"], folder=g["SHORT"][fi]))
            i += 1
    return out


def load_mrdm():
    data = json.load(open(os.path.join(ROOT, "ReggaeDrumMachine/tools/grooves.json"), encoding="utf-8"))["grooves"]
    data = sorted(data, key=lambda g: g["drummer"])            # mesma ordem do GroovePresets.h
    return [dict(idx=i, **g) for i, g in enumerate(data)]


# ------------------------------------------------------------------ regras de escolha
def drummer_for(prod, artist, year):
    t = f"{prod} {artist}".lower()
    if year and year >= 1985:
        return 3 if ("sly" in t or "taxi" in t) else 4
    if any(w in t for w in ("junjo", "volcano", "radics", "linval", "scientist", "sugar minott", "solomonic", "gregory isaacs")):
        return 0
    if any(w in t for w in ("taxi", "sly", "channel one", "hookim", "revolutionaries", "black uhuru", "joe gibbs")):
        return 3
    if any(w in t for w in ("wailers", "tuff gong", "island", "upsetters", "perry", "marley", "black ark", "pablo", "tubby")):
        return 2
    if any(w in t for w in ("bunny lee", "soul syndicate", "niney", "jack ruby", "tosh", "techniques", "riley")):
        return 1
    if year and year < 1979:
        return 2 if year < 1972 else 1
    return 0


def bassist_for(prod, artist, year):
    t = f"{prod} {artist}".lower()
    if year and year >= 1990:
        return 3 if ("sly" in t or "taxi" in t) else 2
    if year and year >= 1985:
        return 6
    if any(w in t for w in ("wailers", "tuff gong", "island", "upsetters", "perry", "marley")):
        return 0
    if any(w in t for w in ("junjo", "volcano", "radics", "linval", "gregory isaacs", "sugar minott", "solomonic")):
        return 1
    if any(w in t for w in ("taxi", "sly", "channel one", "hookim", "black uhuru", "revolutionaries")):
        return 3
    if any(w in t for w in ("bunny lee", "soul syndicate", "niney", "jack ruby", "tosh")):
        return 5
    if any(w in t for w in ("studio one", "coxsone", "treasure isle", "duke reid")):
        return 4
    return 1 if (year or 0) >= 1979 else 5


def style_for(year, prod="", artist=""):
    t = f"{prod} {artist}".lower()
    y = year or 1977
    if y < 1968: return "ROCKSTEADY"
    if y < 1973: return "EARLY REGGAE"
    if y < 1980:
        return "ROCKERS" if any(w in t for w in ("channel one", "hookim", "revolutionaries", "taxi", "sly", "black uhuru", "steel pulse", "joe gibbs")) and y >= 1975 else "ROOTS"
    if y < 1985: return "RUB A DUB"
    if y < 1990: return "DIGITAL"
    if y < 1995: return "RAGGA"
    if y < 2008: return "DANCEHALL"
    return "NEW ROOTS"


MRCM_STYLE = {"EARLY REGGAE": "EARLY REGGAE", "ROOTS": "ROOTS", "ROCKERS": "ROCKERS", "LOVERS ROCK": "LOVERS ROCK",
              "RUB A DUB": "RUB A DUB", "DIGITAL": "DIGITAL", "NEW ROOTS": "NEW ROOTS"}
# ajustes do baixo por estilo (os padrões das pastas do MRBM-69, por época)
BASS_ERA = {
    "ROCKSTEADY":   dict(drum=1, timbre=0, dens=.55, var=.45, swing=.20),
    "EARLY REGGAE": dict(drum=1, timbre=0, dens=.55, var=.50, swing=.15),
    "ROOTS":        dict(drum=1, timbre=0, dens=.55, var=.50, swing=.15),
    "ROCKERS":      dict(drum=0, timbre=0, dens=.60, var=.45, swing=.10),
    "LOVERS ROCK":  dict(drum=1, timbre=0, dens=.55, var=.45, swing=.15),
    "RUB A DUB":    dict(drum=0, timbre=0, dens=.60, var=.45, swing=.10),
    "DIGITAL":      dict(drum=2, timbre=1, dens=.55, var=.40, swing=.05),
    "RAGGA":        dict(drum=3, timbre=1, dens=.55, var=.45, swing=.05),
    "DANCEHALL":    dict(drum=3, timbre=2, dens=.50, var=.45, swing=0.0),
    "NEW ROOTS":    dict(drum=1, timbre=0, dens=.55, var=.45, swing=.10),
}


def pick_groove(grooves, drummer, kit, bpm, year):
    cands = [g for g in grooves if g["drummer"] == drummer]
    same = [g for g in cands if g["style"] == kit] or cands
    return min(same, key=lambda g: (abs(g["bpm"] - bpm), abs(g["year"] - (year or g["year"]))))


def slug(s):
    s = unicodedata.normalize("NFKD", s.lower())
    s = "".join(c for c in s if not unicodedata.combining(c))
    return re.sub(r"[^a-z0-9]+", "-", s).strip("-")


# ------------------------------------------------------------------ montagem
def build():
    cor = json.load(open(os.path.join(OUT, "correcoes.json"), encoding="utf-8"))
    juntar = {k: v for k, v in cor["juntar"].items() if not k.startswith("_")}
    riddim_de = {k: v for k, v in cor["riddimDe"].items() if not k.startswith("_")}
    cm, bm, dm = load_mrcm(), load_mrbm(), load_mrdm()

    parent = {}
    def find(x):
        while parent.setdefault(x, x) != x:
            parent[x] = parent[parent[x]]; x = parent[x]
        return x
    def union(a, b): parent[find(a)] = find(b)
    def kk(k): return juntar.get(k, k)

    items = []
    for p in cm: items.append(("CM", p, [kk(key(p["title"], p["artist"]))]))
    for b in bm:
        ks = [kk(key(b["riddim"], b["artist"]))]
        if norm(b["song"]) and norm(b["song"]) != norm(b["riddim"]):
            ks.append(kk(key(b["song"], b["artist"])))
        items.append(("BM", b, ks))
    for g in dm: items.append(("DM", g, [kk(key(g["song"], g["artist"]))]))
    for i, (_, _, ks) in enumerate(items):
        for k in ks: union(("i", i), ("k", k))
    groups = OrderedDict()
    for i, it in enumerate(items): groups.setdefault(find(("i", i)), []).append(it)

    hcor = {k: v for k, v in cor["harmonia"].items()}
    entries = []
    for g in groups.values():
        c = [x[1] for x in g if x[0] == "CM"]; b = [x[1] for x in g if x[0] == "BM"]; d = [x[1] for x in g if x[0] == "DM"]
        c0, b0, d0 = (c[0] if c else None), (b[0] if b else None), (d[0] if d else None)
        keys = {k for x in g for k in x[2]}
        # identidade
        if c0: nome, artista, ano = c0["title"], c0["artist"], c0["year"]
        elif d0: nome, artista, ano = d0["song"], re.sub(r" \d{4}$", "", d0["artist"]), d0["year"]
        elif " - " in b0["rec"]: nome, artista, ano = b0["song"], b0["artist"], b0["year"]
        else: nome, artista, ano = b0["riddim"] + " riddim", re.sub(r" \d{4}$", "", b0["rec"]), b0["year"]
        if not ano: ano = (d0 or b0 or {}).get("year")
        prod = (b0 or {}).get("prod") or (d0 or {}).get("producer") or ""
        riddim = b0["riddim"] if b0 else next((v for k, v in riddim_de.items() if k in keys), "")
        # estilo
        if c0 and c0["folder"] in MRCM_STYLE: estilo = MRCM_STYLE[c0["folder"]]
        elif b0 and b0["folder"].startswith("ROCKSTEADY"): estilo = "ROCKSTEADY"
        elif b0 and b0["folder"].startswith(("RUB A DUB", "DIGITAL", "RAGGA", "DANCEHALL")): estilo = b0["folder"].rsplit(" ", 1)[0]
        else: estilo = style_for(ano, prod, artista)
        # harmonia
        conferir = []; hstatus = "conferida"
        hk = next((BASE_KEY(k) for k in keys if BASE_KEY(k) in hcor and not (BASE_KEY(k) == "sunisshining" and "blackuhuru" in k)), None)
        if hk:
            hh, _ = parse_h(hcor[hk]["h"]); hfonte = "pesquisa: " + hcor[hk]["fonte"]
            if hcor[hk].get("conferir"): conferir.append("harmonia: confirmar de ouvido"); hstatus = "aproximada"
        elif c0 and not c0["auto"]:
            hh, hfonte = c0["h"], "MRCM-1240 (cifras feitas por pessoas)"
        elif b0 and not b0["approx"]:
            hh, hfonte = b0["h"], "MRBM-69"
        elif c0:
            hh, hfonte = c0["h"], "MRCM-1240 (detecção automática)"; hstatus = "aproximada"
        elif b0:
            hh, hfonte = b0["h"], "MRBM-69 (de ouvido, ~)"; hstatus = "aproximada"
        else:
            hh, hfonte = [], "pendente"; hstatus = "pendente"
        if c0 and b0 and not hk:
            a = htext(c0["h"]); z = htext(b0["h"])
            if a != z: conferir.append("harmonia (bancos divergem: MRCM %s / MRBM %s)" % (a, z))
        # BPM
        cands = {}
        if c0: cands["MRCM"] = c0["bpm"]
        if b0: cands["MRBM"] = b0["bpm"]
        if d0: cands["MRDM"] = d0["bpm"]
        if c0 and c0["real"]: bpm, bfonte = c0["bpm"], "medido na gravação (MRCM-1240)"
        elif d0: bpm, bfonte = d0["bpm"], "groove do MRDM-12" + (" (estimado)" if d0.get("approx") else "")
        elif b0: bpm, bfonte = b0["bpm"], "típico da época (MRBM-69)"
        elif c0: bpm, bfonte = c0["bpm"], "típico da época (MRCM-1240)"
        else: bpm, bfonte = {"DIGITAL": 90, "RAGGA": 94, "DANCEHALL": 96}.get(estilo, 76), "típico da época"
        vs = list(cands.values())
        if len(vs) > 1 and (max(vs) - min(vs)) / min(vs) > 0.15:
            conferir.append("BPM (bancos: %s)" % ", ".join(f"{k} {v}" for k, v in cands.items()))
        # bateria
        if d0:
            drummer, kit = d0["drummer"], d0["style"]
            groove = dict(indice=d0["idx"], musica=f'{d0["song"]} ({d0["artist"]})', original=True)
        else:
            drummer = c0["dr"] if c0 else drummer_for(prod, artista, ano)
            kit = c0["kit"] if c0 else ({0: 0, 1: 1, 2: 0, 3: 0}[b0["drum"]] if b0 else (1 if estilo in ("ROOTS", "EARLY REGGAE", "LOVERS ROCK", "NEW ROOTS") else 0))
            if estilo == "RAGGA" and riddim in ("Dem Bow", "Fish Market", "Poco Man Jam"): kit = 3
            gp = pick_groove(dm, drummer, kit, bpm, ano)
            groove = dict(indice=gp["idx"], musica=f'{gp["song"]} ({gp["artist"]})', original=False)
        # baixo
        era = BASS_ERA[estilo]
        if b0:
            baixo = dict(estilo=BASSISTS[b0["style"]], riddimMRBM=b0["idx"], original=True, quemGravou=b0["bassman"],
                         timbre=b0["timbre"], densidade=b0["dens"], variacao=b0["var"], swing=b0["swing"])
        else:
            bx = c0["bx"] if c0 else bassist_for(prod, artista, ano)
            baixo = dict(estilo=BASSISTS[bx], riddimMRBM=None, original=False, quemGravou="",
                         timbre=era["timbre"], densidade=era["dens"], variacao=era["var"], swing=era["swing"])
        e = OrderedDict(
            id="", nome=nome, artista=artista, ano=ano, estilo=estilo, riddim=riddim, produtor=prod,
            harmonia=[dict(tonica=NOTE[r], menor=bool(m), compassos=DURTXT[du]) for r, m, du in hh],
            harmoniaTexto=htext(hh), harmoniaStatus=hstatus, harmoniaFonte=hfonte, bpm=bpm, bpmFonte=bfonte,
            bateria=OrderedDict(baterista=DRUMMERS[drummer], kit=KITS[kit], groove=groove),
            baixo=baixo,
            completo=bool(hh),
            original=OrderedDict(harmonia=bool(c0 or b0 or hk), baixo=bool(b0), bateria=bool(d0)),
            indices=OrderedDict(MRCM=c0["idx"] if c0 else None, MRBM=b0["idx"] if b0 else None, MRDM=d0["idx"] if d0 else None),
            conferir=conferir)
        entries.append(e)

    for n in cor["novos"]:
        est = style_for(n["ano"], n["produtor"], n["artista"])
        dr = drummer_for(n["produtor"], n["artista"], n["ano"]); bx = bassist_for(n["produtor"], n["artista"], n["ano"])
        bpm = {"DIGITAL": 90, "RAGGA": 94, "DANCEHALL": 96}.get(est, 80)
        gp = pick_groove(dm, dr, 0, bpm, n["ano"]); era = BASS_ERA[est]
        entries.append(OrderedDict(
            id="", nome=n["nome"], artista=n["artista"], ano=n["ano"], estilo=est, riddim=n["riddim"], produtor=n["produtor"],
            harmonia=[], harmoniaTexto="", harmoniaStatus="pendente", harmoniaFonte="pendente (" + n["fonte"] + ")", bpm=bpm, bpmFonte="típico da época",
            bateria=OrderedDict(baterista=DRUMMERS[dr], kit="ROCKERS", groove=dict(indice=gp["idx"], musica=f'{gp["song"]} ({gp["artist"]})', original=False)),
            baixo=dict(estilo=BASSISTS[bx], riddimMRBM=None, original=False, quemGravou="", timbre=era["timbre"],
                       densidade=era["dens"], variacao=era["var"], swing=era["swing"]),
            completo=False, original=OrderedDict(harmonia=False, baixo=False, bateria=False),
            indices=OrderedDict(MRCM=None, MRBM=None, MRDM=None), conferir=[]))

    entries.sort(key=lambda e: (STYLES.index(e["estilo"]), (e["ano"] or 0), e["nome"].lower()))
    seen = Counter()
    for e in entries:
        s = slug(e["nome"] + "-" + (e["artista"] or "")) or "riddim"
        seen[s] += 1
        e["id"] = s if seen[s] == 1 else f"{s}-{seen[s]}"
    return entries, cm, bm, dm


def write(entries, cm, bm, dm):
    stats = OrderedDict(
        total=len(entries), comHarmonia=sum(e["completo"] for e in entries),
        harmoniaPendente=sum(not e["completo"] for e in entries),
        bateriaOriginal=sum(e["original"]["bateria"] for e in entries), baixoOriginal=sum(e["original"]["baixo"] for e in entries),
        tresOriginais=sum(all(e["original"].values()) for e in entries),
        harmoniaConferida=sum(e["harmoniaStatus"] == "conferida" for e in entries),
        harmoniaAproximada=sum(e["harmoniaStatus"] == "aproximada" for e in entries),
        divergencias=sum(bool(e["conferir"]) for e in entries),
        porEstilo=OrderedDict((s, sum(e["estilo"] == s for e in entries)) for s in STYLES))
    bank = OrderedDict(
        _leia=["BANCO ÚNICO DE RIDDIMS da Dub Station. Gerado por RiddimBank/tools/build_bank.py: não editar à mão.",
               "Correções e acréscimos ficam em RiddimBank/correcoes.json.",
               "BPM contado como a bateria do MRDM-12 (caixa no 2 e no 4, nunca acima de 100).",
               "original=true: veio da gravação (banco da máquina tem essa música). false: escolhido pelo músico mais próximo de quem gravou.",
               "indices: posição nos bancos atuais (HarmonyPresets.h, RiddimPresets.h, GroovePresets.h). A ordem desses bancos não muda."],
        estilos=[dict(nome=s, anos=STYLE_YEARS[s]) for s in STYLES], estatisticas=stats, riddims=entries)
    json.dump(bank, open(os.path.join(OUT, "riddims.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)

    cols = ["Estilo", "Música", "Artista", "Ano", "Riddim", "Harmonia", "BPM", "Baterista", "Kit", "Groove (MRDM-12)",
            "Bateria", "Baixista", "Baixo", "Quem gravou o baixo", "Harmonia: status", "Harmonia: fonte", "BPM: fonte", "Conferir", "ID"]
    with open(os.path.join(OUT, "riddims.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f); w.writerow(cols)
        for e in entries:
            w.writerow([e["estilo"], e["nome"], e["artista"], e["ano"] or "", e["riddim"], e["harmoniaTexto"] or "pendente", e["bpm"],
                        e["bateria"]["baterista"], e["bateria"]["kit"], e["bateria"]["groove"]["musica"],
                        "original" if e["bateria"]["groove"]["original"] else "escolhida",
                        e["baixo"]["estilo"], "original (MRBM-69)" if e["baixo"]["original"] else "gerada no estilo",
                        e["baixo"]["quemGravou"], e["harmoniaStatus"], e["harmoniaFonte"], e["bpmFonte"], "; ".join(e["conferir"]), e["id"]])

    L = ["# Catálogo unificado de riddims — Dub Station", "",
         "Gerado por `RiddimBank/tools/build_bank.py` a partir dos bancos do MRCM-1240, do MRBM-69 e do MRDM-12, com as correções de `RiddimBank/correcoes.json`.", "",
         f"**{stats['total']} músicas e riddims** em {len(STYLES)} estilos. {stats['comHarmonia']} com harmonia, {stats['harmoniaPendente']} com harmonia pendente. "
         f"Bateria original em {stats['bateriaOriginal']}, baixo original em {stats['baixoOriginal']}, as três partes originais em {stats['tresOriginais']}. "
         f"Harmonia conferida em {stats['harmoniaConferida']}, aproximada em {stats['harmoniaAproximada']}. {stats['divergencias']} entradas têm divergência entre os bancos (⚠).", "",
         "Legenda: **orig.** = a máquina tem a gravação; **esc.** = escolhido pelo músico mais próximo de quem gravou.", ""]
    for s in STYLES:
        es = [e for e in entries if e["estilo"] == s]
        L += [f"## {s} ({STYLE_YEARS[s]}) — {len(es)}", "", "| Música | Ano | Riddim | Harmonia | BPM | Bateria | Baixo |", "|---|---|---|---|---|---|---|"]
        for e in es:
            bt = f'{e["bateria"]["baterista"]} · {e["bateria"]["kit"]} ({"orig." if e["bateria"]["groove"]["original"] else "esc."})'
            bx = f'{e["baixo"]["estilo"]} ({"orig." if e["baixo"]["original"] else "esc."})'
            h = (e["harmoniaTexto"] + (" ~" if e["harmoniaStatus"] == "aproximada" else "")) if e["harmoniaTexto"] else "*pendente*"
            flag = " ⚠" if e["conferir"] else ""
            L.append(f'| {e["nome"]} — {e["artista"]}{flag} | {e["ano"] or ""} | {e["riddim"]} | {h} | {e["bpm"]} | {bt} | {bx} |')
        L.append("")
    L += ["~ = harmonia aproximada (detecção automática ou de ouvido). ⚠ = os bancos antigos divergiam na harmonia ou no BPM: conferir de ouvido."]
    open(os.path.join(OUT, "CATALOGO.md"), "w", encoding="utf-8").write("\n".join(L) + "\n")
    return stats


if __name__ == "__main__":
    entries, cm, bm, dm = build()
    # checagens
    assert len({e["id"] for e in entries}) == len(entries)
    for e in entries:
        assert 50 <= e["bpm"] <= 100, (e["nome"], e["bpm"])
        assert len(e["harmonia"]) <= 4, e["nome"]
    used = Counter(e["indices"]["MRCM"] for e in entries if e["indices"]["MRCM"] is not None)
    assert len(used) == len(cm), "algum preset do MRCM ficou de fora"
    stats = write(entries, cm, bm, dm)
    print(json.dumps(stats, ensure_ascii=False, indent=1))
