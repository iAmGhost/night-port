// wasm/scripts/resample-test.js -- what the browser port's converter does to the sound board's samples.
//
//   node wasm/scripts/resample-test.js
//
// Three claims, all measurable, none of them "it sounds better to me":
//
//  1. The windowed-sinc filter removes the images that two-tap interpolation leaves. The sound board runs at
//     10000..22050 Hz and the device at 44.1 kHz, so a non-integer ratio mirrors the spectrum about the input
//     Nyquist -- which is exactly the audible band, because the board's own band tops out just under 11 kHz.
//  2. It is unity gain at DC and at 1:1 (a passthrough), which is the check that says the filter is not
//     quietly changing the level, and the check that CD-DA (44100 -> 44100) really is untouched.
//  3. A rate mark switches the ratio at the frame it names, so a queued run that spans a board reprogram is
//     played at both rates -- the desktop host gets the same property from SDL_AudioStream, and flushing
//     (what this host used to do) instead loses the audio at the join.
'use strict';
const R = require('../runtime/resample.js');

const DEV = 44100;
const gainDb = (x) => 20 * Math.log10(Math.max(x, 1e-12));
/* A claim that is printed but never checked is a claim nobody has to keep. */
const problems = [];
const fail = (why) => problems.push(why);

/* Goertzel magnitude at an exact frequency. */
function toneMag(x, f, fs) {
  const w = 2 * Math.PI * f / fs, c = 2 * Math.cos(w);
  let s0 = 0, s1 = 0, s2 = 0;
  for (let i = 0; i < x.length; i++) { s0 = x[i] + c * s1 - s2; s2 = s1; s1 = s0; }
  return Math.hypot(s1 - s2 * Math.cos(w), s2 * Math.sin(w)) * 2 / x.length;
}

/* The reader pulls through a callback, so "how much the producer has written" is a variable the test owns.
 * `avail` is what the ring would hold: the reader may not read past it, and that is the whole point of the
 * design -- the producer runs ahead freely and the reader still gets the frames it needs, in order. */
function run(input, rate, linear, availIn) {
  const lim = availIn === undefined ? input.length : availIn;
  const rd = R.reader((f, hist, off) => {
    if (f >= lim) return false;
    hist[off] = input[f]; hist[off + 1] = input[f];
    return true;
  }, rate, DEV, { linear });
  const outL = new Float64Array(Math.ceil(lim * DEV / rate) + DEV);
  const outR = new Float64Array(outL.length);
  rd.pull(outL.length, outL, outR);            // pull writes from index 0 of what it is handed, and all the
  const produced = rd.framesProduced();        // input is here already, so one call is the whole run
  return { out: outL.subarray(R.HALF, produced), produced, rd, outL };
}

/* 1) images. The sound board samples at 10000..22050 Hz, so its band tops out just under 11 kHz; anything
 * above the input Nyquist is not a sound, it is already an alias. So the tones here (1 kHz loud, 4.5 kHz
 * quiet) sit inside the board's band, and the interesting numbers are what the converter puts at
 * rate_in - f: the mirror two-tap interpolation leaves sitting in the middle of the music. */
function imagesTest() {
  const rate = 11111, secs = 3;
  const n = Math.round(rate * secs);
  const input = new Float64Array(n);
  for (let i = 0; i < n; i++)
    input[i] = 8000 * Math.sin(2 * Math.PI * 1000 * i / rate) + 2000 * Math.sin(2 * Math.PI * 4500 * i / rate);
  const want = [[1000, 'signal'], [4500, 'signal'], [rate - 4500, 'image'], [rate - 1000, 'image'],
                [2 * rate - 4500, 'image']];
  const rows = {};
  for (const [name, linear] of [['sinc', false], ['linear', true]]) {
    const { out } = run(input, rate, linear);
    rows[name] = Object.fromEntries(want.map(([f]) => [f, toneMag(out, f, DEV)]));
  }
  console.log(`images: ${rate} Hz in -> ${DEV} Hz out, 3 s, tones at 1 kHz (-3.5 dBFS) and 4.5 kHz (-12.0 dBFS)`);
  console.log('  freq   kind      sinc      linear   suppression gained');
  for (const [f, kind] of want) {
    const s = rows.sinc[f], l = rows.linear[f];
    console.log(`  ${String(f).padStart(6)} ${kind}  ${gainDb(s / 8000).toFixed(1).padStart(7)} dB  ` +
                `${gainDb(l / 8000).toFixed(1).padStart(7)} dB  ${(gainDb(l) - gainDb(s)).toFixed(1).padStart(12)} dB`);
  }
  for (const [f, kind] of want) if (kind === 'image' && rows.sinc[f] > rows.linear[f])
    fail(`the windowed-sinc leaves MORE at the ${f} Hz image than two-tap interpolation does`);
}

