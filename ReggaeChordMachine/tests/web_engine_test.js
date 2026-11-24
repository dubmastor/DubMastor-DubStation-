// Confere se a versão de celular (site/chords/engine.js) gera exatamente o mesmo MIDI que o plugin.
// Uso (da raiz do repositório):
//   g++ -std=c++17 -O2 ReggaeChordMachine/tests/dump_loop.cpp -o dump_loop && node ReggaeChordMachine/tests/web_engine_test.js ./dump_loop
'use strict';
const { execFileSync } = require('child_process');
const path = require('path');
const M = require(path.join(__dirname, '../../site/chords/engine.js'));

const dumper = process.argv[2] || './dump_loop';
const f32 = Math.fround;

// cenários: aleatórios (semente fixa) + alguns fixos
let seed = 1240;
const rnd = () => { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return seed / 0x7fffffff; };
const scenarios = [];
const fixed = [
  { s: { pattern: 0, centerNote: 60, velocity: 0.8, noteSize: 0, swing: 0, bubble: 0 }, slots: [[9, 1, 2, -1], [2, 1, 2, -1]] },
  { s: { pattern: 0, centerNote: 60, velocity: 0.8, noteSize: 1, swing: 0.6, bubble: 1 }, slots: [[9, 1, 8, -1], [2, 1, 4, -1], [4, 1, 2, -1], [7, 0, 2, -1]] },
  { s: { pattern: 0, centerNote: 55, velocity: 1.0, noteSize: 4, swing: 0.3, bubble: 1 }, slots: [[0, 0, 4, 2], [5, 0, 4, 1], [7, 0, 4, 0], [9, 1, 4, -1]] },
];
for (const f of fixed) scenarios.push(f);
for (let k = 0; k < 300; k++) {
  const n = 1 + Math.floor(rnd() * 4);
  const slots = [];
  for (let i = 0; i < 4; i++) {
    const on = i < n || rnd() < 0.3;
    slots.push([Math.floor(rnd() * 12), rnd() < 0.5 ? 0 : 1, on ? M.DURATIONS[Math.floor(rnd() * 6)].beats : 0, Math.floor(rnd() * 4) - 1]);
  }
  scenarios.push({ s: { pattern: rnd() < 0.3 ? 1 : 0, centerNote: 48 + Math.floor(rnd() * 25),
    velocity: f32(Math.round(rnd() * 90 + 10) / 100), noteSize: Math.floor(rnd() * 5),
    swing: f32(Math.round(rnd() * 100) / 100), bubble: rnd() < 0.6 ? 1 : 0 }, slots });
}

const input = scenarios.map(({ s, slots }) =>
  `${s.pattern} ${s.centerNote} ${s.velocity.toPrecision(9)} ${s.noteSize} ${s.swing.toPrecision(9)} ${s.bubble} | ` +
  slots.map((x) => x.join(' ')).join(' ; ')).join('\n') + '\n';
const out = execFileSync(dumper, { input, maxBuffer: 1 << 28 }).toString().split('\n');

let li = 0, fails = 0, totalEvents = 0;
for (const [k, { s, slots }] of scenarios.entries()) {
  const cpp = { L: 0, ev: [], sug: '' };
  for (; out[li] !== 'END'; li++) {
    const t = out[li].split(' ');
    if (t[0] === 'L') cpp.L = +t[1];
    else if (t[0] === 'S') cpp.sug = t.slice(1).join(' ');
    else cpp.ev.push(t.map(Number));
  }
  li++;
  const js = M.buildLoop(slots.map(([root, quality, beats, inversion]) => ({ root, quality, beats, inversion })),
    { ...s, velocity: f32(s.velocity), swing: f32(s.swing), bubble: !!s.bubble });
  const jsSug = slots.map((_, i) => `${js.voicings.suggested[i]}/${js.voicings.used[i]}`).join(' ');
  let ok = Math.abs(js.loopBeats - cpp.L) < 1e-9 && js.events.length === cpp.ev.length && jsSug === cpp.sug;
  for (let i = 0; ok && i < js.events.length; i++) {
    const a = js.events[i], b = cpp.ev[i];
    ok = Math.abs(a.start - b[0]) < 1e-7 && Math.abs(a.length - b[1]) < 1e-7 && a.note === b[2] &&
         a.velocity === b[3] && a.channel === b[4] && a.slot === b[5];
    if (!ok) console.log(`cenário ${k}, evento ${i}: JS`, a, 'C++', b);
  }
  if (!ok && js.events.length !== cpp.ev.length) console.log(`cenário ${k}: ${js.events.length} eventos no JS, ${cpp.ev.length} no C++`);
  if (!ok && jsSug !== cpp.sug) console.log(`cenário ${k}: inversões JS ${jsSug} / C++ ${cpp.sug}`);
  totalEvents += cpp.ev.length;
  if (!ok) fails++;
}
console.log(`${scenarios.length} cenários, ${totalEvents} notas comparadas: ${fails === 0 ? 'JS IGUAL AO C++' : fails + ' DIFERENTE(S)'}`);
process.exit(fails === 0 ? 0 : 1);
