
/* =====================================================================
   RIDDIM MACHINE DAW RMD-0420 — aba ESTÚDIO
   Timeline da música (partes do Arranjo × machines), faixas de áudio com
   gravação do microfone e importação de arquivos, mixer das faixas,
   exportar a música (WAV / MP3) e os stems, projeto salvo no aparelho.
   Tudo na contagem Mastor: caixa no 2 e no 4 (o metrônomo acentua o 2 e o 4).
   ===================================================================== */
const ST=byId.studio;
window.DAWT=[];const DAWT=window.DAWT;
const BUFS={},abCache=new WeakMap(),tch={};let tchCtx=null;
const TCOL=['#d9573f','#5b7fc4','#3fa89a','#b06ab0','#c0a040','#7fae6a'];
const SNAPS=[[1,'compasso'],[4,'tempo'],[16,'1/16'],[0,'livre']];
const lsGet=(k,d)=>{try{const v=localStorage.getItem(k);return v===null?d:v}catch(e){return d}};
const lsSet=(k,v)=>{try{localStorage.setItem(k,v)}catch(e){}};
let PPB=+lsGet('rmd-ppb',40),snapI=1,posBar=0,selClip=null,armT=null,metro=false,countIn=true,monitor=false;
let LAT=lsGet('rmd-lat',null);LAT=LAT===null?null:+LAT;
const BD=()=>stepDur()*16;
const uid=()=>Math.random().toString(36).slice(2,9);
const tAud=t=>!t.mute&&(!anySolo()||t.solo);
const dB=g=>g<=0?'-∞':(20*Math.log10(g)>=0?'+':'')+(20*Math.log10(g)).toFixed(1);
function newTrack(name){const t={id:'a'+uid(),name:name||`Voz ${DAWT.length+1}`,color:TCOL[DAWT.length%TCOL.length],vol:.85,pan:0,echo:0,mute:false,solo:false,clips:[]};DAWT.push(t);armT=t.id;return t}
const trackOf=c=>DAWT.find(t=>t.clips.includes(c));
const findClip=id=>{for(const t of DAWT)for(const c of t.clips)if(c.id===id)return c;return null};

/* ---------- áudio das faixas ---------- */
function trackNodes(c,busN,eInN){const g=c.createGain(),p=c.createStereoPanner(),s=c.createGain();g.connect(p);p.connect(busN);g.connect(s);s.connect(eInN);return{g,p,s}}
function liveTch(t){if(!ctx)return null;if(tchCtx!==ctx){for(const k in tch)delete tch[k];tchCtx=ctx}if(!tch[t.id])tch[t.id]=trackNodes(ctx,bus,eIn);return tch[t.id]}
function applyT(t){const n=liveTch(t);if(!n)return;n.g.gain.value=tAud(t)?t.vol:0;n.p.pan.value=t.pan;n.s.gain.value=t.echo}
const applyAllT=()=>DAWT.forEach(applyT);
function getAB(c,id){let m=abCache.get(c);if(!m){m={};abCache.set(c,m)}if(m[id])return m[id];const b=BUFS[id],ab=c.createBuffer(b.ch.length,b.ch[0].length,b.sr);b.ch.forEach((d,i)=>ab.getChannelData(i).set(d));return m[id]=ab}
function peaks(id){const b=BUFS[id],N=256,len=Math.ceil(b.ch[0].length/N),pk=new Float32Array(len);for(let i=0;i<len;i++){let m=0;for(const d of b.ch){const e=Math.min(d.length,(i+1)*N);for(let j=i*N;j<e;j+=2){const a=d[j]<0?-d[j]:d[j];if(a>m)m=a}}pk[i]=m}b.pk=pk;b.pkN=N}

/* ---------- transporte ---------- */
let srcs=[],songT0=0,t0=0,startBar=0,iterNext=1,recState=null,schedTimer=null,metroNext=-1,dawPlaying=false;
function schedIter(k,fromT){const L=arrBars()*BD(),base=songT0+k*L;
 for(const t of DAWT){const n=liveTch(t);if(!n)continue;for(const c of t.clips){const st=base+c.bar*BD();let off=c.off,dur=Math.min(c.dur,base+L-st),when=st;if(dur<=0)continue;
  if(when<fromT){const d=fromT-when;off+=d;dur-=d;when=fromT}if(dur<=.005)continue;
  const s=ctx.createBufferSource(),g=ctx.createGain();s.buffer=getAB(ctx,c.buf);g.gain.value=c.gain;s.connect(g);g.connect(n.g);s.start(when,off,dur);srcs.push(s)}}}
function click(t,q){const o=ctx.createOscillator(),g=ctx.createGain(),ac=q%2===1;o.frequency.value=ac?1760:1050;g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(ac?.32:.18,t+.002);g.gain.exponentialRampToValueAtTime(.001,t+.05);o.connect(g);g.connect(ctx.destination);o.start(t);o.stop(t+.07)}
function studioTick(){if(!playing||!dawPlaying)return;const now=ctx.currentTime,L=arrBars()*BD();
 if(ARR.loop&&L>0)while(songT0+iterNext*L<now+1.5){schedIter(iterNext,songT0+iterNext*L);iterNext++}
 if(metro){const bt=BD()/4;if(metroNext<0)metroNext=Math.max(0,Math.ceil((now-t0)/bt));while(t0+metroNext*bt<now+.3){const tt=t0+metroNext*bt;if(tt>=now-.01)click(tt,metroNext%4);metroNext++}}else metroNext=-1}
