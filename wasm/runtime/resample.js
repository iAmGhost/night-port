/* wasm/runtime/resample.js -- the browser port's sample-rate converter.
 *
 * Why this file exists on its own: the sound board runs at 10000, 10989, 11111 or 22050 Hz and the browser's
 * AudioContext runs at 44.1 kHz (or 48), so every sound effect is resampled at a non-integer ratio like
 * 4.013, while CD-DA (44100 -> 44100) passes through untouched. That asymmetry is audible, and it is the
 * whole difference between "the music sounds right" and "some effects sound a bit off".
 *
 * The first version interpolated between two neighbours. On a non-integer ratio that
 *   - mirrors the spectrum about the input Nyquist: a sound whose content reaches 10 kHz at 11.1 kHz input
 *     gets images folded back to 1.1 kHz and everywhere between, in the most audible part of the band, and
 *   - rolls the top end off with the sinc-squared envelope of a 2-tap box, so a bright sound comes back dull.
 * The desktop host does not have that problem: the same bytes go through SDL_AudioStream, which filters.
 *
 * So: a windowed-sinc interpolator, Blackman-windowed, TAPS taps, evaluated through a PHASES-entry polyphase
 * table built per ratio. Images land at the window's sidelobe floor. `?resample=linear` in the page still
 * gets the old two-tap filter, so the two can be A/B'd on the same run.
 *
 * The reader reads through a callback rather than having frames handed to it, and that is not a style
 * preference: the first version had the caller dump every frame the producer had written into a 16-frame
 * tap history, which is fine until the producer runs ahead of the reader -- it does, the game holds the page
 * for tens of milliseconds at a time -- and then the frames the reader needs have already been overwritten
 * by the frames it does not need yet. The reader owns its window; the ring is the carry buffer.
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.NightResample = factory();
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';

  const TAPS = 16, PHASES = 64, HALF = TAPS / 2;
  const sinc = (x) => (x === 0 ? 1 : Math.sin(Math.PI * x) / (Math.PI * x));
  const blackman = (x) => (Math.abs(x) >= 1 ? 0 : 0.42 + 0.5 * Math.cos(Math.PI * x) + 0.08 * Math.cos(2 * Math.PI * x));

  /* TAPS taps for one ratio, where ratio = input frames per output frame. Tap t sits at input frame
   * floor(pos) - HALF + 1 + t.
   *
   * The kernel's width follows whichever band is narrower. Upsampling (ratio < 1) keeps the input's whole
   * band, so the prototype is the plain interpolator: sinc over the input's Nyquist. Downsampling (ratio > 1)
   * has to pre-cut the band the output cannot carry, and the kernel *widens* in input-sample units by that
   * ratio -- which is why the scale is the inverse of the ratio and not the ratio: getting this backwards (it
   * was, once) puts the cutoff at a quarter of where it belongs and swallows the board's band outright. */
  function buildTable(ratio) {
    const t = new Float32Array(PHASES * TAPS);
    const a = ratio > 1 ? 1 / ratio : 1;
    for (let p = 0; p < PHASES; p++) {
      const ph = p / PHASES;
      let sum = 0;
      for (let i = 0; i < TAPS; i++) {
        const x = (i - HALF + 1 - ph) * a;
        const h = sinc(x) * blackman(x / HALF);
        t[p * TAPS + i] = h; sum += h;
      }
      if (sum !== 0) for (let i = 0; i < TAPS; i++) t[p * TAPS + i] /= sum;
    }
    return t;
  }

  /* A streaming reader.
   *   source(frame) -> true when input frame `frame` is available (it must have been written into `hist`),
   *                    false when the producer has not written it yet.
   *   pull(n, L, R) -> produce up to n output frames, adding into Float32Array L/R; reads only what its taps
   *                    need, stops when the source runs dry or a mark says the rate changed.
   *   setMark(f, rate) -- "input frame f onwards was written at rate", applied when the reader gets to f.
   *   released()     -> input frames below this index are no longer reachable; the producer may overwrite.
   * `linear` selects the two-tap filter this file replaced, for A/B. */
  function reader(source, rate, devRate, opts) {
    const linear = !!(opts && opts.linear);
    let ratio = rate / devRate, table = linear ? null : buildTable(ratio);
    const hist = new Float32Array(TAPS * 2);
    const marks = [];          // {frame, rate}, increasing; applied as the reader walks past them
    let mi = 0;
    let haveTo = 0;       // input frames fetched into hist (absolute, from 0); hist holds [haveTo-TAPS, haveTo)
    let pos = 0;          // fractional absolute input position of the NEXT output frame
    let outAbs = 0;       // output frames produced (absolute)

    function ensure(to) {      // fetch input frames up to and including `to`; false if the producer is short
      while (haveTo <= to) {
        if (!source(haveTo, hist, (haveTo & (TAPS - 1)) * 2)) return false;
        haveTo++;
      }
      return true;
    }
    function applyRate(rate2) {
      const r = rate2 / devRate;
      if (r === ratio) return;
      ratio = r; table = linear ? null : buildTable(r);
    }

    return {
      getTaps: () => TAPS,
      getHalf: () => HALF,
      getPos: () => pos,
      getRatio: () => ratio,
      getRate: () => ratio * devRate,
      setRate: applyRate,
      setMark(frame, rate2) { marks.push({ frame, rate: rate2 }); },
      framesFetched: () => haveTo,
      framesProduced: () => outAbs,
      released: () => Math.max(0, haveTo - TAPS),
      pull(n, outL, outR) {
        let k = 0;
        while (k < n) {
          const f = Math.floor(pos);
          /* The taps reach input frames f-HALF+1 .. f+HALF. The window must hold them, and the frame just
           * below the window must already be behind `released()`, which it is: the window slides one frame
           * per input frame consumed. */
          if (!ensure(f + HALF)) break;                     // the producer has not written that far yet
          while (mi < marks.length && marks[mi].frame <= f) { applyRate(marks[mi].rate); mi++; }
          if (linear) {
            if (!ensure(f + 1)) break;
            const fr = pos - f;
            const q0 = (f & (TAPS - 1)) * 2, q1 = ((f + 1) & (TAPS - 1)) * 2;
            outL[k] += hist[q0] + (hist[q1] - hist[q0]) * fr;
            outR[k] += hist[q0 + 1] + (hist[q1 + 1] - hist[q0 + 1]) * fr;
          } else {
            const first = f - HALF + 1, row = (((pos - f) * PHASES) | 0) * TAPS;
            let a = 0, b = 0;
            for (let t = 0; t < TAPS; t++) {
              const h = table[row + t], q = (((first + t) & (TAPS - 1)) * 2);
              a += h * hist[q]; b += h * hist[q + 1];
            }
            outL[k] += a; outR[k] += b;
          }
          pos += ratio; k++; outAbs++;
        }
        return k;
      },
    };
  }

  return { TAPS, PHASES, HALF, buildTable, reader };
});
