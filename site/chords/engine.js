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
    { name: '1/2', sub: 'MÍNIMA', beats: 2.0 }, { name: '1/64', sub: 'SEMIFUSA', beats: 0.0625 }];   // 1/64 no fim: o índice 0 continua 1/32
  const NOTE_SIZE_64 = 5;

  // PIANISTAS (cópia do ReggaeChordMachine/Source/DubEngine.h: o mesmo MIDI no plugin e no navegador)
  // part: 0 = as duas mãos, 1 = só a mão direita, 2 = só a mão esquerda. Caixa no 2 e no 4, skank no "&".
  const PIANISTS = [
    { name: 'CLASSICO', who: 'SKANK NO CONTRATEMPO' },
    { name: 'BECKFORD', who: 'THEOPHILUS BECKFORD · SHUFFLE DO SKA' },
    { name: 'GLADDY', who: 'GLADSTONE ANDERSON · FRAGMENTOS GRAVES' },
    { name: 'STERLING', who: 'KEITH STERLING · SECO, COM ESPAÇO' },
    { name: 'DOWNIE', who: 'TYRONE DOWNIE · RESPOSTAS SINCOPADAS' },
    { name: 'LYN', who: 'ROBBIE LYN · LOVERS ROCK' },
    { name: 'STEELY', who: 'STEELY · DIGITAL, GRADE PRESA' },
    { name: 'LENKY', who: 'LENKY · STABS DOS ANOS 2000' }];
  // FILLS: seletor separado do pianista (mesma ordem do PianoFill do DubEngine.h)
  const FILLS = [
    { name: 'SEM FILL', who: 'SÓ O SKANK' },
    { name: 'BECKFORD', who: 'CORRIDA DO SKA · COLCHEIAS CRESCENDO' },
    { name: 'GLADDY', who: 'FRAGMENTOS GRAVES DESCENDO' },
    { name: 'STERLING', who: 'UM ACORDE SEGURADO E UMA NOTA NO ALTO' },
    { name: 'DOWNIE', who: 'RESPOSTAS SINCOPADAS' },
    { name: 'LYN', who: 'ARPEJO SUBINDO (LOVERS ROCK)' },
    { name: 'STEELY', who: 'SEMICOLCHEIAS DIGITAIS NO 4' },
    { name: 'LENKY', who: 'STABS NOS "E" E NOS "A"' }];
  const FILL_NONE = 0, FILL_BECKFORD = 1, FILL_GLADDY = 2, FILL_STERLING = 3, FILL_DOWNIE = 4, FILL_LYN = 5, FILL_STEELY = 6, FILL_LENKY = 7;
  const fillNoSwing = (f) => f === FILL_STEELY || f === FILL_LENKY;
  function pianistRecipe(p) {
    const H = (step, vel, part, lenMul) => ({ step, vel: f32(vel), part, lenMul: f32(lenMul) });
    const skank = (v, len) => [H(2, v, 0, len), H(6, v, 0, len), H(10, v, 0, len), H(14, v, 0, len)];
    const r = { barA: null, barB: null, noSwing: false, layBack: 0.0 };
    switch (clamp(p | 0, 0, PIANISTS.length - 1)) {
      case 1: r.barA = [H(0, 0.45, 2, 1), H(2, 1, 1, 1), H(4, 0.5, 2, 1), H(6, 1, 1, 1), H(8, 0.45, 2, 1), H(10, 1, 1, 1), H(12, 0.5, 2, 1), H(14, 1, 1, 1)];
        r.barB = r.barA; break;
      case 2: r.barA = skank(0.9, 0.8); r.barB = r.barA; break;
      case 3: r.barA = [H(2, 0.55, 1, 0.8), H(6, 1, 0, 0.8), H(10, 0.55, 1, 0.8), H(14, 1, 0, 0.8)];
        r.barB = [H(6, 1, 0, 0.8), H(10, 0.5, 1, 0.8), H(14, 1, 0, 0.8)]; break;
      case 4: r.barA = skank(0.95, 1);
        r.barB = [H(2, 0.95, 0, 1), H(6, 0.95, 0, 1), H(10, 0.95, 0, 1), H(11, 0.7, 1, 1), H(14, 0.95, 0, 1), H(15, 0.65, 1, 1)];
        r.layBack = 0.015; break;
      case 5: r.barA = skank(0.8, 2); r.barB = r.barA; break;
      case 6: r.barA = [H(2, 1, 0, 0.75), H(6, 1, 0, 0.75), H(10, 1, 0, 0.75), H(14, 1, 0, 0.75), H(15, 0.8, 1, 0.75)];
        r.barB = r.barA; r.noSwing = true; break;
      case 7: r.barA = [H(2, 0.9, 0, 0.75), H(6, 1, 0, 0.75), H(9, 0.85, 1, 0.75), H(14, 1, 0, 0.75)];
        r.barB = [H(2, 0.9, 0, 0.75), H(6, 1, 0, 0.75), H(11, 0.85, 1, 0.75), H(14, 1, 0, 0.75)]; r.noSwing = true; break;
      default: r.barA = skank(1, 1); r.barB = r.barA;
    }
    return r;
  }
  function pianoFill(fill, left, stepInBar, start, slot, velocity, events) {
    if (!left.length) return;
    const barEnd = start - (start - Math.floor(start / 4) * 4) + 4;   // nenhuma nota passa do compasso
    const add = (note, len, v) => events.push({ start, length: Math.max(0.03, Math.min(len, barEnd - start - 0.02)), note: clamp(note, 0, 127),
      velocity: clamp(lround(f32(f32(v * velocity) * 127)), 1, 127), channel: ChSkank, slot });
    const n = left.length;
    if (fill === FILL_BECKFORD) {
      if (stepInBar % 2 === 0) for (const x of left) add(x + 12, 0.2, f32(f32(0.7) + f32(f32(0.1) * ((stepInBar - 8) / 2))));
    } else if (fill === FILL_STERLING) {
      if (stepInBar === 10) for (const x of left) add(x + 12, 1.4, f32(0.85));
      if (stepInBar === 14) add(left[n - 1] + 24, 0.4, f32(0.6));
    } else if (fill === FILL_STEELY) {
      if (stepInBar >= 12) for (const x of left) add(x + 12, 0.1, f32(stepInBar % 2 === 0 ? 1.0 : 0.75));
    } else if (fill === FILL_LENKY) {
      if (stepInBar === 9 || stepInBar === 11 || stepInBar === 13) for (const x of left) add(x + 12, 0.15, f32(0.85));
      if (stepInBar === 15) add(left[n - 1] + 24, 0.2, f32(0.8));
    } else if (fill === FILL_GLADDY) {
      const steps = [8, 10, 12, 14];
      for (let k = 0; k < 4; k++)
        if (stepInBar === steps[k]) add(left[((n - 1 - k % n) + n) % n] - 12 - (k >= n ? 12 : 0), 0.45, f32(f32(0.78) - f32(f32(0.04) * k)));
    } else if (fill === FILL_DOWNIE) {
      if (stepInBar === 10 || stepInBar === 11 || stepInBar === 14)
        for (const x of left) add(x + 12, 0.2, f32(stepInBar === 11 ? 0.7 : 0.9));
    } else if (fill === FILL_LYN) {
      if (stepInBar >= 12) { const k = stepInBar - 12; add(k < n ? left[k] + 12 : left[0] + 24, 0.22, f32(0.68)); }
    }
  }
  const PATTERNS = ['CONTRATEMPO'];
  const INVERSIONS = ['FUND', '1ª INV', '2ª INV'];

  // só o contratempo (os outros padrões saíram na 1.7)
  function patternHits() {
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
    let totalBeats = 0;
    for (const sl of slots) if (sl.beats > 0) totalBeats += sl.beats;
    const loopBars = Math.max(1, lround(Math.ceil(totalBeats / 4)));
    const fillEvery = Math.min(4, loopBars);
    const pianist = clamp((s.pianist || 0) | 0, 0, PIANISTS.length - 1);
    const rec = pianistRecipe(pianist);
    const fill = clamp((s.fill || 0) | 0, 0, FILLS.length - 1);
    for (let si = 0; si < slots.length; si++) {
      const slot = slots[si];
      if (slot.beats <= 0) continue;
      const left = voicings.notes[si];
      const skank = left.concat(left.map((n) => n + 12));
      const organRight = left, organLeft = left.map((n) => n - 12);
      const firstStep = lround(pos * 4), numSteps = lround(slot.beats * 4);
      const rightHand = left.map((n) => n + 12), leftHand = left;
      for (let st = firstStep; st < firstStep + numSteps; st++) {
        const stepInBar = st % 16;
        const swingOffset = stepInBar % 2 === 1 ? swing * 0.125 : 0.0;
        const fillBar = fill !== FILL_NONE && stepInBar >= 8 && Math.floor(st / 16) % fillEvery === fillEvery - 1;
        if (fillBar) {
          const fo = (!fillNoSwing(fill) && stepInBar % 2 === 1) ? swing * 0.125 : 0.0;
          pianoFill(fill, left, stepInBar, st * 0.25 + fo, si, velocity, events);
        } else if (pianist !== 0) {
          const bar = Math.floor(st / 16);
          const so = (!rec.noSwing && stepInBar % 2 === 1) ? swing * 0.125 : 0.0;
          {
            const bh = bar % 2 === 0 ? rec.barA : rec.barB;
            for (const h of bh) {
              if (h.step !== stepInBar) continue;
              let gap = 16;
              for (const o of bh) { let d = o.step - h.step; if (d <= 0) d += 16; gap = Math.min(gap, d); }
              const maxLen = gap * 0.25 - so - 0.02;
              const len = Math.max(0.03, Math.min(noteBeats * h.lenMul, maxLen));
              const vel = clamp(lround(f32(f32(h.vel * velocity) * 127)), 1, 127);
              const notes = h.part === 1 ? rightHand : (h.part === 2 ? leftHand : skank);
              for (const note of notes) events.push({ start: st * 0.25 + so + rec.layBack, length: len, note: clamp(note, 0, 127), velocity: vel, channel: ChSkank, slot: si });
            }
          }
        }
        for (let h = 0; !fillBar && pianist === 0 && h < hits.length; h++) {
          if (hits[h].step !== stepInBar) continue;
          const start = st * 0.25 + swingOffset;
          const len = hitLength(hits, h, noteBeats, swingOffset);
          const vel = clamp(lround(f32(f32(f32(f32(hits[h].vel) * velocity) * 1.0) * 127)), 1, 127);
          for (const note of skank) events.push({ start, length: len, note: clamp(note, 0, 127), velocity: vel, channel: ChSkank, slot: si });
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

  root.MRCM = { Maj, Min, ChSkank, ChBubble, ROOTS, DURATIONS, NOTE_SIZES, NOTE_SIZE_64, PATTERNS, INVERSIONS, PIANISTS, FILLS,
    rootName, qualityName, chordLabel, voiceChord, inversionOf, voiceProgression, buildLoop, bubbleHits, patternHits };
  if (typeof module !== 'undefined') module.exports = root.MRCM;
})(typeof window !== 'undefined' ? window : globalThis);