function barToArr(bar){let b=Math.floor(bar),i=0;while(i<ARR.order.length){const x=ARR.secs[ARR.order[i]],n=x?x.bars:0;if(b<n)break;b-=n;i++}return[i,b]}
function arrToBar(i){let s=0;for(let k=0;k<i&&k<ARR.order.length;k++){const x=ARR.secs[ARR.order[k]];s+=x?x.bars:0}return s}
function beginClips(sb){startBar=sb;t0=nextT;songT0=t0-startBar*BD();srcs=[];applyAllT();schedIter(0,t0);iterNext=1;metroNext=-1;dawPlaying=true;clearInterval(schedTimer);schedTimer=setInterval(studioTick,80)}
async function studioStart(rec){
 if(!H.ready){wantStudio=true;openSetup(true);return}
 if(!ARR.order.length){toastMsg('Monte a ordem da música na aba Arranjo');return}
 if(!ctx)initAudio();try{await ctx.resume()}catch(e){}
 if(rec&&!(await recPrepare()))return;
 const tot=arrBars();if(posBar>=tot)posBar=0;
 ARR.on=true;if(rec&&ARR.loop){ARR.loop=false;toastMsg('Repetir desligado durante a gravação')}
 origPlay({isTrusted:false});if(!playing)return;
 const sb=Math.floor(posBar),[i,b]=barToArr(sb);arrI=i;arrB=b;cur=0;arrJump=-1;arrEndAt=0;
 const ci=rec&&countIn;nextT=ctx.currentTime+.1+(ci?BD():0);
 beginClips(sb);if(ci)for(let q=0;q<4;q++)click(t0-BD()+q*BD()/4,q);
 if(rec)recBegin();renderTransport()}
const origPlay=$('play').onclick;
const studioOn=()=>{const p=$('p-studio');return!!p&&p.classList.contains('active')};
$('play').onclick=e=>{if(!playing&&studioOn()){studioStart(false);return}const was=playing;origPlay(e);
 if(!was&&playing&&arrActive()){beginClips(arrToBar(arrI))}
 if(was&&!playing)studioStopped()};
function studioStopped(){srcs.forEach(s=>{try{s.stop()}catch(e){}});srcs=[];clearInterval(schedTimer);if(recState)recEnd();if(dawPlaying)posBar=startBar;dawPlaying=false;renderTransport()}
function seek(bar){posBar=Math.max(0,bar);if(playing&&dawPlaying){$('play').click();studioStart(false)}}

/* ---------- gravação do microfone ---------- */
let micStream=null;
const latSec=()=>LAT!==null?LAT/1000:((ctx&&(ctx.outputLatency||ctx.baseLatency))||.02)+.012;
async function recPrepare(){if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia){toastMsg('Este navegador não deixa gravar o microfone');return false}
 try{if(!micStream||!micStream.active)micStream=await navigator.mediaDevices.getUserMedia({audio:{echoCancellation:false,noiseSuppression:false,autoGainControl:false}})}
 catch(e){toastMsg('Microfone bloqueado: libere o microfone para este site no navegador');return false}
 if(!DAWT.length)newTrack();if(!DAWT.find(x=>x.id===armT))armT=DAWT[0].id;renderStudio();return true}
function recBegin(){const t=DAWT.find(x=>x.id===armT);const src=ctx.createMediaStreamSource(micStream),pr=ctx.createScriptProcessor(4096,1,1),z=ctx.createGain();z.gain.value=0;
 const R={t,src,pr,z,chunks:[],first:null,mon:null,peak:0};
 pr.onaudioprocess=e=>{const d=e.inputBuffer.getChannelData(0);if(R.first===null)R.first=ctx.currentTime-e.inputBuffer.duration;R.chunks.push(new Float32Array(d));let m=0;for(let i=0;i<d.length;i+=8){const a=d[i]<0?-d[i]:d[i];if(a>m)m=a}R.peak=Math.max(m,R.peak*.85)};
 src.connect(pr);pr.connect(z);z.connect(ctx.destination);
 if(monitor){R.mon=ctx.createGain();src.connect(R.mon);R.mon.connect(liveTch(t).g)}
 recState=R}
function recEnd(){const R=recState;recState=null;try{R.src.disconnect();R.pr.disconnect();R.z.disconnect();R.mon&&R.mon.disconnect()}catch(e){}
 if(!R.chunks.length||R.first===null){toastMsg('Nada foi gravado');return}
 const sr=ctx.sampleRate,n=R.chunks.reduce((a,c)=>a+c.length,0),all=new Float32Array(n);let o=0;R.chunks.forEach(c=>{all.set(c,o);o+=c.length});
 const songSec=R.first-songT0-latSec(),crop=Math.max(0,Math.round((startBar*BD()-songSec)*sr));
 if(crop>=n-sr*.2){toastMsg('Gravação curta demais');return}
 const id='b'+uid();BUFS[id]={sr,ch:[all.slice(crop)]};peaks(id);
 const c={id:'c'+uid(),buf:id,bar:(songSec+crop/sr)/BD(),off:0,dur:(n-crop)/sr,gain:1};R.t.clips.push(c);selClip=c.id;
 toastMsg(`Gravado em ${R.t.name}: ${mmss(c.dur)}`);renderStudio();autosave()}

/* ---------- importar áudio ---------- */
async function importAudio(file){if(!ctx)initAudio();
 try{const raw=await file.arrayBuffer(),ab=await new Promise((ok,no)=>{const p=ctx.decodeAudioData(raw,ok,no);if(p&&p.then)p.then(ok,no)});
  const id='b'+uid();BUFS[id]={sr:ab.sampleRate,ch:Array.from({length:Math.min(2,ab.numberOfChannels)},(_,i)=>ab.getChannelData(i).slice())};peaks(id);
  const t=DAWT.find(x=>x.id===armT)||newTrack(file.name.replace(/\.[^.]+$/,'').slice(0,20));
  const c={id:'c'+uid(),buf:id,bar:posBar,off:0,dur:ab.duration,gain:1};t.clips.push(c);selClip=c.id;renderStudio();autosave();toastMsg(`${file.name} em ${t.name}, compasso ${fmtPos(posBar)}`)}
 catch(e){console.error(e);toastMsg('Não consegui abrir este arquivo de áudio')}}

