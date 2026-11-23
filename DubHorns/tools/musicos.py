# Gera DubHorns/Source/Musicians.h e o bloco JS da versão web a partir de uma tabela só.
# Uso (na pasta DubHorns):  python3 tools/musicos.py cpp > Source/Musicians.h   ·   python3 tools/musicos.py js  (cole no site/web/dubhorns/index.html)
import json, sys
INST=['trumpet','flugel','alto','tenor','trombone','bari']
SH={'trumpet':'TPT','flugel':'FLG','alto':'ALT','tenor':'TEN','trombone':'TBN','bari':'BAR'}
# timbre: bright breath vib scoop attack release drive band lowcut hiss human width growl room
TIMBRE=['bright','breath','vib','scoop','attack','release','drive','band','lowcut','hiss','human','width','growl','room']
BASE=dict(bright=.55,breath=.40,vib=.30,scoop=.40,attack=1.0,release=.30,drive=.35,band=9000,lowcut=80,hiss=.15,human=.50,width=.70,growl=.30,room=.25,
          echoOn=1,sync=5,fb=.45,send=.30,tone=3200,eq=[0]*10)
def sec(**k):
    # k: inst=(lvl,pan,oct,harm)
    out={}
    for i in INST:
        if i in k: l,p,o,h=k[i]; out[i]=dict(on=1,lvl=l,pan=p,oct=o,harm=h)
        else: out[i]=dict(on=0,lvl=.7,pan=0,oct=0,harm=0)
    return out
F=['SKA 63-66 · SKATALITES','ROCKSTEADY 66-68','STUDIO ONE 68-79','ROOTS · CHANNEL ONE 75-79','WAILERS · ISLAND 73-80','80s · DEAN FRASER','DUB · KING TUBBY / BLACK ARK']
P=[]
def add(folder,name,role,desc,naipe,artic='Fall',**t):
    d=dict(BASE); d.update(t); d['eq']=t.get('eq',[0]*10)
    P.append(dict(folder=F[folder],name=name,role=role,desc=desc,naipe=naipe,artic=artic,**{k:d[k] for k in TIMBRE+['echoOn','sync','fb','send','tone','eq']}))
EQ_WARM=[1,1.5,1,.5,1,0,-1,-1.5,-3,-5]; EQ_MID=[0,0,-1,0,1.5,2.5,1.5,0,-2,-4]; EQ_BRIGHT=[0,0,0,-1,0,1,2,2.5,1,-1]; EQ_RADIO=[-6,-4,-2,0,2,4,3,0,-4,-8]
# ---- SKA
add(0,'Roland Alphonso','SAX TENOR · STUDIO ONE','Tenor de jazz no ska: solto, vibrato largo e bastante sopro. O "Chief Musician".',
    sec(tenor=(.85,0,0,0)),'Ten',bright=.55,breath=.55,vib=.55,scoop=.45,attack=1.1,release=.40,drive=.30,band=8200,lowcut=90,hiss=.20,human=.60,width=.5,growl=.40,room=.40,sync=4,fb=.30,send=.18,eq=EQ_WARM)
add(0,'Don Drummond','TROMBONE · SKATALITES','Trombone grave e melancólico, frases em tom menor, ataque macio e cauda longa.',
    sec(trombone=(.9,0,0,0)),'Ten',bright=.42,breath=.45,vib=.25,scoop=.50,attack=1.4,release=.50,drive=.30,band=7600,lowcut=70,hiss=.20,human=.55,width=.4,growl=.25,room=.45,sync=4,fb=.30,send=.20,eq=EQ_WARM)
add(0,'Johnny "Dizzy" Moore','TROMPETE · SKATALITES','Trompete brilhante de ska: ataque rápido e notas curtas, certeiras.',
    sec(trumpet=(.85,0,0,0)),'Stac',bright=.78,breath=.30,vib=.30,scoop=.30,attack=.7,release=.22,drive=.35,band=9500,lowcut=110,hiss=.18,human=.50,width=.5,growl=.15,room=.35,sync=4,fb=.30,send=.18,eq=EQ_BRIGHT)
add(0,'Naipe Skatalites','TPT · ALT · TEN · TBN','O naipe de quatro do ska: trompete em cima, alto e tenor no meio, trombone embaixo.',
    sec(trumpet=(.75,-.35,0,0),alto=(.65,.25,0,2),tenor=(.75,.1,-1,0),trombone=(.7,-.1,0,3)),'Stac',bright=.62,breath=.40,vib=.38,scoop=.35,attack=.9,release=.28,drive=.35,band=8400,lowcut=95,hiss=.20,human=.65,width=.75,growl=.30,room=.45,sync=4,fb=.30,send=.18,eq=EQ_WARM)
