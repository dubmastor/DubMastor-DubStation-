// Próximas harmonias (cadências do reggae) para as versões web da CHORD MACHINE (MRK-80) e da BASS MACHINE (MRB-69).
// Cópia de ReggaeChordMachine/Source/CadenceHints.h: mudou um, mude o outro (as mesmas 3 sugestões, na mesma ordem).
// Precisa do banco (riddimbank.js) carregado antes.
(function (root) {
const DEGREES = ['I', 'bII', 'II', 'bIII', 'III', 'IV', 'bV', 'V', 'bVI', 'VI', 'bVII', 'VII'];
const cadCode = (iv, minor) => (((iv % 12) + 12) % 12) * 2 + (minor ? 1 : 0);
const CAD_RULES = [
  [0, cadCode(0, 0), [cadCode(5, 0), cadCode(7, 0), cadCode(2, 1), cadCode(9, 1)]],
  [0, cadCode(5, 0), [cadCode(7, 0), cadCode(0, 0), cadCode(2, 1)]],
  [0, cadCode(7, 0), [cadCode(5, 0), cadCode(0, 0), cadCode(9, 1)]],
  [0, cadCode(2, 1), [cadCode(7, 0), cadCode(5, 0)]],
  [0, cadCode(9, 1), [cadCode(5, 0), cadCode(2, 1), cadCode(7, 0)]],
  [0, cadCode(4, 1), [cadCode(5, 0), cadCode(9, 1)]],
  [0, cadCode(10, 0), [cadCode(5, 0), cadCode(0, 0)]],
  [1, cadCode(0, 1), [cadCode(10, 0), cadCode(5, 1), cadCode(7, 1), cadCode(8, 0)]],
  [1, cadCode(5, 1), [cadCode(0, 1), cadCode(10, 0), cadCode(7, 1)]],
  [1, cadCode(7, 1), [cadCode(5, 1), cadCode(0, 1)]],
  [1, cadCode(8, 0), [cadCode(10, 0), cadCode(7, 1)]],
  [1, cadCode(10, 0), [cadCode(0, 1), cadCode(8, 0), cadCode(5, 1)]],
  [1, cadCode(3, 0), [cadCode(5, 1), cadCode(10, 0)]],
];
function cadenceHints(tonic, tonicMinor, prev, prevMinor, maxHints = 3) {
  const from = cadCode(prev - tonic, prevMinor), count = new Array(24).fill(0); let total = 0;
  for (const e of (window.RIDDIM_BANK || { entries: [] }).entries) {
    const c = e.chords || [];
    if (e.harmony >= 2 || c.length < 2 || (c[0][1] !== 0) !== (tonicMinor !== 0)) continue;
    const t = c[0][0];
    for (let k = 1; k < Math.min(c.length, 4); k++) if (cadCode(c[k - 1][0] - t, c[k - 1][1]) === from) { count[cadCode(c[k][0] - t, c[k][1])]++; total++; }
  }
  const theory = new Array(24).fill(99);
  for (const [mk, f, to] of CAD_RULES) if (mk === (tonicMinor ? 1 : 0) && f === from) to.forEach((c, k) => { theory[c] = k; });
  const order = [];
  for (let c = 0; c < 24; c++) if (c !== from && ((total >= 3 && count[c] > 0) || theory[c] < 99)) order.push(c);
  order.sort((a, b) => { const ca = total >= 3 ? count[a] : 0, cb = total >= 3 ? count[b] : 0; return ca !== cb ? cb - ca : theory[a] - theory[b] || a - b; });
  const out = [];
  for (const c of order) {
    const root = (tonic + (c >> 1)) % 12;
    if (out.some((h) => h.root === root)) continue;
    out.push({ root, minor: c & 1, rank: out.length + 1, label: DEGREES[(c >> 1)] + (c & 1 ? 'm' : '') });
    if (out.length >= maxHints) break;
  }
  return out;
}
root.cadenceHints = cadenceHints;
root.CADENCE_DEGREES = DEGREES;
})(typeof window !== 'undefined' ? window : globalThis);
