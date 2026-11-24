// Mastor Reggae Chord Machine MRCM-1240 — motor de harmonia (porte fiel de DubChords/Source/DubEngine.h)
// Mesmas regras do plugin: voicings com condução de vozes, inversões (AUTO = notas mais próximas,
// inclusive do último acorde para o primeiro), skank com as duas mãos, bubble de órgão com as duas
// mãos e os padrões de ritmo. O teste DubChords/tests/web_engine_test.js compara com o C++ nota a nota.
(function (root) {
  'use strict';

  const f32 = Math.fround;
  const Maj = 0, Min = 1;
  const ChSkank = 1, ChBubble = 3;

  const ROOTS = ['C', 'C#', 'D', 'Eb', 'E', 'F', 'F#', 'G', 'Ab', 'A', 'Bb', 'B'];
  const mod12 = (x) => ((x % 12) + 12) % 12;
  const clamp = (x, a, b) => Math.min(b, Math.max(a, x));
  // std::lround: arredonda meio para longe do zero
  const lround = (x) => (x < 0 ? -Math.round(-x) : Math.round(x));

  const intervals = (q) => (q === Min ? [0, 3, 7] : [0, 4, 7]);
  const qualityName = (q) => (q === Min ? 'm' : '');
  const rootName = (r) => ROOTS[mod12(r)];
  const chordLabel = (c) => rootName(c.root) + qualityName(c.quality);

  const DURATIONS = [ // duração de cada acorde
    { name: '1/2', beats: 2 }, { name: '1', beats: 4 }, { name: '2', beats: 8 },
    { name: '3', beats: 12 }, { name: '4', beats: 16 }, { name: '8', beats: 32 }];
  const NOTE_SIZES = [ // tamanho da nota do skank
    { name: '1/32', sub: 'FUSA', beats: 0.125 }, { name: '1/16', sub: 'SEMICOLCHEIA', beats: 0.25 },
    { name: '1/8', sub: 'COLCHEIA', beats: 0.5 }, { name: '1/4', sub: 'SEMÍNIMA', beats: 1.0 },
    { name: '1/2', sub: 'MÍNIMA', beats: 2.0 }];
  const PATTERNS = ['CONTRATEMPO', 'BANG-BANG'];
  const INVERSIONS = ['FUND', '1ª INV', '2ª INV'];

  // contratempo (skank nos "e") ou bang-bang: o acorde no "e" + só a mão direita 1/16 depois, mais fraca
  // (igual ao DubEngine.h do plugin; o mesmo double chop do Mastor Reggae Guitar MRG-68)
  function patternHits(p) {
    if (p === 1) return [{ step: 2, vel: 1.0 }, { step: 3, vel: 0.62, upper: true }, { step: 6, vel: 1.0 }, { step: 7, vel: 0.62, upper: true },
      { step: 10, vel: 1.0 }, { step: 11, vel: 0.62, upper: true }, { step: 14, vel: 1.0 }, { step: 15, vel: 0.62, upper: true }];
    return [{ step: 2, vel: 1.0 }, { step: 6, vel: 1.0 }, { step: 10, vel: 1.0 }, { step: 14, vel: 1.0 }];
  }


  // bubble de órgão com as duas mãos: "e" e "a" = mão esquerda, "&" = mão direita
  function bubbleHits() {
    const h = [];
    for (let beat = 0; beat < 4; beat++) {
      h.push({ step: beat * 4 + 1, vel: 0.72, right: false });
      h.push({ step: beat * 4 + 2, vel: 0.95, right: true });
      h.push({ step: beat * 4 + 3, vel: 0.62, right: false });
    }
    return h;
  }
  const BUBBLE_LEFT_LEN = 0.125, BUBBLE_RIGHT_LEN = 0.125;   // 1/32, como no plugin

  // voicing com condução de vozes (igual ao C++)
  function voiceChord(rootPc, quality, center, prev, forced = -1) {
    const iv = intervals(quality), n = iv.length;
    const pcs = iv.map((i) => mod12(rootPc + i));
    let bestScore = 1e9, best = [];
    for (let inv = 0; inv < n; inv++) {
      if (forced >= 0 && inv !== forced % n) continue;
      for (let bottom = center - 12; bottom <= center + 6; bottom++) {
        if (mod12(bottom) !== pcs[inv]) continue;
        const cand = [bottom];
        for (let k = 1; k < n; k++) {
          const pc = pcs[(inv + k) % n];
          let note = cand[cand.length - 1] + 1;
          while (mod12(note) !== pc) note++;
          cand.push(note);
        }
        let mean = 0; for (const x of cand) mean += x; mean /= n;
        let score = 0.35 * Math.abs(mean - center);
        if (prev.length) {
          const m = Math.min(prev.length, cand.length);
          for (let k = 0; k < m; k++) score += Math.abs(cand[k] - prev[k]);
          let pm = 0; for (const x of prev) pm += x; pm /= prev.length;
          score += 0.5 * Math.abs(mean - pm);
        }
        if (cand[cand.length - 1] - cand[0] > 14) score += 4;
        if (score < bestScore) { bestScore = score; best = cand; }
      }
    }
    return best;
  }

  function inversionOf(voicing, rootPc, quality) {
    if (!voicing.length) return 0;
    const iv = intervals(quality), bassPc = mod12(voicing[0]);
    for (let k = 0; k < iv.length; k++) if (mod12(rootPc + iv[k]) === bassPc) return k;
    return 0;
  }

  const sameNotes = (a, b) => a.length === b.length && a.every((x, i) => x.length === b[i].length && x.every((y, j) => y === b[i][j]));

  // voicings da progressão: suggested = AUTO (notas mais próximas), used = o que toca
  function voiceProgression(slots, center) {
    const v = { notes: slots.map(() => []), suggested: slots.map(() => 0), used: slots.map(() => 0) };
    const pass = (prev) => {
      for (let i = 0; i < slots.length; i++) {
        const s = slots[i];
        if (s.beats <= 0) continue;
        const best = voiceChord(s.root, s.quality, center, prev);
        v.suggested[i] = inversionOf(best, s.root, s.quality);
        v.notes[i] = s.inversion < 0 ? best : voiceChord(s.root, s.quality, center, prev, s.inversion);
        v.used[i] = inversionOf(v.notes[i], s.root, s.quality);
        prev = v.notes[i];
      }
      return prev;
    };
    let last = pass([]);
    for (let k = 0; k < 6; k++) {
      const before = v.notes.map((x) => x.slice());
      last = pass(last);
      if (sameNotes(v.notes, before)) break;
    }
    return v;
  }

  function hitLength(hits, index, noteBeats, swingOffset) {
    const step = hits[index].step;
    let gap = 16;
    for (const h of hits) { let d = h.step - step; if (d <= 0) d += 16; gap = Math.min(gap, d); }
    const maxLen = gap * 0.25 - swingOffset - 0.02;
    return Math.max(0.03, Math.min(noteBeats, maxLen));
  }

  // settings: { pattern, centerNote, velocity, noteSize, swing, bubble }
  // slots: [{ root, quality, beats (0 = desligado), inversion (-1 = AUTO) }]
  function buildLoop(slots, s) {
    const events = [];
    const hits = patternHits(s.pattern), bub = bubbleHits();
    const noteBeats = NOTE_SIZES[clamp(s.noteSize, 0, NOTE_SIZES.length - 1)].beats;
    const velocity = f32(s.velocity), swing = f32(s.swing);
    const voicings = voiceProgression(slots, s.centerNote);
    let pos = 0;
    for (let si = 0; si < slots.length; si++) {
      const slot = slots[si];
      if (slot.beats <= 0) continue;
      const left = voicings.notes[si];
      const skank = left.concat(left.map((n) => n + 12)), skankUpper = left.map((n) => n + 12);
      const organRight = left, organLeft = left.map((n) => n - 12);
      const firstStep = lround(pos * 4), numSteps = lround(slot.beats * 4);
      for (let st = firstStep; st < firstStep + numSteps; st++) {
        const stepInBar = st % 16;
        const swingOffset = stepInBar % 2 === 1 ? swing * 0.125 : 0.0;
        for (let h = 0; h < hits.length; h++) {
          if (hits[h].step !== stepInBar) continue;
          const start = st * 0.25 + swingOffset;
          const len = hitLength(hits, h, noteBeats, swingOffset);
          const vel = clamp(lround(f32(f32(f32(f32(hits[h].vel) * velocity) * 1.0) * 127)), 1, 127);
          for (const note of (hits[h].upper ? skankUpper : skank)) events.push({ start, length: len, note: clamp(note, 0, 127), velocity: vel, channel: ChSkank, slot: si });
        }
        if (s.bubble) {
          for (let h = 0; h < bub.length; h++) {
            if (bub[h].step !== stepInBar) continue;
            const start = st * 0.25 + swingOffset;
            const len = hitLength(bub, h, bub[h].right ? BUBBLE_RIGHT_LEN : BUBBLE_LEFT_LEN, swingOffset);
            const vel = clamp(lround(f32(f32(f32(f32(bub[h].vel) * velocity) * f32(0.9)) * 127)), 1, 127);
            for (const note of (bub[h].right ? organRight : organLeft))
              events.push({ start, length: len, note: clamp(note, 0, 127), velocity: vel, channel: ChBubble, slot: si });
          }
        }
      }
      pos += slot.beats;
    }
    // std::stable_sort por início
    const idx = events.map((e, i) => [e, i]);
    idx.sort((a, b) => (a[0].start - b[0].start) || (a[1] - b[1]));
    return { events: idx.map((x) => x[0]), loopBeats: pos, voicings };
  }

  root.MRCM = { Maj, Min, ChSkank, ChBubble, ROOTS, DURATIONS, NOTE_SIZES, PATTERNS, INVERSIONS,
    rootName, qualityName, chordLabel, voiceChord, inversionOf, voiceProgression, buildLoop, bubbleHits, patternHits };
  if (typeof module !== 'undefined') module.exports = root.MRCM;
})(typeof window !== 'undefined' ? window : globalThis);