# ---- ROCKSTEADY
add(1,'Tommy McCook · Supersonics','SAX TENOR · TREASURE ISLE','Tenor suave e aveludado da Treasure Isle: pouco brilho, vibrato tranquilo, tudo redondo.',
    sec(tenor=(.85,0,0,0)),'Ten',bright=.36,breath=.48,vib=.45,scoop=.35,attack=1.3,release=.42,drive=.20,band=8000,lowcut=70,hiss=.18,human=.50,width=.5,growl=.20,room=.40,sync=5,fb=.35,send=.20,eq=EQ_WARM)
add(1,'Naipe Supersonics','TPT · TEN · TBN','Trio macio do rocksteady: frases longas em terças, sem agressividade.',
    sec(trumpet=(.7,-.3,0,0),tenor=(.8,.15,-1,0),trombone=(.7,.35,0,2)),'Ten',bright=.40,breath=.45,vib=.45,scoop=.35,attack=1.3,release=.40,drive=.20,band=8200,lowcut=70,hiss=.18,human=.55,width=.65,growl=.20,room=.40,sync=5,fb=.35,send=.20,eq=EQ_WARM)
# ---- STUDIO ONE
add(2,'Vin Gordon "Don D Junior"','TROMBONE · STUDIO ONE','Trombone gordo e cantado, herdeiro do Don Drummond: legato, scoop no ataque e fall comprido.',
    sec(trombone=(.9,0,0,0)),'Fall',bright=.50,breath=.42,vib=.30,scoop=.55,attack=1.3,release=.45,drive=.40,band=7800,lowcut=65,hiss=.16,human=.55,width=.4,growl=.35,room=.35,sync=5,fb=.40,send=.25,eq=EQ_WARM)
add(2,'Bobby Ellis','TROMPETE · STUDIO ONE','Trompete claro e lírico do arranjador da Studio One: afinação firme, vibrato curto.',
    sec(trumpet=(.85,0,0,0)),'Ten',bright=.62,breath=.32,vib=.28,scoop=.30,attack=.9,release=.30,drive=.30,band=9000,lowcut=100,hiss=.15,human=.40,width=.5,growl=.10,room=.35,sync=5,fb=.40,send=.25,eq=EQ_MID)
add(2,'Cedric "Im" Brooks','SAX TENOR · STUDIO ONE','Tenor espiritual e largo, muito ar na palheta, vibrato lento; pede eco longo.',
    sec(tenor=(.85,0,0,0)),'Scoop',bright=.40,breath=.65,vib=.50,scoop=.50,attack=1.4,release=.55,drive=.30,band=7400,lowcut=75,hiss=.22,human=.60,width=.5,growl=.45,room=.45,sync=2,fb=.50,send=.30,eq=EQ_WARM)
add(2,'Im & Dave','TEN + TPT','Tenor e trompete em uníssono de oitava: a dupla de sopros da Studio One.',
    sec(trumpet=(.75,-.3,0,0),tenor=(.8,.3,-1,0)),'Ten',bright=.52,breath=.50,vib=.40,scoop=.40,attack=1.1,release=.40,drive=.30,band=8200,lowcut=85,hiss=.18,human=.55,width=.7,growl=.35,room=.40,sync=5,fb=.40,send=.25,eq=EQ_WARM)
# ---- ROOTS / CHANNEL ONE
add(3,'Revolutionaries horns','TEN · ALT · TBN','O naipe de Channel One (McCook, Headley Bennett, Vin Gordon): pontadas secas com punch de rockers.',
    sec(alto=(.65,-.3,0,1),tenor=(.8,.05,-1,0),trombone=(.72,.35,0,2)),'Stac',bright=.75,breath=.30,vib=.25,scoop=.35,attack=.6,release=.20,drive=.50,band=11000,lowcut=110,hiss=.10,human=.45,width=.75,growl=.35,room=.20,sync=5,fb=.40,send=.25,eq=EQ_MID)
add(3,'McCook & Ellis · Blazing Horns','TEN + TPT · DUB','Tenor e trompete feitos para o dub: gravados em Channel One, mixados no Tubby. Eco generoso.',
    sec(trumpet=(.78,-.35,0,0),tenor=(.82,.3,-1,0)),'Fall',bright=.58,breath=.42,vib=.32,scoop=.42,attack=1.0,release=.35,drive=.55,band=7600,lowcut=130,hiss=.28,human=.55,width=.8,growl=.35,room=.25,sync=5,fb=.55,send=.38,tone=2800,eq=EQ_MID)