/* 2) passthrough and unity gain. */
function gainTest() {
  const n = DEV;                                    // one second
  const dc = new Float64Array(n).fill(1000);
  const { out: o1 } = run(dc, DEV, false);
  const dcErr = Math.max(...Array.from(o1.slice(R.HALF, n)).map((v) => Math.abs(v - 1000) / 1000));
  const tone = new Float64Array(n);
  for (let i = 0; i < n; i++) tone[i] = 8000 * Math.sin(2 * Math.PI * 1000 * i / DEV);
  const { out: o2 } = run(tone, DEV, false);
  console.log(`  44100 -> 44100 1 kHz: ${(gainDb(toneMag(o2, 1000, DEV) / 8000)).toFixed(3)} dB   (the CD-DA path)`);
  console.log('\nunity gain:');
  console.log(`  44100 -> 44100 DC: max error ${(dcErr * 100).toFixed(4)} %    (the CD-DA path, must be 0)`);
  const up = new Float64Array(Math.round(22050 * 2));
  for (let i = 0; i < up.length; i++) up[i] = 8000 * Math.sin(2 * Math.PI * 1000 * i / 22050);
  const { out: o3 } = run(up, 22050, false);
  console.log(`  22050 -> 44100 1 kHz: ${gainDb(toneMag(o3, 1000, DEV) / 8000).toFixed(3)} dB, ` +
              `image at 21050 Hz: ${gainDb(toneMag(o3, 21050, DEV) / toneMag(o3, 1000, DEV)).toFixed(1)} dB`);
  /* Every rate the board is measured to program, not just one of them: a polyphase table is built per ratio,
   * so a normalisation slip can hide in the ratios nobody tested -- and a filter that is quietly 2x loud
   * sounds like nothing in particular until it is a sound effect under the music. */
  let worstDb = 0, worstRate = 0;
  for (const rate of [10000, 10989, 11025, 11111, 22050]) {
    const x = new Float64Array(Math.round(rate * 1));
    for (let i = 0; i < x.length; i++) x[i] = 8000 * Math.sin(2 * Math.PI * 1000 * i / rate);
    const { out } = run(x, rate, false);
    const db = gainDb(toneMag(out, 1000, DEV) / 8000);
    if (Math.abs(db) > Math.abs(worstDb)) { worstDb = db; worstRate = rate; }
    console.log(`  ${String(rate).padStart(5)} -> 44100 1 kHz: ${db.toFixed(3)} dB`);
  }
  console.log(`  worst over the board's rates: ${worstDb.toFixed(3)} dB at ${worstRate} Hz ` +
              `(${Math.abs(worstDb) <= 1 ? 'within 1 dB' : 'OUT OF 1 dB -- the table is not normalised'})`);
  if (Math.abs(worstDb) > 1) fail('the converter is more than 1 dB off unity at ' + worstRate + ' Hz');
  /* The worst case is not a tone, it is an impulse: max|out| / max|in| is the filter's infinity-norm gain,
   * and if it were above 1 the converter could turn a full-scale effect into a clipped one by itself. 16 taps
   * of Blackman-windowed sinc come out at 1.000 -- so a peak that exceeds its input is a measurement problem,
   * not a filter problem, which is exactly how this claim earned its keep. */
  let worstImp = 0, impRate = 0;
  for (const rate of [10000, 10989, 11025, 11111, 22050, 44100]) {
    const n = 4096, x = new Float64Array(n); x[1000] = 32767;
    const { out } = run(x, rate, false);
    let peak = 0; for (const v of out) peak = Math.max(peak, Math.abs(v));
    const g = peak / 32767;
    if (g > worstImp) { worstImp = g; impRate = rate; }
  }
  console.log(`  worst impulse (L-inf) gain over the board's rates: ${worstImp.toFixed(3)} x at ${impRate} Hz ` +
              `(${worstImp <= 1.02 ? 'no gain -- the filter cannot clip you by itself' : 'ABOVE 1: the filter adds level'})`);
  if (worstImp > 1.02) fail(`the converter's infinity-norm gain is ${worstImp.toFixed(3)} x at ${impRate} Hz`);
}

