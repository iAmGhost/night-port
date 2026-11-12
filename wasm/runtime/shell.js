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
  const canvas = $('screen'), statusEl = $('status'), logEl = $('log'), hudEl = $('hud');
  const holderEl = $('screen-holder'), scaleEl = $('btn-scale'), padEl = $('pad'), padBtnEl = $('btn-pad');
  const gateEl = $('gate'), gateStart = $('btn-start'), gateStatusEl = $('gate-status'),
        gatePadEl = $('gate-pad');

  /* `say` writes the HUD line; `gate` writes the gate's line while it is still the thing being looked at.
   * Two lines because the preload runs while the overlay covers the canvas, and a status nobody can see
   * during the one wait of the session is a status that does not exist. */
  let gateMsg = '';
  function gate(msg) {
    gateMsg = msg;
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
  /* There is no progress bar across the top any more: what it measured now rides on the gate's own line,
   * which is the line a player waiting on a 20 MB download is actually looking at. */
  function progress(frac) {
    if (!gateEl || gateEl.classList.contains('hidden') || !gateStatusEl || !gateMsg) return;
    gateStatusEl.textContent = gateMsg + '  ' + Math.round(Math.max(0, Math.min(1, frac)) * 100) + '%';
  }
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
  /* How tall the bottom bar is, as CSS -- because the only thing that has to know it is float mode, which
   * parks the deck above it, and it is a number CSS cannot derive: the bar is two rows on a narrow screen
   * (the status line takes a row of its own there), one row on a wide one, and `display: none` -- zero --
   * once a touch has put it away. So it is measured, and measured again on every layout change, which is
   * what makes a rotation, a font change and a hidden bar all need no special case. */
  function measureBar() {
    if (!hudEl || !document.documentElement.style.setProperty) return 0;
    const h = Math.round(hudEl.getBoundingClientRect().height);
    document.documentElement.style.setProperty('--bar-h', h + 'px');
    return h;
  }
  function showScale() {
    if (scaleEl) { scaleEl.textContent = stretch ? 'fit' : 'stretch';
                   scaleEl.title = stretch ? 'keep the bitmap ratio (integer scale)'
                                           : 'stretch to 4:3, the ratio the original showed'; }
    canvas.style.imageRendering = 'pixelated';      /* both modes: no blurring at any scale */
    if (scaleEl) scaleEl.classList.toggle('off', !stretch);
  }
  function fit() {
    measureBar();                                 /* float mode parks the deck above the bar: it needs the number */
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
  /* ---- turning the phone ----
   * A rotation is the one moment the whole canvas box changes at once, and the notice you get about it is
   * unreliable: iOS sends `orientationchange` before the layout has settled, so a fit() run from it measures
   * the box the phone just left; Chrome sometimes sends only `resize`; a toolbar collapsing or a soft
   * keyboard moves the box with no orientation event at all. So every one of them asks for the same thing,
   * which is one re-fit per frame, and that re-fit measures twice -- the second pass, a frame later, is what
   * fixes a rotation measured in the middle. The pad's own layout changes with the orientation too (band
   * stacks, float moves to the corners), and that is a change to the box as much as a rotation is. */
  let refitQueued = 0;
  function refit() {
    if (refitQueued) return;
    refitQueued = requestAnimationFrame(() => {
      refitQueued = 0;
      fit();
      requestAnimationFrame(fit);
    });
  }
  window.addEventListener('resize', refit);
  window.addEventListener('orientationchange', refit);
  if (window.visualViewport) {
    visualViewport.addEventListener('resize', refit);
    visualViewport.addEventListener('scroll', refit);
  }
  const orientMQ = window.matchMedia && matchMedia('(orientation: landscape)');
  if (orientMQ) { if (orientMQ.addEventListener) orientMQ.addEventListener('change', refit);
                  else orientMQ.addListener(refit); }
  if (screen && screen.orientation && screen.orientation.addEventListener)
    screen.orientation.addEventListener('change', refit);

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
    if (!sc) return;
    keyPushSc(sc, make);
  }
  /* The one writer of the key queue, for the keyboard path and the pad alike. A code the DOS table does not
   * have is not a key the game was ever written to receive, so it is dropped rather than guessed at. */
  function keyPushSc(sc, make) {
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
  window.addEventListener('blur', () => { gpReleaseAll(); padReleaseAll(); for (const c in DOS) keyPush(c, false); });

  /* ---- the virtual pad: a touch front end for those same bytes ----
   * There is no new input path here, which is the point: every press and release is one slot in the key
   * queue, carrying the DOS make code the keyboard would have sent for the same physical key, so the pad
   * cannot drift from the keyboard as far as keymap.c is concerned (docs/91, 92 -- including the aliases,
   * since Z/X/C land on the CTRL/ALT/SHIFT slots in the guest, not in this file).
   *
   * Directions are read from GEOMETRY rather than from four buttons. The game's move model is two held
   * bytes (0x4b plus 0x48 is a diagonal), so a pad that only had four arms would be missing half of the
   * verbs a thumb can reach. One pointer therefore sweeps the 3x3 grid the CSS draws -- slide from an arm
   * into a corner and you get both bytes -- and a second pointer adds its own directions to the union,
   * which is how a thumb and a finger walk diagonally while punching.
   *
   * A held virtual key sends ONE make, exactly as the keyboard path does (which drops e.repeat): the
   * screens that need a repeating cursor -- fn_038f3d's row cursor repeats a HELD key every 0x13 frames,
   * docs/65 -- earn it from the held byte inside the guest, not from the host. DOS typematic repeat is a
   * different question again (docs/272, 281, 282) and is deliberately not synthesised here. */
  const dpadEl = $('dpad'), padKeysEl = $('pad-keys'), padSysEl = $('pad-sys');
  const PAD_KEY = 'night.pad';
  const PAD_MODES = ['band', 'float', 'off'];
  const qsPad = new URLSearchParams(location.search).get('pad');
  const TOUCHY = !!(window.matchMedia && matchMedia('(hover: none) and (pointer: coarse)').matches);
  /* Touch gets the pad without being asked (a phone has no other keyboard), a mouse does not, and
   * `?pad=band|float|off` forces one for a run. The choice persists beside the scale mode. */
  let padMode = PAD_MODES.indexOf(qsPad) >= 0 ? qsPad
              : ((localStorage.getItem(PAD_KEY) || (TOUCHY ? 'band' : 'off')));
  if (PAD_MODES.indexOf(padMode) < 0) padMode = 'off';
  const padSeen = { down: 0, up: 0 };           // presses the pad made, counted for the harness
  const padPtr = new Map();                     // pointerId -> the codes it holds (empty = on the cross, in
                                               // ... its dead centre: still a finger this page is watching)
  const padPad = new Set();                     // the pointers that went down ON the cross
  const padSent = new Set();                    // the union of them: what the game believes is held
  const padCells = dpadEl ? Array.from(dpadEl.children) : [];
  const padBtns = Array.from(document.querySelectorAll('#pad-keys [data-key], #pad-sys [data-key]'));
  const DIR_SC = { up: DOS.ArrowUp, down: DOS.ArrowDown, left: DOS.ArrowLeft, right: DOS.ArrowRight };
  /* The dead zone is the centre cell of the grid exactly, so what you see is what you press. */
  const PAD_DZ = 1 / 3;
  function dirsAt(clientX, clientY) {
    const r = dpadEl.getBoundingClientRect();
    const dx = (clientX - r.left - r.width / 2) / (r.width / 2);
    const dy = (clientY - r.top - r.height / 2) / (r.height / 2);
    const out = [];
    if (dx < -PAD_DZ) out.push(DIR_SC.left);
    if (dx > PAD_DZ) out.push(DIR_SC.right);
    if (dy < -PAD_DZ) out.push(DIR_SC.up);
    if (dy > PAD_DZ) out.push(DIR_SC.down);
    return out;
  }
  /* A cell lights when the directions it stands for are ALL held, which for a corner means the diagonal is
   * really on -- an arm lighting because its corner shares the byte would be a light that lies.
   *
   * The buttons light when the byte they stand for is held, and "the byte they stand for" needs one
   * allowance to be true: a controller arms the modifier SLOTS (X punch -> 0x1d, A kick -> 0x38) where the
   * deck's own Z and X would send the letters (0x2c, 0x2d), because keymap.c's alias table -- the shipped
   * handler's 0x0003fd9d, docs/91 -- is what makes those the same byte in the game. So holding the
   * controller's punch really does turn on the punch, and a Z lamp that stays dark while the punch it labels
   * is landing is a lamp that lies. The allowance is for the DISPLAY only: the codes pushed into the ring
   * are whatever the source sent, and this table is read by padShow and by nothing else, so if it ever
   * drifts from keymap.c the worst it can do is light the wrong lamp. */
  const KEY_ALIAS = {
    [DOS.KeyZ]: DOS.ControlLeft, [DOS.KeyV]: DOS.ControlLeft,          /* punch -> the CTRL slot */
    [DOS.KeyX]: DOS.AltLeft,     [DOS.KeyB]: DOS.AltLeft,              /* kick  -> the ALT slot  */
    [DOS.KeyC]: DOS.ShiftLeft,   [DOS.KeyF]: DOS.ShiftLeft, [DOS.KeyG]: DOS.ShiftLeft,
  };
  function padHeld(sc) { return padSent.has(sc) || (KEY_ALIAS[sc] !== undefined && padSent.has(KEY_ALIAS[sc])); }
  function padShow() {
    for (const c of padCells) {
      const dirs = (c.dataset.d || '').split(/\s+/).filter(Boolean);
      c.classList.toggle('lit', dirs.length > 0 && dirs.every((d) => padSent.has(DIR_SC[d])));
    }
    for (const b of padBtns) b.classList.toggle('lit', padHeld(DOS[b.dataset.key]));
  }
  /* Diff the union of every live pointer against what the game was last told, and send only the edges. A
   * thumb that slides right -> up-right therefore makes `up` without ever breaking `right`. */
  function padApply() {
    const now = new Set();
    for (const s of padPtr.values()) for (const sc of s) now.add(sc);
    for (const sc of padSent) if (!now.has(sc)) { keyPushSc(sc, false); padSeen.up++; }
    for (const sc of now) if (!padSent.has(sc)) { keyPushSc(sc, true); padSeen.down++; }
    padSent.clear();
    for (const sc of now) padSent.add(sc);
    padShow();
  }
  /* A press is remembered even where the cross reads as nothing. A thumb does not aim at an arm: it lands in
   * the middle of the pad and rolls out to the direction it meant, so a pointer that went down on the hub is
   * watched all the way to the arm it slides into -- which is why `padPad` exists and is not simply "the
   * pointer holds a direction". */
  function padSet(id, codes) { padPtr.set(id, codes); padApply(); }
  /* Both maps have to be cleared, which is why this is two deletes and not an `||`: the short-circuit that
   * made it one expression left the pointer in `padPtr` holding its last direction after the finger was
   * already off the glass -- a d-pad that keeps walking you left after you let go. On a real device the very
   * next event (`lostpointercapture`, which always follows a release when the press captured) called this
   * again and cleared it, so the bug only showed in the harness, where a synthetic press never gets a
   * capture -- which is what a synthetic press is for. */
  function padEnd(id) {
    const on = padPad.delete(id), held = padPtr.delete(id);
    if (on || held) padApply();
  }
  /* Release what the THUMB was holding. A controller's share of `padPtr` is deliberately left alone: the
   * `pad` button hides the touch pad, and the pad you hid is not a pad still holding Right -- but a real
   * controller you are still leaning on is nobody to release just because you wanted the buttons gone. Hiding
   * it here instead would have been the quieter bug: the controller's own state says "still held", so nothing
   * would have been re-sent until the player let go and pressed again. */
  function padReleaseAll() {
    if (!padPtr.size && !padPad.size && !padSent.size) return;
    padPad.clear();
    for (const k of [...padPtr.keys()]) if (typeof k === 'number') padPtr.delete(k);
    padApply();                                 // the union is now only what a controller still holds
  }
  function padGrab(el, id) { try { el.setPointerCapture(id); } catch (e) { /* no capture, no slide */ } }
  function wirePad() {
    if (!padEl) return;
    dpadEl.addEventListener('pointerdown', (e) => {
      e.preventDefault();                       // no scroll, no pinch-zoom, no synthetic click through it
      if (!started) return;                     // before the gate's press there is nothing to press
      padGrab(dpadEl, e.pointerId);
      padPad.add(e.pointerId);
      padSet(e.pointerId, dirsAt(e.clientX, e.clientY));
    });
    dpadEl.addEventListener('pointermove', (e) => {
      if (!padPad.has(e.pointerId)) return;     // a finger that did not press here does not steer here
      padSet(e.pointerId, dirsAt(e.clientX, e.clientY));
    });
    for (const el of padBtns) {
      const sc = DOS[el.dataset.key];
      el.addEventListener('pointerdown', (e) => {
        e.preventDefault();
        if (!started || !sc) return;
        padGrab(el, e.pointerId);
        padSet(e.pointerId, [sc]);
      });
    }
    /* Release comes from three events: a lifted finger, a gesture the browser took away, and a capture that
     * went away on its own. The document sees the cancelled ones too, because an element handler does not
     * fire for a pointer it has already lost -- either way, a hand that left must latch nothing. */
    for (const el of [dpadEl].concat(padBtns))
      for (const ev of ['pointerup', 'pointercancel', 'lostpointercapture'])
        el.addEventListener(ev, (e) => padEnd(e.pointerId));
    document.addEventListener('pointercancel', () => padReleaseAll());
    document.addEventListener('visibilitychange', () => { if (document.hidden) { gpReleaseAll(); padReleaseAll(); } });
    /* The other thing a held thumb brings up on iOS: after a moment on a button it raises the text
     * magnifier over the label and then the copy callout, which sits on top of the game and swallows the
     * press. -webkit-touch-callout:none and user-select:none keep it from starting; these three are the
     * events it asks for, cancelled where they happen.
     *
     * Cancelled on the DOCUMENT and not only on the pad, because iOS 17's loupe does not confine itself to
     * controls: the reported press was on the play area -- a finger resting on the canvas or its letterbox
     * for a whole fight -- and the magnifier came up over the game with no button under it at all. The one
     * surface exempt is the log, which is text a player is meant to copy from: CSS gives its selection
     * back, and this asks the target where it landed before refusing. */
    const inLog = (t) => !!(t && t.closest && t.closest('#log'));
    for (const ev of ['contextmenu', 'selectstart', 'dragstart'])
      document.addEventListener(ev, (e) => { if (!inLog(e.target)) e.preventDefault(); });
    /* The touch half of the same story, which is the half that was missing: iOS runs its long-press and
     * double-tap-to-select recognizers off TOUCH events, and `preventDefault()` on `pointerdown` -- what
     * every cell and button here already does -- says nothing to them. A thumb that taps the cross twice in
     * a row is, to that recognizer, a double click on a word, and it answers with the loupe over the game.
     * One listener on the deck covers every cell and button by bubbling. Nothing the pad does needs the
     * touch event: its whole input path is pointer events, and iOS sends `pointerdown` before `touchstart`
     * and is not stopped by this. The one cost is a fling started on a deck button that happens to float
     * over the log, which is not a scroll anyone means to do. */
    padEl.addEventListener('touchstart', (e) => e.preventDefault(), { passive: false });
  }
  function showPad() {
    if (!padEl) return;
    padEl.classList.toggle('hidden', padMode === 'off');
    document.body.classList.toggle('pad-float', padMode === 'float');
    if (padBtnEl) {                             /* says what pressing it will switch TO, like `fit` */
      const next = PAD_MODES[(PAD_MODES.indexOf(padMode) + 1) % PAD_MODES.length];
      padBtnEl.textContent = next === 'off' ? 'pad off' : 'pad ' + next;
      padBtnEl.title = 'virtual d-pad and buttons, labelled with the DOS key each one sends; tap for ' + next;
      padBtnEl.classList.toggle('off', padMode === 'off');
    }
  }
  function setPadMode(mode) {
    if (PAD_MODES.indexOf(mode) < 0) return;
    padMode = mode;
    try { localStorage.setItem(PAD_KEY, mode); } catch (e) { /* private mode */ }
    if (mode === 'off') padReleaseAll();        // the pad you hid is not a pad still holding Right
    showPad();
    fit();                                      /* the band took space the canvas now has less of */
  }

  /* ---- a real gamepad: pocket's map, into the same queue ----
   * There is no new input path here either, which is the same rule the touch pad follows: every press and
   * release is one slot in the key ring with a DOS make code in it, landed in the shared `padPtr` union and
   * pushed by the same `padApply` the fingers use. So a controller and a thumb cannot disagree about who is
   * holding Right, and `keymap.c` cannot tell the three apart (docs/91, 92).
   *
   * THE MAP IS POCKET'S (docs/333 §1 -- pocket/src/night/main_of.c's g_pad_keys), in its Xbox names -- X
   * punch, A kick, B jump, Y *and* RB duck, START confirm, SELECT pause, LB F1, the cross moves -- and the
   * SDL host drives the same layout, so all three ports of this game answer a controller the same way. That
   * map was measured rather than invented: pocket's census ran the same buttons through the pad path and
   * through `night_key_apply` directly and got the identical 26-write sequence, slot by slot, make/break by
   * make/break. Two of its choices are worth noticing:
   *
   *   - The attacks are the MODIFIER SLOTS themselves (0x1d CTRL = punch, 0x38 ALT = kick, 0x2a SHIFT =
   *     jump), not the letters the touch pad sends. keymap.c's alias table (the shipped handler's
   *     0x0003fd9d) is what turns Z/V into 0x1d in the first place (docs/91), so the two are the same byte
   *     in the guest -- but a controller has no letters printed on it, so there is no reason to go round the
   *     alias to arrive there.
   *   - Duck is held longer than anything else in the game, so it has two homes: Y and RB.
   *
   * THE LEFT THUMBSTICK IS THE D-PAD, to the same two held bytes as the cross: the game's move model is
   * `0x4b` plus `0x48` for a diagonal, and an axis pair reaches both more comfortably than four buttons do.
   * It gets a dead zone of its own (two thresholds, just below), well above the few-hundredths a resting
   * stick reports, and the crossing in and out of it is an edge like any other -- so leaning into a diagonal
   * and back off is what it looks like. The d-pad buttons are read as well, and the two are a union. */
  const GP_POLL_MS = 16;
  /* Two thresholds, not one. A single threshold makes a stick parked on it chatter make/break at the poll
   * rate -- 60 edge pairs a second, straight into the key ring -- and a stick parked near the edge is not a
   * rare thing, it is where a thumb rests after a dash. So a direction comes IN at 0.30 and goes OUT again at
   * 0.22, and the dead zone a player feels is neither number but the gap between them. */
  const GP_STICK_IN = 0.30, GP_STICK_OUT = 0.22;
  const GP_MAP = [
    [2,  DOS.ControlLeft, 'X -> CTRL (punch)'],
    [0,  DOS.AltLeft,     'A -> ALT (kick)'],
    [1,  DOS.ShiftLeft,   'B -> SHIFT (jump)'],
    [3,  DOS.Space,       'Y -> SPACE (duck)'],
    [5,  DOS.Space,       'RB -> SPACE (duck)'],
    [4,  DOS.F1,          'LB -> F1'],
    [8,  DOS.Escape,      'select -> ESC'],
    [9,  DOS.Enter,       'start -> ENTER'],
    [12, DOS.ArrowUp,     'up'], [13, DOS.ArrowDown, 'down'],
    [14, DOS.ArrowLeft,   'left'], [15, DOS.ArrowRight, 'right'],
  ];
  const gpPads = new Map();                     // gamepad index -> { id, mapping, codes: Set of sc } codes it holds
  const gpSeen = { down: 0, up: 0 };            // edges the CONTROLLER made -- see the note in gpPoll
  /* Standard ("W3R") indices are assumed, because that is what everything reports; a pad that says otherwise
   * is logged loudly rather than guessed at, since its buttons mean nothing at these indices. */
  function gpCodes(gp, st) {
    const out = [];
    for (const m of GP_MAP) {
      const b = gp.buttons[m[0]];
      if (b && b.pressed) out.push(m[1]);
    }
    /* axes 0/1 are the left stick. Each direction is compared against its OWN signed value, so all four
     * questions are the same question, "is this far that way", with the hysteresis decided per direction --
     * leaning left then swinging right is one make, not a hold that survives the crossing. */
    const ax = gp.axes[0] || 0, ay = gp.axes[1] || 0;
    const dirs = [[DIR_SC.left, -ax], [DIR_SC.right, ax], [DIR_SC.up, -ay], [DIR_SC.down, ay]];
    const stick = new Set();
    for (const d of dirs)
      if (d[1] > (st.stick.has(d[0]) ? GP_STICK_OUT : GP_STICK_IN)) stick.add(d[0]);
    st.stick = stick;
    for (const c of stick) out.push(c);
    return out;
  }
  function gpSame(a, b) {
    if (a.size !== b.size) return false;
    for (const x of a) if (!b.has(x)) return false;
    return true;
  }
  function gpPoll() {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    const live = new Set();
    for (const gp of pads) {
      if (!gp) continue;                                     // the array is sparse
      live.add(gp.index);
      let st = gpPads.get(gp.index);
      if (!st) {
        st = { id: gp.id, mapping: gp.mapping, codes: new Set(), stick: new Set() };
        gpPads.set(gp.index, st);
        logLine('gamepad ' + gp.index + ': ' + gp.id +
                (gp.mapping === 'standard' ? '' : ' [mapping ' + gp.mapping + '!]'));
        logLine('gamepad ' + gp.index + ': ' + GP_MAP.map((m) => m[2]).join(' \u00b7 ') +
                ' \u00b7 left stick = d-pad');
        /* Two ways to say it, because there are two moments a controller appears: before the press, where a
         * line under the gate is the only place it can go, and after, where the toast is. */
        if (gateEl && !gateEl.classList.contains('hidden') && gatePadEl) gatePadEl.classList.remove('hidden');
        else hint('gamepad ready', 1500);
      }
      /* One read of the hardware per pad per tick, and it happens before the started test, because the
       * started test needs to know what is pressed too. */
      const now = new Set(gpCodes(gp, st));
      /* Before the press there is nothing to press -- the same guard the touch pad carries -- with one
       * exception worth making: a controller is a way of playing with no finger on the glass, and a page
       * whose first act is "press this button" should not need a button a controller-owner can reach.
       * So START (or A) starts the game.
       *
       * What it cannot do is grant the activation the browser demands before a page may make a sound: the
       * Gamepad spec does not count a button press as user activation, in any browser, so the AudioContext
       * still comes up locked and the sound waits for the first touch of the screen -- which the hint says
       * out loud, because silence from a game that is visibly running is the worst of the three outcomes a
       * player can be left to guess between. */
      if (!started) {
        if ((now.has(DOS.Enter) || now.has(DOS.ControlLeft)) && gateStart && !gateStart.disabled) {
          logLine('gamepad: start pressed at the gate -- starting (the sound still waits for a touch)');
          startGame();
          if (ac && ac.state !== 'running') hint('tap once for sound', 2600);
        }
        if (st.codes.size) { st.codes = new Set(); padPtr.delete('gp' + gp.index); padApply(); }
        continue;
      }
      if (gpSame(now, st.codes)) continue;
      /* These two count the controller's OWN edges. If a thumb is holding the same byte the ring write
       * belongs to whichever source lost the race, so the count is not a census of the ring -- `pad`'s own
       * down/up are exactly that, and the game's behaviour is the real assertion. */
      for (const sc of st.codes) if (!now.has(sc)) gpSeen.up++;
      for (const sc of now) if (!st.codes.has(sc)) gpSeen.down++;
      st.codes = now;
      padPtr.set('gp' + gp.index, [...now]);
      padApply();
    }
    for (const i of [...gpPads.keys()])
      if (!live.has(i)) {
        gpPads.delete(i);
        padPtr.delete('gp' + i);
        logLine('gamepad ' + i + ' disconnected');
        padApply();                                          // whatever it held is nobody's to hold now
      }
  }
  function gpReleaseAll() {
    if (!gpPads.size) return;
    for (const [i, st] of gpPads) {
      if (!st.codes.size) continue;
      gpSeen.up += st.codes.size;
      st.codes = new Set();
      padPtr.delete('gp' + i);
    }
    padApply();
  }
  function wireGamepad() {
    if (!('getGamepads' in navigator)) { logLine('gamepad: no Gamepad API in this browser'); return; }
    /* The events are the fast notice and the poll is the reliable one. Chrome will not name a controller to
     * a page that has never seen it touched, so a pad connected before the page opened stays invisible until
     * a button is pressed -- and a page that only listened for the event would report no controller to someone
     * whose press arrived between two of its own ticks. */
    for (const [ev, down] of [['gamepadconnected', 1], ['gamepaddisconnected', 0]])
      window.addEventListener(ev, (e) => {
        const gp = e.gamepad;
        if (down) { if (!gpPads.has(gp.index)) gpPoll(); }
        else if (gpPads.has(gp.index)) gpPoll();
      });
    setInterval(gpPoll, GP_POLL_MS);
    /* pocket's pump runs every 8 ms; this runs every frame's worth, which is finer than any human release
     * and cannot lose an edge, because every edge is a difference between two consecutive reads. */
    logLine('gamepad: polling every ' + GP_POLL_MS + ' ms; ' + GP_MAP.length + ' buttons + left stick');
  }

  /* ---- the bottom bar: away when you are playing ----
   * A touch anywhere on the screen box -- the canvas or the letterbox beside it -- puts the row of buttons
   * and its status line away, and a second touch brings it back. It has to be the screen box and not the
   * document: the pad floats over the canvas in float mode, and a press meant for the d-pad is not a request
   * to lose the buttons that switch it off. With the row gone the canvas box is taller, so fit() is the
   * other half of the toggle. */
  const hintEl = $('hint');
  let hintTimer = 0, barOn = true;
  function hint(msg, ms) {
    if (!hintEl) return;
    hintEl.textContent = msg;
    hintEl.classList.remove('hidden');
    clearTimeout(hintTimer);
    hintTimer = setTimeout(() => hintEl.classList.add('hidden'), ms || 1800);
  }
  function setBar(on) {
    barOn = !!on;
    document.body.classList.toggle('no-bar', !barOn);
    fit();                                  /* the height the bar took belongs to the canvas now */
    hint(barOn ? 'bar shown' : 'bar hidden — tap the screen to bring it back', barOn ? 1200 : 2600);
  }

  /* A touch on the screen puts the bar away -- except where the deck is, and where the deck is depends on how
   * the phone is being held.
   *
   * Sideways in float mode the deck is four corners over the screen, so the only place a thumb is NOT is the
   * strip between the cross and the fight buttons: there, and only there, a tap toggles.
   *
   * Upright the deck is a band along the bottom, and the gap in the middle of that band is not free space --
   * it is where a thumb rests between the two groups -- so upright a tap toggles only if it lands ABOVE the
   * deck. (In band mode the deck is never inside the screen box at all, so the same test simply never
   * refuses anything; one rule, both modes.)
   *
   * With the deck switched off, every tap on the screen is just a tap on the screen. */
  function barToggleAt(clientX, clientY) {
    if (padMode === 'off' || !dpadEl || !padKeysEl || !padSysEl) return true;
    const l = dpadEl.getBoundingClientRect(), r = padKeysEl.getBoundingClientRect(),
          s = padSysEl.getBoundingClientRect();
    if (!l.width || !r.width) return true;         /* nothing on screen: nothing to keep clear of */
    const sideways = window.matchMedia && matchMedia('(orientation: landscape)').matches;
    if (padMode === 'float' && sideways) return clientX >= l.right && clientX <= r.left;
    return clientY < Math.min(l.top, r.top, s.top);
  }

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
    gate('downloading NIGHT.RES (22 MB gzipped, 51 MB in memory)');
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

  /* ---- the way out: the game ended, or it stopped drawing ----
   * Two of these doors are the game's own: the `quit` button asks the main to leave, and the end of the
   * program writes NK_ST_EXIT=2. The third door is nobody's, and it is the one that matters: an uncaught
   * exception in the frame pump unwinds straight through the game's own stack (docs/457's CD-DA bug did
   * exactly that), and what you get is a canvas holding its last picture with nothing wrong on its face --
   * the page still pumps, the audio callback still fires, the game is simply gone.
   *
   * A picture that stopped changing is the only symptom all three have in common, so it is the one thing
   * watched: NK_ST_FRAMES has not moved for STALL_MS on a game that started and did not exit. Ten seconds
   * is deliberately long. Every wait this port knows about -- the logo freeze, the 238 s episode card -- is a
   * clock deadline measured in FRAMES (docs/335, 459), so the game presents right through it and the counter
   * runs; a counter that has really stopped for ten seconds is a main that is not coming back.
   *
   * The button matters more than the sentence. In a browser tab there is a reload control; in an installed
   * app on a phone there is not, because the app IS the app, and a game that ends with no door is a game you
   * have to swat away from the multitask screen to play again. */
  const POLL_MS = 250;
  const STALL_MS = +(new URLSearchParams(location.search).get('stallms') || 10000);
  let endWhy = '', lastFrames = -1, stillPolls = 0, lastFault = '';
  function endGame(why, note) {
    const endEl = $('end');
    if (!endEl || endWhy === why) return;
    endWhy = why;
    logLine('end: ' + why + (note ? ' -- ' + note : ''));
    const whyEl = $('end-why'), noteEl = $('end-note');
    if (whyEl) whyEl.textContent = why;
    if (noteEl) noteEl.textContent = note || '';
    endEl.classList.remove('hidden');
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
        (exit === 1 ? ' · leaving' : exit === 2 ? ' · the game ended' : ''));
    if (exit === 2) {
      endGame('the game ended', lastFault ? 'the page also saw: ' + lastFault :
              'a fresh copy of the page starts the game over; its save files live in memory, so nothing carries over');
    } else if (started && frames > 0 && frames === lastFrames) {
      if (++stillPolls * POLL_MS >= STALL_MS) {
        endGame('the game stopped drawing',
                (Math.round(STALL_MS / 1000) + ' s with nothing new on the canvas') +
                (lastFault ? '; the page saw: ' + lastFault : '') +
                ' -- play again loads a fresh copy');
      }
    } else {
      stillPolls = 0;
    }
    lastFrames = frames;
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
    setInterval(pollStatus, POLL_MS);
  }

  /* ---- installed: the manifest, the worker, and the full screen that comes with them ----
   * A browser tab always shows the browser, which for a phone is a bar at the top eating the height the
   * canvas wants. A page installed from its manifest gets a launch with no chrome at all -- the only full
   * screen iOS will ever give a web page -- so the port ships a manifest, icons cut from the original's own
   * 32x32 icon, and a worker small enough that it only ever answers the shell (docs/457).
   *
   * Three consequences of "we are the app now", all of them here:
   *
   *   The bottom bar starts hidden. Nothing about an app justifies a row of debug buttons under the game; a
   *   touch on the screen still brings it back.
   *   The fullscreen button says what it does. It asks for the document's full screen, which Chrome and
   *   Firefox honour and iOS Safari does not offer at all -- so it also says how to get that screen instead.
   *   The install button exists only when the browser can actually be asked. Chrome fires
   *   `beforeinstallprompt`; Safari has no such event and wants a finger on Share -> Add to Home Screen, and
   *   pretending otherwise would be a button that does nothing.
   */
  const installEl = $('btn-install'), fullEl = $('btn-full'), installHintEl = $('gate-install');
  const STANDALONE = !!(window.matchMedia && (matchMedia('(display-mode: standalone)').matches ||
                                              matchMedia('(display-mode: fullscreen)').matches)) ||
                     navigator.standalone === true;      /* the iOS sign, and the only one it gives */
  const CAN_FULL = !!(document.documentElement.requestFullscreen);
  let appInstalled = STANDALONE, deferredPrompt = null;

  function showInstall() {
    if (fullEl) {
      fullEl.classList.toggle('hidden', STANDALONE);     /* already the whole screen: nothing left to ask */
      fullEl.textContent = document.fullscreenElement ? 'exit full' : 'fullscreen';
      fullEl.title = CAN_FULL ? 'fill this window (the browser keeps its own bar)'
                               : 'this browser will not let a page go full screen -- install it instead';
    }
    if (installEl) installEl.classList.toggle('hidden', !deferredPrompt || STANDALONE);
    if (!installHintEl) return;
    const IOS = /iPad|iPhone|iPod/.test(navigator.userAgent) ||
                (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    const msg = appInstalled
      ? 'installed as an app: full screen, and the pad is where your thumbs are'
      : (deferredPrompt ? '' : (IOS ? 'to install: Share -> Add to Home Screen (that is the full screen)'
                                    : 'to install: the browser menu -> Add to Home Screen'));
    installHintEl.textContent = msg;
    installHintEl.classList.toggle('hidden', msg === '');
  }

  /* The worker: registered only where it is allowed, and skippable. `?nosw=1` is the opt-out, and a
   * measurement run is NOT one of them by default: `make check` is a better test for having gone through a
   * service worker than for having gone around one, and the worker is network-first precisely so that it
   * cannot serve the previous build. What it does skip is a file:// page, which gets no worker anyway. */
  function registerWorker() {
    if (!('serviceWorker' in navigator)) { logLine('pwa: no service worker in this browser'); return; }
    if (/[?&]nosw=1/.test(location.search)) { logLine('pwa: worker skipped (?nosw=1)'); return; }
    const host = location.hostname;
    if (!/^https?:$/.test(location.protocol) ||
        !(location.protocol === 'https:' || host === 'localhost' || host === '127.0.0.1' || host === '[::1]')) {
      logLine('pwa: worker needs https or localhost, not ' + location.host); return;
    }
    navigator.serviceWorker.register('sw.js').then(
      (r) => logLine('pwa: worker active, scope ' + r.scope),
      (e) => logLine('pwa: worker registration failed: ' + e.message));
  }

  function wirePwa() {
    window.addEventListener('beforeinstallprompt', (e) => {
      e.preventDefault();                        // the browser's own banner competes with a button that logs
      deferredPrompt = e; showInstall();
      logLine('pwa: installable (beforeinstallprompt)');
    });
    window.addEventListener('appinstalled', () => {
      appInstalled = true; deferredPrompt = null; showInstall();
      logLine('pwa: installed -- reload for the chrome-free launch');
    });
    if (installEl) installEl.addEventListener('click', () => {
      if (!deferredPrompt) return;
      deferredPrompt.prompt();
      deferredPrompt.userChoice.then((c) => {
        logLine('pwa: install prompt ' + c.outcome);
        deferredPrompt = null; showInstall();
      });
    });
    if (fullEl) fullEl.addEventListener('click', () => {
      if (document.fullscreenElement) { document.exitFullscreen(); return; }
      if (!CAN_FULL) { hint('this browser will not let a page go full screen -- install the app instead', 3200); return; }
      document.documentElement.requestFullscreen({ navigationUI: 'hide' })
        .catch((e) => hint('full screen refused: ' + e.message, 3200));
    });
    document.addEventListener('fullscreenchange', showInstall);
    showInstall();
    registerWorker();
  }

  /* ---- controls ---- */
  function wire() {
    if (scaleEl) scaleEl.addEventListener('click', () => setStretch(!stretch));
    showScale();
    wirePad();
    wireGamepad();
    if (holderEl) holderEl.addEventListener('pointerdown', (e) => {
      /* The refusal is logged and the toggle is not: a control that does nothing where the deck is, is a
       * control whose refusal has to be findable, and a bar that visibly disappeared needs no diary. */
      if (!barToggleAt(e.clientX, e.clientY)) {
        logLine('bar: tap at ' + Math.round(e.clientX) + ',' + Math.round(e.clientY) + ' is the deck, ignored');
        return;
      }
      setBar(!barOn);
    });
    /* The play area's own half of "no magnifier" (see wirePad): consuming a touch at its start is the one
     * thing iOS's long-press recognizer has to be told before it begins, and the screen box is where a
     * thumb rests for a whole fight. Nothing is lost by consuming it -- this box scrolls nothing (CSS
     * `touch-action: none`), and the holder's only control is the pointerdown above, which iOS sends
     * before the touch event anyway. The deck has its own line for the same reason (see wirePad). */
    if (holderEl) holderEl.addEventListener('touchstart', (e) => e.preventDefault(), { passive: false });
    if (padBtnEl) padBtnEl.addEventListener('click', () =>
      setPadMode(PAD_MODES[(PAD_MODES.indexOf(padMode) + 1) % PAD_MODES.length]));
    showPad();
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
    /* The way out of the end-of-game box. `location.reload()` is the whole handler, and it is worth
     * knowing why a button is needed for something so small: in a tab the browser supplies a reload, and in
     * an installed app nothing does. */
    if ($('btn-again')) $('btn-again').addEventListener('click', () => location.reload());
    /* The third door: an exception that gets away. Logged with the rest of the host's log so the `log` panel
     * still tells the story, and remembered, because the honest thing the end box can say is "the page saw
     * this" rather than "the game stopped" on its own. */
    window.addEventListener('error', (e) => {
      lastFault = (e && e.message) || 'uncaught error';
      logLine('fault: ' + lastFault);
    });
    window.addEventListener('unhandledrejection', (e) => {
      lastFault = (e && e.reason && (e.reason.message || String(e.reason))) || 'unhandled promise rejection';
      logLine('fault: ' + lastFault);
    });
    $('btn-log').addEventListener('click', () => {
      logOn = !logOn;
      logEl.classList.toggle('hidden', !logOn);
      $('btn-log').textContent = logOn ? 'hide log' : 'log';
      fit();                                      /* the log took space the canvas now has less of (or vice versa) */
    });
    /* The other half of "double tapping zooms": iOS Safari's pinch is its own gesturestart/gesturechange
     * pair, which no touch-action stops -- only cancelling it does -- and a page with no double click in it
     * has no reason to let a double tap change the zoom. Both are page-wide, not pad-only, because a zoom
     * you cannot undo in the middle of a fight costs the run either way. */
    for (const ev of ['gesturestart', 'gesturechange', 'gestureend'])
      document.addEventListener(ev, (e) => e.preventDefault());
    document.addEventListener('dblclick', (e) => e.preventDefault());
    document.addEventListener('pointerdown', ensureAudioOnce);   // any press re-tries a blocked AudioContext
    document.addEventListener('fullscreenchange', refit);
    wirePwa();
    /* The gate's press: user activation for the AudioContext and the game's start, in one press. */
    if (gateStart) gateStart.addEventListener('click', () => { startGame(); });
  }

  window.NightWasm = {
    boot, present, nextFrame, module: () => M, fit, startSink, startGame,
    /* The harness drives the pad through the DOM a finger would not need, but the mode has to be reachable
     * from JS for a headless run to exercise the pad's own code path. */
    pad: (mode) => { if (mode !== undefined) setPadMode(mode); return padMode; },
    bar: (on) => { if (on !== undefined) setBar(on); return barOn; },
    pwa: () => ({ standalone: STANDALONE, installed: appInstalled,
                  canPrompt: !!deferredPrompt, canFullscreen: CAN_FULL,
                  fullscreen: !!document.fullscreenElement }),
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
                    pad: { mode: padMode, down: padSeen.down, up: padSeen.up, held: [...padSent] },
                    gp: { connected: gpPads.size, down: gpSeen.down, up: gpSeen.up,
                          pads: [...gpPads.entries()].map(([i, s]) =>
                            ({ index: i, id: s.id, mapping: s.mapping, held: [...s.codes] })) },
                    canvas: [canvas.width, canvas.height, canvas.clientWidth, canvas.clientHeight] };
    },
  };

  if (document.readyState === 'loading') window.addEventListener('DOMContentLoaded', () => { wire(); boot('night.js').catch(e => say('boot failed: ' + e.message)); });
  else { wire(); boot('night.js').catch(e => say('boot failed: ' + e.message)); }
})();