/* ---------- painel ---------- */
const fmtPos=b=>`${Math.floor(b)+1}.${Math.floor((b%1)*4+1e-6)+1}`;
const sp=mk(`<section class="panel" id="p-studio" style="${thVars(ST)}">${cab(ST,'Estúdio','Riddim Machine DAW · timeline, gravação e mixagem')}
<div class="pbody">
<div id="dban"></div>
<div class="dstart" id="dstart" hidden><h3>Comece pelo riddim</h3><p class="hint">Escolha o estilo, a harmonia, o BPM e os músicos (ou carregue um preset). Depois a música aparece aqui na timeline, pronta para gravar a voz.</p><button class="btn big green" id="dnew">CRIAR O RIDDIM</button></div>
<div class="dawbar">
 <button class="btn" id="dhome" aria-label="Voltar ao início">⏮</button>
 <button class="btn big green" id="dplay">▶ PLAY</button>
 <button class="btn big rec" id="drec">● REC</button>
 <div class="lcd"><small>COMPASSO</small><span id="dpos">1.1</span></div>
 <div class="lcd"><small>TEMPO</small><span id="dtime">0:00</span></div>
 <div class="dmeter" title="Nível do microfone"><i id="dmet"></i></div>
 <span class="grow"></span>
 <button class="btn" id="dsnap">Grade: tempo</button>
 <button class="btn" id="dzm" aria-label="Menos zoom">−</button><button class="btn" id="dzp" aria-label="Mais zoom">+</button>
</div>
<div class="dawbar">
 <button class="btn" id="dmetro">Metrônomo: não</button>
 <button class="btn" id="dcount">Contagem antes de gravar: sim</button>
 <button class="btn" id="dloop">Repetir: não</button>
 <button class="btn" id="dmon">Ouvir o microfone: não</button>
</div>
<div class="dawwrap" id="dwrap"><div class="daw" id="daw"></div></div>
<p class="dhint">Toque na régua para pôr o cursor. Nas machines, toque numa parte para ligar ou desligar a machine nela (vale para todas as vezes que a parte aparece). Arraste os clipes de áudio para mudar o lugar. Use fone de ouvido para gravar.</p>
<div class="dawbar" id="dclip" style="margin-top:10px"></div>
<div class="dawbar" style="margin-top:6px">
 <button class="btn green" id="dadd">+ Faixa de áudio</button>
 <label class="btn" style="cursor:pointer">⇪ Importar áudio<input type="file" accept="audio/*" id="dimp" hidden></label>
 <button class="btn" id="dparts">Editar as partes (Arranjo)</button>
 <button class="btn" id="dmixer">Mixer das machines</button>
</div>
<div class="group" style="margin-top:12px"><h3>Exportar</h3>
 <div class="dexp"><button class="btn green" id="dwav">⬇ MÚSICA EM WAV</button><button class="btn green" id="dmp3">⬇ MÚSICA EM MP3</button><button class="btn" id="dstems">⬇ STEMS EM WAV (uma faixa por arquivo)</button></div>
 <p class="hint" style="margin:8px 0 0">A música sai inteira, na ordem do Arranjo, com as vozes, o mixer e os efeitos do master, sem precisar tocar. Os stems saem todos com o mesmo tamanho, começando no compasso 1, para alinhar em qualquer DAW. O MIDI da música está na aba Arranjo. MP3 com o <a href="https://lame.sourceforge.io/" target="_blank" rel="noopener" style="color:var(--amber)">LAME</a> (LGPL).</p></div>
<div class="group"><h3>Projeto</h3>
 <div class="dexp"><button class="btn" id="dsave">💾 Salvar projeto (arquivo)</button><label class="btn" style="cursor:pointer">📂 Abrir projeto<input type="file" accept=".rmd,.json,application/json" id="dopen" hidden></label><button class="btn" id="dclear">Novo projeto</button></div>
 <div class="row" style="gap:8px;align-items:center;margin-top:10px;flex-wrap:wrap"><span class="hint" style="margin:0">Atraso da gravação (latência):</span><button class="btn" id="dlm">−</button><b id="dlat" style="font-family:var(--mono)">auto</b><button class="btn" id="dlp">+</button><button class="btn" id="dla">Auto</button></div>
 <p class="hint" style="margin:8px 0 0">O projeto (riddim, arranjo, faixas e gravações) fica salvo sozinho neste aparelho. Para levar para outro aparelho, use Salvar projeto. Se a voz gravada ficar atrasada em relação à batida, aumente o atraso; se ficar adiantada, diminua.</p></div>
</div></section>`);
panels.append(sp);
const daw=$('daw'),dwrap=$('dwrap');let ph=null;