/* 3) the rate mark switches at the frame it names: half a run at 11111, half at 22050. The tone aliases at
 * both rates, so the check is the output length: 8192 frames at 11111 Hz is 32.5 ms of audio, at 22050 Hz it
 * is 16.4 ms, and a reader that ignored the mark would stretch the second half. */
function markTest() {
  const rate1 = 11111, rate2 = 22050, n = 20480;
  const F = 8192;
  const input = new Float64Array(n);
  for (let i = 0; i < n; i++) input[i] = 8000 * Math.sin(2 * Math.PI * 2200 * i / rate1);
  const outL = new Float64Array(Math.ceil(n * DEV / rate1)), outR = new Float64Array(outL.length);
  const go = (mark) => {
    const rd = R.reader((f, hist, off) => {
      if (f >= n) return false;
      hist[off] = input[f]; hist[off + 1] = input[f];
      return true;
    }, rate1, DEV, {});
    if (mark) rd.setMark(F, rate2);
    rd.pull(outL.length, outL, outR);
    return rd.framesProduced();
  };
  const withMark = go(true), without = go(false);
  const expect = F * DEV / rate1 + (n - F) * DEV / rate2;
  console.log('\nrate mark:');
  console.log(`  ${n} frames in (${F} at ${rate1} Hz, ${n - F} at ${rate2} Hz):`);
  console.log(`    marked   ${withMark} frames out, expected ${expect.toFixed(0)} (the tail within TAPS=` +
              `${R.TAPS} is the taps still waiting for input: ${Math.abs(withMark - expect) <= R.TAPS ? 'OK' : 'MISMATCH'})`);
  console.log(`    unmarked ${without} frames out: without the mark the second half is played ` +
              `${((without / expect - 1) * 100).toFixed(0)} % too long, i.e. ${((without - expect) / DEV * 1000).toFixed(0)} ms of drift`);
  if (Math.abs(withMark - expect) > R.TAPS) fail('the rate mark did not switch the ratio at the frame it named');
  if (!(without > expect * 1.02)) fail('the unmarked run is not longer, so this test proves nothing about the mark');
}

/* 4) the producer runs ahead, and that must change nothing. This is the regression the push-based reader
 * had: the page holds the game for tens of milliseconds, the host fills the ring meanwhile, and a reader
 * whose tap history was filled by the producer had already overwritten the frames it was about to need.
 * Same input, two schedules -- dribbled in 2 % blocks versus handed over whole -- and identical output. */
function aheadTest() {
  const rate = 10989, n = 20011;                     // odd n: no luck with a round number
  const input = new Float64Array(n);
  for (let i = 0; i < n; i++) input[i] = Math.sin(2 * Math.PI * 333 * i / rate) + 0.5 * Math.sin(2 * Math.PI * 6111 * i / rate);
  const full = run(input, rate, false);
  let avail = 0, produced = 0, released = 0, backwards = 0;
  const outL = new Float64Array(Math.ceil(n * DEV / rate) + DEV), outR = new Float64Array(outL.length);
  const rd = R.reader((f, hist, off) => {
    if (f >= avail) return false;
    hist[off] = input[f]; hist[off + 1] = input[f];
    return true;
  }, rate, DEV, {});
  for (let block = 0; block < 4000 && produced < outL.length; block++) {
    avail = Math.min(n, avail + Math.ceil(n / 50));            // the host dribbles in at 2 % per audio block
    const k = rd.pull(outL.length - produced, outL.subarray(produced), outR.subarray(produced));
    produced += k;
    if (rd.released() < released) backwards++;
    released = rd.released();
    if (k === 0 && avail >= n) break;
  }
  const relErr = (() => {
    let worst = 0, where = -1, cmp = Math.min(produced, full.produced);
    for (let i = 0; i < cmp; i++) {
      const d = Math.abs(outL[i] - full.outL[i]);
      if (d > worst) { worst = d; where = i; }
    }
    return { worst, where };
  })();
  const expect = n * DEV / rate;
  console.log('\nproducer runs ahead:');
  console.log(`  ${n} frames at ${rate} Hz, dribbled in 2 % blocks: ${produced} frames out, all-at-once ` +
              `${full.produced}, expected ${expect.toFixed(0)} -- ${Math.abs(produced - full.produced) <= 1 ? 'same' : 'DIFFERENT'}`);
  console.log(`  identical samples: ${relErr.worst === 0 ? 'yes' : `no, worst ${relErr.worst.toFixed(3)} of 32767 ` +
              `at output frame ${relErr.where} of ${produced}`}, ` +
              `released() monotonic: ${backwards === 0 ? 'yes' : 'NO'} (${released}/${n} frames behind the filter)`);
  if (relErr.worst !== 0) fail('a producer that runs ahead changes the samples the reader hears');
  if (backwards !== 0) fail('released() went backwards -- the host would be told to overwrite frames in use');
}

