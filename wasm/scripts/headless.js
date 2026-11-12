#!/usr/bin/env node
/* headless.js -- drive the browser port in headless Chrome over the DevTools protocol, with no npm
 * dependencies (Node's own fetch/WebSocket).  docs/457.
 *
 *   node scripts/headless.js <url> [ms] [screenshot.png] [page.js expression]
 *
 * It is the only way this port is testable from a shell: the build is a requestAnimationFrame-driven
 * loop in a page, so `--dump-dom` races the wasm instantiation and prints nothing, and an assertion
 * about "the game is running" has to be a frame count read out of the shared block after N seconds of
 * real time. The console comes back as the host's log, which is what makes a failure readable.
 */
const { spawn } = require('child_process');
const fs = require('fs');

const CHROME = process.env.CHROME ||
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome';
const url = process.argv[2] || 'http://127.0.0.1:8091/index.html';
const ms = parseInt(process.argv[3] || '15000', 10);
const png = process.argv[4] || '';
const expr = process.argv[5] || '(function(){var M=window.NightWasm.module();if(!M)return "no module";' +
  'var b=M._nk_block_addr()>>2,H=M.HEAPU32;' +
  'return JSON.stringify({frames:H[b+1],w:H[b+2],h:H[b+3],exit:H[b+4],tracks:H[b+5],' +
  'cddaKB:Math.round(H[b+7]*4/1024),keys:H[b+10],disc:H[b+8],keysDropped:H[b+34],' +
  'audio:window.NightWasm.stats()});})()';
const port = parseInt(process.env.CDP_PORT || '9341', 10);
const sleep = (t) => new Promise((r) => setTimeout(r, t));

/* NIGHT_HEADFUL=1 opens a real window with a real audio device. Headless Chrome's WebAudio renderer
 * errors out with no device (`The AudioContext encountered an error from the audio device or the WebAudio
 * renderer'), so the one question headless cannot answer -- does the game actually make a sound -- needs a
 * window. Input gets there the same way either way, over the protocol. */
const headful = !!process.env.NIGHT_HEADFUL;
const flags = ['--no-sandbox', '--autoplay-policy=no-user-gesture-required',
               `--remote-debugging-port=${port}`, '--window-size=960,760',
               `--user-data-dir=/tmp/night-cdp-${port}`, 'about:blank'];
if (!headful) flags.unshift('--headless=new', '--disable-gpu', '--mute-audio');
/* Chrome throttles a window it thinks is occluded -- rAF nearly stops and intervals drop to 1 Hz -- and a
 * test window behind the terminal is exactly that. These three turn the throttle off; a visitor who puts
 * the game behind another window really does get a slow game, which is how every browser game behaves. */
flags.push('--disable-backgrounding-occluded-windows', '--disable-renderer-backgrounding',
           '--disable-background-timer-throttling');

/* NIGHT_KEYS="Space:200 Enter:150 ArrowRight:400" -- real key events through the protocol, delivered at
 * the midpoint of the run. The game's own behaviour is the assertion; the count in the shared block is
 * what says the bytes reached it. */
const keys = (process.env.NIGHT_KEYS || '').split(/[ ,]+/).filter(Boolean);

const chrome = spawn(CHROME, flags, { stdio: ['ignore', 'ignore', 'pipe'] });
chrome.stderr.on('data', () => {});