function laneW(){return Math.max(arrBars(),4)*PPB+6*PPB}
function renderStudio(){if(!$('daw'))return;
 $('dstart').hidden=!!H.ready;
 const W=laneW(),secs=[];let ob=0;ARR.order.forEach((k,i)=>{const x=ARR.secs[k];if(!x)return;secs.push({k,i,x,b0:ob});ob+=x.bars});
 const tot=ob;daw.style.width=`calc(var(--hdw) + ${W}px)`;daw.style.setProperty('--ppb',PPB+'px');
 const lane=(cls,inner,attr='')=>`<div class="dln ${cls}" style="width:${W}px" ${attr}>${inner}</div>`;
 let h='';
 /* régua: partes + compassos */
 let ru=secs.map(s=>`<div class="dsec" style="left:${s.b0*PPB+1}px;width:${s.x.bars*PPB-3}px;--sc:${s.x.c};--sb:${rgba(s.x.c,.35)}">${esc(s.x.n)}</div>`).join('');
 const every=PPB>=30?1:PPB>=16?2:4;for(let b=0;b<Math.max(tot,4)+6;b+=every)ru+=`<div class="dbar" style="left:${b*PPB}px">${b+1}</div>`;
 h+=`<div class="drow druler"><div class="dhd">PARTES · ${tot} COMP.</div>${lane('',ru,'id="druler"')}</div>`;
 /* acordes */
 let ch='';secs.forEach(s=>{for(let ab=0;ab<s.x.bars;ab++){const rb=ab%H.bars,a=H.chords[chIdx(rb,0)],b=H.chords[chIdx(rb,8)],txt=a===b?cname(a):cname(a)+' '+cname(b);ch+=`<div class="dch" style="left:${(s.b0+ab)*PPB}px;width:${PPB}px">${PPB>=26||ab%2===0?txt:''}</div>`}});
 h+=`<div class="drow dchord" style="min-height:34px"><div class="dhd"><div class="tn" style="--c:var(--amber)">Acordes</div></div>${lane('',ch)}</div>`;
 /* faixas de áudio */
 for(const t of DAWT){const off=!tAud(t);
  const cl=t.clips.map(c=>`<div class="dclip ${c.id===selClip?'sel':''}" data-cid="${c.id}" style="left:${c.bar*PPB}px;width:${Math.max(6,c.dur/BD()*PPB)}px;--c:${t.color}"><canvas></canvas><span>${esc(t.name)}${c.gain!==1?' · '+dB(c.gain)+' dB':''}</span></div>`).join('');
  h+=`<div class="drow ${off?'off':''} ${t.id===armT?'arm':''}" style="--c:${t.color};min-height:78px"><div class="dhd"><div class="tr"><button class="sb r ${t.id===armT?'on':''}" data-ta="${t.id}" title="Gravar nesta faixa">●</button><input type="text" value="${esc(t.name)}" data-tn="${t.id}" aria-label="Nome da faixa"><button class="sb" data-tx="${t.id}" title="Apagar a faixa">×</button></div>
   <div class="tr"><button class="sb m ${t.mute?'on':''}" data-tm="${t.id}">M</button><button class="sb s ${t.solo?'on':''}" data-ts="${t.id}">S</button><input type="range" min="0" max="1.5" step=".01" value="${t.vol}" data-tv="${t.id}" aria-label="Volume"></div>
   <div class="tr"><label>PAN<input type="range" min="-1" max="1" step=".05" value="${t.pan}" data-tp="${t.id}"></label><label>ECO<input type="range" min="0" max="1" step=".01" value="${t.echo}" data-te="${t.id}"></label></div></div>${lane('dalane',cl,`data-lane="${t.id}"`)}</div>`}
 if(!DAWT.length)h+=`<div class="drow" style="min-height:56px"><div class="dhd"><div class="tn" style="--c:var(--muted)">Áudio</div></div>${lane('dalane','<div class="dbar" style="top:18px;border:0;left:8px;font-size:12px">Toque em ● REC para gravar a voz ou em + Faixa de áudio</div>','data-lane=""')}</div>`;
 /* machines */
 for(const I of M){const off=!aud(I);let bl='';
  secs.forEach(s=>{const on=s.x.on[I.id]!==false;bl+=`<div class="dblk ${on?'on':'of'}" data-blk="${I.id}" data-sk="${s.k}" style="left:${s.b0*PPB+1}px;width:${s.x.bars*PPB-2}px;--c:${I.color}" title="${esc(s.x.n)}: ${on?'toca':'não toca'}">${s.x.bars*PPB>44?esc(s.x.n):''}</div>`});
  h+=`<div class="drow ${off?'off':''}" style="--c:${I.color}"><div class="dhd"><div class="tn"><i></i>${esc(mlabel(I))}</div><div class="tr"><button class="sb m ${I.mute?'on':''}" data-mm="${I.id}">M</button><button class="sb s ${I.solo?'on':''}" data-ms="${I.id}">S</button><input type="range" min="0" max="1.2" step=".01" value="${I.p.vol}" data-mv="${I.id}" aria-label="Volume ${esc(mlabel(I))}"></div></div>${lane('',bl)}</div>`}
 h+=`<div class="dph" id="dph"></div>`;
 daw.innerHTML=h;ph=$('dph');
 daw.querySelectorAll('.dclip').forEach(el=>drawClip(el));
 renderClipBar();renderTransport()}
function drawClip(el){const c=findClip(el.dataset.cid);if(!c)return;const b=BUFS[c.buf];if(!b||!b.pk)return;const cv=el.querySelector('canvas'),w=Math.min(4096,Math.max(4,Math.round(el.offsetWidth||c.dur/BD()*PPB))),hh=60;cv.width=w;cv.height=hh;
 const g=cv.getContext('2d'),col=getComputedStyle(el).getPropertyValue('--c').trim()||'#e6b83a';g.fillStyle=col;const spp=c.dur*b.sr/w;
 for(let x=0;x<w;x++){const i0=Math.floor((c.off*b.sr+x*spp)/b.pkN),i1=Math.max(i0+1,Math.floor((c.off*b.sr+(x+1)*spp)/b.pkN));let m=0;for(let i=i0;i<i1&&i<b.pk.length;i++)if(b.pk[i]>m)m=b.pk[i];const a=Math.min(1,m*c.gain)*(hh/2-2);g.fillRect(x,hh/2-a,1,Math.max(1,a*2))}}
function renderTransport(){if(!$('dplay'))return;const p=$('dplay');p.textContent=playing?'■ STOP':'▶ PLAY';p.className='btn big '+(playing?'red':'green');
 $('drec').classList.toggle('on',!!recState);$('drec').textContent=recState?'● GRAVANDO':'● REC';
 $('dmetro').textContent='Metrônomo: '+(metro?'sim':'não');$('dmetro').classList.toggle('on',metro);
 $('dcount').textContent='Contagem antes de gravar: '+(countIn?'sim':'não');$('dcount').classList.toggle('on',countIn);
 $('dloop').textContent='Repetir: '+(ARR.loop?'sim':'não');$('dloop').classList.toggle('on',ARR.loop);
 $('dmon').textContent='Ouvir o microfone: '+(monitor?'sim':'não');$('dmon').classList.toggle('on',monitor);
 $('dsnap').textContent='Grade: '+SNAPS[snapI][1];$('dlat').textContent=LAT===null?'auto':LAT+' ms'}
