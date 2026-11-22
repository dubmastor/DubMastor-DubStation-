// Confere que o gerador do navegador (site/drums/generator.js) faz exatamente o mesmo que o do plugin.
// Uso: ./dump_patterns > padroes.jsonl && node tests/web_generator_test.js padroes.jsonl
const fs = require('fs');
const path = require('path');
const G = require(path.join(__dirname, '..', '..', 'site', 'drums', 'generator.js'));
const lines = fs.readFileSync(process.argv[2] || 'padroes.jsonl', 'utf8').trim().split('\n');
let bad = 0, notes = 0;
for (const line of lines) {
  const ref = JSON.parse(line);
  const p = G.generate(ref.s);
  const cmp = (a, b, what) => {
    if (a.length !== b.length) { if (bad++ < 5) console.log('DIFERENTE (' + what + ' qtde)', JSON.stringify(ref.s), a.length, b.length); return; }
    for (let i = 0; i < a.length; ++i) {
      const [beat, len, note, vel, bar] = b[i]; const e = a[i]; ++notes;
      if (Math.abs(e.beat - beat) > 1e-7 || e.note !== note || e.velocity !== vel || e.bar !== bar || Math.abs(e.length - len) > 1e-3) {
        if (bad++ < 5) console.log('DIFERENTE (' + what + ' nota ' + i + ')', JSON.stringify(ref.s), JSON.stringify(e), JSON.stringify(b[i]));
        return;
      }
    }
  };
  if (Math.abs(p.lengthInBeats - ref.len) > 1e-9 || Math.abs(p.introLengthInBeats - ref.introLen) > 1e-9) bad++;
  cmp(p.events, ref.events, 'loop');
  cmp(p.intro, ref.intro, 'intro');
}
console.log(`web x plugin: ${lines.length} combinacoes, ${notes} notas conferidas, ${bad} diferencas`);
process.exit(bad === 0 ? 0 : 1);