add(3,'Herman Marquis','SAX ALTO · AGGROVATORS','Alto cortante e nasal das sessões de Bunny Lee: corta a mix do rockers.',
    sec(alto=(.85,0,0,0)),'Stac',bright=.70,breath=.40,vib=.35,scoop=.40,attack=.8,release=.25,drive=.45,band=9500,lowcut=140,hiss=.18,human=.50,width=.5,growl=.40,room=.25,sync=5,fb=.40,send=.25,eq=EQ_MID)
add(3,'Rico Rodriguez','TROMBONE · WAREIKA','Trombone rústico e bluesy, sempre perto do blues: rouco, scoop largo, falls preguiçosos.',
    sec(trombone=(.9,0,0,0)),'Fall',bright=.48,breath=.50,vib=.35,scoop=.65,attack=1.2,release=.50,drive=.45,band=7200,lowcut=70,hiss=.22,human=.70,width=.4,growl=.65,room=.40,sync=5,fb=.45,send=.28,eq=EQ_WARM)
# ---- WAILERS
add(4,'Naipe Exodus','TPT · TEN · TBN','O naipe Madden · DaCosta · Gordon dos discos do Marley: preciso, junto e limpo.',
    sec(trumpet=(.78,-.35,0,0),tenor=(.8,.05,-1,0),trombone=(.72,.35,0,2)),'Stac',bright=.66,breath=.30,vib=.25,scoop=.28,attack=.8,release=.25,drive=.30,band=11000,lowcut=100,hiss=.08,human=.30,width=.7,growl=.20,room=.30,sync=5,fb=.35,send=.20,eq=EQ_BRIGHT)
add(4,'Glen DaCosta','SAX TENOR · ZAP POW','Tenor preciso e cheio, de arranjador: ataque limpo, vibrato no fim da nota.',
    sec(tenor=(.85,0,0,0)),'Ten',bright=.58,breath=.38,vib=.35,scoop=.30,attack=.9,release=.32,drive=.30,band=10500,lowcut=90,hiss=.10,human=.35,width=.5,growl=.30,room=.30,sync=5,fb=.35,send=.20,eq=EQ_MID)
add(4,'David Madden','TROMPETE · ZAP POW','Trompete jazzístico do "Mystic Mood": brilhante mas macio, doits no fim das frases.',
    sec(trumpet=(.85,0,0,0)),'Doit',bright=.66,breath=.34,vib=.35,scoop=.35,attack=.9,release=.32,drive=.30,band=10500,lowcut=100,hiss=.10,human=.40,width=.5,growl=.15,room=.30,sync=5,fb=.35,send=.20,eq=EQ_BRIGHT)
# ---- 80s DEAN FRASER
add(5,'Dean Fraser · Black Horn Man','SAX TENOR · JOE GIBBS','Tenor rico e encorpado, rouco quente e vibrato expressivo: as pontadas e falls que viraram assinatura.',
    sec(tenor=(.9,0,0,0)),'Fall',bright=.60,breath=.45,vib=.45,scoop=.45,attack=.9,release=.38,drive=.40,band=9500,lowcut=85,hiss=.15,human=.55,width=.5,growl=.70,room=.30,sync=5,fb=.40,send=.25,eq=[1,1.5,1,0,1,1.5,1,0,-1.5,-3])
add(5,'Dean Fraser · Dancehall Sax','SAX TENOR · DIGITAL 80s','O tenor que dava o toque humano no reggae digital: mais brilho, mais limpo, eco curto.',
    sec(tenor=(.9,0,0,0)),'Stac',bright=.72,breath=.35,vib=.40,scoop=.40,attack=.7,release=.28,drive=.30,band=13000,lowcut=110,hiss=.05,human=.45,width=.5,growl=.55,room=.30,sync=7,fb=.30,send=.18,eq=EQ_BRIGHT)
add(5,'Fraser · Nambo · Chico','TEN · TBN · TPT','O naipe dos anos 80 (Dean Fraser, Nambo Robinson, Chico Chin): pontadas cheias e afiadas.',
    sec(trumpet=(.75,-.35,0,0),tenor=(.85,0,-1,0),trombone=(.75,.35,0,2)),'Stac',bright=.72,breath=.30,vib=.32,scoop=.35,attack=.7,release=.24,drive=.35,band=12500,lowcut=110,hiss=.05,human=.40,width=.8,growl=.45,room=.30,sync=5,fb=.30,send=.18,eq=EQ_BRIGHT)
add(5,'Nambo Robinson','TROMBONE · 80s','Trombone gordo e cantado dos anos 80, firme no grave e com fall largo.',
    sec(trombone=(.9,0,0,0)),'Fall',bright=.58,breath=.38,vib=.32,scoop=.45,attack=1.1,release=.40,drive=.35,band=11000,lowcut=70,hiss=.06,human=.45,width=.4,growl=.40,room=.30,sync=5,fb=.35,send=.20,eq=EQ_MID)