function renderClipBar(){const el=$('dclip'),c=selClip&&findClip(selClip);if(!c){el.innerHTML='';el.hidden=true;return}el.hidden=false;const t=trackOf(c);
 el.innerHTML=`<span class="hint" style="margin:0"><b style="color:${t.color}">Clipe de ${esc(t.name)}</b> · compasso ${fmtPos(c.bar)} · ${mmss(c.dur)} · ${dB(c.gain)} dB</span>
 <button class="btn" data-ca="l" aria-label="Mover para a esquerda">◀</button><button class="btn" data-ca="r" aria-label="Mover para a direita">▶</button>
 <button class="btn" data-ca="gm">Volume −</button><button class="btn" data-ca="gp">Volume +</button>
 <button class="btn" data-ca="cut">✂ Cortar no cursor</button><button class="btn" data-ca="dup">⧉ Duplicar</button><button class="btn" data-ca="cur">Levar ao cursor</button>
 ${DAWT.length>1?`<select class="btn" data-cmv aria-label="Mover para a faixa">${DAWT.map(x=>`<option value="${x.id}" ${x===t?'selected':''}>Faixa: ${esc(x.name)}</option>`).join('')}</select>`:''}
 <button class="btn red" data-ca="del">Apagar</button>`}

/* ---------- interação ---------- */
const snapB=b=>{const q=SNAPS[snapI][0];return q?Math.round(b*q)/q:b};
const laneX=(e,el)=>(e.clientX-el.getBoundingClientRect().left)/PPB;
let drag=null;
daw.addEventListener('pointerdown',e=>{const cl=e.target.closest('.dclip');
 if(cl){const c=findClip(cl.dataset.cid);if(!c)return;selClip=c.id;daw.querySelectorAll('.dclip.sel').forEach(x=>x.classList.remove('sel'));cl.classList.add('sel');renderClipBar();
  drag={c,el:cl,x0:e.clientX,b0:c.bar,moved:false};try{cl.setPointerCapture(e.pointerId)}catch(_){}e.preventDefault();return}
 const ru=e.target.closest('#druler');if(ru){seek(Math.floor(laneX(e,ru)*4)/4);return}
 const ln=e.target.closest('.dalane');if(ln&&!e.target.closest('.dclip')){if(ln.dataset.lane)armT=ln.dataset.lane;selClip=null;posBar=Math.max(0,snapB(laneX(e,ln)));if(playing&&dawPlaying)seek(posBar);else renderStudio()}});
daw.addEventListener('pointermove',e=>{if(!drag)return;const dx=e.clientX-drag.x0;if(Math.abs(dx)>4)drag.moved=true;if(!drag.moved)return;drag.c.bar=Math.max(0,snapB(drag.b0+dx/PPB));drag.el.style.left=drag.c.bar*PPB+'px'});
const endDrag=()=>{if(!drag)return;const m=drag.moved;drag=null;renderClipBar();if(m)autosave()};
daw.addEventListener('pointerup',endDrag);daw.addEventListener('pointercancel',endDrag);
daw.addEventListener('click',e=>{const d=e.target.dataset;if(!d)return;
 if(d.blk){const x=ARR.secs[d.sk];x.on[d.blk]=x.on[d.blk]===false;renderArr();renderStudio()}
 else if(d.mm){const I=byId[d.mm];I.mute=!I.mute;if(I.mute)I.solo=false;renderToggles();renderStudio();applyAllT()}
 else if(d.ms){const I=byId[d.ms];I.solo=!I.solo;if(I.solo)I.mute=false;renderToggles();renderStudio();applyAllT()}
 else if(d.ta){armT=d.ta;renderStudio()}
 else if(d.tm){const t=DAWT.find(x=>x.id===d.tm);t.mute=!t.mute;if(t.mute)t.solo=false;applyAllT();renderToggles();renderStudio();autosave()}
 else if(d.ts){const t=DAWT.find(x=>x.id===d.ts);t.solo=!t.solo;if(t.solo)t.mute=false;applyAllT();renderToggles();renderStudio();autosave()}
 else if(d.tx){const t=DAWT.find(x=>x.id===d.tx);if(t.clips.length&&!confirm(`Apagar a faixa ${t.name} e as gravações dela?`))return;DAWT.splice(DAWT.indexOf(t),1);if(tch[t.id]){try{tch[t.id].g.disconnect()}catch(_){}delete tch[t.id]}if(armT===t.id)armT=DAWT[0]?DAWT[0].id:null;if(selClip&&!findClip(selClip))selClip=null;renderStudio();autosave()}});
daw.addEventListener('input',e=>{const d=e.target.dataset,v=+e.target.value;
 if(d.mv){const I=byId[d.mv];I.p.vol=v;if(ctx)applyCh(I);setK(I.id+'.vol',v)}
 else if(d.tv||d.tp||d.te){const t=DAWT.find(x=>x.id===(d.tv||d.tp||d.te));if(d.tv)t.vol=v;else if(d.tp)t.pan=v;else t.echo=v;applyT(t);autosave()}});
daw.addEventListener('change',e=>{const d=e.target.dataset;if(d.tn){const t=DAWT.find(x=>x.id===d.tn);t.name=e.target.value.trim().slice(0,24)||t.name;renderStudio();autosave()}});
$('dclip').addEventListener('click',e=>{const a=e.target.dataset.ca;if(!a)return;const c=findClip(selClip);if(!c)return;const t=trackOf(c),q=SNAPS[snapI][0]||16;
 if(a==='l')c.bar=Math.max(0,c.bar-1/q);else if(a==='r')c.bar+=1/q;
 else if(a==='gm')c.gain=Math.max(0,c.gain/1.189);else if(a==='gp')c.gain=Math.min(8,c.gain*1.189);
 else if(a==='cur')c.bar=posBar;
 else if(a==='dup'){const n={...c,id:'c'+uid(),bar:snapB(c.bar+c.dur/BD())};t.clips.push(n);selClip=n.id}
 else if(a==='cut'){const at=(posBar-c.bar)*BD();if(at<=.02||at>=c.dur-.02){toastMsg('Ponha o cursor em cima do clipe para cortar');return}const n={...c,id:'c'+uid(),bar:posBar,off:c.off+at,dur:c.dur-at};c.dur=at;t.clips.push(n);selClip=n.id}
 else if(a==='del'){t.clips.splice(t.clips.indexOf(c),1);selClip=null}
 renderStudio();autosave()});