/* 5) the mixer, which is where one of the two voices used to go. Both voices arrive in int16 units and the
 * SUM is what has to be divided by 32768 -- once. Dividing each voice as it is mixed into a shared
 * destination divides the voice mixed FIRST a second time: its 0.244 of full scale became 0.244/32768,
 * i.e. silence. It only went missing when the disc was playing at the same time -- which, in this game, is
 * nearly always. Every frame counter stayed green through it, because frames flowed into the filter and out
 * of it; the defect is a level, so the assertions below are all levels. */
function mixTest() {
  const n = 1024;
  const voice = (freq, amp, k, pan) => {
    const l = new Float64Array(n), r = new Float64Array(n);
    for (let i = 0; i < n; i++) { const v = amp * Math.sin(2 * Math.PI * freq * i / DEV); l[i] = v; r[i] = v * pan; }
    return { l, r, k };
  };
  const music = voice(250, 20000, n, 0.5), sfx = voice(4000, 8000, n, 1);
  const mute = { l: new Float64Array(n), r: new Float64Array(n), k: 0 };
  const go = (a, b) => {
    const dstL = new Float64Array(n), dstR = new Float64Array(n);
    const m = R.mix(dstL, dstR, n, a, b);
    let peak = 0;
    for (let i = 0; i < n; i++) peak = Math.max(peak, Math.abs(dstL[i]), Math.abs(dstR[i]));
    return { m, peak };
  };
  const aloneA = go(sfx, mute), aloneB = go(mute, music), both = go(sfx, music);
  const same = (x, y) => Math.abs(x - y) < 1e-12;
  console.log('\nmixing two voices (float full scale):');
  console.log(`  effect alone: contribution ${aloneA.m.peakA.toFixed(4)}  with music: ${both.m.peakA.toFixed(4)}  ` +
              `(music alone: ${aloneB.m.peakB.toFixed(4)}, with effect: ${both.m.peakB.toFixed(4)})`);
  console.log(`  shared block: both=${both.m.both} output peak ${both.peak.toFixed(4)} clipped samples ${both.m.clip}`);
  console.log(`  effect alone output peak ${aloneA.peak.toFixed(4)}, sum of the two voices' own peaks ` +
              `${(aloneA.m.peakA + aloneB.m.peakB).toFixed(4)}`);
  if (!both.m.both) fail('mix() says two full voices did not share a block');
  if (!same(both.m.peakA, aloneA.m.peakA))
    fail(`a voice's own level changed because another voice was mixed with it ` +
         `(${aloneA.m.peakA.toFixed(4)} -> ${both.m.peakA.toFixed(4)}) -- the scale is applied per voice`);
  if (!same(both.m.peakB, aloneB.m.peakB)) fail("the music voice's level changed when an effect joined");
  if (!(Math.abs(both.m.peakA - 8000 / 32768) < 0.01))
    fail(`the effect voice is not at its own scale (got ${both.m.peakA.toFixed(4)}, want ${(8000 / 32768).toFixed(4)})`);
  if (aloneA.m.both || both.m.clip !== 0) fail('mix() reports overlap or clipping where there is none');
  /* And the clamp: two in-phase full-scale voices have nowhere to go but the rail, which is what the card's
   * analog summer does too. It has to be reported, not absorbed. */
  const hot = go(voice(250, 32000, n, 1), voice(250, 32000, n, 1));
  console.log(`  two in-phase full-scale voices: peak ${hot.peak.toFixed(4)} (pinned at 1), ` +
              `clipped samples ${hot.m.clip} of ${2 * n}`);
  if (!(hot.m.clip > 0 && Math.abs(hot.peak - 1) < 1e-9)) fail('the sum neither clamps nor reports the clipping');
}

imagesTest(); gainTest(); markTest(); aheadTest(); mixTest();
if (problems.length) {
  console.log('\nFAILED:');
  for (const p of problems) console.log('  - ' + p);
  process.exitCode = 1;
} else console.log('\nall claims hold');
