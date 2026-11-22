// Mastor Reggae Bass Machine MRBM-69 — gerador de linhas de baixo para o navegador.
// Porte fiel de BassMachine/Source/BassEngine.h e AccompEngine.h: mesmas células dos 7 baixistas,
// mesmo gerador aleatório (splitmix64 em BigInt) e as mesmas contas em float de 32 bits
// (Math.fround), então a mesma IDEIA com os mesmos controles gera a mesma linha do plugin.
// O teste BassMachine/tests/web_engine_test.js compara com o C++ nota a nota (roda no CI).
// As células (BOOK) saem do próprio C++: tests/dump_engine.cpp book.
(function (root) {
  'use strict';
  const f = Math.fround;

  // ---------------------------------------------------------------- dados (gerados do C++)
  const BOOK = [{"name":"FAMILY MAN","tagline":"ONE DROP MELODICO","main":[[[0.5,0.9,0,1,0],[1.5,0.4,0,0.699999988,1],[2,0.9,2,0.949999988,0],[3,0.45,4,0.850000024,0],[3.5,0.4,3,0.75,1]],[[0.5,0.45,0,1,0],[1,0.45,1,0.800000012,0],[1.5,0.9,2,0.949999988,0],[3,0.4,6,0.800000012,1],[3.5,0.45,1,0.850000024,0]],[[1,0.9,0,1,0],[2,0.5,2,0.899999976,0],[2.5,0.4,1,0.699999988,1],[3,0.3,2,0.800000012,0],[3.3333333333333335,0.3,7,0.75,0],[3.6666666666666665,0.3333333333333333,4,0.850000024,0]],[[0.5,0.7,0,1,0],[1.25,0.2,0,0.600000024,1],[1.5,0.45,9,0.800000012,0],[2,0.9,8,0.899999976,0],[3.5,0.4,9,0.75,1]]],"fills":[[[0.5,0.9,0,1,0],[2,0.45,2,0.899999976,0],[2.5,0.45,1,0.800000012,0],[3,0.3,5,0.75,0],[3.3333333333333335,0.3,0,0.75,0],[3.6666666666666665,0.3333333333333333,11,0.899999976,0]],[[0.5,0.45,4,1,0],[1,0.45,3,0.850000024,0],[1.5,0.9,2,0.899999976,0],[2.5,0.45,1,0.800000012,1],[3,0.45,5,0.75,0],[3.5,0.45,11,0.899999976,0]],[[1,0.9,0,1,0],[2,0.3,4,0.899999976,0],[2.3333333333333335,0.3,3,0.800000012,0],[2.6666666666666665,0.3333333333333333,2,0.800000012,0],[3,0.45,6,0.75,1],[3.5,0.45,11,0.899999976,0]]],"swing":0.349999994,"varScale":1,"newCellChance":0.600000024,"phraseBars":2,"lag":0},{"name":"FLABBA","tagline":"RUB-A-DUB HIPNOTICO","main":[[[0,1.4,0,1,0],[1.5,0.4,0,0.699999988,1],[2,0.9,0,0.949999988,0],[3,0.9,8,0.899999976,0]],[[0,0.9,0,1,0],[1,0.4,0,0.699999988,1],[1.5,0.45,9,0.800000012,0],[2,1.4,8,0.949999988,0],[3.5,0.4,8,0.699999988,1]],[[0,0.7,0,1,0],[0.75,0.2,0,0.600000024,1],[1,0.9,1,0.850000024,0],[2,0.45,0,0.899999976,0],[2.5,0.45,9,0.800000012,0],[3,0.9,8,0.899999976,0]],[[0,0.45,0,1,0],[0.5,0.45,0,0.75,1],[1.5,0.45,8,0.850000024,0],[2,0.9,0,0.949999988,0],[3,0.45,1,0.800000012,1],[3.5,0.45,8,0.800000012,0]]],"fills":[[[0,1.4,0,1,0],[2,0.9,0,0.949999988,0],[3,0.45,8,0.850000024,0],[3.5,0.45,11,0.899999976,0]],[[0,1.4,0,1,0],[2,0.45,0,0.899999976,0],[2.5,0.45,8,0.850000024,0],[3,0.45,9,0.800000012,0],[3.5,0.45,11,0.899999976,0]]],"swing":0.150000006,"varScale":0.349999994,"newCellChance":0,"phraseBars":4,"lag":0},{"name":"DERRICK BARNETT","tagline":"DANCEHALL AO VIVO","main":[[[0,0.6,0,1,0],[0.75,0.5,0,0.850000024,0],[1.5,0.4,4,0.899999976,0],[2,0.6,0,1,0],[2.75,0.5,0,0.850000024,0],[3.5,0.4,2,0.850000024,0]],[[0,0.4,0,1,0],[0.5,0.2,4,0.850000024,0],[0.75,0.2,0,0.800000012,1],[1.5,0.4,2,0.899999976,0],[2,0.4,0,1,0],[2.5,0.2,4,0.850000024,0],[2.75,0.2,3,0.699999988,1],[3,0.4,2,0.899999976,0],[3.5,0.4,1,0.800000012,1]],[[0,0.3,0,1,0],[0.75,0.25,0,0.850000024,0],[1.5,0.25,1,0.800000012,1],[1.75,0.7,2,0.899999976,0],[3,0.25,9,0.800000012,0],[3.25,0.25,0,0.75,1],[3.5,0.4,4,0.899999976,0]],[[0,0.4,0,1,0],[0.5,0.2,0,0.649999976,1],[1,0.4,2,0.850000024,0],[1.5,0.4,4,0.949999988,0],[2.25,0.4,0,0.850000024,0],[3,0.25,2,0.800000012,0],[3.25,0.25,3,0.75,1],[3.5,0.4,4,0.899999976,0]]],"fills":[[[0,0.6,0,1,0],[0.75,0.5,0,0.850000024,0],[1.5,0.4,4,0.899999976,0],[2,0.22,2,0.899999976,0],[2.25,0.22,3,0.800000012,0],[2.5,0.22,4,0.899999976,0],[2.75,0.22,3,0.800000012,0],[3,0.22,2,0.850000024,0],[3.25,0.22,1,0.800000012,0],[3.5,0.22,5,0.800000012,0],[3.75,0.22,11,0.949999988,0]],[[0,0.4,0,1,0],[0.5,0.2,4,0.850000024,0],[1,0.4,0,0.899999976,0],[1.5,0.2,4,0.850000024,0],[2,0.22,0,0.899999976,0],[2.5,0.22,9,0.800000012,0],[2.75,0.22,8,0.800000012,1],[3,0.22,0,0.899999976,0],[3.25,0.22,4,0.850000024,0],[3.5,0.22,8,0.800000012,1],[3.75,0.22,11,0.949999988,0]]],"swing":0,"varScale":0.800000012,"newCellChance":0.5,"phraseBars":2,"lag":0},{"name":"ROBBIE","tagline":"ROCKERS DA CHANNEL ONE","main":[[[0,0.45,0,1,0],[0.75,0.2,0,0.699999988,1],[1,0.45,2,0.899999976,0],[1.5,0.4,4,0.949999988,0],[2,0.45,0,1,0],[2.5,0.2,3,0.75,1],[2.75,0.4,4,0.899999976,0],[3.5,0.4,2,0.850000024,0]],[[0,0.7,0,1,0],[0.75,0.2,1,0.75,0],[1,0.45,2,0.899999976,0],[1.75,0.6,0,0.899999976,0],[2.5,0.45,9,0.850000024,0],[3,0.45,0,0.949999988,0],[3.5,0.4,1,0.800000012,1]],[[0,0.3,0,1,0],[0.5,0.2,0,0.649999976,1],[0.75,0.2,4,0.850000024,0],[1,0.2,3,0.75,0],[1.25,0.2,4,0.699999988,1],[1.5,0.45,2,0.899999976,0],[2.25,0.4,0,0.899999976,0],[3,0.22,1,0.800000012,0],[3.25,0.22,6,0.75,0],[3.5,0.4,2,0.899999976,0]]],"fills":[[[0,0.45,0,1,0],[0.75,0.2,0,0.699999988,1],[1,0.45,2,0.899999976,0],[1.5,0.4,4,0.949999988,0],[2,0.45,0,1,0],[3,0.22,2,0.850000024,0],[3.25,0.22,7,0.800000012,0],[3.5,0.22,3,0.850000024,0],[3.75,0.22,11,0.949999988,0]],[[0,0.7,0,1,0],[0.75,0.2,1,0.75,0],[1,0.45,2,0.899999976,0],[2,0.22,4,0.899999976,0],[2.25,0.22,3,0.800000012,0],[2.5,0.22,2,0.850000024,0],[2.75,0.22,1,0.800000012,0],[3,0.45,0,0.899999976,0],[3.5,0.45,11,0.949999988,0]]],"swing":0,"varScale":0.699999988,"newCellChance":0.400000006,"phraseBars":2,"lag":0},{"name":"LEROY SIBBLES","tagline":"STUDIO ONE CANTADO","main":[[[0,0.9,0,1,0],[1,0.45,1,0.800000012,0],[1.5,0.9,2,0.899999976,0],[2.5,0.45,7,0.800000012,1],[3,0.9,2,0.850000024,0]],[[0,1.4,0,1,0],[1.5,0.45,2,0.850000024,0],[2,0.9,4,0.899999976,0],[3,0.45,3,0.800000012,1],[3.5,0.45,2,0.800000012,0]],[[0,0.45,4,1,0],[0.5,0.45,3,0.850000024,0],[1,0.9,2,0.899999976,0],[2,0.45,1,0.850000024,0],[2.5,0.45,5,0.75,1],[3,0.9,0,0.899999976,0]],[[0,0.7,0,1,0],[1,0.45,0,0.699999988,1],[1.5,0.45,1,0.800000012,0],[2,1.4,2,0.899999976,0],[3.5,0.45,7,0.75,1]]],"fills":[[[0,0.9,0,1,0],[1,0.45,2,0.850000024,0],[1.5,0.45,1,0.800000012,0],[2,0.45,5,0.75,0],[2.5,0.45,0,0.850000024,0],[3,0.45,9,0.800000012,0],[3.5,0.45,11,0.899999976,0]],[[0,1.4,0,1,0],[2,0.45,1,0.850000024,0],[2.5,0.45,2,0.850000024,0],[3,0.45,7,0.800000012,0],[3.5,0.45,11,0.899999976,0]]],"swing":0.200000003,"varScale":0.5,"newCellChance":0.300000012,"phraseBars":2,"lag":0.035},{"name":"FULLY FULLWOOD","tagline":"RIFF DE RIDDIM 70s","main":[[[0,0.4,0,1,0],[0.5,0.2,0,0.649999976,1],[0.75,0.2,1,0.800000012,0],[1,0.9,2,0.899999976,0],[2,0.45,4,0.949999988,0],[2.5,0.45,3,0.850000024,0],[3,0.45,2,0.850000024,0],[3.5,0.45,1,0.800000012,0]],[[0.5,0.45,0,1,0],[1,0.2,0,0.699999988,0],[1.25,0.2,1,0.75,0],[1.5,0.45,2,0.899999976,0],[2,0.9,4,0.949999988,0],[3,0.22,3,0.800000012,0],[3.25,0.22,2,0.800000012,0],[3.5,0.45,1,0.850000024,0]],[[0,0.9,0,1,0],[1,0.45,9,0.800000012,0],[1.5,0.45,0,0.850000024,0],[2,0.45,1,0.850000024,0],[2.5,0.45,2,0.850000024,0],[3,0.45,2,0.699999988,1],[3.5,0.45,4,0.899999976,0]]],"fills":[[[0,0.4,0,1,0],[0.75,0.2,1,0.800000012,0],[1,0.9,2,0.899999976,0],[2,0.45,4,0.949999988,0],[3,0.22,2,0.850000024,0],[3.25,0.22,1,0.800000012,0],[3.5,0.22,5,0.800000012,0],[3.75,0.22,11,0.949999988,0]]],"swing":0.25,"varScale":0.600000024,"newCellChance":0.200000003,"phraseBars":2,"lag":0},{"name":"JAMMYS","tagline":"SLENG TENG DIGITAL 85","main":[[[0,0.2,0,1,0],[0.5,0.2,0,0.800000012,0],[0.75,0.2,1,0.850000024,0],[1.25,0.2,0,0.800000012,0],[1.5,0.2,6,0.850000024,0],[2,0.2,2,0.949999988,0],[2.5,0.2,0,0.800000012,0],[2.75,0.2,1,0.850000024,0],[3.25,0.2,6,0.800000012,1],[3.5,0.2,1,0.850000024,0]],[[0,0.25,0,1,0],[0.75,0.25,0,0.850000024,0],[1.5,0.25,1,0.850000024,0],[2,0.25,6,0.899999976,0],[2.25,0.2,2,0.75,1],[2.5,0.25,6,0.850000024,0],[3,0.25,1,0.850000024,0],[3.5,0.25,0,0.899999976,0]],[[0,0.2,0,1,0],[0.25,0.2,0,0.649999976,1],[0.5,0.2,4,0.899999976,0],[1,0.2,0,0.850000024,0],[1.5,0.2,1,0.850000024,0],[1.75,0.2,6,0.800000012,0],[2,0.45,2,0.949999988,0],[3,0.2,6,0.800000012,0],[3.5,0.2,1,0.850000024,0]]],"fills":[[[0,0.2,0,1,0],[0.5,0.2,0,0.800000012,0],[0.75,0.2,1,0.850000024,0],[1.25,0.2,0,0.800000012,0],[1.5,0.2,6,0.850000024,0],[2,0.2,2,0.949999988,0],[2.5,0.2,4,0.899999976,0],[3,0.2,2,0.850000024,0],[3.25,0.2,6,0.800000012,0],[3.5,0.2,1,0.850000024,0],[3.75,0.2,11,0.949999988,0]]],"swing":0,"varScale":0.300000012,"newCellChance":0,"phraseBars":4,"lag":0}];

  const Q = { Maj: 0, Min: 1, Dom7: 2, Min7: 3 };
  const QUALITY_NAMES = ['', 'm', '7', 'm7'];
  const QUALITY_LONG = ['MAIOR', 'MENOR', 'SETIMA', 'MENOR 7'];
  const ROOT_NAMES = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B'];
  const DURATIONS = [2, 4, 8, 12, 16, 32];
  const DURATION_NAMES = ['1/2', '1', '2', '3', '4', '8'];
  const Tok = { R: 0, T3: 1, T5: 2, T7: 3, OCT: 4, S2: 5, S4: 6, S6: 7, LOW5: 8, LOW7: 9, LOW3: 10, APP: 11 };

  const pc = n => ((n % 12) + 12) % 12;
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));

  function tokSemis(tok, q) {
    const minor = q === Q.Min || q === Q.Min7;
    const third = minor ? 3 : 4, seventh = q === Q.Maj ? 11 : 10;
    switch (tok) {
      case Tok.R: return 0; case Tok.T3: return third; case Tok.T5: return 7; case Tok.T7: return seventh;
      case Tok.OCT: return 12; case Tok.S2: return 2; case Tok.S4: return 5; case Tok.S6: return q === Q.Min ? 8 : 9;
      case Tok.LOW5: return -5; case Tok.LOW7: return seventh - 12; case Tok.LOW3: return third - 12;
      default: return 0;
    }
  }

  function fitRange(n) { while (n < 28) n += 12; while (n > 67) n -= 12; return n; }

  // splitmix64, igual ao mbm::Rng
  const M64 = (1n << 64n) - 1n;
  class Rng {
    constructor(seed) { this.s = (BigInt(seed) * 0x9E3779B97F4A7C15n + 0x632BE59BD9B4E019n) & M64; }
    next() {
      this.s = (this.s + 0x9E3779B97F4A7C15n) & M64;
      let z = this.s;
      z = ((z ^ (z >> 30n)) * 0xBF58476D1CE4E5B9n) & M64;
      z = ((z ^ (z >> 27n)) * 0x94D049BB133111EBn) & M64;
      return z ^ (z >> 31n);
    }
    uniform() { return f(Number(this.next() >> 40n) * (1.0 / 16777216.0)); }
    pick(n) { return n <= 1 ? 0 : Number(this.next() % BigInt(n)); }
  }

  function approachNote(nextRoot, base, rng) {
    const target = base + pc(nextRoot - base);
    const r = rng.uniform();
    let n;
    if (r < 0.5) n = target - 1;
    else if (r < f(0.8)) n = target + 1;
    else n = target - 2;
    return fitRange(n);
  }

  function monophonic(ev, loopBeats) {
    ev.sort((a, b) => a.start - b.start);
    for (let i = 0; i < ev.length; i++) {
      const nextStart = i + 1 < ev.length ? ev[i + 1].start : ev[0].start + loopBeats;
      const maxLen = nextStart - ev[i].start - 0.02;
      ev[i].length = Math.max(0.04, Math.min(ev[i].length, maxLen));
    }
  }

  // slots: [{root, quality, beats}] (beats 0 = desligado); s: {style, seed, density, variation, base, velocity, gate, swing}
  function buildLoop(slotsIn, s) {
    const ev = [];
    const slots = [];
    slotsIn.forEach((c, i) => { if (c.beats > 0) slots.push([i, c]); });
    if (!slots.length) return { events: ev, loopBeats: 0 };

    const bk = BOOK[clamp(s.style, 0, BOOK.length - 1)];
    const rng = new Rng(s.seed * 7919 + s.style * 104729 + 17);
    const density = f(s.density), velocity = f(s.velocity);
    const vr = f(f(clamp(f(s.variation), 0, 1)) * f(bk.varScale));
    const swing = f(clamp(f(f(s.swing) + f(bk.swing)), 0, 1));
    const sustain = f(f(0.35) + f(f(0.85) * f(clamp(f(s.gate), 0, 1))));

    const riff = rng.pick(bk.main.length);
    let pos = 0, prevNote = s.base + 7, barCounter = 0;

    for (let k = 0; k < slots.length; k++) {
      const [slotIdx, ch] = slots[k];
      const next = slots[(k + 1) % slots.length][1];
      const changes = pc(next.root) !== pc(ch.root) || next.quality !== ch.quality;
      const rootNote = s.base + pc(ch.root - s.base);

      let cellIdx = riff;
      if (rng.uniform() < f(f(bk.newCellChance) * vr)) cellIdx = rng.pick(bk.main.length);

      const bars = Math.ceil(ch.beats / 4.0 - 1e-9);
      for (let b = 0; b < bars; b++, barCounter++) {
        const barStart = pos + b * 4.0;
        const barLen = Math.min(4.0, ch.beats - b * 4.0);
        const lastBar = b === bars - 1;
        const phraseEnd = ((barCounter + 1) % bk.phraseBars) === 0;

        let useFill = (lastBar && changes) || (phraseEnd && rng.uniform() < vr);
        if (lastBar && changes && rng.uniform() > f(f(0.35) + f(f(0.65) * vr))) useFill = false;
        const bar = useFill ? bk.fills[rng.pick(bk.fills.length)] : bk.main[cellIdx];

        const firstOfBar = ev.length;
        for (const c of bar) {
          const [cStart, cLen, tok, cVel, opt] = c;
          if (cStart >= barLen - 1e-6) continue;
          if (opt && rng.uniform() > density) continue;

          let note;
          if (tok === Tok.APP) {
            if (!changes && !lastBar) note = fitRange(rootNote + tokSemis(Tok.T5, ch.quality));
            else if (!changes) note = fitRange(rootNote + tokSemis(Tok.LOW7, ch.quality));
            else note = approachNote(next.root, s.base, rng);
          } else note = fitRange(rootNote + tokSemis(tok, ch.quality));

          let st = cStart;
          const frac = st - Math.floor(st);
          if (Math.abs(frac - 0.5) < 1e-6) st += swing * (2.0 / 3.0 - 0.5);
          else if (Math.abs(frac - 0.25) < 1e-6 || Math.abs(frac - 0.75) < 1e-6) st += swing * (1.0 / 12.0);
          st += bk.lag;

          const human = f(f(0.92) + f(f(0.16) * rng.uniform()));
          const vel = clamp(Math.round(f(f(f(f(cVel) * velocity) * human) * 127)), 1, 127);
          ev.push({ start: barStart + st, length: cLen * sustain, note, velocity: vel, slot: slotIdx, fill: useFill });
          prevNote = note;
        }

        if (lastBar && changes && barLen < 4.0) {
          const appAt = barStart + barLen - 0.5;
          while (ev.length > firstOfBar && ev[ev.length - 1].start >= appAt - 1e-6) ev.pop();
          const n = approachNote(next.root, s.base, rng);
          const vel = clamp(Math.round(f(f(f(0.9) * velocity) * 127)), 1, 127);
          ev.push({ start: appAt, length: 0.45 * sustain, note: n, velocity: vel, slot: slotIdx, fill: true });
          prevNote = n;
        }
      }
      pos += ch.beats;
    }
    monophonic(ev, pos);
    return { events: ev, loopBeats: pos };
  }

  // ---------------------------------------------------------------- bateria de acompanhamento
  const DrumStyles = ['ROOTS', 'ONE DROP', 'DIGITAL', 'DANCEHALL'];
  const drum = { Kick: 36, Rim: 37, Snare: 38, Clap: 39, ClosedHat: 42, OpenHat: 46, TomLow: 43, TomMid: 47, TomHigh: 50, TimbaleHi: 65, TimbaleLo: 66 };
  const isDigitalKit = st => st === 2 || st === 3;

  function drumBar(style, fill) {
    const h = [];
    const add = (step, note, vel, offset = 0) => h.push({ step, note, vel: f(vel), offset });
    const hats8 = (on, off, until) => { for (let s = 0; s < until; s += 2) add(s, drum.ClosedHat, s % 4 === 0 ? on : off); };
    switch (style) {
      case 0:
        add(0, drum.Kick, .95); add(8, drum.Kick, .95); add(4, drum.Snare, .9); if (!fill) add(12, drum.Snare, .9);
        hats8(.7, .55, fill ? 12 : 16);
        add(3, drum.ClosedHat, .3); add(11, drum.ClosedHat, .3);
        if (!fill) add(14, drum.OpenHat, .6);
        else { add(12, drum.Snare, .9); add(13, drum.TomHigh, .7); add(14, drum.TomMid, .75); add(15, drum.TomLow, .85); }
        break;
      case 1:
        add(8, drum.Kick, 1); add(4, drum.Snare, .95); add(12, drum.Snare, .95);
        for (let s = 0; s < (fill ? 12 : 16); s += 2) add(s, drum.ClosedHat, s % 4 === 0 ? .65 : .5, s % 4 === 2 ? 0.06 : 0);
        if (fill) { add(12, drum.TomHigh, .75, 1 / 3); add(12, drum.TomMid, .8, 2 / 3); }
        break;
      case 2:
        add(0, drum.Kick, 1); add(8, drum.Kick, .95); add(4, drum.Snare, .95); add(12, drum.Snare, .95); add(4, drum.Clap, .8); add(12, drum.Clap, .8);
        hats8(.65, .55, 16); add(14, drum.Rim, .45);
        if (fill) { add(13, drum.Clap, .6); add(14, drum.Clap, .75); add(15, drum.Clap, .9); }
        else add(15, drum.ClosedHat, .4);
        break;
      default:
        for (const s of [0, 4, 8, 12]) add(s, drum.Kick, s === 0 ? 1 : .9);
        add(4, drum.Snare, .95); add(12, drum.Snare, .95);
        hats8(.55, .6, 16);
        if (fill) { add(12, drum.TimbaleHi, .8); add(13, drum.TimbaleHi, .7); add(14, drum.TimbaleLo, .8); add(15, drum.TimbaleLo, .9); }
        else { add(7, drum.TimbaleHi, .55); add(15, drum.TimbaleLo, .6); }
    }
    return h;
  }

  function buildDrums(loopBeats, style, swing) {
    const ev = [];
    if (loopBeats <= 0) return ev;
    const sw = f(swing);
    const target = isDigitalKit(style) ? 1 : 0;
    const bars = Math.ceil(loopBeats / 4.0 - 1e-9);
    const fillBar = Math.max(0, Math.floor((loopBeats - 1e-6) / 4.0) - ((loopBeats % 4.0) > 1e-6 ? 1 : 0));
    for (let b = 0; b < bars; b++)
      for (const hit of drumBar(style, b === fillBar && bars > 1)) {
        let st = b * 4.0 + hit.step * 0.25 + hit.offset;
        const sib = hit.step % 4;
        if (sib === 2 && hit.offset < 1e-9) st += sw * (2.0 / 3.0 - 0.5) * 0.6;
        else if (sib % 2 === 1) st += sw * (1.0 / 12.0) * 0.6;
        if (st >= loopBeats - 1e-6) continue;
        ev.push({ start: st, note: hit.note, velocity: clamp(Math.round(f(hit.vel * f(127))), 1, 127), target });
      }
    // stable_sort por início, como no C++
    return ev.map((e, i) => [e, i]).sort((a, b) => a[0].start - b[0].start || a[1] - b[1]).map(x => x[0]);
  }

  const chordLabel = c => ROOT_NAMES[pc(c.root)] + QUALITY_NAMES[c.quality];

  root.MRBM = { BOOK, Q, QUALITY_NAMES, QUALITY_LONG, ROOT_NAMES, DURATIONS, DURATION_NAMES, DrumStyles, drum,
                buildLoop, buildDrums, monophonic, chordLabel, isDigitalKit, styleNames: BOOK.map(b => b.name),
                taglines: BOOK.map(b => b.tagline) };
})(typeof window !== 'undefined' ? window : globalThis);