$('dclip').addEventListener('change',e=>{if(!('cmv' in e.target.dataset))return;const c=findClip(selClip),from=trackOf(c),to=DAWT.find(x=>x.id===e.target.value);if(!c||!to||to===from)return;from.clips.splice(from.clips.indexOf(c),1);to.clips.push(c);renderStudio();autosave()});
$('dplay').onclick=()=>$('play').click();
$('drec').onclick=()=>{if(playing){$('play').click();return}studioStart(true)};
$('dhome').onclick=()=>seek(0);
let wantStudio=false;
$('dnew').onclick=()=>{wantStudio=true;openSetup(false)};
$('dmetro').onclick=()=>{metro=!metro;metroNext=-1;renderTransport()};
$('dcount').onclick=()=>{countIn=!countIn;renderTransport()};
$('dloop').onclick=()=>{ARR.loop=!ARR.loop;renderTl();renderTransport()};
$('dmon').onclick=()=>{monitor=!monitor;if(recState){if(monitor&&!recState.mon){recState.mon=ctx.createGain();recState.src.connect(recState.mon);recState.mon.connect(liveTch(recState.t).g)}else if(!monitor&&recState.mon){recState.mon.disconnect();recState.mon=null}}renderTransport()};
$('dsnap').onclick=()=>{snapI=(snapI+1)%SNAPS.length;renderTransport()};
const zoom=f=>{const mid=(dwrap.scrollLeft+dwrap.clientWidth/2)/PPB;PPB=Math.max(10,Math.min(160,Math.round(PPB*f)));lsSet('rmd-ppb',PPB);renderStudio();dwrap.scrollLeft=mid*PPB-dwrap.clientWidth/2};
$('dzm').onclick=()=>zoom(1/1.35);$('dzp').onclick=()=>zoom(1.35);
$('dadd').onclick=()=>{newTrack();renderStudio();autosave()};
$('dimp').onchange=e=>{const f=e.target.files[0];e.target.value='';if(f)importAudio(f)};
$('dparts').onclick=()=>show('arr');$('dmixer').onclick=()=>show('mix');
const setLat=v=>{LAT=v;if(v===null){try{localStorage.removeItem('rmd-lat')}catch(e){}}else lsSet('rmd-lat',v);renderTransport()};
$('dlm').onclick=()=>setLat(Math.max(-200,(LAT===null?Math.round(latSec()*1000):LAT)-5));
$('dlp').onclick=()=>setLat(Math.min(500,(LAT===null?Math.round(latSec()*1000):LAT)+5));
$('dla').onclick=()=>setLat(null);

/* cursor tocando */
function dawFrame(){requestAnimationFrame(dawFrame);if(!ph||!studioOn())return;let b=posBar,cnt=false;
 if(playing&&dawPlaying&&ctx){const now=ctx.currentTime;if(now<t0){cnt=true;b=startBar}else{b=startBar+(now-t0)/BD();const tot=arrBars();if(ARR.loop&&tot)b%=tot}}
 const x=b*PPB;ph.style.transform=`translateX(${x}px)`;ph.classList.toggle('rec',!!recState);
 $('dpos').textContent=cnt?'1, 2, 3, 4':fmtPos(b);$('dtime').textContent=mmss(b*BD());
 $('dmet').style.width=recState?Math.min(100,recState.peak*130)+'%':'0';
 if(playing&&dawPlaying&&!drag){const hd=daw.querySelector('.dhd'),hw=hd?hd.offsetWidth:0,vis=dwrap.clientWidth-hw;if(x<dwrap.scrollLeft||x>dwrap.scrollLeft+vis-40)dwrap.scrollLeft=Math.max(0,x-40)}}
requestAnimationFrame(dawFrame);