(async () => {
  let target = null;
  for (let i = 0; i < 150 && !target; i++) {
    await sleep(100);
    try {
      const j = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
      const p = j.find((t) => t.type === 'page');
      if (p && p.webSocketDebuggerUrl) target = p;
    } catch (e) { /* chrome not listening yet */ }
  }
  /* A fresh profile in a windowed Chrome can come up with no page target at all, so ask for one rather
   * than giving up on the only route that has an audio device. */
  if (!target) {
    try {
      const j = await (await fetch(`http://127.0.0.1:${port}/json/new?about%3Ablank`, { method: 'PUT' })).json();
      target = j;
    } catch (e) { /* fall through to the error below */ }
  }
  if (!target) throw new Error('no DevTools target');

  const ws = new WebSocket(target.webSocketDebuggerUrl);
  let id = 1;
  const pending = new Map();
  const logs = [];
  const send = (method, params = {}) => new Promise((res, rej) => {
    const my = id++;
    pending.set(my, { res, rej });
    ws.send(JSON.stringify({ id: my, method, params }));
  });
  ws.onmessage = (ev) => {
    const m = JSON.parse(ev.data);
    if (m.id && pending.has(m.id)) {
      const p = pending.get(m.id); pending.delete(m.id);
      if (m.error) p.rej(new Error(JSON.stringify(m.error))); else p.res(m.result);
      return;
    }
    if (m.method === 'Runtime.consoleAPICalled') {
      logs.push(m.params.args.map((a) => (a.value !== undefined ? String(a.value)
        : (a.unserializableValue || a.type))).join(' '));
    } else if (m.method === 'Runtime.exceptionThrown') {
      const d = m.params.exceptionDetails;
      logs.push('EXCEPTION: ' + ((d.exception && (d.exception.description || d.exception.value)) || d.text));
    } else if (m.method === 'Log.entryAdded' && m.params.entry.level !== 'info') {
      logs.push(`[${m.params.entry.level}] ${m.params.entry.text}`);
    }
  };
  await new Promise((r) => { ws.onopen = r; });
  await send('Page.enable'); await send('Runtime.enable'); await send('Log.enable');
  await send('Page.navigate', { url });

  /* The DOS make codes are keyed by the same table shell.js uses, only here for the protocol fields. */
  const VK = { Space: [32, ' '], Enter: [13, 'Enter'], Escape: [27, 'Escape'], F1: [112, 'F1'],
               ArrowLeft: [37, 'ArrowLeft'], ArrowUp: [38, 'ArrowUp'], ArrowRight: [39, 'ArrowRight'],
               ArrowDown: [40, 'ArrowDown'], KeyZ: [90, 'z'], KeyX: [88, 'x'], KeyC: [67, 'c'],
               KeyQ: [81, 'q'], KeyE: [69, 'e'], KeyS: [83, 's'] };
  async function press(spec) {
    const [code, hold] = spec.split(':');
    const vk = VK[code] || [0, code];
    const base = { type: 'keyDown', code, key: vk[1], windowsVirtualKeyCode: vk[0], nativeVirtualKeyCode: vk[0] };
    await send('Input.dispatchKeyEvent', base);
    await sleep(parseInt(hold || '80', 10));
    await send('Input.dispatchKeyEvent', { ...base, type: 'keyUp' });
  }
  /* A real pointer press: user activation is what lets the AudioContext start in a browser that does not
   * honour --autoplay-policy, and the audio path is the thing being looked at. One press at page load is
   * not enough to rely on -- measured here, the context came up suspended on about half the runs, because
   * the press lands while the page is still fetching the 20 MB resource pack -- so the list can repeat,
   * `x:y@ms` each, and the default script presses once early and once after the game has booted. */
  const clicks = (process.env.NIGHT_CLICK || '').split(',').map((s) => s.trim()).filter(Boolean);
  async function click(x, y) {
    for (const type of ['mousePressed', 'mouseReleased']) {
      await send('Input.dispatchMouseEvent', { type, x, y, button: 'left', clickCount: 1, buttons: type === 'mousePressed' ? 1 : 0 });
    }
  }
  const early = [];
  for (const spec of clicks) {
    const [pos, at] = spec.split('@');
    const [x, y] = pos.split(':').map(Number);
    if (!at) { await click(x, y); await sleep(400); } else early.push([Number(at), x, y]);
  }
  early.sort((a, b) => a[0] - b[0]);
  await sleep(Math.floor(ms / 2));
  let waited = 0;
  for (const [at, x, y] of early) { await sleep(Math.max(0, at - waited)); waited = at; await click(x, y); }
  for (const spec of keys) { await press(spec); }
  await sleep(Math.ceil(ms / 2));
  let stateValue = null;
  try {
    const r = await send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true });
    stateValue = r.result && r.result.value;
    console.log('STATE: ' + JSON.stringify(stateValue));
    if (r.exceptionDetails) console.log('EXCEPTION: ' + JSON.stringify(r.exceptionDetails.exception || r.exceptionDetails));
    if (r.result && r.result.type !== 'object' && r.result.type !== 'string') console.log('RAW: ' + JSON.stringify(r.result));
  } catch (e) { console.log('EVAL FAILED: ' + e.message); }
  if (png) {
    const s = await send('Page.captureScreenshot', { format: 'png' });
    fs.writeFileSync(png, Buffer.from(s.data, 'base64'));
    console.log('wrote ' + png);
  }

  /* NIGHT_CHECK=1 turns the harness into a test: the numbers above become assertions with an exit code.
   * What is being asserted is not "the picture is right" -- that is what docs/63's asset comparison and a
   * screenshot are for -- but that the machine runs: the loop is driven, the clock advances, the disc
   * answers, the rings drain, and a key lands in the game. */
  if (process.env.NIGHT_CHECK) {
    let st = null;
    try { st = typeof stateValue === 'string' ? JSON.parse(stateValue) : stateValue; } catch (e) { /* keep going */ }
    const bad = [];
    const errs = logs.filter((l) => /^EXCEPTION|^aborted|Aborted|SegmentationError|out of memory/i.test(l));
    if (errs.length) bad.push('console threw: ' + errs[0]);
    if (!st) bad.push('no state object read from the page');
    else {
      const fps = st.frames / (ms / 1000);
      const a = st.audio || {};
      if (st.exit !== 0) bad.push('the game exited with state ' + st.exit);
      if (st.frames < 120) bad.push('only ' + st.frames + ' frames in ' + ms + ' ms');
      if (fps < 20) bad.push(fps.toFixed(1) + ' frames/s');
      if (!st.disc) bad.push('no disc byte source answered any read');
      if (!st.tracks) bad.push('the game never asked the CD to play a track');
      if (!a.pulled || a.pulled.blocks <= 0) bad.push('nothing drained the PCM rings');
      if (!st.keys) bad.push('no key event reached the game');
      if (!a.canvas || a.canvas[2] < 320) bad.push('the canvas has no CSS size: ' + JSON.stringify(a.canvas));
      /* The two counters that say effects are actually being heard rather than shuffled through the ring: a
       * mark the reader never applied means a sound played at the wrong rate, and a full-signal byte count
       * with a silent filter means it never got that far. Both were real bugs in this file's history. */
      if (a.aud && a.aud.markMiss && (a.aud.markMiss[0] || a.aud.markMiss[1]))
        bad.push('rate marks missed by the reader: ' + JSON.stringify(a.aud.markMiss));
      if (a.aud && a.pulled && a.pulled.sfx > 1000 && !(a.aud.inPeak && a.aud.inPeak[0] > 1000))
        bad.push('the filter was handed no effect signal (inPeak ' + JSON.stringify(a.aud.inPeak) + ')');
      /* Ratio between what the board wrote and what the filter emitted. The board runs at 10000..22050 Hz and
       * the device at whatever the AudioContext said, so the frames out must be 2.0x..4.4x the frames in on a
       * 44.1 kHz device. A reader that ignored the rate marks emits one time or four times too few frames, and
       * no other counter here notices -- underrun and drop both stay quiet while the ring is drained too fast.
       * The bound is deliberately loose: rates vary within a run, and the queue holds a bit. */
      const pumps = logs.filter((l) => l.startsWith('pump: '));
      if (pumps.length && a.pulled && a.pulled.sfx > 1000 && a.dev) {
        const last = pumps[pumps.length - 1];
        const mB = last.match(/sfx=(\d+)B/), mR = last.match(/audio=(\d+)Hz/);
        if (mB && mR) {
          const inFrames = Number(mB[1]) / 2;                          // mono 16-bit bytes -> frames
          const ratio = a.pulled.sfx / inFrames, want = a.dev / Number(mR[1]);
          if (ratio < want * 0.4 || ratio > want * 2.5)
            bad.push(`effects played at the wrong rate: ${inFrames.toFixed(0)} board frames in, ` +
                     `${a.pulled.sfx} frames out (x${ratio.toFixed(2)}, expected ~x${want.toFixed(2)})`);
        }
      }
      /* An audio measurement that never opened the device measures nothing at all, and it happens: the page
       * gets a user gesture while it is still fetching the resource pack. `?sink=1` needs no device, and is
       * what `make check` runs; ask for this only when a real AudioContext is the point. */
      if (process.env.NIGHT_REQUIRE_AUDIO && a.ac !== 'running') bad.push('AudioContext is ' + a.ac);
    }
    if (bad.length) { console.log('CHECK: FAIL\n  ' + bad.join('\n  ')); process.exitCode = 1; }
    else console.log('CHECK: PASS (' + st.frames + ' frames, ' + (st.frames / (ms / 1000)).toFixed(1) +
                     ' fps, ' + st.keys + ' keys, ' + st.tracks + ' CD tracks played)');
  }
  console.log('--- console (last 60 lines) ---');
  for (const l of logs.slice(-60)) console.log(l);
  ws.close();
  chrome.kill('SIGKILL');
  process.exit(0);
})().catch((e) => { console.error('FAIL ' + e.message); chrome.kill('SIGKILL'); process.exit(1); });
