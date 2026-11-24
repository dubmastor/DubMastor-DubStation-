// Menu do BANCO ÚNICO DE RIDDIMS no navegador (MRCM-1240, MRDM-12, MRBM-69 e MRM-0420): o mesmo dos plugins.
// ESTILOS · RIDDIMS A–Z (cada riddim com as músicas gravadas nele) · MÚSICAS A–Z, e as luzes H B D
// (acesa = harmonia, baixo ou bateria vieram da gravação original). Usa riddimbank.js (RIDDIM_BANK).
(function (root) {
  const CSS = `
.rbm{position:relative;margin:2px 0 10px}
.rbm>button.pbtn{width:100%;padding:12px;border:0;border-radius:6px;font-family:inherit;font-weight:900;font-size:13px;line-height:1.3;letter-spacing:.14em;color:var(--gold,#e8b64a);
  cursor:pointer;background:linear-gradient(180deg,#2c2925,#141210);box-shadow:inset 0 -3px 0 rgba(232,182,74,.55),0 2px 0 #000;text-align:left}
.rbm .pm{position:absolute;z-index:50;left:0;right:0;top:calc(100% + 4px);max-height:72vh;overflow:auto;background:#1a1714;border-radius:6px;
  box-shadow:0 10px 30px rgba(0,0,0,.55);padding:6px;color:#efe5cc}
.rbm .pm[hidden]{display:none}
.rbm .hd{padding:8px 10px 6px;font-weight:900;font-size:11px;letter-spacing:.14em;color:#c9a45c}
.rbm .tabs{display:flex;gap:4px;margin:0 0 6px}
.rbm .tabs button{flex:1;border:0;border-radius:4px;padding:10px 4px;font-family:inherit;font-weight:900;font-size:11px;letter-spacing:.1em;color:#efe5cc;background:#0f0d0b;cursor:pointer}
.rbm .tabs button.on{background:#3a2f1c;color:#ffc061}
.rbm .only{display:flex;align-items:center;gap:8px;padding:6px 10px 8px;font-size:11px;font-weight:700;letter-spacing:.04em;cursor:pointer;opacity:.85}
.rbm details{border-radius:4px}
.rbm summary{list-style:none;cursor:pointer;padding:10px 10px;font-weight:900;font-size:12px;letter-spacing:.12em;border-radius:4px;display:flex;justify-content:space-between;gap:8px}
.rbm summary::-webkit-details-marker{display:none}
.rbm summary:hover,.rbm details[open]>summary{background:#2c2925}
.rbm .lv1>summary{color:#e8b64a}
.rbm .lv2{margin:2px 0 2px 12px}
.rbm .lv2>summary{font-size:11px;color:#efe5cc;letter-spacing:.06em}
.rbm summary small{opacity:.55;font-weight:700;letter-spacing:.02em;white-space:nowrap}
.rbm .song{display:block;width:calc(100% - 12px);margin:2px 0 2px 12px;text-align:left;border:0;border-radius:4px;background:#0f0d0b;color:#efe5cc;
  padding:8px 10px;font-family:inherit;font-weight:700;font-size:12px;line-height:1.35;cursor:pointer}
.rbm .lv2 .song{margin-left:24px;width:calc(100% - 24px)}
.rbm .song:hover,.rbm .song.on{background:#3a2f1c;color:#ffc061}
.rbm .song:disabled{opacity:.4;cursor:default}
.rbm .song small{display:block;opacity:.65;font-weight:600;font-size:10.5px;letter-spacing:.04em}
.rbm .song .orig{font-size:9px;letter-spacing:.14em;color:#c9a45c;margin-right:6px}
.rbled{display:inline-flex;align-items:center;gap:3px;margin-left:8px;font-size:10px;font-weight:900;letter-spacing:0}
.rbled i{width:7px;height:7px;border-radius:50%;display:inline-block;opacity:.2;flex:none}
.rbled i.on{opacity:1;box-shadow:0 0 6px currentColor}
.rbm .note{padding:8px 10px;font-size:10.5px;opacity:.6;line-height:1.4}`;

  const NOTES = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B'];
  const LED = [['H', '#e5484d'], ['B', '#f5c542'], ['D', '#3fb950']];
  const chordsText = (e) => e.chords.length ? e.chords.map((c) => NOTES[c[0]] + (c[1] ? 'm' : '')).join('-') + (e.harmony === 1 ? ' ~' : '') : 'harmonia pendente';
  const lights = (e) => '<span class="rbled">' + LED.map(([l, c], k) => `<i style="color:${c};background:${c}" class="${(e.orig >> k) & 1 ? 'on' : ''}"></i>${l}`).join(' ') + '</span>';
  const title = (e) => e.name + (e.artist ? ' — ' + e.artist : '') + (e.year ? ' (' + e.year + ')' : '');
  const usable = (B, i) => i >= 0 && i < B.entries.length && B.entries[i].chords.length > 0;
  const firstLetter = (name) => { let s = name.trim(); if (/^the /i.test(s)) s = s.slice(4); const c = s.charAt(0).toUpperCase(); return /[A-Z]/.test(c) ? c : 'A'; };

  const addCss = () => {
    if (typeof document === 'undefined' || document.getElementById('rbm-css')) return;
    const s = document.createElement('style'); s.id = 'rbm-css'; s.textContent = CSS; document.head.appendChild(s);
  };
  addCss();

  function riddimBankMenu(host, opts) {
    const B = root.RIDDIM_BANK;
    addCss();
    host.classList.add('rbm');
    host.innerHTML = '';
    const btn = document.createElement('button'); btn.className = 'pbtn'; btn.type = 'button'; btn.textContent = opts.label || 'BANCO DE RIDDIMS ▾';
    const pm = document.createElement('div'); pm.className = 'pm'; pm.hidden = true;
    host.append(btn, pm);
    const hover = root.matchMedia && root.matchMedia('(hover: hover) and (pointer: fine)').matches;
    let view = 0, only = false;
    try { only = root.localStorage.getItem('rbm-only') === '1'; view = +(root.localStorage.getItem('rbm-view') || 0); } catch (e) {}

    const wire = (d) => {
      d.addEventListener('toggle', () => {
        if (!d.open) return;
        for (const s of d.parentElement.children) if (s !== d && s.tagName === 'DETAILS') s.open = false;
      });
      const sm = d.querySelector('summary');
      if (hover) sm.addEventListener('mouseenter', () => { if (!d.open) { d.open = true; d.hoverAt = performance.now(); } });
      sm.addEventListener('click', (e) => { if (d.open && performance.now() - (d.hoverAt || 0) < 700) e.preventDefault(); });
    };
    const shown = (i) => !only || B.entries[i].orig === 7;
    const song = (i, cur, prefix) => {
      const e = B.entries[i];
      const b = document.createElement('button'); b.type = 'button'; b.className = 'song' + (i === cur ? ' on' : '');
      b.disabled = !usable(B, i) || (opts.enabled ? !opts.enabled(e, i) : false);
      b.innerHTML = `${prefix ? '<span class="orig">ORIGEM</span>' : ''}${title(e)}${lights(e)}<small>${chordsText(e)} · ${e.bpmMeasured ? '' : '~'}${e.bpm} BPM${e.riddim && e.riddim !== e.name ? ' · ' + e.riddim + ' riddim' : ''}${e.credits ? '<br>' + e.credits : ''}</small>`;
      b.onclick = () => { close(); opts.onPick(e, i); };
      return b;
    };
    const group = (cls, label, count, here) => {
      const d = document.createElement('details'); d.className = cls;
      d.innerHTML = `<summary><span>${label}</span><small>${count}</small></summary>`;
      if (here) d.open = true;
      wire(d);
      return d;
    };

    function render() {
      const cur = opts.current ? opts.current() : -1;
      pm.innerHTML = `<div class="hd">BANCO DE RIDDIMS · ${B.entries.length} MÚSICAS · ${B.families.length} RIDDIMS</div>`;
      const tabs = document.createElement('div'); tabs.className = 'tabs';
      ['ESTILOS', 'RIDDIMS A–Z', 'MÚSICAS A–Z'].forEach((t, k) => {
        const b = document.createElement('button'); b.type = 'button'; b.textContent = t; b.className = k === view ? 'on' : '';
        b.onclick = () => { view = k; try { root.localStorage.setItem('rbm-view', k); } catch (e) {} render(); };
        tabs.appendChild(b);
      });
      pm.appendChild(tabs);
      const ol = document.createElement('label'); ol.className = 'only';
      ol.innerHTML = `<input type="checkbox" ${only ? 'checked' : ''}> Só completos (harmonia, baixo e bateria da gravação)`;
      ol.querySelector('input').onchange = (ev) => { only = ev.target.checked; try { root.localStorage.setItem('rbm-only', only ? '1' : '0'); } catch (e) {} render(); };
      pm.appendChild(ol);

      if (view === 0) {
        B.styles.forEach((s, k) => {
          const list = B.songOrder.filter((i) => B.entries[i].style === k && shown(i));
          if (!list.length) return;
          const d = group('lv1', `${s} <small>${B.styleYears[k]}</small>`, list.length, cur >= 0 && B.entries[cur].style === k);
          list.forEach((i) => d.appendChild(song(i, cur)));
          pm.appendChild(d);
        });
      } else if (view === 1) {
        for (let L = 65; L <= 90; L++) {
          const ch = String.fromCharCode(L);
          const fams = B.families.filter((f) => f.letter === ch && B.members.slice(f.first, f.first + f.count).some(shown));
          if (!fams.length) continue;
          const here = cur >= 0 && fams.some((f) => B.members.slice(f.first, f.first + f.count).includes(cur));
          const d = group('lv1', ch, fams.length, here);
          fams.forEach((f) => {
            const mem = B.members.slice(f.first, f.first + f.count).filter(shown);
            if (mem.length === 1 && f.count === 1) { d.appendChild(song(mem[0], cur)); return; }
            const fd = group('lv2', `${f.name} (${f.year})`, mem.length + ' músicas', mem.includes(cur));
            mem.forEach((i, k) => fd.appendChild(song(i, cur, B.members[f.first] === i)));
            d.appendChild(fd);
          });
          pm.appendChild(d);
        }
      } else {
        for (let L = 65; L <= 90; L++) {
          const ch = String.fromCharCode(L);
          const list = B.songOrder.filter((i) => firstLetter(B.entries[i].name) === ch && shown(i));
          if (!list.length) continue;
          const d = group('lv1', ch, list.length, list.includes(cur));
          list.forEach((i) => d.appendChild(song(i, cur)));
          pm.appendChild(d);
        }
      }
      const n = document.createElement('div'); n.className = 'note';
      n.textContent = (opts.enabledNote ? opts.enabledNote + ' ' : '') + 'Luzes H B D: harmonia, baixo e bateria. Acesa = veio da gravação original; apagada = escolhida pelo músico mais próximo de quem gravou. ~ = aproximada.';
      pm.appendChild(n);
    }

    const open = () => { render(); pm.hidden = false; };
    const close = () => { pm.hidden = true; };
    btn.onclick = (e) => { e.stopPropagation(); pm.hidden ? open() : close(); };
    pm.addEventListener('click', (e) => e.stopPropagation());
    document.addEventListener('click', close);
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    return { open, close };
  }
  root.riddimBankMenu = riddimBankMenu;
  root.riddimBankText = { chordsText, lights, title, usable };
})(typeof window !== 'undefined' ? window : globalThis);
