// What the page's mixer costs per block, in JS, for the two device rates a browser port actually meets.
// Desktop Chrome honours the AudioContext({sampleRate:44100}) hint; phones do not -- Android and iOS both
// give 48000. So the music ring, which is at 44100, goes through the sinc reader on a phone and (the code
// assumes) does not on a desktop. This says what that assumption is worth in microseconds.
'use strict';
const R = require('../../wasm/runtime/resample.js');

const CAP = 1 << 13, MASK = CAP - 1;
const ring = new Int16Array(CAP * 2);
for (let i = 0; i < ring.length; i++) ring[i] = (i * 7919 % 32768) - 16384;   // deterministic noise-ish

function bench(inRate, devRate, linear, seconds) {
  const L = new Float32Array(1024), Rr = new Float32Array(1024);
  let base = 0, produced = 0;
  const src = (f, hist, off) => {
    const p = ((base + f) & MASK) * 2;
    hist[off] = ring[p]; hist[off + 1] = ring[p + 1];
    return true;
  };
  const rd = R.reader(src, inRate, devRate, { linear });
  const need = Math.floor(seconds * devRate);
  let blocks = 0;
  const t0 = process.hrtime.bigint();
  while (produced < need) {
    L.fill(0); Rr.fill(0);
    const k = rd.pull(1024, L, Rr);
    if (!k) break;
    produced += k; blocks++;
  }
  const us = Number(process.hrtime.bigint() - t0) / 1000;
  console.log(`in=${inRate} dev=${devRate} ${linear ? 'linear' : 'sinc  '}: ` +
              `${blocks} blocks of 1024 in ${us.toFixed(0)}us -> ${(us / blocks).toFixed(1)}us/block ` +
              `(${(us / (produced / devRate)).toFixed(0)}us per second of audio, ` +
              `${((us / blocks) / 21300 * 100).toFixed(2)}% of a 21.3ms block)`);
}

bench(44100, 44100, false, 20);   // desktop: the "needs no resampling at all" case
bench(44100, 48000, false, 20);   // phone: what the code actually does when the device is 48k
bench(44100, 48000, true, 20);    // ?resample=linear on that same phone
bench(11025, 48000, false, 20);   // the sound board's rate on a phone, for scale