/* ---------- exportar ---------- */
const cv16=x=>Math.max(-32768,Math.min(32767,Math.round(x*32767)));
function wavBlob(buf,end){const sr=buf.sampleRate,h0=Math.round(.05*sr),n=Math.max(0,end-h0),L=buf.getChannelData(0),R=buf.numberOfChannels>1?buf.getChannelData(1):L,ab=new ArrayBuffer(44+n*4),v=new DataView(ab),w=(o,s)=>{for(let i=0;i<s.length;i++)v.setUint8(o+i,s.charCodeAt(i))};
 w(0,'RIFF');v.setUint32(4,36+n*4,true);w(8,'WAVE');w(12,'fmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,2,true);v.setUint32(24,sr,true);v.setUint32(28,sr*4,true);v.setUint16(32,4,true);v.setUint16(34,16,true);w(36,'data');v.setUint32(40,n*4,true);
 for(let i=0,o=44;i<n;i++,o+=4){v.setInt16(o,cv16(L[i+h0]),true);v.setInt16(o+2,cv16(R[i+h0]),true)}return new Blob([ab],{type:'audio/wav'})}
async function mp3Blob(buf,end,say){await loadLame();const SR=buf.sampleRate,L=buf.getChannelData(0),R=buf.getChannelData(1),enc=new lamejs.Mp3Encoder(2,SR,192),out=[],B=1152,l16=new Int16Array(B),r16=new Int16Array(B);
 for(let i=Math.round(.05*SR),n=0;i<end;i+=B,n++){const m=Math.min(B,end-i);for(let j=0;j<m;j++){l16[j]=cv16(L[i+j]);r16[j]=cv16(R[i+j])}const d=enc.encodeBuffer(m<B?l16.subarray(0,m):l16,m<B?r16.subarray(0,m):r16);if(d.length)out.push(new Uint8Array(d));if(n%200===0){say(`Codificando MP3… ${Math.round(i/end*100)}%`);await sleep0()}}
 const fl=enc.flush();if(fl.length)out.push(new Uint8Array(fl));return new Blob(out,{type:'audio/mpeg'})}
function download(blob,name){const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=name.normalize('NFD').replace(/[\u0300-\u036f]/g,'');document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(a.href),60000)}
function trimEnd(buf){const L=buf.getChannelData(0),R=buf.getChannelData(1),SR=buf.sampleRate;let end=L.length;while(end>SR&&Math.abs(L[end-1])<1e-4&&Math.abs(R[end-1])<1e-4)end--;return Math.min(L.length,end+SR/4|0)}
/* renderiza a música inteira offline (mesmo motor, mixer e master); only = id de uma machine ou faixa (stem) */
async function renderSong(only,say){if(!ctx)initAudio();for(let i=0;i<100&&!dmReady;i++)await new Promise(r=>setTimeout(r,50));
 const SRATE=44100,sd=stepDur(),bars=arrBars(),L=bars*16*sd,len=Math.ceil((L+4)*SRATE),off=new OfflineAudioContext(2,len,SRATE);
 const keepM=M.map(I=>[I,I.mute,I.solo]),keepT=DAWT.map(t=>[t,t.mute,t.solo]);
 if(only){const on=new Set([...M.filter(aud).map(I=>I.id),...DAWT.filter(tAud).map(t=>t.id)]);M.forEach(I=>{I.solo=false;I.mute=!(I.id===only&&on.has(I.id))});DAWT.forEach(t=>{t.solo=false;t.mute=!(t.id===only&&on.has(t.id))})}
 const cap=()=>({ctx,noiseBuf,bus,hp,lp,dry,wet,shaper,big,an,eIn,dl,fb,ef,eOut,chans:{...chans},lastBass,openHats:[...openHats]});
 const put=g=>{({ctx,noiseBuf,bus,hp,lp,dry,wet,shaper,big,an,eIn,dl,fb,ef,eOut,lastBass}=g);Object.assign(chans,g.chans);openHats.length=0;openHats.push(...g.openHats)};
 const live=cap();let og;
 try{
  try{initAudio(off);lastBass=null;openHats.length=0;og=cap()}finally{put(live)}
  for(const t of DAWT){if(!tAud(t))continue;const n=trackNodes(off,og.bus,og.eIn);n.g.gain.value=t.vol;n.p.pan.value=t.pan;n.s.gain.value=t.echo;
   for(const c of t.clips){const st=.05+c.bar*16*sd,dur=Math.min(c.dur,.05+L-st);if(dur<=0)continue;const s=off.createBufferSource(),g=off.createGain();s.buffer=getAB(off,c.buf);g.gain.value=c.gain;s.connect(g);g.connect(n.g);s.start(st,c.off,dur)}}
  const plan=[];let t=.05;ARR.order.forEach(k=>{const sec=ARR.secs[k];if(!sec)return;for(let ab=0;ab<sec.bars;ab++){plan.push({t,sec,ab});t+=16*sd}});
  const q=128/SRATE,doBar=i=>{const P=plan[i];put(og);try{for(let s=0;s<16;s++)sched(s,P.ab%H.bars,P.t+s*sd,{sec:P.sec,last:P.ab===P.sec.bars-1})}finally{og=cap();put(live);hitQ.length=0}};
  if(plan.length)doBar(0);if(plan.length>1)doBar(1);
  for(let i=2;i<plan.length;i++){const at=Math.max(q,Math.floor((plan[i].t-.5)/q)*q);off.suspend(at).then(()=>{doBar(i);say&&say(`Gravando… ${Math.round(i/plan.length*100)}%`);off.resume()})}
  return await off.startRendering()}
 finally{keepM.forEach(([I,m,s])=>{I.mute=m;I.solo=s});keepT.forEach(([x,m,s])=>{x.mute=m;x.solo=s})}}
let busy=false;
const songName=()=>`RMD-0420 ${fileTag()} (${H.bpm} BPM)`;
async function exportJob(btn,job){if(busy)return;if(!H.ready){toastMsg('Crie ou carregue um riddim antes de exportar');return}if(!ARR.order.length){toastMsg('Monte a ordem da música antes de exportar');return}
 busy=true;const lbl=btn.textContent,say=m=>btn.textContent=m;btn.disabled=true;
 try{if(playing)$('play').click();await job(say)}catch(e){console.error(e);toastMsg(e&&e.message==='lame'?'Não consegui carregar o codificador de MP3':'Não consegui exportar neste navegador')}
 finally{busy=false;btn.disabled=false;btn.textContent=lbl}}
$('dwav').onclick=e=>exportJob(e.currentTarget,async say=>{say('Gravando a música…');const b=await renderSong(null,say),end=trimEnd(b);say('Montando o WAV…');await sleep0();const bl=wavBlob(b,end);download(bl,songName()+'.wav');toastMsg(`WAV pronto: ${mmss(end/b.sampleRate)} · ${(bl.size/1048576).toFixed(1)} MB`)});
$('dmp3').onclick=e=>exportJob(e.currentTarget,async say=>{say('Carregando o codificador…');await loadLame();say('Gravando a música…');const b=await renderSong(null,say),end=trimEnd(b),bl=await mp3Blob(b,end,say);download(bl,songName()+'.mp3');toastMsg(`MP3 pronto: ${mmss(end/b.sampleRate)} · ${(bl.size/1048576).toFixed(1)} MB`)});
$('dstems').onclick=e=>exportJob(e.currentTarget,async say=>{
 const list=[...M.filter(I=>aud(I)&&ARR.order.some(k=>ARR.secs[k]&&ARR.secs[k].on[I.id]!==false)).map(I=>({id:I.id,n:mlabel(I)})),...DAWT.filter(t=>tAud(t)&&t.clips.length).map(t=>({id:t.id,n:t.name}))];
 if(!list.length){toastMsg('Nenhuma faixa tocando para exportar');return}
 for(let i=0;i<list.length;i++){const s=list[i];say(`Stem ${i+1} de ${list.length}: ${s.n}…`);const b=await renderSong(s.id);download(wavBlob(b,b.length),`${String(i+1).padStart(2,'0')} ${s.n.replace(/[\\/:*?"<>|]+/g,'')} - ${songName()}.wav`);await new Promise(r=>setTimeout(r,700))}
 toastMsg(`${list.length} stems prontos (se o navegador perguntar, permita vários downloads)`)});
$('arrmp3').onclick=()=>$('dmp3').click();

