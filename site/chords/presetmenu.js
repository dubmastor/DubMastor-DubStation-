// Menu de PRESETS em cascata (MRCM-1240 e MRM-0420 no navegador): pasta (estilo e época) > letras (A - G...) > música.
// Abre clicando (toque no celular) ou passando o mouse (computador). Usa o banco de presets.js (DUB_PRESETS).
(function (root) {
  const CSS = `
.pmenu{position:relative;margin:2px 0 10px}
.pmenu>button.pbtn{width:100%;padding:12px;border:0;border-radius:6px;font-family:inherit;font-weight:900;font-size:13px;line-height:1.3;letter-spacing:.14em;color:var(--gold,#e8b64a);
  cursor:pointer;background:linear-gradient(180deg,#2c2925,#141210);box-shadow:inset 0 -3px 0 rgba(232,182,74,.55),0 2px 0 #000;text-align:left}
.pmenu .pm{position:absolute;z-index:50;left:0;right:0;top:calc(100% + 4px);max-height:70vh;overflow:auto;background:#1a1714;border-radius:6px;
  box-shadow:0 10px 30px rgba(0,0,0,.55);padding:6px;color:#efe5cc}
.pmenu .pm[hidden]{display:none}
.pmenu details{border-radius:4px}
.pmenu summary{list-style:none;cursor:pointer;padding:10px 10px;font-weight:900;font-size:12px;letter-spacing:.12em;border-radius:4px;display:flex;justify-content:space-between;gap:8px}
.pmenu summary::-webkit-details-marker{display:none}
.pmenu summary::after{content:'▸';opacity:.6}
.pmenu details[open]>summary::after{content:'▾'}
.pmenu summary:hover,.pmenu details[open]>summary{background:#2c2925}
.pmenu .pf>summary{color:#e8b64a}
.pmenu .pg{margin:2px 0 2px 12px}
.pmenu .pg>summary{font-size:11px;color:#efe5cc}
.pmenu .pg>summary small{opacity:.55;font-weight:700;letter-spacing:.02em;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;flex:1;text-align:right}
.pmenu .song{display:block;width:calc(100% - 12px);margin:2px 0 2px 12px;text-align:left;border:0;border-radius:4px;background:#0f0d0b;color:#efe5cc;
  padding:8px 10px;font-family:inherit;font-weight:700;font-size:12px;line-height:1.35;cursor:pointer}
.pmenu .song:hover,.pmenu .song.on{background:#3a2f1c;color:#ffc061}
.pmenu .song small{display:block;opacity:.65;font-weight:600;font-size:10.5px;letter-spacing:.04em}`;

  function presetMenu(host, opts) {
    const P = root.DUB_PRESETS;
    if (!document.getElementById('pmenu-css')) {
      const s = document.createElement('style'); s.id = 'pmenu-css'; s.textContent = CSS; document.head.appendChild(s);
    }
    host.classList.add('pmenu');
    host.innerHTML = '';
    const btn = document.createElement('button'); btn.className = 'pbtn'; btn.type = 'button'; btn.textContent = opts.label;
    const pm = document.createElement('div'); pm.className = 'pm'; pm.hidden = true;
    host.append(btn, pm);
    const hover = root.matchMedia && root.matchMedia('(hover: hover) and (pointer: fine)').matches;
    const order = P.menuOrder || P.presets.map((_, i) => i);
    const songs = [];

    // abrir um details fecha os irmãos; no computador, passar o mouse abre
    const wire = (d) => {
      d.addEventListener('toggle', () => {
        if (!d.open) return;
        for (const s of d.parentElement.children) if (s !== d && s.tagName === 'DETAILS') s.open = false;
      });
      const sm = d.querySelector('summary');
      if (hover) sm.addEventListener('mouseenter', () => { if (!d.open) { d.open = true; d.hoverAt = performance.now(); } });
      // o clique logo depois de abrir pelo mouse não fecha de novo
      sm.addEventListener('click', (e) => { if (d.open && performance.now() - (d.hoverAt || 0) < 700) e.preventDefault(); });
    };

    P.folders.forEach((f) => {
      const idx = order.filter((i) => P.presets[i].folder === f);
      const fd = document.createElement('details'); fd.className = 'pf';
      fd.innerHTML = `<summary><span>${f}</span><span style="opacity:.6">${idx.length}</span></summary>`;
      let gd = null, group = null;
      idx.forEach((i) => {
        const p = P.presets[i];
        if (p.group !== group) {
          group = p.group;
          const names = idx.filter((j) => P.presets[j].group === group).map((j) => P.presets[j].name.replace(/ \(.*$/, ''));
          gd = document.createElement('details'); gd.className = 'pg';
          gd.innerHTML = `<summary><span>MÚSICAS ${group}</span><small>${names.join(' · ')}</small></summary>`;
          fd.appendChild(gd); wire(gd);
        }
        const b = document.createElement('button'); b.type = 'button'; b.className = 'song'; b.dataset.i = i;
        b.innerHTML = `${p.name}<small>${opts.itemText(p)}</small>`;
        b.onclick = () => { close(); opts.onPick(p, i); };
        gd.appendChild(b); songs.push(b);
      });
      pm.appendChild(fd); wire(fd);
    });

    const open = () => {
      const cur = opts.current ? opts.current() : -1;
      songs.forEach((b) => b.classList.toggle('on', +b.dataset.i === cur));
      pm.hidden = false;
    };
    const close = () => { pm.hidden = true; };
    btn.onclick = (e) => { e.stopPropagation(); pm.hidden ? open() : close(); };
    pm.addEventListener('click', (e) => e.stopPropagation());
    document.addEventListener('click', close);
    document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    return { open, close };
  }
  root.presetMenu = presetMenu;
})(typeof window !== 'undefined' ? window : globalThis);
