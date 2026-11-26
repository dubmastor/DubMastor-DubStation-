"""Gera site/daw/index.html (Riddim Machine DAW RMD-0420) a partir da versão web do MRM-0420 (site/riddim/index.html)
mais a aba ESTÚDIO (RiddimDAW/web/daw.js e daw.css). Rodar: python3 RiddimDAW/build_web.py"""
import sys,pathlib
RAIZ=pathlib.Path(__file__).resolve().parent.parent
R=RAIZ/'site'
s=(R/'riddim/index.html').read_text()
B=RAIZ/'RiddimDAW'/'web'
def rep(a,b,n=1):
    global s
    c=s.count(a)
    assert c==n,(a,c)
    s=s.replace(a,b)
rep('<title>Mastor Riddim Machine MRM-0420 2.7 — DubStation</title>','<title>Riddim Machine DAW RMD-0420 1.1 — DubStation</title>')
rep('<div class="logo"><h1>Mastor Riddim Machine</h1>','<div class="logo"><h1>Riddim Machine DAW</h1>')
rep('<span class="badge">MRM-0420</span><span class="ver">versão 2.7</span>','<span class="badge">RMD-0420</span><span class="ver">versão 1.1</span>')
rep('versão <span id="ver">2.7</span>','versão <span id="ver">1.1</span>')
rep("const VERSION = '2.7';   // mesma versão do plugin (RiddimMachine/CMakeLists.txt)","const VERSION = '1.1';   // Riddim Machine DAW RMD-0420 (projeto separado do VST MRM-0420)")
rep("{id:'main',name:'Principal'","{id:'studio',name:'Estúdio',full:'Riddim Machine DAW',code:'RMD-0420',color:'#d9573f',th:['#5a2018','#1e0a07','#ffd0a0','#e8703a','#f6e0c8'],studio:true},\n{id:'main',name:'Principal'")
rep("T.main?'Principal · controla todas as machines'","T.studio?'Estúdio · timeline, gravação e mixagem':T.main?'Principal · controla todas as machines'")
rep("const anySolo=()=>M.some(i=>i.solo);","const anySolo=()=>M.some(i=>i.solo)||(window.DAWT||[]).some(t=>t.solo);")
rep("sc.src='lame.min.js'","sc.src='../riddim/lame.min.js'")
rep('<meta name="theme-color" content="#1f1610">','<meta name="theme-color" content="#1f1610">\n<meta name="description" content="Riddim Machine DAW RMD-0420: a Riddim Machine com timeline, gravação de voz, mixer e exportação em WAV, MP3 e stems, no navegador.">')
css=(B/'daw.css').read_text();js=(B/'daw.js').read_text()
rep('</style>',css+'</style>')
tail="show('main');\n}\n</script>"
assert s.count(tail)==1
s=s.replace(tail,"show('main');\n"+js+"\n}\n</script>")
out=R/'daw/index.html';out.parent.mkdir(exist_ok=True);out.write_text(s);print(len(s))