# ---- DUB
add(6,'Tubby · naipe no eco','TPT · TEN · TBN · DUB','Naipe estreito de console, chiado e fita: notas soltas jogadas no Tape Echo.',
    sec(trumpet=(.75,-.35,0,0),tenor=(.8,.15,-1,0),trombone=(.7,.35,0,2)),'Stac',bright=.52,breath=.42,vib=.30,scoop=.45,attack=1.0,release=.35,drive=.55,band=7600,lowcut=130,hiss=.30,human=.55,width=.8,growl=.35,room=.20,sync=5,fb=.62,send=.45,tone=2600,eq=EQ_MID)
add(6,'Black Ark · fumaça','TEN · TBN · BAR','O naipe enfumaçado da Black Ark: abafado, com muito ar, sala grande e eco escuro.',
    sec(tenor=(.8,-.2,-1,0),trombone=(.72,.25,0,2),bari=(.6,0,-1,3)),'Scoop',bright=.40,breath=.58,vib=.36,scoop=.50,attack=1.2,release=.45,drive=.65,band=6200,lowcut=100,hiss=.42,human=.65,width=.9,growl=.45,room=.60,sync=2,fb=.55,send=.40,tone=2200,eq=[2,2,1,1,0,-1,-1,-2,-4,-6])
add(6,'Rádio AM de Kingston','TPT · TEN','Naipe pelo radinho: só médio, saturado, para quebras e intros de dub.',
    sec(trumpet=(.78,-.2,0,0),tenor=(.8,.2,-1,0)),'Stac',bright=.60,breath=.30,vib=.30,scoop=.30,attack=.9,release=.30,drive=.75,band=4800,lowcut=320,hiss=.35,human=.50,width=.3,growl=.30,room=.15,sync=4,fb=.50,send=.35,tone=2400,eq=EQ_RADIO)

ART={'Ten':0,'Stac':1,'Fall':2,'Scoop':3,'Doit':4}
def cstr(s): return '"'+s.replace('"','\\"')+'"'
def fmt(v): 
    return ('%.3ff'%v).rstrip('0').replace('.f','.0f') if isinstance(v,float) else str(v)
if sys.argv[1]=='cpp':
    o=['// GERADO por musicos.py — presets "MÚSICOS" (mesma tabela da versão web site/web/dubhorns).',
       '// Cada preset é a ESTÉTICA de um músico ou naipe jamaicano: naipe, timbre, eco e EQ. Tom e escala ficam os seus.',
       '#pragma once','#include <array>','namespace musicos','{',
       'struct Sec { int on, oct, harm; float lvl, pan; };',
       'struct Preset { const char* folder; const char* name; const char* role; const char* desc; int artic;',
       '                Sec sec[6]; float t[14]; int echoOn, sync; float fb, send, tone; float eq[10]; };',
       'inline constexpr const char* timbreIds[14] = { '+', '.join(cstr(x) for x in TIMBRE)+' };',
       'inline constexpr int numFolders = %d;'%len(F),
       'inline constexpr const char* folders[numFolders] = { '+', '.join(cstr(x) for x in F)+' };',
       'inline constexpr int numPresets = %d;'%len(P),
       'inline const Preset presets[numPresets] = {']
    for p in P:
        secs=', '.join('{ %d, %d, %d, %s, %s }'%(p['naipe'][i]['on'],p['naipe'][i]['oct'],p['naipe'][i]['harm'],fmt(float(p['naipe'][i]['lvl'])),fmt(float(p['naipe'][i]['pan']))) for i in INST)
        t=', '.join(fmt(float(p[k])) for k in TIMBRE)
        eq=', '.join(fmt(float(x)) for x in p['eq'])
        o.append('    { %s, %s, %s,\n      %s, %d,\n      { %s },\n      { %s }, %d, %d, %s, %s, %s, { %s } },'%(cstr(p['folder']),cstr(p['name']),cstr(p['role']),cstr(p['desc']),ART[p['artic']],secs,t,p['echoOn'],p['sync'],fmt(float(p['fb'])),fmt(float(p['send'])),fmt(float(p['tone'])),eq))
    o+=['};','} // namespace musicos','']
    print('\n'.join(o))
else:
    out=[dict(f=p['folder'],n=p['name'],r=p['role'],d=p['desc'],a=ART[p['artic']],sec=[p['naipe'][i] for i in INST],t={k:p[k] for k in TIMBRE},echo=dict(on=p['echoOn'],sync=p['sync'],fb=p['fb'],send=p['send'],tone=p['tone']),eq=p['eq']) for p in P]
    print('const MUSICOS_FOLDERS = '+json.dumps(F,ensure_ascii=False)+';')
    print('const MUSICOS = '+json.dumps(out,ensure_ascii=False,separators=(',',':'))+';')
