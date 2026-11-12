/* shell.js -- the browser side of the NIGHT wasm port.  docs/457.
 *
 * WHAT LIVES HERE
 * ---------------
 * Three jobs, and only one of them ever calls a wasm function:
 *
 *   present()   blit the host's composed RGBA buffer to the canvas.
 *   nextFrame() the promise a yield awaits: it resolves on the next requestAnimationFrame, which is the
 *               moment the host hands the thread to the browser, and the moment the browser hands it back.
 *   the audio   two PCM rings in wasm memory (the sound board, CD-DA), summed into a ScriptProcessorNode.
 *   the keys    pushed into a ring in wasm memory as BYTES, never as a call.
 *
 * That last part is not tidiness: between frames the game's whole stack is mid-unwind (wasm/src/wasm_block.h
 * and the top of main_wasm.c), and any second entry into wasm in that window -- a keydown handler, an
 * AudioContext callback -- would write its own frames into the same unwind bookkeeping and corrupt the
 * sleeping stack. So everything crosses as memory, and the only JS -> wasm entry after boot is the one
 * that resumes a yield.
 */
(function () {
  'use strict';

  /* ---- wasm_block.h mirrored (the host prints the same numbers at boot; keep in sync) ---- */
  const ST = { BOOTED: 0, FRAMES: 1, VID_W: 2, VID_H: 3, EXIT: 4, TRACKS: 5, SFX: 6, CDDA: 7, DISC: 8, FLAGS: 9, KEYS: 10 };
  const CFG = { AUDIO_RATE: 16, MUTED: 17, WANT_QUIT: 18, FLAGS: 19, SFX_MS: 20, CDDA_MS: 21 };
  const KEY = { HEAD: 32, TAIL: 33, OVER: 34, SIZE: 35, SLOT_BYTES: 36 };
  const RING = { HEAD: 0, TAIL: 1, CAP: 2, MASK: 3, RATE: 4, PCM: 5, MARK_GEN: 6, WORDS: 8 };
  const NK_MARK_WORDS = 2 + 64 * 2;              // NK_RING_MARK_STRIDE in wasm_block.h
  /* The rate-mark table is a ring of (frame, rate) entries; entry ordinal o sits at slot o % CAP. The entry
   * at firstOrdinal is the base -- the rate the live frames started at -- and every later one is applied as
   * the reader walks past its frame. */
  const MK = { N: 0, ORD: 1, ENTRY: 2, EWORDS: 2, CAP: 64 };
  const mkSlot = (ord) => MK.ENTRY + (ord % MK.CAP) * MK.EWORDS;
  const RING_SFX = 0, RING_CDDA = 1;

  const $ = (id) => document.getElementById(id);
  const canvas = $('screen'), statusEl = $('status'), logEl = $('log'), barEl = $('bar');
  const holderEl = $('screen-holder'), scaleEl = $('btn-scale');
  const gateEl = $('gate'), gateStart = $('btn-start'), gateStatusEl = $('gate-status');

  /* `say` writes the HUD line; `gate` writes the gate's line while it is still the thing being looked at.
   * Two lines because the preload runs while the overlay covers the canvas, and a status nobody can see
   * during the one wait of the session is a status that does not exist. */
  function gate(msg) {
    if (gateEl && !gateEl.classList.contains('hidden') && gateStatusEl) gateStatusEl.textContent = msg;
    say(msg);
  }

  let M = null;                                  // the instantiated module
  let preloaded = false;                          // module instantiated and NIGHT.RES in the filesystem
  /* ?sink=1 / ?auto=1 start without a press (see boot); a real visitor gets the gate. */
  const AUTO_START = /[?&](sink|auto)=1/.test(location.search);
  /* ALLOW_MEMORY_GROWTH replaces the ArrayBuffer, so the heap's buffer is read afresh every time -- and
   * it is reached through HEAPU8, because the module object does not export `buffer` itself. */
  const heap = () => M.HEAPU8.buffer;
  let blockBase = 0, ringBases = [0, 0], ringMarkBases = [0, 0];   // fixed wasm addresses, read once at boot
  let logOn = false, muted = false;

  function say(msg) { if (statusEl) statusEl.textContent = msg; }
  function progress(frac) { if (barEl) barEl.style.width = Math.max(0, Math.min(1, frac)) * 100 + '%'; }
  function logLine(s) {
    console.log(s);                                  // the host's log is the port's only diagnostic channel
    if (!logOn || !logEl) return;
    logEl.textContent += s + '\n';
    logEl.scrollTop = logEl.scrollHeight;
  }

  /* ---- the frame promise: the one place wasm is entered from JS ---- */
  /* The frame clock and the blit, counted: if the game is running slow in a browser, this says whether
   * rAF is the limit (the loop is driven by it) or the blit is (putImageData in software is far from
   * free at 640x480). */
  const raf = { calls: 0, gaps: 0, last: 0 }, timer = { calls: 0 };
  const blit = { calls: 0, ms: 0, max: 0 };
  /* The yield's job is to hand the browser its turn, not to wait for a paint. rAF is the smooth way to do
   * that, but it is not a clock you can rely on: a window Chrome decides is occluded gets almost none
   * (measured: 36 frames in 22 s, with the audio callback running the whole time), and a hidden tab gets
   * one per second. So the promise resolves on whichever comes first -- the next animation frame, or a
   * timer -- and the game keeps stepping and keeps its music, which is what the desktop host does with a
   * window nobody is looking at. */
  const YIELD_FALLBACK_MS = 30;
  function nextFrame() {
    return new Promise((resolve) => {
      let done = false;
      const settle = (by, t) => {
        if (done) return;
        done = true;
        if (by === 'raf') { if (raf.last) raf.gaps += t - raf.last; raf.last = t; raf.calls++; }
        else timer.calls++;
        resolve();
      };
      requestAnimationFrame((t) => settle('raf', t));
      setTimeout(() => settle('timer'), YIELD_FALLBACK_MS);
    });
  }

  /* ---- presentation ---- */
  let ctx = null, img = null, imgU32 = null, modeW = 0, modeH = 0, pdbg = 0;
  function present(ptr, w, h) {
    if (!ctx) ctx = canvas.getContext('2d', { alpha: false });
    if (w !== modeW || h !== modeH) {
      canvas.width = modeW = w;
      canvas.height = modeH = h;
      img = ctx.createImageData(w, h);
      imgU32 = new Uint32Array(img.data.buffer);
      fit();                                        /* the integer scale changes with the mode */
    }
    /* The host composes in ImageData order (R in the low byte), so this is a whole-word copy. The view
     * cannot be cached: ALLOW_MEMORY_GROWTH swaps the buffer out from under it. */
    const t0 = performance.now();
    imgU32.set(new Uint32Array(heap(), ptr, w * h));
    ctx.putImageData(img, 0, 0);
    const dt = performance.now() - t0;
    blit.calls++; blit.ms += dt; if (dt > blit.max) blit.max = dt;
    if (pdbg < 3) {                                   /* the first frames are the whole diagnosis */
      pdbg++;
      const src = new Uint32Array(heap(), ptr, w * h), mid = w * h >> 1;
      console.log('present ' + w + 'x' + h + ' ptr=' + ptr + ' buf=' + heap().byteLength +
                  ' mem=' + [0, 1, mid].map(i => src[i].toString(16)).join(',') +
                  ' img=' + [0, 1, mid].map(i => imgU32[i].toString(16)).join(',') +
                  ' len=' + imgU32.length + '/' + src.length);
      const px = ctx.getImageData(w >> 1, h >> 1, 1, 1).data;
      console.log('  canvas px at centre = ' + [...px].join(','));
    }
  }
  /* ---- what the screen showed, and how big to make it ----
   * The bitmap is 320x200 in the room path and 640x480 everywhere else, but the picture the original put on a
   * CRT was 4:3 in both cases: mode 13h's pixels are square at the DAC and double-square on the screen, which is
   * why a room frame scaled to 640x480 (the desktop host's view, docs/66) lines up with the slides and the title
   * melt. Stretching 320x200 to its own 16:10 ratio -- what this page did -- makes the room 24 pixels narrower
   * than everything around it. STRETCH is now the default and persists; `fit` keeps the bitmap's ratio and
   * scales it by a whole number, and `?scale=fit` forces that for a run.
   *
   * No smoothing anywhere. A fractional scale with nearest-neighbour repeats some source rows and columns
   * before others -- a slightly uneven line thickness -- and that is the look wanted here; bilinear was the
   * default for one revision of this function and it made the whole game look out of focus. `fit` is the mode
   * that is exact in both axes, because its scale is a whole number.
   */
  const SCALE_KEY = 'night.scale';
  const DISPLAY_AR = 4 / 3;
  const qsScale = new URLSearchParams(location.search).get('scale');
  let stretch = qsScale ? qsScale !== 'fit'
                        : (localStorage.getItem(SCALE_KEY) || 'stretch') !== 'fit';
  function showScale() {
    if (scaleEl) { scaleEl.textContent = stretch ? 'fit' : 'stretch';
                   scaleEl.title = stretch ? 'keep the bitmap ratio (integer scale)'
                                           : 'stretch to 4:3, the ratio the original showed'; }
    canvas.style.imageRendering = 'pixelated';      /* both modes: no blurring at any scale */
    if (scaleEl) scaleEl.classList.toggle('off', !stretch);
  }
  function fit() {
    if (!modeW) return;
    const box = holderEl || document.body;
    let bw = box.clientWidth, bh = box.clientHeight;
    if (!bw || !bh) { bw = window.innerWidth; bh = window.innerHeight; }
    if (!bw || !bh) return;
    const ar = stretch ? DISPLAY_AR : modeW / modeH;
    let w = bw, h = w / ar;
    if (h > bh) { h = bh; w = h * ar; }
    if (!stretch) {                                  /* integer only: no blurry whole-number scale */
      const s = Math.max(1, Math.min(Math.floor(w / modeW), Math.floor(h / modeH)));
      w = modeW * s; h = modeH * s;
    }
    canvas.style.width = Math.round(w) + 'px';
    canvas.style.height = Math.round(h) + 'px';
  }
  function setStretch(on) {
    stretch = on;
    try { localStorage.setItem(SCALE_KEY, on ? 'stretch' : 'fit'); } catch (e) { /* private mode */ }
    showScale(); fit();
  }
  window.addEventListener('resize', fit);

  /* ---- keys: KeyboardEvent.code -> the DOS make code, then bytes into the ring ----
   * The scancodes are the ones keymap.c (docs/91) maps onto the game's DS key bytes; the letters follow
   * the DOS table src/app/main.c uses (SDL scancodes are HID order, so they are not arithmetic). */
  const DOS = {
    Escape: 0x01, Enter: 0x1c, NumpadEnter: 0x1c, Space: 0x39, Backspace: 0x0e, Tab: 0x0f,
    F1: 0x3b, F2: 0x3c, F3: 0x3d, F4: 0x3e, F5: 0x3f, F6: 0x40, F7: 0x41, F8: 0x42,
    ArrowLeft: 0x4b, ArrowUp: 0x48, ArrowRight: 0x4d, ArrowDown: 0x50,
    ShiftLeft: 0x2a, ShiftRight: 0x36, ControlLeft: 0x1d, ControlRight: 0x1d,
    AltLeft: 0x38, AltRight: 0x38,
    KeyQ: 0x10, KeyW: 0x11, KeyE: 0x12, KeyR: 0x13, KeyT: 0x14, KeyY: 0x15, KeyU: 0x16, KeyI: 0x17,
    KeyO: 0x18, KeyP: 0x19, KeyA: 0x1e, KeyS: 0x1f, KeyD: 0x20, KeyF: 0x21, KeyG: 0x22, KeyH: 0x23,
    KeyJ: 0x24, KeyK: 0x25, KeyL: 0x26, KeyZ: 0x2c, KeyX: 0x2d, KeyC: 0x2e, KeyV: 0x2f, KeyB: 0x30,
    KeyN: 0x31, KeyM: 0x32,
    Digit1: 0x02, Digit2: 0x03, Digit3: 0x04, Digit4: 0x05, Digit5: 0x06, Digit6: 0x07,
    Digit7: 0x08, Digit8: 0x09, Digit9: 0x0a, Digit0: 0x0b,
  };
  const seen = { down: 0, up: 0, push: 0 };   // did the page see the key at all, before wasm is involved
  const SWALLOW = new Set(['Space', 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', 'F1']);
  function keyPush(code, make) {
    const sc = DOS[code];
    if (!sc || !M) return;
    const H = M.HEAPU32, b = blockBase >> 2;
    const head = H[b + KEY.HEAD], tail = H[b + KEY.TAIL], size = H[b + KEY.SIZE];
    if (head - tail >= size) { H[b + KEY.OVER] = H[b + KEY.OVER] + 1; return; }
    const slots = (blockBase + H[b + KEY.SLOT_BYTES]) >> 2;
    H[slots + (head & (size - 1))] = sc | ((make ? 1 : 0) << 8);
    H[b + KEY.HEAD] = head + 1;
  }
  window.addEventListener('keydown', (e) => {
    ensureAudioOnce();                            // autoplay policy: a gesture is what opens the AudioContext
    /* Before the gate's press the game is not running, and swallowing Space/Enter here would swallow a
     * keyboard user's press of the Start button. `seen` still counts, so the harness can tell that the page
     * got the key even when nothing was listening for it. */
    if (!started) { seen.down++; return; }
    if (SWALLOW.has(e.code)) e.preventDefault();
    seen.down++;
    if (e.repeat) return;
    seen.push++;
    keyPush(e.code, true);
  }, { passive: false });
  window.addEventListener('keyup', (e) => {
    if (!started) { seen.up++; return; }
    if (SWALLOW.has(e.code)) e.preventDefault();
    seen.up++; seen.push++;
    keyPush(e.code, false);
  }, { passive: false });
  window.addEventListener('blur', () => { for (const c in DOS) keyPush(c, false); });

  /* ---- audio: two rings summed by a ScriptProcessorNode, pure JS ---- */
  let ac = null, sp = null, audioAsked = false;
  function ensureAudioOnce() {
    if (audioAsked) { if (ac && ac.state !== 'running') ac.resume(); return; }
    audioAsked = true;
    ensureAudio();
  }
  function ensureAudio() {
    if (ac) return;
    const Ctor = window.AudioContext || window.webkitAudioContext;
    if (!Ctor) { say('no Web Audio in this browser -- the game runs silent'); return; }
    /* Ask for 44.1 kHz: that is what a Red Book track is, so CD-DA needs no resampling at all. Browsers
     * that refuse the hint get their own rate and the host resamples instead (writeRate tells it which). */
    try { ac = new Ctor({ sampleRate: 44100 }); } catch (e) { ac = new Ctor(); }
    sp = ac.createScriptProcessor(1024, 0, 2);
    sp.onaudioprocess = audioPull;
    sp.connect(ac.destination);
    if (ac.state !== 'running') ac.resume();
    writeRate();
    /* If a gesture still did not open the device, say it where the player can see: a run whose AudioContext
     * stayed suspended reads every audio counter at a healthy zero, and silence is the one symptom the
     * counters do not show. Any later press calls this again (the pointerdown listener is not `once`). */
    if (ac.state !== 'running') say('sound blocked by the browser -- press again');
  }
  function writeRate() {
    if (!M || !ac) return;
    M.HEAPU32[(blockBase >> 2) + CFG.AUDIO_RATE] = Math.round(ac.sampleRate);
  }
  /* Read a ring at its OWN rate into the AudioContext's clock, across the rate marks the host leaves in it.
   *
   * FILTER. The ring is at the board's rate -- 10000, 10989, 11111 or 22050 Hz -- and the device is at 44.1 kHz,
   * so every sound effect is resampled while CD-DA (44100 -> 44100) needs nothing at all. That asymmetry is
   * exactly what "the music is fine, some effects are off" says, and resample.js is the answer to it: the
   * desktop host filters those bytes through SDL_AudioStream, and until this file the browser host interpolated
   * between two neighbours, which mirrors the spectrum about the input Nyquist. `?resample=linear` gets the old
   * filter back for an A/B.
   *
   * RATE. The board reprograms its rate per DMA transfer, and the ring holds up to ~120 ms of audio, so a
   * change lands in the middle of a queued run. The host records it as a mark (frame, rate) and this reader
   * switches ratio at exactly that frame; it used to flush, which lost 1621 frames (37 ms) in a 34 s run.
   *
   * The ring is still the carry buffer: the fractional read position lives here, not in wasm. */
  const RESAMPLE = new URLSearchParams(location.search).get('resample') === 'linear' ? 'linear' : 'sinc';
  const mixState = [null, null];
  function mixRing(which, L, R, n) {
    if (!ringBases[which]) return 0;
    const I = new Uint32Array(heap(), ringBases[which], RING.WORDS * 4);
    const cap = I[RING.CAP], mask = I[RING.MASK], dev = ac ? ac.sampleRate : 1;
    const mk = new Uint32Array(heap(), ringMarkBases[which], NK_MARK_WORDS);
    if (!cap || !dev) return 0;
    let st = mixState[which];
    /* (Re)anchor: boot, or after a host flush -- which is the only moment the mark table restarts. The
     * reader reads the ring through this closure, so it asks for exactly the frames its taps need. */
    if (!st || st.gen !== I[RING.MARK_GEN]) {
      st = mixState[which] = { gen: I[RING.MARK_GEN], base: I[RING.TAIL], avail: 0, ord: mk[MK.ORD], inPeak: 0 };
      st.reader = NightResample.reader((f, hist, off) => {
        if (f >= st.avail) return false;                  // the host has not produced that frame yet
        const p = ((st.base + f) & st.mask) * 2;
        const l = st.pcm[p], r = st.pcm[p + 1];
        hist[off] = l; hist[off + 1] = r;
        /* peakIn is the run-level peak into the filter; st.inPeak restarts at every re-anchor (a flush for a
         * new transfer, a cdda_play, a stop -- 62 of them in one 45 s opening) and is therefore not a number
         * that can be compared against ringPeak, which accumulates. Comparing the two is what says whether
         * the converter has gain: output peak over input peak, over the same window, must be ~1. */
        const mag = Math.max(Math.abs(l), Math.abs(r));
        if (mag > st.inPeak) st.inPeak = mag;
        if (mag > aud.peakIn[which]) aud.peakIn[which] = mag;
        return true;
      }, mk[mkSlot(st.ord) + 1], dev, { linear: RESAMPLE === 'linear' });   // the base entry's rate
    }
    st.mask = mask;                                       // the heap can grow; the views are rebuilt per call
    st.pcm = new Int16Array(heap(), I[RING.PCM], cap * 2);
    st.avail = (I[RING.HEAD] - st.base) >>> 0;             // frames the host has produced since the anchor
    if (st.avail < aud.minFill[which]) aud.minFill[which] = st.avail;
    if (st.avail <= 0) {
      aud.underrun[which]++;
      /* An empty ring after audio was flowing within the last five blocks is a hole you hear; an empty ring
      * in a scene with nothing to say is what silence sounds like. Only the first is a defect. */
      if (aud.act[which] && pulled.blocks - aud.act[which] <= 5) aud.drop[which]++;
      return 0;
    }
    aud.act[which] = pulled.blocks;
    if (st.avail < Math.ceil(n * st.reader.getRatio())) aud.short[which]++;
    /* A mark the page has handed to the reader can be trimmed by the host at any time, so the only loss that
     * counts is an entry below the host's live window that the page never handed on. `st.ord` is the highest
     * ordinal already given to the reader, and the base can advance underneath it legitimately. */
    if (st.ord + 1 < mk[MK.ORD]) {                        // a change the page never got to apply
      aud.markMiss[which]++; st.gen = 0; return 0;        // too slow to be right, so re-anchor instead of guessing
    }
    while (Math.max(st.ord + 1, mk[MK.ORD]) < mk[MK.ORD] + mk[MK.N]) {   // every mark after the base, in order
      st.ord = Math.max(st.ord + 1, mk[MK.ORD]);
      const s = mkSlot(st.ord);
      st.reader.setMark((mk[s] - st.base) >>> 0, mk[s + 1]);
    }
    const k = st.reader.pull(n, L, R);
    /* Release only the frames the filter's window no longer reaches. Releasing everything produced would tell
     * the host the queue is empty and let it refill to the ring's full 372 ms, which would then be the latency. */
    I[RING.TAIL] = st.base + st.reader.released();
    return k;
  }
  /* What left the rings, counted here rather than in wasm: the host counts what it puts in, and the
   * difference between the two is underrun or a stalled page. */
  const pulled = { blocks: 0, sfx: 0, cdda: 0 };
  /* Audio-callback health. A ScriptProcessorNode is serviced by the MAIN THREAD, and the game holds the
   * main thread between yields -- so this is the instrument that says whether what you hear is the port's
   * mixing or the browser not getting a chance. underrun = the ring was empty, short = a block served
   * partly full (a hole), lateMs = how far behind the block's deadline the callback actually ran. */
  const aud = { underrun: [0, 0], drop: [0, 0], act: [0, 0], short: [0, 0], markMiss: [0, 0],
                minFill: [1e9, 1e9], last: 0, lateMs: 0, maxLateMs: 0, lateBlocks: 0, drift: [0, 0],
                /* bothBlocks = output blocks in which BOTH rings contributed; ringPeak = what each voice
                 * actually contributed to the output, in float, measured only during those blocks; clip =
                 * samples clamped by the sum. These three exist because a swallowed voice is invisible to
                 * every counter that counts frames -- see mixBoth. */
                bothBlocks: 0, ringPeak: [0, 0], clip: 0, peakIn: [0, 0] };
  /* One mixing pass, both rings into the output. This used to be two mixRing calls back to back into L/R, and
   * that is where "an effect cannot be heard while the music is playing" came from: resample.js ACCUMULATES
   * (`outL[k] += a`), so the two rings do sum -- but each call then scaled the whole buffer by 1/32768, and
   * the second call therefore scaled the FIRST ring's samples a second time. An effect mixed into a block that
   * also held CD-DA came out 32768× below the music it was supposed to sit under, i.e. silent; the same effect
   * in a block where the disc ring had run dry was at the right level. Hence the symptom exactly as reported:
   * effects fine on their own, gone under the music -- and it was the sink path too, so every counter that
   * counts frames (pulled, inPeak, drop) stayed green while the output lost one of the two voices. inPeak is
   * measured at the filter's INPUT; nothing here had ever measured the output. */
  const scr = [{ l: null, r: null }, { l: null, r: null }];
  function scratch(i, n) {
    /* Lower-case l/r/k because that is the shape resample.js's mix() takes -- the first version of this passed
     * the page's capitalised scratch buffers to it and the audio callback threw on the first sample. */
    const s = scr[i];
    if (!s.l || s.l.length < n) { s.l = new Float32Array(n); s.r = new Float32Array(n); }
    return s;
  }
  function mixBoth(L, R, n) {
    const name = ['sfx', 'cdda'];
    for (let i = 0; i < 2; i++) {
      const s = scratch(i, n);
      s.l.fill(0, 0, n); s.r.fill(0, 0, n);
      s.k = mixRing(i === 0 ? RING_SFX : RING_CDDA, s.l, s.r, n);
      pulled[name[i]] += s.k;
    }
    /* Each voice goes into its own buffer and the SUM is scaled once, in resample.js -- where `make resample`
     * can hold the mixer to a level, rather than in a page callback that nothing outside a browser can test.
     * ringPeak is what each voice actually contributed to the OUTPUT, and only during the blocks where both
     * were present: the measurement that says "an effect is audible under the music", which no frame counter
     * can make, because frames flowing is not the same as sound coming out. */
    const m = NightResample.mix(L, R, n, scr[0], scr[1]);
    aud.clip += m.clip;
    if (m.both) {
      aud.bothBlocks++;
      if (m.peakA > aud.ringPeak[0]) aud.ringPeak[0] = m.peakA;
      if (m.peakB > aud.ringPeak[1]) aud.ringPeak[1] = m.peakB;
    }
    return scr[0].k + scr[1].k;
  }

  function audioPull(e) {
    if (!M) return;
    const now = performance.now();
    if (aud.last) {
      const late = (now - aud.last) - (e.outputBuffer.length / ac.sampleRate) * 1000;
      aud.lateMs += Math.max(0, late);
      if (late > 5) aud.lateBlocks++;
      if (late > aud.maxLateMs) aud.maxLateMs = late;
    }
    aud.last = now;
    const L = e.outputBuffer.getChannelData(0), R = e.outputBuffer.getChannelData(1);
    const n = L.length;
    L.fill(0); R.fill(0);
    pulled.blocks++;
    if (muted) return;
    mixBoth(L, R, n);
  }
  /* ?sink=1: pull the rings into a scratch buffer on a timer, with no output device at all. Headless
   * Chrome has none (its AudioContext errors out), and the ring/budget maths is the part worth testing
   * there -- this keeps it under test without a speaker. It also means a browser whose audio never
   * starts still drains the rings like a device would. */
  function startSink() {
    const L = new Float32Array(1024), R = new Float32Array(1024);
    setInterval(() => {
      if (!M || muted) return;
      L.fill(0); R.fill(0);
      pulled.blocks++;
      mixBoth(L, R, 1024);
    }, 20);
    logLine('audio: silent sink at 20 ms (no output device)');
  }

  /* ---- the data, into the in-memory filesystem before the game starts ---- */
  async function fetchBytes(url, onChunk) {
    const r = await fetch(url);
    if (!r.ok) throw new Error(url + ': ' + r.status);
    const total = +(r.headers.get('content-length') || 0);
    if (!r.body || !total || !onChunk) { if (onChunk) onChunk(1); return new Uint8Array(await r.arrayBuffer()); }
    const reader = r.body.getReader();
    const out = new Uint8Array(total);
    let got = 0;
    for (;;) {
      const { done, value } = await reader.read();
      if (done) break;
      out.set(value, got); got += value.length;
      onChunk(got / total);
    }
    return out.subarray(0, got);
  }
  async function loadRes() {
    say('downloading NIGHT.RES (22 MB gzipped, 51 MB in memory)');
    const gz = await fetchBytes('data/NIGHT.RES.gz', (f) => progress(f * 0.8));
    let bytes = gz;
    if (typeof DecompressionStream !== 'undefined') {
      gate('inflating NIGHT.RES');
      const ds = new DecompressionStream('gzip');
      const buf = await new Response(new Blob([gz]).stream().pipeThrough(ds)).arrayBuffer();
      bytes = new Uint8Array(buf);
    }
    /* The DOS loader opens "NIGHT.RES" relative to the directory the host names, so the game's view of
     * the data is this directory -- and it is a filesystem, not URLs: cd_iso reads the CUE with fopen. */
    M.FS.mkdirTree('/data');
    M.FS.writeFile('/data/NIGHT.RES', bytes);
    const cue = await fetchBytes('data/TG_NIGHT.CUE');
    M.FS.writeFile('/data/TG_NIGHT.CUE', cue);
    progress(1);
    return bytes.length;
  }

  /* ---- the status readout, from memory ---- */
  function pollStatus() {
    if (!M) return;
    const H = M.HEAPU32, b = blockBase >> 2;
    const frames = H[b + ST.FRAMES], exit = H[b + ST.EXIT];
    const disc = H[b + ST.DISC] ? 'disc' : 'no disc';
    const kb = (H[b + ST.CDDA] * 4 / 1024) | 0;
    const audio = !ac ? 'no device' : Math.round(ac.sampleRate / 100) / 10 + 'k ' + ac.state;
    /* The old "click for sound" badge became this clause: while the device is not running the HUD says so
     * where the player is looking, and it stops saying it by itself the moment the device runs. */
    say('frame ' + frames + ' · ' + (H[b + ST.VID_W] || '?') + 'x' + (H[b + ST.VID_H] || '?') +
        ' · ' + H[b + ST.TRACKS] + ' CD tracks · ' + kb + ' KB streamed · ' + audio +
        (ac && ac.state !== 'running' ? ' · PRESS FOR SOUND' : '') +
        ' · ' + (((pulled.sfx + pulled.cdda) * 4 / 1024) | 0) + ' KB played · ' + disc +
        (exit === 2 ? ' · the game ended' : ''));
    if (exit === 2) $('end').classList.remove('hidden');
  }

  /* ---- boot ---- */
  async function boot(moduleSrc) {
    gate('loading engine');
    const script = document.createElement('script');
    script.src = moduleSrc;
    document.head.appendChild(script);
    await new Promise((res, rej) => {
      script.onload = res;
      script.onerror = () => rej(new Error('failed to load ' + moduleSrc));
    });
    if (typeof NightModule !== 'function') throw new Error('NightModule is not defined -- built without -sMODULARIZE');

    M = await NightModule({
      print: logLine,
      printErr: logLine,
      onAbort: (why) => say('aborted: ' + why),
    });
    blockBase = M._nk_block_addr();
    /* Queue depth is the page's to ask for, because it is the page that has to survive its own stalls. The
     * audio callback runs on the main thread and the game holds that for tens of milliseconds at a stretch
     * (measured: up to 60 ms), so a shallower queue than the worst stall is a hole you hear.
     *
     * 300 ms on the effects is not a latency taste, it is a drop count: au_start() copies a whole DMA
     * transfer into the board's ring at arm time, so a scene with many sounds queues them in bulk, and with
     * a 60 ms queue the intro scene threw away 28030 bytes of effects in 45 s. The desktop host lets the SDL
     * stream hold up to 96 KB -- about half a second -- before it stops pulling, so this matches it.
     * `?sfxms=` / `?cddams=` tune it. */
    for (const [name, word, dflt] of [['sfxms', CFG.SFX_MS, 300], ['cddams', CFG.CDDA_MS, 120]]) {
      const v = Number(new URLSearchParams(location.search).get(name) || 0);
      M.HEAPU32[(blockBase >> 2) + word] = v > 0 ? Math.min(v, 400) : dflt;
      logLine('queue: ' + name + ' ' + M.HEAPU32[(blockBase >> 2) + word] + ' ms');
    }
    /* The A0000 window charge, which is what paces the melting title, the logo fade and the room's page flip
     * (docs/72). `?vgaws=0` is the old free-write build; anything else is ns/byte, 63 being the desktop host's
     * calibrated default that the host itself uses when the parameter is absent. */
    const qs = new URLSearchParams(location.search);
    const argv = ['--res-dir', '/data', '--cue', '/data/TG_NIGHT.CUE',
                  '--cd-dir', '/data/cd', '--cd-url', 'data/cd'];
    if (qs.has('vgaws')) argv.push('--vgaws', String(Math.max(0, Number(qs.get('vgaws')) || 0)));
    if (qs.has('chargeus')) argv.push('--chargeus', String(Math.max(0, Number(qs.get('chargeus')) || 0)));
    if (qs.has('diagms')) argv.push('--diagms', String(Math.max(100, Number(qs.get('diagms')) || 5000)));
    /* `?skipintro=1` skips the logo hold and the opening movie, like the desktop host's --skip-intro. The page
     * still boots through the title card; it exists so a headless run can reach the room loop, and with it the
     * pause menu, inside a test's window. */
    if (qs.has('skipintro')) argv.push('--skip-intro');
    /* `?bgmoff=15000` clears the game's BGM enable byte at that moment, i.e. it simulates the one press of the
     * pause menu's BGM MODE row that a headless run cannot reach (docs/459). */
    if (qs.has('bgmoff')) argv.push('--bgmoff', String(Math.max(0, Number(qs.get('bgmoff')) || 0)));
    gameArgv = argv;
    ringBases = [M._nk_ring_addr(RING_SFX), M._nk_ring_addr(RING_CDDA)];
    ringMarkBases = [M._nk_ring_marks_addr(RING_SFX), M._nk_ring_marks_addr(RING_CDDA)];
    logLine('block ' + blockBase + ' rings ' + ringBases[0] + '/' + ringBases[1] +
            ' marks ' + ringMarkBases[0] + '/' + ringMarkBases[1] + ' resample ' + RESAMPLE);

    const n = await loadRes();
    logLine('NIGHT.RES: ' + n + ' bytes in the in-memory filesystem');
    preloaded = true;

    /* A browser will not let a page make a sound until the user has pressed something -- the AudioContext comes
     * up `suspended` and every audio counter then reads a healthy zero, which is how two measurement runs
     * here reported silence as a result. So the game sits behind a gate and this press is the user activation:
     * it opens the context AND starts the game, so the first thing the player hears is the game's own first
     * sound, not a sound they had to wait for a second press to get. Everything slow -- the module, the 20 MB
     * resource pack -- happens behind the gate, so the press itself is instant.
     *
     * ?sink=1 (no output device; the ring maths still runs on a timer) and ?auto=1 skip the gate: the headless
     * harness has no finger, and a check that needs a gesture to start is a check that tests its own harness. */
    if (gateStart) {
      gateStart.disabled = false;
      gateStart.textContent = 'press to play';
    }
    if (AUTO_START) { await startGame(); }
    else { gate('ready'); }
  }

  /* The press starts the game: the AudioContext is created inside the gesture, so it starts running rather
   * than suspended, and the host learns the device rate before its first ring write. */
  /* The press starts the game: the AudioContext is created inside the gesture, so it starts running rather
   * than suspended, and the host learns the device rate before its first ring write. */
  let started = false;
  let gameArgv = null;                      // built in boot(), once the ?vgaws= knob has been read
  async function startGame() {
    if (started || !M) return;
    started = true;
    if (gateEl) gateEl.classList.add('hidden');
    ensureAudio();                                // inside the gesture: this is the activation that counts
    if (location.search.indexOf('sink=1') >= 0) startSink();
    say('starting');
    fit();                                        // the canvas box is final once the gate is gone
    M.callMain(gameArgv || ['--res-dir', '/data', '--cue', '/data/TG_NIGHT.CUE',
                            '--cd-dir', '/data/cd', '--cd-url', 'data/cd']);
    setInterval(pollStatus, 250);
  }

  /* ---- controls ---- */
  function wire() {
    if (scaleEl) scaleEl.addEventListener('click', () => setStretch(!stretch));
    showScale();
    $('btn-full').addEventListener('click', () => {
      const el = document.documentElement;
      if (document.fullscreenElement) document.exitFullscreen();
      else el.requestFullscreen && el.requestFullscreen();
    });
    $('btn-mute').addEventListener('click', () => {
      muted = !muted;
      if (M) M.HEAPU32[(blockBase >> 2) + CFG.MUTED] = muted ? 1 : 0;
      $('btn-mute').textContent = muted ? 'unmute' : 'mute';
    });
    $('btn-quit').addEventListener('click', () => {
      if (M) M.HEAPU32[(blockBase >> 2) + CFG.WANT_QUIT] = 1;
    });
    $('btn-log').addEventListener('click', () => {
      logOn = !logOn;
      logEl.classList.toggle('hidden', !logOn);
      $('btn-log').textContent = logOn ? 'hide log' : 'log';
      fit();                                      /* the log took space the canvas now has less of (or vice versa) */
    });
    document.addEventListener('pointerdown', ensureAudioOnce);   // any press re-tries a blocked AudioContext
    document.addEventListener('fullscreenchange', fit);
    /* The gate's press: user activation for the AudioContext and the game's start, in one press. */
    if (gateStart) gateStart.addEventListener('click', () => { startGame(); });
  }

  window.NightWasm = {
    boot, present, nextFrame, module: () => M, fit, startSink, startGame,
    /* The harness needs to know the gate is armed before it presses it, and needs to tell "the game has not
     * started" from "the game started and heard nothing". */
    ready: () => ({ preloaded, started, gate: gateEl ? !gateEl.classList.contains('hidden') : false }),
    stats: () => {
      /* inPeak = the loudest frame the filter was actually handed, per ring, in int16. It is the counter that
       * catches "the rings were full and nothing was heard": underrun counts say nothing about a reader that
       * consumed the ring four times too fast, which is exactly the bug this file had. */
      aud.inPeak = mixState.map((st) => (st ? st.inPeak : 0));
      return { frames: M ? M.HEAPU32[(blockBase >> 2) + ST.FRAMES] : 0,
                    keys: M ? M.HEAPU32[(blockBase >> 2) + ST.KEYS] : 0,
                    flags: M ? M.HEAPU32[(blockBase >> 2) + ST.FLAGS] : 0,
                    dev: M ? M.HEAPU32[(blockBase >> 2) + CFG.AUDIO_RATE] : 0,
                    pulled, seen, raf, timer, blit, aud, ac: ac ? ac.state : 'none',
                    canvas: [canvas.width, canvas.height, canvas.clientWidth, canvas.clientHeight] };
    },
  };

  if (document.readyState === 'loading') window.addEventListener('DOMContentLoaded', () => { wire(); boot('night.js').catch(e => say('boot failed: ' + e.message)); });
  else { wire(); boot('night.js').catch(e => say('boot failed: ' + e.message)); }
})();