/* ---------- projeto: salvar no aparelho (IndexedDB) e em arquivo ---------- */
const f2b64=f=>{const i16=new Int16Array(f.length);for(let i=0;i<f.length;i++)i16[i]=cv16(f[i]);const u8=new Uint8Array(i16.buffer);let s='';for(let i=0;i<u8.length;i+=32768)s+=String.fromCharCode.apply(null,u8.subarray(i,i+32768));return btoa(s)};
const b642f=s=>{const bin=atob(s),u8=new Uint8Array(bin.length);for(let i=0;i<bin.length;i++)u8[i]=bin.charCodeAt(i);const i16=new Int16Array(u8.buffer),f=new Float32Array(i16.length);for(let i=0;i<i16.length;i++)f[i]=i16[i]/32768;return f};
function projObj(raw){const used=new Set();DAWT.forEach(t=>t.clips.forEach(c=>used.add(c.buf)));
 return{app:'RMD-0420',v:1,date:Date.now(),mrm:snap('projeto'),posBar,armT,tracks:DAWT.map(t=>({...t,clips:t.clips.map(c=>({...c}))})),
  bufs:Object.fromEntries([...used].filter(id=>BUFS[id]).map(id=>[id,{sr:BUFS[id].sr,ch:raw?BUFS[id].ch:BUFS[id].ch.map(f2b64)}]))}}
function loadProj(o,raw){if(!o||o.app!=='RMD-0420')throw new Error('formato');
 if(playing)$('play').click();for(const k in tch){try{tch[k].g.disconnect()}catch(e){}delete tch[k]}
 for(const k in BUFS)delete BUFS[k];Object.entries(o.bufs||{}).forEach(([id,b])=>{BUFS[id]={sr:b.sr,ch:raw?b.ch:b.ch.map(b642f)};peaks(id)});
 DAWT.length=0;(o.tracks||[]).forEach(t=>DAWT.push({...t,clips:t.clips.filter(c=>BUFS[c.buf])}));armT=o.armT||(DAWT[0]&&DAWT[0].id);posBar=o.posBar||0;selClip=null;
 restore(o.mrm);applyAllT();renderStudio()}
const IDB=()=>new Promise((ok,no)=>{const r=indexedDB.open('rmd-0420',1);r.onupgradeneeded=()=>r.result.createObjectStore('p');r.onsuccess=()=>ok(r.result);r.onerror=()=>no(r.error)});
async function idb(mode,fn){const db=await IDB();return new Promise((ok,no)=>{const tx=db.transaction('p',mode),rq=fn(tx.objectStore('p'));tx.oncomplete=()=>ok(rq&&rq.result);tx.onerror=()=>no(tx.error)})}
let asT=null,asHold=false;
function autosave(){if(!H.ready||asHold)return;clearTimeout(asT);asT=setTimeout(()=>{idb('readwrite',s=>s.put(projObj(true),'atual')).catch(()=>{})},1500)}
$('dsave').onclick=()=>{if(!H.ready){toastMsg('Crie um riddim antes de salvar');return}const js=JSON.stringify(projObj(false));download(new Blob([js],{type:'application/json'}),songName()+'.rmd');toastMsg(`Projeto salvo (${(js.length/1048576).toFixed(1)} MB)`)};
$('dopen').onchange=async e=>{const f=e.target.files[0];e.target.value='';if(!f)return;try{loadProj(JSON.parse(await f.text()),false);asHold=false;$('dban').innerHTML='';autosave();toastMsg('Projeto aberto: '+f.name)}catch(err){console.error(err);toastMsg('Este arquivo não é um projeto da Riddim Machine DAW')}};
$('dclear').onclick=async()=>{if(!confirm('Começar um projeto novo? O projeto atual sai deste aparelho (salve em arquivo antes, se quiser guardar).'))return;try{await idb('readwrite',s=>s.delete('atual'))}catch(e){}location.reload()};
/* ao abrir: oferece o último projeto salvo neste aparelho */
idb('readonly',s=>s.get('atual')).then(o=>{if(!o||!o.mrm||!o.mrm.ready)return;asHold=true;const d=new Date(o.date);
 $('dban').innerHTML=`<div class="dbanner"><span>Projeto salvo neste aparelho: <b>${esc(o.mrm.cur&&o.mrm.cur.s||'riddim')}</b> · ${o.tracks.length} faixa(s) de áudio · ${d.toLocaleDateString('pt-BR')} ${d.toLocaleTimeString('pt-BR',{hour:'2-digit',minute:'2-digit'})}</span><span class="grow" style="flex:1"></span><button class="btn green" id="dcont">CONTINUAR</button><button class="btn" id="dnov">Começar outro</button></div>`;
 $('dcont').onclick=()=>{try{loadProj(o,true);toastMsg('Projeto aberto')}catch(e){console.error(e);toastMsg('Não consegui abrir o projeto salvo')}asHold=false;$('dban').innerHTML=''};
 $('dnov').onclick=()=>{asHold=false;$('dban').innerHTML=''}}).catch(()=>{});

/* mantém a timeline em dia com as outras abas (setup, arranjo, mixer, presets) e salva sozinho */
let lastSig='',lastSnap='';
setInterval(()=>{if(asHold&&H.ready){asHold=false;$('dban').innerHTML=''}
 if(wantStudio&&H.ready){wantStudio=false;show('studio');if(playing&&!dawPlaying){$('play').click();posBar=0;studioStart(false)}}
 const sig=JSON.stringify([H.ready,H.bpm,H.chords,ARR.order,ARR.loop,Object.entries(ARR.secs).map(([k,x])=>[k,x.n,x.c,x.bars,x.on]),M.map(I=>[I.mute,I.solo])]);
 if(sig!==lastSig){lastSig=sig;applyAllT();if(!drag)renderStudio()}
 const s=snap('');s.date=0;const js=JSON.stringify(s);if(js!==lastSnap){if(lastSnap)autosave();lastSnap=js}},700);
tabs.addEventListener('click',()=>setTimeout(()=>{if(studioOn())renderStudio()},0));
renderStudio();show('studio');
