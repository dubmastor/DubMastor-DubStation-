// Confere que a versão de navegador (site/bassmachine/engine.js) gera exatamente as mesmas linhas e a
// mesma bateria que o plugin. Uso (o CI roda assim):
//   g++ -std=c++17 -O2 tests/dump_engine.cpp -o dump_engine
//   ./dump_engine book > book.json; ./dump_engine lines > lines.json; ./dump_engine drums > drums.json
//   node tests/web_engine_test.js book.json lines.json drums.json
'use strict';
const fs = require('fs'), path = require('path'), vm = require('vm');
const [bookFile, linesFile, drumsFile] = process.argv.slice(2);
const ctx = { Math, BigInt, Number, Array, Object, globalThis: {} };
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(path.join(__dirname, '../../site/bassmachine/engine.js'), 'utf8'), ctx);
const M = ctx.globalThis.MRBM;
let fails = 0;
const fail = (...a) => { if (fails++ < 15) console.log('FALHOU:', ...a); };

// 1. células embutidas = células do C++
const book = JSON.parse(fs.readFileSync(bookFile, 'utf8'));
if (JSON.stringify(book) !== JSON.stringify(M.BOOK)) fail('BOOK do engine.js diferente do C++ (rode tests/dump_engine.cpp book e regenere)');

// 2. linhas
const harmonies = [
  [{ root: 9, quality: 1, beats: 2 }, { root: 2, quality: 1, beats: 2 }],
  [{ root: 9, quality: 1, beats: 8 }, { root: 2, quality: 1, beats: 4 }, { root: 4, quality: 3, beats: 4 }, { root: 7, quality: 0, beats: 2 }],
  [{ root: 0, quality: 0, beats: 4 }, { root: 5, quality: 2, beats: 4 }, { root: 7, quality: 2, beats: 8 }],
  [{ root: 4, quality: 3, beats: 32 }],
  [{ root: 10, quality: 0, beats: 12 }, { root: 3, quality: 1, beats: 2 }, { root: 5, quality: 2, beats: 16 }],
];
const lines = JSON.parse(fs.readFileSync(linesFile, 'utf8'));
let notes = 0;
for (const c of lines) {
  const r = M.buildLoop(harmonies[c.h], c);
  const tag = `h${c.h} estilo ${c.style} ideia ${c.seed} dens ${c.density} var ${c.variation} sw ${c.swing} base ${c.base}`;
  if (Math.abs(r.loopBeats - c.L) > 1e-12) { fail(tag, 'loop', r.loopBeats, c.L); continue; }
  if (r.events.length !== c.ev.length) { fail(tag, 'notas', r.events.length, 'esperado', c.ev.length); continue; }
  c.ev.forEach((e, i) => {
    const g = r.events[i];
    if (Math.abs(g.start - e[0]) > 1e-9 || Math.abs(g.length - e[1]) > 1e-9 || g.note !== e[2] || g.velocity !== e[3] || (g.fill ? 1 : 0) !== e[4])
      fail(tag, 'nota', i, JSON.stringify(g), 'esperado', JSON.stringify(e));
    notes++;
  });
}

// 3. bateria
const drums = JSON.parse(fs.readFileSync(drumsFile, 'utf8'));
for (const c of drums) {
  const d = M.buildDrums(c.L, c.style, c.swing);
  if (d.length !== c.ev.length) { fail('bateria', c.style, c.L, d.length, c.ev.length); continue; }
  c.ev.forEach((e, i) => {
    const g = d[i];
    if (Math.abs(g.start - e[0]) > 1e-9 || g.note !== e[1] || g.velocity !== e[2] || g.target !== e[3]) fail('bateria', c.style, c.L, i, JSON.stringify(g), JSON.stringify(e));
  });
}

console.log(`${lines.length} linhas (${notes} notas) e ${drums.length} baterias comparadas com o C++`);
console.log(fails === 0 ? 'NAVEGADOR = PLUGIN (TODOS OS TESTES PASSARAM)' : `${fails} DIFERENCAS`);
process.exit(fails === 0 ? 0 : 1);
