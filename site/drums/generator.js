// Mastor Reggae Drum Machine MRDM-12 — gerador de bateria (porte fiel de DubDrums/Source/PatternGenerator.cpp)
// Mesmo gerador aleatório do plugin (mt19937 + distribuições da libstdc++): com a mesma semente e os mesmos
// controles, sai o mesmo padrão. O teste DubDrums/tests/web_generator_test.js compara com o C++ nota a nota.
(function (root) {
  'use strict';

  const gm = { Kick: 36, SideStick: 37, Snare: 38, ClosedHat: 42, PedalHat: 44, OpenHat: 46,
               TomLow: 43, TomMid: 47, TomHigh: 50, Crash: 49 };
  const Style = { Rockers: 0, OneDrop: 1, BoomBap: 2 };
  const Hat = { Eighths: 0, Sixteenths: 1, Offbeat: 2, Triplets: 3 };
  const Swing = { Straight: 0, Light: 1, Medium: 2, Heavy: 3, Shuffle: 4 };
  const Dyn = { Soft: 0, Medium: 1, Loud: 2, Accented: 3, Crescendo: 4, Breathing: 5 };
  const Drummer = { StyleScott: 0, SantaDavis: 1, CarltonBarrett: 2, SlyDunbar: 3, Digikal: 4 };
  const Backbeat = { Snare: 0, Rim: 1, Both: 2 };
  const kStep = 0.25, kStepsPerBar = 16;

  // ---------------------------------------------------------------- mt19937 + distribuições (libstdc++)
  class MT {
    constructor(seed) {
      this.mt = new Uint32Array(624); this.i = 624;
      this.mt[0] = seed >>> 0;
      for (let k = 1; k < 624; k++) {
        const p = this.mt[k - 1] ^ (this.mt[k - 1] >>> 30);
        this.mt[k] = (Math.imul(1812433253, p) + k) >>> 0;
      }
    }
    next() {
      const mt = this.mt;
      if (this.i >= 624) {
        for (let k = 0; k < 624; k++) {
          const y = (mt[k] & 0x80000000) | (mt[(k + 1) % 624] & 0x7fffffff);
          mt[k] = mt[(k + 397) % 624] ^ (y >>> 1) ^ ((y & 1) ? 0x9908b0df : 0);
        }
        this.i = 0;
      }
      let y = mt[this.i++];
      y ^= y >>> 11; y ^= (y << 7) & 0x9d2c5680; y ^= (y << 15) & 0xefc60000; y ^= y >>> 18;
      return y >>> 0;
    }
  }
  class Rng {
    constructor(seed) { this.eng = new MT(seed >>> 0); }
    uni() {   // generate_canonical<double, 53> com 2 chamadas de 32 bits
      const a = this.eng.next(), b = this.eng.next();
      let r = (a + b * 4294967296) / 18446744073709551616;
      if (r >= 1) r = 1 - Number.EPSILON / 2;
      return r;
    }
    range(a, b) { return a + (b - a) * this.uni(); }
    chance(p) { return this.uni() < p; }
    pick(n) {   // uniform_int_distribution(0, n-1): Lemire com produto de 64 bits
      const range = BigInt(n);
      let product = BigInt(this.eng.next()) * range;
      let low = product & 0xffffffffn;
      if (low < range) {
        const threshold = ((0x100000000n - range) % 0x100000000n) % range;
        while (low < threshold) { product = BigInt(this.eng.next()) * range; low = product & 0xffffffffn; }
      }
      return Number(product >> 32n);
    }
  }

  // ---------------------------------------------------------------- utilidades
  const nearly = (a, b) => Math.abs(a - b) < 1.0e-6;
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
  const trunc = Math.trunc;
  const barStart = (bar) => bar * 4.0;
  const stepPos = (bar, step) => barStart(bar) + step * kStep;
  const maskHas = (mask, e8) => ((mask >> e8) & 1) === 1;
  const isHat = (n) => n === gm.ClosedHat || n === gm.OpenHat || n === gm.PedalHat;
  const isMT40 = (s) => (s.seed % 2) === 1;

  function swingRatio(sw) {
    switch (sw) { case Swing.Light: return 0.54; case Swing.Medium: return 0.58; case Swing.Heavy: return 0.62; default: return 0.5; }
  }
  function applySwing(beat, swing, delta) {
    if (swing === Swing.Straight && delta <= 0.0) return beat;
    const whole = Math.floor(beat), frac = beat - whole;
    if (swing === Swing.Shuffle) {
      if (nearly(frac, 0.25)) return whole + 1.0 / 3.0;
      if (nearly(frac, 0.50)) return whole + 2.0 / 3.0;
      if (nearly(frac, 0.75)) return whole + 5.0 / 6.0;
      return beat;
    }
    const r = clamp(swingRatio(swing) + delta, 0.5, 0.67);
    if (nearly(frac, 0.25)) return whole + 0.5 * r;
    if (nearly(frac, 0.75)) return whole + 0.5 + 0.5 * r;
    return beat;
  }

  // swingDelta, late[4], hatOrnaments, hatOrnamentBase, openHat, ghosts
  function feelOf(d) {
    switch (d) {
      case Drummer.StyleScott:     return { swingDelta: -0.02, late: [0.000, -0.004, -0.004, 0.000], hatOrnaments: 1.0, hatOrnamentBase: 0.00, openHat: 0.10, ghosts: 0.7 };
      case Drummer.SantaDavis:     return { swingDelta:  0.02, late: [0.000,  0.006,  0.000, 0.004], hatOrnaments: 1.7, hatOrnamentBase: 0.06, openHat: 0.05, ghosts: 1.4 };
      case Drummer.CarltonBarrett: return { swingDelta:  0.04, late: [0.006,  0.018,  0.006, 0.010], hatOrnaments: 0.6, hatOrnamentBase: 0.00, openHat: 0.00, ghosts: 1.2 };
      case Drummer.SlyDunbar:      return { swingDelta:  0.00, late: [-0.003, -0.005, -0.002, -0.003], hatOrnaments: 1.3, hatOrnamentBase: 0.05, openHat: 0.08, ghosts: 0.9 };
      case Drummer.Digikal:        return { swingDelta:  0.00, late: [0, 0, 0, 0], hatOrnaments: 0.0, hatOrnamentBase: 0.00, openHat: 0.00, ghosts: 0.0 };
    }
    return { swingDelta: 0, late: [0, 0, 0, 0], hatOrnaments: 1, hatOrnamentBase: 0, openHat: 0, ghosts: 1 };
  }

  class Builder {
    constructor(s) { this.s = s; this.ev = []; this.fills = []; }
    add(beat, note, vel, len = 0.1) {
      this.ev.push({ beat, length: len, note, velocity: clamp(vel, 1, 127), bar: Math.floor(beat / 4.0) });
    }
    has(beat, note) { return this.ev.some((e) => e.note === note && nearly(e.beat, beat)); }
    removeRange(from, to, notes) {
      this.ev = this.ev.filter((e) => !(e.beat >= from - 1.0e-6 && e.beat < to - 1.0e-6 && notes.includes(e.note)));
    }
  }

  // ---------------------------------------------------------------- BUMBO
  function buildKick(b, rng) {
    const s = b.s, r = s.randKick;
    for (let bar = 0; bar < s.bars; ++bar) {
      for (let e8 = 0; e8 < 8; ++e8)
        if (maskHas(s.kickMask, e8)) {
          const onBeat = e8 % 2 === 0;
          b.add(stepPos(bar, e8 * 2), gm.Kick, onBeat ? (e8 === 0 ? 114 : 110) : 100);
        }
      const busy = s.drummer === Drummer.CarltonBarrett ? 0.5 : s.drummer === Drummer.StyleScott ? 0.7
                 : s.drummer === Drummer.Digikal ? 0.0 : 1.0;
      const pickup = (st, p, vel) => {
        if (st % 2 === 0 && maskHas(s.kickMask, st / 2)) return;
        if (rng.chance(p * busy)) b.add(stepPos(bar, st), gm.Kick, vel);
      };
      switch (s.style) {
        case Style.Rockers: for (const st of [3, 7, 11, 15]) pickup(st, r * 0.22, 76); break;
        case Style.OneDrop: pickup(15, r * 0.18, 78); pickup(7, r * 0.10, 70); break;
        case Style.BoomBap: for (const st of [7, 10, 15]) pickup(st, r * 0.30, 88); break;
      }
      if (s.style === Style.BoomBap) continue;
      const extra = (st, p, vel) => {
        if (st % 2 === 0 && maskHas(s.kickMask, st / 2)) return;
        if (!b.has(stepPos(bar, st), gm.Kick) && rng.chance(p)) b.add(stepPos(bar, st), gm.Kick, vel);
      };
      switch (s.drummer) {
        case Drummer.SlyDunbar: extra(7, 0.30 + r * 0.40, 86); extra(15, 0.10 + r * 0.25, 78); break;
        case Drummer.SantaDavis: extra(11, 0.15 + r * 0.30, 80); break;
        case Drummer.StyleScott: extra(14, 0.08 + r * 0.20, 82); break;
        case Drummer.CarltonBarrett: break;
        case Drummer.Digikal: if (isMT40(s)) extra(6, 1.0, 104); else extra(7, 0.5 + r * 0.4, 100); break;
      }
    }
  }

  // ---------------------------------------------------------------- CAIXA / ARO
  function addBackbeat(b, pos, bb, vel) {
    switch (bb) {
      case Backbeat.Snare: b.add(pos, gm.Snare, vel); break;
      case Backbeat.Rim: b.add(pos, gm.SideStick, vel - 4); break;
      case Backbeat.Both: b.add(pos, gm.SideStick, vel - 10); b.add(pos, gm.Snare, vel - 2); break;
    }
  }

  function buildSnare(b, rng) {
    const s = b.s, r = s.randSnare;
    const backbeats = [];
    for (let e8 = 0; e8 < 8; ++e8) if (maskHas(s.snareMask, e8)) backbeats.push(e8 * 2);
    for (let bar = 0; bar < s.bars; ++bar) {
      const lastBar = bar === s.bars - 1;
      for (const st of backbeats) {
        if (lastBar && s.bars > 1 && backbeats.length && st === backbeats[backbeats.length - 1]
            && s.drummer !== Drummer.Digikal && rng.chance(r * 0.5)) {
          b.add(stepPos(bar, st) - 0.03, gm.Snare, 60);
          b.add(stepPos(bar, st), gm.Snare, 114);
        } else addBackbeat(b, stepPos(bar, st), s.backbeat, 110);
      }
      const ghost = feelOf(s.drummer).ghosts;
      for (const st of [3, 6, 7, 11, 14, 15]) {
        let near = false;
        for (const bb of backbeats) near = near || Math.abs(st - bb) <= 1;
        if (!near && rng.chance(r * 0.30 * ghost)) b.add(stepPos(bar, st), gm.Snare, trunc(rng.range(26, 44)));
      }
      if (s.style === Style.BoomBap)
        for (const st of [7, 9, 15])
          if (rng.chance(0.15 + r * 0.35)) b.add(stepPos(bar, st), gm.Snare, trunc(rng.range(34, 52)));
      if (s.style === Style.BoomBap || (s.machine && s.drummer !== Drummer.Digikal)) continue;
      switch (s.drummer) {
        case Drummer.CarltonBarrett:
          for (const st of backbeats)
            if (st > 0 && rng.chance(0.22 + r * 0.40)) b.add(stepPos(bar, st) - 1.0 / 3.0, gm.Snare, trunc(rng.range(30, 44)));
          break;
        case Drummer.SantaDavis:
          if (backbeats.length && rng.chance(0.20 + r * 0.40)) b.add(stepPos(bar, backbeats[backbeats.length - 1]) - 0.03, gm.Snare, 54);
          break;
        case Drummer.StyleScott:
          if (!lastBar && rng.chance(0.10 + r * 0.25)) b.add(stepPos(bar, 15), gm.Snare, 84);
          break;
        case Drummer.Digikal:
          if (!isMT40(s)) for (const st of backbeats) if (!b.has(stepPos(bar, st), gm.SideStick)) b.add(stepPos(bar, st), gm.SideStick, 96);
          break;
        case Drummer.SlyDunbar:
          if (backbeats.length && rng.chance(0.15 + r * 0.35)) {
            const at = stepPos(bar, backbeats[0]);
            b.add(at - 0.0625, gm.Snare, 40);
            b.add(at - 0.03125, gm.Snare, 46);
          }
          if (rng.chance(0.12 + r * 0.30)) b.add(stepPos(bar, 14), gm.SideStick, 78);
          break;
      }
    }
  }

  // ---------------------------------------------------------------- HI-HAT
  function hatSignature(b, bar, rng, r) {
    const s = b.s, t0 = barStart(bar);
    const removeHats = (from, to) => b.removeRange(t0 + from, t0 + to, [gm.ClosedHat, gm.OpenHat, gm.PedalHat]);
    const hatAt = (beat) => b.ev.some((e) => isHat(e.note) && nearly(e.beat, t0 + beat));
    const inBar = (e) => e.beat >= t0 && e.beat < t0 + 4.0;

    if (s.hatPattern === Hat.Triplets) {
      if (s.drummer === Drummer.CarltonBarrett)
        for (const e of b.ev)
          if (isHat(e.note) && inBar(e) && nearly(e.beat - Math.floor(e.beat), 2.0 / 3.0)) e.velocity = Math.min(127, e.velocity + 18);
      return;
    }
    switch (s.drummer) {
      case Drummer.CarltonBarrett: {
        for (const beat of [1, 2]) {
          const p = beat === 1 ? 0.40 + r * 0.40 : 0.20 + r * 0.30;
          if (!rng.chance(p)) continue;
          removeHats(beat, beat + 1.0);
          const full = rng.chance(0.5);
          b.add(t0 + beat, gm.ClosedHat, 72);
          if (full) b.add(t0 + beat + 1.0 / 3.0, gm.ClosedHat, 54);
          b.add(t0 + beat + 2.0 / 3.0, gm.ClosedHat, 88);
        }
        if (s.style === Style.OneDrop) removeHats(0.0, 0.01);
        if (rng.chance(0.80 + r * 0.15)) { removeHats(3.5, 4.0); b.add(t0 + 3.5, gm.OpenHat, 92, 0.45); }
        break;
      }
      case Drummer.SantaDavis: {
        for (let st = 1; st < kStepsPerBar; st += 2)
          if (!hatAt(st * kStep) && rng.chance(0.45 + r * 0.30)) b.add(t0 + st * kStep, gm.ClosedHat, trunc(rng.range(32, 44)));
        for (const beat of [0.0, 1.0, 2.0, 3.0]) {
          removeHats(beat + 0.5, beat + 0.75);
          b.add(t0 + beat + 0.5, gm.OpenHat, 100, 0.40);
          if (!hatAt(beat)) b.add(t0 + beat, gm.ClosedHat, 60);
        }
        break;
      }
      case Drummer.Digikal: {
        removeHats(0.0, 4.0);
        if (isMT40(s)) { for (let e8 = 0; e8 < 8; ++e8) b.add(t0 + e8 * 0.5, gm.ClosedHat, 90); }
        else {
          for (let st = 0; st < kStepsPerBar; ++st) {
            if (st === 14 && rng.chance(0.6 + r * 0.3)) { b.add(t0 + st * kStep, gm.OpenHat, 100, 0.22); ++st; continue; }
            b.add(t0 + st * kStep, gm.ClosedHat, st % 4 === 2 ? 104 : (st % 4 === 0 ? 80 : 60));
          }
        }
        break;
      }
      case Drummer.SlyDunbar: {
        for (const e of b.ev) {
          if (!isHat(e.note) || !inBar(e)) continue;
          const frac = e.beat - Math.floor(e.beat);
          if (nearly(frac, 0.5)) e.velocity = 100; else if (nearly(frac, 0.0)) e.velocity = 72; else e.velocity = 46;
        }
        for (let st = 3; st < kStepsPerBar; st += 4)
          if (!hatAt(st * kStep) && rng.chance(0.45 + r * 0.35)) b.add(t0 + st * kStep, gm.ClosedHat, trunc(rng.range(40, 52)));
        for (const beat of [0.5, 2.5])
          if (rng.chance(0.35 + r * 0.35)) { removeHats(beat, beat + 0.26); b.add(t0 + beat, gm.OpenHat, 96, 0.14); }
        for (const beat of [1.5, 3.5]) if (!hatAt(beat)) b.add(t0 + beat, gm.ClosedHat, 100);
        break;
      }
      case Drummer.StyleScott: {
        for (const e of b.ev) {
          if (!isHat(e.note) || !inBar(e)) continue;
          const frac = e.beat - Math.floor(e.beat);
          if (nearly(frac, 0.5)) e.velocity = 104; else if (nearly(frac, 0.0)) e.velocity = 56; else e.velocity = 38;
        }
        b.ev = b.ev.filter((e) => {
          if (!isHat(e.note) || !inBar(e)) return true;
          const frac = e.beat - Math.floor(e.beat);
          const sixteenth = nearly(frac, 0.25) || nearly(frac, 0.75);
          return !(sixteenth && s.hatPattern !== Hat.Sixteenths && rng.chance(0.7));
        });
        for (const beat of [0.5, 1.5, 2.5, 3.5]) if (!hatAt(beat)) b.add(t0 + beat, gm.ClosedHat, 104);
        if (rng.chance(0.30 + r * 0.30)) { removeHats(3.5, 3.76); b.add(t0 + 3.5, gm.OpenHat, 106, 0.14); }
        break;
      }
    }
  }

  function buildHats(b, rng) {
    const s = b.s, r = s.randHat, feel = feelOf(s.drummer);
    const orn = feel.hatOrnamentBase + r * feel.hatOrnaments;
    for (let bar = 0; bar < s.bars; ++bar) {
      if (s.hatPattern === Hat.Triplets) {
        for (let beat = 0; beat < 4; ++beat)
          for (let k = 0; k < 3; ++k) {
            if (k > 0 && rng.chance(r * 0.12)) continue;
            const pos = barStart(bar) + beat + k / 3.0;
            const vel = k === 0 ? 88 : (k === 2 ? 70 : 56);
            const open = k === 2 && rng.chance(r * 0.12);
            b.add(pos, open ? gm.OpenHat : gm.ClosedHat, vel, open ? 0.25 : 0.1);
          }
        hatSignature(b, bar, rng, r);
        continue;
      }
      let skipNext = false;
      for (let st = 0; st < kStepsPerBar; ++st) {
        if (skipNext) { skipNext = false; continue; }
        const onBeat = st % 4 === 0, offEight = st % 4 === 2, sixteen = st % 2 === 1;
        let play = false, vel = 0;
        switch (s.hatPattern) {
          case Hat.Eighths:
            play = !sixteen; vel = onBeat ? 88 : 72;
            if (sixteen && rng.chance(orn * 0.18)) { play = true; vel = 46; }
            break;
          case Hat.Sixteenths:
            play = true; vel = onBeat ? 90 : (offEight ? 74 : 56);
            break;
          case Hat.Offbeat:
            play = offEight; vel = 94;
            if (sixteen && rng.chance(orn * 0.15)) { play = true; vel = 48; }
            break;
        }
        if (!play) continue;
        if (!onBeat && rng.chance(r * 0.10)) continue;
        let open = false;
        if (offEight) {
          const baseOpen = (s.hatPattern === Hat.Offbeat && st === 14) ? 0.35 : 0.0;
          open = rng.chance(baseOpen + feel.openHat * 0.5 + r * 0.10);
        }
        if (open) { b.add(stepPos(bar, st), gm.OpenHat, vel + 4, 0.22); skipNext = true; }
        else b.add(stepPos(bar, st), gm.ClosedHat, vel);
      }
      hatSignature(b, bar, rng, r);
    }
  }

  // ---------------------------------------------------------------- VIRADAS
  const H = (beat, note, vel) => ({ beat, note, vel });
  const FILLS = {
    [Drummer.StyleScott]: [
      [H(3.0, gm.Snare, 110), H(3.25, gm.Snare, 72), H(3.5, gm.TomMid, 100), H(3.75, gm.TomLow, 110)],
      [H(2.75, gm.Snare, 106), H(3.25, gm.Snare, 110), H(3.5, gm.Snare, 76), H(3.75, gm.TomLow, 112)],
      [H(3.0, gm.TomHigh, 100), H(3.25, gm.TomHigh, 88), H(3.5, gm.TomMid, 102), H(3.75, gm.TomLow, 114)]],
    [Drummer.SantaDavis]: [
      [H(3.0, gm.Snare, 50), H(3.125, gm.Snare, 58), H(3.25, gm.Snare, 66), H(3.375, gm.Snare, 74),
       H(3.5, gm.Snare, 82), H(3.625, gm.Snare, 92), H(3.75, gm.Snare, 102), H(3.875, gm.Snare, 114)],
      [H(2.5, gm.Snare, 80), H(2.75, gm.Snare, 70), H(3.0, gm.TomHigh, 96), H(3.25, gm.TomHigh, 86),
       H(3.5, gm.TomMid, 100), H(3.75, gm.TomLow, 112)],
      [H(2.97, gm.Snare, 50), H(3.0, gm.Snare, 104), H(3.47, gm.Snare, 50), H(3.5, gm.Snare, 108), H(3.75, gm.TomLow, 110)]],
    [Drummer.CarltonBarrett]: [
      [H(2.97, gm.Snare, 48), H(3.0, gm.Snare, 72), H(3.333, gm.Snare, 86), H(3.667, gm.Snare, 102)],
      [H(3.0, gm.TomHigh, 92), H(3.333, gm.TomHigh, 84), H(3.667, gm.TomMid, 100)],
      [H(2.667, gm.Snare, 60), H(2.97, gm.Snare, 48), H(3.0, gm.Snare, 84), H(3.333, gm.TomMid, 92), H(3.667, gm.TomLow, 106)]],
    [Drummer.SlyDunbar]: [
      [H(3.0, gm.Snare, 104), H(3.125, gm.Snare, 58), H(3.25, gm.Snare, 106), H(3.5, gm.TomHigh, 112), H(3.75, gm.TomHigh, 116)],
      [H(2.5, gm.Snare, 70), H(2.75, gm.Snare, 78), H(3.0, gm.SideStick, 104), H(3.25, gm.Snare, 88),
       H(3.5, gm.Snare, 96), H(3.625, gm.Snare, 70), H(3.75, gm.TomLow, 114)],
      [H(3.0, gm.Snare, 100), H(3.125, gm.Snare, 76), H(3.5, gm.TomMid, 104), H(3.625, gm.TomMid, 80), H(3.75, gm.TomLow, 114)]],
    [Drummer.Digikal]: [
      [H(3.0, gm.Snare, 100), H(3.25, gm.Snare, 100), H(3.5, gm.Snare, 100), H(3.75, gm.Snare, 100)],
      [H(3.0, gm.Kick, 100), H(3.25, gm.Kick, 100), H(3.5, gm.Kick, 100), H(3.75, gm.Kick, 100)],
      [H(3.0, gm.TomHigh, 110), H(3.25, gm.TomHigh, 100), H(3.5, gm.TomMid, 110), H(3.75, gm.TomLow, 118)]],
  };

  function placeFill(b, bar, rng, r) {
    const all = FILLS[b.s.drummer] || FILLS[Drummer.CarltonBarrett];
    let f = all[rng.pick(all.length)].slice();
    if (b.s.drummer === Drummer.Digikal) f = (isMT40(b.s) ? all[rng.pick(2)] : all[2]).slice();
    if (b.s.drummer !== Drummer.Digikal && rng.chance(0.20 + r * 0.5)) f.unshift(H(2.5, gm.Snare, 66));
    let first = 4.0;
    for (const h of f) first = Math.min(first, h.beat);
    const from = barStart(bar) + Math.floor(first * 4.0 - 1.0e-6) / 4.0;
    b.removeRange(from, barStart(bar + 1), [gm.ClosedHat, gm.OpenHat, gm.Snare, gm.SideStick, gm.TomHigh, gm.TomMid, gm.TomLow]);
    b.fills.push([from, barStart(bar + 1)]);
    for (let i = 0; i < f.length; ++i) {
      const h = f[i];
      if (b.s.drummer === Drummer.Digikal) { b.add(barStart(bar) + h.beat, h.note, h.vel); continue; }
      if (i > 0 && i + 1 < f.length && rng.chance(r * 0.12)) continue;
      b.add(barStart(bar) + h.beat, h.note, h.vel + trunc(rng.range(-6, 6)));
    }
    return from;
  }

  function buildPerc(b, rng) {
    const s = b.s, r = s.randPerc;
    for (let bar = 0; bar < s.bars; ++bar) {
      const fillBar = s.fill && bar === s.bars - 1;
      if (!fillBar && s.drummer !== Drummer.Digikal && rng.chance(r * 0.30)) {
        b.add(stepPos(bar, 6), gm.TomHigh, trunc(rng.range(70, 90)));
        if (rng.chance(0.5)) b.add(stepPos(bar, 7), gm.TomMid, trunc(rng.range(70, 90)));
      }
    }
    if (s.drummer === Drummer.SlyDunbar && !s.machine)
      for (let bar = 0; bar < s.bars; ++bar)
        if (!(s.fill && bar === s.bars - 1) && rng.chance(0.10 + r * 0.35)) b.add(stepPos(bar, 15), gm.TomHigh, trunc(rng.range(96, 112)));
    if (s.fill) placeFill(b, s.bars - 1, rng, r);
    if (s.fill || s.intro) b.add(0.0, gm.Crash, 104, 1.0);
  }

  function buildIntro(b, rng) {
    const from = placeFill(b, 0, rng, b.s.randPerc);
    for (let beat = 0; beat < 4; ++beat)
      if (beat + 0.001 < from) b.add(beat, gm.ClosedHat, beat === 0 ? 70 : 58);
  }

  function dynamicsScale(s, beat, loopLen) {
    const bar = Math.trunc(beat / 4.0);
    switch (s.dynamics) {
      case Dyn.Soft: return 0.62; case Dyn.Medium: return 0.85; case Dyn.Loud: return 1.08; case Dyn.Accented: return 0.92;
      case Dyn.Crescendo: return 0.55 + 0.50 * (beat / loopLen);
      case Dyn.Breathing: return (bar % 2 === 0) ? 0.98 : 0.72;
    }
    return 1.0;
  }
  function pieceOf(n) {
    if (n === gm.Kick) return 0;
    if (n === gm.Snare || n === gm.SideStick) return 1;
    if (isHat(n)) return 2;
    return 3;
  }
  // os mesmos arredondamentos de float do C++ (aleatoriedade guardada em float)
  const f32 = Math.fround;

  // Mesma peça batendo muito perto dela mesma: fica só a principal (igual a cleanCloseHits no C++)
  function cleanCloseHits(out, fills) {
    const inFill = (beat) => fills.some((f) => beat >= f[0] - 0.15 && beat < f[1] + 0.01);
    const drop = new Array(out.length).fill(false);
    for (let i = 0; i < out.length; ++i) {
      for (let j = i + 1; j < out.length && !drop[i] && out[j].beat - out[i].beat < 0.2; ++j) {
        if (drop[j] || out[j].note !== out[i].note) continue;
        const gap = out[j].beat - out[i].beat, piece = pieceOf(out[i].note);
        const fill = inFill(out[i].beat) || inFill(out[j].beat);
        const main = out[j].velocity > out[i].velocity ? j : i, weak = main === i ? j : i;
        const ghostVel = Math.max(1, Math.round(out[main].velocity * 0.30));
        if (gap < 0.09) {
          const flam = piece === 1 && weak === i && gap >= 0.015 && gap <= 0.06;
          if (gap < 0.012) drop[weak] = true;
          else if (flam || fill) out[weak].velocity = Math.min(out[weak].velocity, ghostVel);
          else drop[weak] = true;
        } else if (piece !== 2 && !fill) {
          out[weak].velocity = Math.min(out[weak].velocity, Math.max(1, Math.round(out[main].velocity * 0.35)));
        }
      }
    }
    const kept = out.filter((_, i) => !drop[i]);
    out.length = 0; for (const e of kept) out.push(e);
  }

  function generate(input) {
    const s = Object.assign({}, defaults(), input);
    s.bars = clamp(s.bars, 1, 4);
    s.randKick = f32(s.randKick); s.randSnare = f32(s.randSnare); s.randHat = f32(s.randHat); s.randPerc = f32(s.randPerc);
    if (s.drummer === Drummer.Digikal) { s.machine = true; s.swing = Swing.Straight; }
    const b = new Builder(s);
    const seed = s.seed >>> 0;
    const mk = (k) => new Rng((Math.imul(seed, 7919) + k) >>> 0);
    const kickRng = mk(1), snareRng = mk(2), hatRng = mk(3), percRng = mk(4), humanRng = mk(5), introRng = mk(6);
    buildKick(b, kickRng); buildSnare(b, snareRng); buildHats(b, hatRng); buildPerc(b, percRng);

    const p = { events: [], lengthInBeats: s.bars * 4.0, intro: [], introLengthInBeats: 0.0 };
    const feel = feelOf(s.drummer);
    if (s.machine) { feel.swingDelta = 0.0; feel.late = [0, 0, 0, 0]; }
    const rand = [s.randKick, s.randSnare, s.randHat, s.randPerc];
    const timingAmount = [0.006, 0.012, 0.014, 0.012];

    const finish = (raw, len, barOverride, out, fills) => {
      for (const src of raw) {
        const e = Object.assign({}, src);
        const piece = pieceOf(e.note), hat = piece === 2;
        if (barOverride !== 0) e.bar = barOverride;
        if (hat || s.swingAll) e.beat = applySwing(e.beat, s.swing, feel.swingDelta);
        let vel = e.velocity;
        if (s.dynamics === Dyn.Accented) {
          const frac = e.beat - Math.floor(e.beat);
          vel = frac < 0.02 ? vel * 1.15 : 64.0 + (vel - 64.0) * 0.6 - 10.0;
        }
        vel *= dynamicsScale(s, e.beat, len);
        const r = rand[piece];
        if (s.machine) vel = vel >= 92.0 ? 118.0 : (vel >= 50.0 ? 88.0 : 60.0);
        else {
          vel += humanRng.range(-18.0, 18.0) * r;
          if (e.beat > 0.0) e.beat += humanRng.range(-1.0, 1.0) * timingAmount[piece] * r + feel.late[piece];
        }
        e.velocity = clamp(Math.round(vel), 1, 127);
        e.beat = clamp(e.beat, 0.0, len - 0.001);
        out.push(e);
      }
      out.sort((x, y) => x.beat - y.beat);
      cleanCloseHits(out, fills);
    };
    finish(b.ev, p.lengthInBeats, 0, p.events, b.fills);
    if (s.intro) {
      const is = Object.assign({}, s, { bars: 1 });
      const ib = new Builder(is);
      buildIntro(ib, introRng);
      p.introLengthInBeats = 4.0;
      finish(ib.ev, p.introLengthInBeats, -1, p.intro, ib.fills);
    }
    return p;
  }

  function defaultKickMask(st) { return st === Style.OneDrop ? 0x44 : st === Style.BoomBap ? 0x29 : 0x11; }
  function defaultSnareMask() { return 0x44; }
  function defaults() {
    return { style: Style.Rockers, kickMask: 0x11, snareMask: 0x44, bars: 4, hatPattern: Hat.Eighths, swing: Swing.Straight,
             swingAll: true, dynamics: Dyn.Medium, randKick: 0.15, randSnare: 0.25, randHat: 0.30, randPerc: 0.20,
             drummer: Drummer.CarltonBarrett, backbeat: Backbeat.Snare, machine: false, fill: false, intro: false, seed: 1 };
  }

  const api = { gm, Style, Hat, Swing, Dyn, Drummer, Backbeat, generate, defaults, defaultKickMask, defaultSnareMask, isMT40, Rng };
  if (typeof module !== 'undefined' && module.exports) module.exports = api; else root.MDDM = api;
})(typeof window !== 'undefined' ? window : globalThis);
