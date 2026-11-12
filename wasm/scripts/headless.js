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
  'audio:window.NightWasm.stats(),gate:window.NightWasm.ready()});})()';
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
  /* NIGHT_CLICKLOG=1 logs every click the page gets, from before the navigation. It exists because a run
     whose only press was the gate came back with the page MUTED, and the only thing in this page that
     mutes is a click on one button -- so either the page is being clicked by something invisible, or the
     page's own state is not what its counters say. Both are worth knowing. */
  if (process.env.NIGHT_CLICKLOG) {
    await send('Page.addScriptToEvaluateOnNewDocument', { source:
      `window.addEventListener('click', (e) => console.log('DOM click: ' + (e.target.id || e.target.tagName) +
        ' at ' + Math.round(e.clientX) + ',' + Math.round(e.clientY) + ' trusted=' + e.isTrusted +
        '\n' + (new Error().stack || '').split('\n').slice(1, 5).join('\n')), true);
       window.addEventListener('keydown', (e) => console.log('DOM key: ' + e.key + ' on ' +
        (e.target.id || e.target.tagName) + ' active=' + (document.activeElement && document.activeElement.id)), true);
       window.addEventListener('focusin', (e) => console.log('focus: ' + (e.target.id || e.target.tagName)), true);` });
  }
  /* NIGHT_VIEWPORT="390x844x3,mobile" puts the page in a phone's viewport. It exists because the pad's
     layout -- cell sizes from vmax, a three-column deck, a middle column that shrinks first -- is a question
     about a 390 px screen and not about the 960 px window, and because with mobile touch on,
     matchMedia('(pointer: coarse)') answers the way a phone answers it, so the pad defaults itself on and a
     run sees what a visitor gets. The scale factor is CSS pixels by DPR, so 3 keeps it honest. */
  const vp = process.env.NIGHT_VIEWPORT;
  if (vp) {
    const m = /(\d+)x(\d+)(?:x([\d.]+))?/.exec(vp);
    if (!m) { console.log('NIGHT_VIEWPORT wants WxH or WxHxDPR, got ' + vp); }
    else {
      const mobile = /mobile/i.test(vp);
      await send('Emulation.setDeviceMetricsOverride', { width: +m[1], height: +m[2],
        deviceScaleFactor: m[3] ? +m[3] : 1, mobile });
      try { await send('Emulation.setTouchEmulationEnabled', { enabled: true, maxTouchPoints: 5 }); }
      catch (e) { console.log('touch emulation unavailable: ' + e.message); }
      console.log('viewport ' + m[1] + 'x' + m[2] + ' dpr ' + (m[3] || 1) + (mobile ? ' mobile' : ''));
    }
  }
  /* NIGHT_ROTATE_AT="5000:844x390x2,mobile" turns the emulated phone at that millisecond of the run. It
     exists because the re-fit on rotation is a code path with exactly one trigger -- a real turn -- and a
     phone in the harness that never turns is a phone whose rotation bugs survive into a real hand. */
  const rot = process.env.NIGHT_ROTATE_AT;
  if (rot) {
    const m = /^(\d+):(\d+)x(\d+)(?:x([\d.]+))?/.exec(rot);
    if (!m) console.log('NIGHT_ROTATE_AT wants MS:WxH[xDPR], got ' + rot);
    else setTimeout(() => {
      const w = +m[2], h = +m[3], land = w > h;
      send('Emulation.setScreenOrientationOverride',
           { angle: land ? 90 : 0, type: land ? 'landscapePrimary' : 'portraitPrimary' })
        .catch((e) => console.log('orientation override: ' + e.message));
      send('Emulation.setDeviceMetricsOverride', { width: w, height: h,
        deviceScaleFactor: m[4] ? +m[4] : 1, mobile: /mobile/i.test(rot) })
        .then(() => console.log('rotated at ' + m[1] + 'ms to ' + w + 'x' + h))
        .catch((e) => console.log('rotate failed: ' + e.message));
    }, +m[1]);
  }
  /* NIGHT_NET="latency=250 offline=8000-14000" puts a real network in front of the Range reads: a
     round-trip cost in ms (`latency`), a downlink cap in bytes/s (`throughput`, -1 = unlimited), and a
     window of milliseconds during which the connection is DEAD (`offline=A-B`, which makes `fetch` reject
     the way a dropped Wi-Fi does). It exists because the disc's byte source is the one part of this port
     whose failure mode is not in the port's control, and "the music stopped and never came back" is only
     reproducible if the network can be told to misbehave on a schedule. Chrome applies this per target,
     so it goes on before the navigation. */
  const netSpec = (process.env.NIGHT_NET || '').split(/\s+/).filter(Boolean);
  let net = { offline: false, latency: 0, download: -1, upload: -1 };
  const netWin = [];
  for (const tok of netSpec) {
    let m = /^latency=(\d+)$/.exec(tok);
    if (m) { net.latency = +m[1]; continue; }
    m = /^throughput=(\d+)$/.exec(tok);
    if (m) { net.download = +m[1]; continue; }
    m = /^offline=(\d+)-(\d+)$/.exec(tok);
    if (m) { netWin.push([+m[1], +m[2]]); continue; }
    console.log('NIGHT_NET: cannot read ' + tok + ' (want latency=N | throughput=N | offline=A-B)');
  }
  if (netSpec.length) {
    await send('Network.enable');
    const apply = (offline) => send('Network.emulateNetworkConditions',
        { offline, latency: net.latency, downloadThroughput: net.download, uploadThroughput: net.download });
    await apply(false);
    console.log(`net: latency=${net.latency}ms throughput=${net.download}B/s` +
                (netWin.length ? ' offline at ' + netWin.map((w) => w[0] + '-' + w[1]).join(',') : ''));
    for (const [a, b] of netWin) {
      setTimeout(() => apply(true).then(() => console.log(`net: OFFLINE at ${a}ms`)).catch(() => {}), a);
      setTimeout(() => apply(false).then(() => console.log(`net: back at ${b}ms`)).catch(() => {}), b);
    }
  }
  await send('Page.navigate', { url });

  /* NIGHT_GAMEPAD="start@3000 lb@5000:400 up@8000 ls:0.9,0@9000:500" -- a controller without a controller.
   *
   * Chrome's protocol has no gamepad injection (its Gamepad tests drive a device::GamepadTestHelper inside
   * the browser process, which nothing on the wire can reach), so this installs a page-side stand-in:
   * Navigator.prototype.getGamepads is replaced by one that returns a fabricated standard-mapping
   * controller, and the specs move its buttons and its left stick. That puts under test everything the PAGE
   * owns -- the map, the edge detection, the stick's dead zone, the union with a thumb, the ring writes --
   * and nothing about Chrome's own Gamepad API. A real pad has never driven this port (docs/457).
   *
   * Names are the standard indices by name (a b x y lb rb lt rt select start lstick rstick up down left
   * right); `ls:x,y` is the left stick, and the page maps only LB of the three shoulder inputs, so an `rb`
   * press that changes nothing is a pass, not a failure. */
  const gpSpecs = (process.env.NIGHT_GAMEPAD || '').split(/\s+/).filter(Boolean);
  const GPBTN = { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, lt: 6, rt: 7, select: 8, back: 8, start: 9,
                  lstick: 10, rstick: 11, up: 12, down: 13, left: 14, right: 15 };
  async function installFakePad() {
    let ok = 'never ran';
    for (let i = 0; i < 100; i++) {
      ok = await evalJson(`(function () {
        /* Wait for the page under test, not just for a document: navigated to a server that is not running,
         * the error page still has a navigator with a getGamepads, and the fake would install there and report
         * a success that measured nothing but Chrome's own error page. */
        if (!window.NightWasm) return 'no page';
        if (!window.__gp) {
          const btn = () => ({ pressed: false, touched: false, value: 0 });
          window.__gp = { index: 0, connected: true, mapping: 'standard', timestamp: 0, id:
            'Night Harness Pad (STID: 045e-028e)', buttons: Array.from({ length: 17 }, btn),
            axes: [0, 0, 0, 0, 0, 0, 0, 0], vibrate: () => false };
          window.__gpBtn = (i, down) => { const b = window.__gp.buttons[i];
            b.pressed = !!down; b.touched = !!down; b.value = down ? 1 : 0; };
          window.__gpStick = (x, y) => { window.__gp.axes[0] = x; window.__gp.axes[1] = y; };
          const real = Navigator.prototype.getGamepads;
          Object.defineProperty(Navigator.prototype, 'getGamepads', {
            configurable: true, writable: true, value: function () { const r = real.call(this); r[0] = window.__gp; return r; } });
        }
        return 'fake pad at index 0';
      })()`);
      if (ok === 'fake pad at index 0') { console.log('gamepad: ' + ok); return true; }
      await sleep(100);
    }
    console.log('gamepad: the fake never installed (' + ok + ')');
    return false;
  }
  async function gpDrive(spec) {
    const m = /^(.*?)@(\d+)(?::(\d+))?$/.exec(spec);
    if (!m) { console.log('gamepad: cannot read spec ' + spec); return; }
    const at = +m[2], hold = +(m[3] || 150);
    let act;
    if (m[1].startsWith('ls:')) {
      const [x, y] = m[1].slice(3).split(',').map(Number);
      act = (on) => evalJson(`window.__gpStick(${on ? x : 0}, ${on ? y : 0})`);
    } else {
      const i = GPBTN[m[1]];
      if (i === undefined) { console.log('gamepad: unknown button ' + m[1]); return; }
      act = (on) => evalJson(`window.__gpBtn(${i}, ${on ? 1 : 0})`);
    }
    setTimeout(() => {
      act(true).then(() => sleep(hold)).then(() => act(false)).catch(() => {});
    }, Math.max(0, at));
  }
  if (gpSpecs.length) {
    if (await installFakePad()) for (const spec of gpSpecs) await gpDrive(spec);
  }

  /* Press the gate, if the page showed one. It has to be a real mouse event at the button's centre: a
   * scripted element.click() does NOT grant user activation, so it would start the game and still leave the
   * AudioContext suspended -- a run that hears nothing and reports every counter at a healthy zero, which is
   * the failure NIGHT_REQUIRE_AUDIO exists to catch. The gate only arms once the module and the 20 MB resource
   * pack are in, so wait for that instead of guessing at a duration. With ?sink=1 or ?auto=1 the page starts
   * itself and this returns at once. And with a NIGHT_GAMEPAD spec that presses start or a, the CONTROLLER is
   * the thing meant to start it -- the page starts the game on that press, and a mouse racing it would hide
   * which of the two did it. */
  async function evalJson(expression) {
    const r = await send('Runtime.evaluate', { expression: '(' + expression + ')', returnByValue: true });
    return r.result && r.result.value;
  }
  async function pressGate(timeoutMs) {
    /* getBoundingClientRect() is a DOMRect, and DOMRect's x/width/... live on its prototype, so a
     * by-value serialization of it is an empty object -- the first version of this clicked at NaN. */
    const read = () => evalJson(`(() => { const N = window.NightWasm; if (!N) return {};
        const rd = N.ready(), b = document.getElementById('btn-start');
        const r = b ? b.getBoundingClientRect() : null;
        return { preloaded: rd.preloaded, started: rd.started, gate: rd.gate, armed: b && !b.disabled,
                 box: r ? { x: r.x, y: r.y, w: r.width, h: r.height } : null }; })()`);
    const deadline = Date.now() + timeoutMs;
    for (;;) {
      const r = await read();
      if (r.started) return true;                              /* auto-start path: nothing to press */
      if (r.gate && r.armed && r.box && r.box.w > 0) {
        const x = Math.round(r.box.x + r.box.w / 2), y = Math.round(r.box.y + r.box.h / 2);
        if (process.env.NIGHT_SHOT_GATE) {          /* look at the gate without pressing it */
          const s = await send('Page.captureScreenshot', { format: 'png' });
          fs.writeFileSync(png || '/tmp/gate.png', Buffer.from(s.data, 'base64'));
          console.log('gate armed: ' + JSON.stringify(r.box));
          console.log('wrote ' + (png || '/tmp/gate.png'));
          process.exit(0);
        }
        /* Ask what is actually under that point before pressing it. `btn-start` in the gate and `btn-mute`
         * in the bar can occupy the same pixels, and a click whose mousedown and mouseup land on different
         * elements does not deliver the click where either expects -- which is how a measurement run whose
         * only press was the gate came back with the page muted and every audio counter at zero. */
        const hit = await evalJson(`(() => { const e = document.elementFromPoint(${x}, ${y});
            const b = document.getElementById('btn-mute'), r = b && b.getBoundingClientRect();
            return { hit: e ? (e.id || e.tagName) : null,
                     mute: r ? [Math.round(r.left), Math.round(r.top), Math.round(r.right), Math.round(r.bottom)] : null }; })()`);
        await click(x, y);
        console.log(`gate pressed at ${x},${y} (under it: ${hit.hit}, mute button at ${JSON.stringify(hit.mute)})`);
        const until = Date.now() + 15000;
        while (Date.now() < until && !(await evalJson('window.NightWasm.ready().started'))) await sleep(100);
        return await evalJson('window.NightWasm.ready().started');
      }
      if (Date.now() > deadline) { console.log('the gate never armed: ' + JSON.stringify(r)); return false; }
      await sleep(150);
    }
  }
  const gpStartsGame = gpSpecs.some((s) => /^(start|a)@/.test(s));
  const gateStarted = gpStartsGame ? true : await pressGate(Math.max(30000, Math.min(90000, ms * 3)));
  if (!gateStarted) console.log('WARNING: the game never started -- nothing measured here can have been heard');

  /* The DOS make codes are keyed by the same table shell.js uses, only here for the protocol fields. */
  const VK = { Space: [32, ' '], Enter: [13, 'Enter'], Escape: [27, 'Escape'], F1: [112, 'F1'],
               ArrowLeft: [37, 'ArrowLeft'], ArrowUp: [38, 'ArrowUp'], ArrowRight: [39, 'ArrowRight'],
               ArrowDown: [40, 'ArrowDown'], KeyZ: [90, 'z'], KeyX: [88, 'x'], KeyC: [67, 'c'],
               KeyQ: [81, 'q'], KeyE: [69, 'e'], KeyS: [83, 's'] };
  async function press(spec) {
    if (spec === 'wait') { console.log('   (wait)'); return; }
    if (spec.startsWith('wait:')) { const ms = parseInt(spec.slice(5), 10); await sleep(ms); return; }
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
  /* NIGHT_TOUCH drives the on-screen deck without a hand, which is the only way to ask "does the d-pad let
   * go" at all. A spec is a schedule, in milliseconds from the start of the touch phase:
   *
   *   c@11900             claim the driver's fingers are live: replace `document.pointerIdList` with the
   *                       ids this driver is holding, so the deck can only heal by its own rule and not by
   *                       asking the browser (which is what the sweep below does on a real phone)
   *   d:left@12000        put a finger on the cross cell `left` (up/down/left/right/hub) at t=12 s
   *   d:right@12400       ...and a SECOND one on `right`, with the first still down
   *   h:right@12500       assert the directions the deck believes are held -- a bare `h:` says none
   *   u@13000             lift every finger this driver is holding
   *
   * The interesting case is the one that was untestable until now: a finger that never sends a release,
   * which is what iOS losing a `pointerup` looks like to the page. `d:left` followed by `d:right` with no
   * release in between IS a stuck d-pad by any browser's reckoning, and the `h:` that follows is the fix
   * (docs/461). A held direction with no finger behind it also shows up in the `h:` line, which prints the
   * cross's owner and the pointers behind it, so a failure says which rule let it through. */
  const touchSpec = (process.env.NIGHT_TOUCH || '').split(/\s+/).filter(Boolean);
  let touchBad = [];                              /* problems the NIGHT_CHECK branch below inherits */
  const DIR_NAME = { up: 'up', down: 'down', left: 'left', right: 'right' };
  /* Press and release on the cross by dispatching the very pointer events the deck listens to. Chrome's real
   * touch pipeline (`Input.dispatchTouchEvent`) was tried first and hung the harness -- it drags in viewport
   * emulation while the disc is streaming, and it measures nothing the deck itself does -- while the bug
   * under test here IS the deck's own bookkeeping, which a dispatched `pointerdown` reaches exactly. A
   * release is sent as `pointerup` AND `lostpointercapture`, because the deck expects both on a device. */
  const PAD_DRIVER = `(window.__pad = {
    claim() {
      const self = this;
      try {
        Object.defineProperty(document, 'pointerIdList',
          { configurable: true, get() { return self.down.slice(); } });
        return true;
      } catch (e) { return false; }
    },
    down: [],
    press(d) {
      const el = document.querySelector('#dpad [data-d="' + d + '"]') || document.getElementById('dpad');
      return this.__p(el);
    },
    key(name) {
      const el = document.querySelector('#pad-keys [data-key="' + name + '"], #pad-sys [data-key="' + name + '"]');
      return this.__p(el);
    },
    __p(el) {
      if (!el) return null;
      const r = el.getBoundingClientRect();
      const x = Math.round(r.left + r.width / 2), y = Math.round(r.top + r.height / 2);
      const id = (this.last = (this.last || 10) + 1);
      this.down.push(id);
      el.dispatchEvent(new PointerEvent('pointerdown', { pointerId: id, pointerType: 'touch',
        isPrimary: this.down.length === 1, clientX: x, clientY: y, bubbles: true, cancelable: true, buttons: 1 }));
      return { x, y, id };
    },
    lift() {
      for (const el of document.querySelectorAll('#dpad, #pad-keys > *, #pad-sys > *')) {
        for (const id of this.down) {
          for (const kind of ['pointerup', 'lostpointercapture'])
            el.dispatchEvent(new PointerEvent(kind, { pointerId: id, pointerType: 'touch',
              bubbles: true, cancelable: true, buttons: 0 }));
        }
      }
      this.down = [];
    },
  }, true)`;
  async function runTouch() {
    const bad = [];
    if (!await evalJson(`!!(window.NightWasm && document.getElementById('dpad'))`)) {
      return ['NIGHT_TOUCH: no page or no #dpad to press'];
    }
    const mode = await evalJson(`window.NightWasm.pad('band')`);
    if (mode !== 'band') return ['NIGHT_TOUCH: could not put the pad on (' + mode + ')'];
    if (!await evalJson(PAD_DRIVER)) return ['NIGHT_TOUCH: the pad driver would not install'];
    const held = () => evalJson(`window.NightWasm.padState()`);
    let clock = 0;
    for (const spec of touchSpec) {
      const at = spec.indexOf('@');
      const body = at < 0 ? spec : spec.slice(0, at);
      const m = /^(c|d|h|k|K|u)(?::(.*))?$/.exec(body);
      if (!m) { bad.push('NIGHT_TOUCH: cannot read ' + spec); continue; }
      const [, what, arg] = m;
      const t = at < 0 ? null : Number(spec.slice(at + 1));
      if (at >= 0 && !Number.isFinite(t)) { bad.push('NIGHT_TOUCH: no time in ' + spec); continue; }
      const wait = (t === null ? 30 : t - clock);
      if (wait > 0) await sleep(wait);
      if (t !== null) clock = t;
      if (what === 'c') {
        /* Without this, a synthetic finger is invisible to `document.pointerIdList`, so the sweep would
         * release it and the test would pass for the wrong reason. With it, the page is told "the browser
         * says both fingers are still down" -- the stuck case no sweep can reach. */
        const ok = await evalJson(`window.__pad.claim()`);
        if (!ok) bad.push('touch: the page would not pretend a finger was still down (pointerIdList is read-only here)');
        else console.log('touch: the driver now claims its fingers are on the glass');
      } else if (what === 'k') {
        const c = await evalJson(`window.__pad.key(${JSON.stringify(arg || '')})`);
        if (!c) { bad.push('touch: no deck button ' + JSON.stringify(arg)); continue; }
        console.log(`touch: finger ${c.id} down at ${arg}`);
      } else if (what === 'd') {
        const c = await evalJson(`window.__pad.press(${JSON.stringify(arg || '')})`);
        if (!c) { bad.push('NIGHT_TOUCH: no d-pad cell ' + JSON.stringify(arg)); continue; }
        console.log(`touch: finger ${c.id} down at ${arg} (${c.x},${c.y})`);
      } else if (what === 'u') {
        await evalJson(`window.__pad.lift()`);
        console.log('touch: every finger lifted');
      } else if (what === 'K') {
        await sleep(40);
        const st = await held();
        const want = (arg || '').split('+').filter(Boolean).sort();
        const got = [...(st.keys || [])].sort();
        const w = want.join('+'), g = got.join('+');
        const line = `buttons [${g || 'nothing'}], dirs [${(st.dirs || []).join('+') || 'nothing'}]`;
        if (w !== g) bad.push(`touch: at ${t === null ? clock : t} ms ${line}; expected buttons [${w || 'nothing'}]`);
        else console.log(`touch: ${line} as expected`);
      } else {
        await sleep(40);                          /* the handlers are synchronous; this is for the clock */
        const st = await held();
        const want = (arg || '').split('+').filter(Boolean).map((d) => DIR_NAME[d] || d).sort();
        const got = [...(st.dirs || [])].sort();
        const w = want.join('+'), g = got.join('+');
        const line = `held [${g || 'nothing'}], cross owner ${st.owner}, pointers behind it [${(st.ptrs || []).join(',')}]`;
        if (w !== g) bad.push(`touch: at ${t === null ? clock : t} ms ${line}; expected [${w || 'nothing'}]`);
        else console.log(`touch: ${line} as expected`);
      }
    }
    return bad;
  }

  const early = [];
  for (const spec of clicks) {
    const [pos, at] = spec.split('@');
    const [x, y] = pos.split(':').map(Number);
    if (!at) { await click(x, y); await sleep(400); } else early.push([Number(at), x, y]);
  }
  early.sort((a, b) => a[0] - b[0]);
  /* Specs carrying `@ms` fire on their own clock at that absolute millisecond of the run; the rest still run
   * back to back from the midpoint, which is what `make check` has always done. */
  const timed = [], seq = [];
  for (const spec of keys) {
    const m = /^(.*?)@(\d+)$/.exec(spec);
    if (m) timed.push([Number(m[2]), m[1]]); else seq.push(spec);
  }
  timed.sort((a, b) => a[0] - b[0]);
  const timers = timed.map(([at, spec]) => setTimeout(() => { press(spec).catch(() => {}); }, Math.max(0, at)));
  /* NIGHT_MUTE="6000,12000" presses the page's own mute button at those milliseconds. "the music does not
   * resume after mute -> unmute" is a question about what the page does to the rings while nobody is
   * listening, and the only honest way to answer it is to do it and read the pump line across the gap. */
  const mutes = (process.env.NIGHT_MUTE || '').split(',').map(Number).filter(Number.isFinite).sort((a, b) => a - b);
  const muteTimers = mutes.map((at) => setTimeout(() => {
    send('Runtime.evaluate', { expression: `(() => { const b = document.getElementById('btn-mute');
        if (!b) return 'no button'; b.click(); return b.textContent; })()` })
      .then((r) => console.log(`mute: pressed at ${at} ms -> ${JSON.stringify(r.result && r.result.value)}`))
      .catch(() => {});
  }, Math.max(0, at)));
  /* NIGHT_PROBE="8000,14000" dumps what the audio path looks like from the page at those milliseconds, which
   * is how "the music stopped and the counters said nothing" gets answered: the flags word, whether the page
   * believes it is muted, and -- in `cddaReader` -- what the ring's own reader thinks it is doing, which is
   * the only counter that can tell a stopped reader from an empty ring. Timed from the start of the run, like
   * the mute presses, and created before the waits rather than after them. */
  const probes = (process.env.NIGHT_PROBE || '').split(',').map(Number).filter(Number.isFinite).sort((a, b) => a - b);
  const probeTimers = probes.map((at) => setTimeout(() => {
    evalJson(`(() => { const s = window.NightWasm ? NightWasm.stats() : null;
        const b = document.getElementById('btn-mute'), m = NightWasm.mix ? NightWasm.mix() : null;
        return { t: Math.round(performance.now()), flags: s && s.flags, btn: b && b.textContent,
                 cdda: s && s.pulled.cdda, dev: s && s.dev, ac: s && s.ac, cddaReader: m && m[1] }; })()`)
      .then((v) => console.log(`probe: ${JSON.stringify(v)}`))
      .catch((e) => console.log(`probe: ${e.message}`));
  }, Math.max(0, at)));
  await sleep(Math.floor(ms / 2));
  let waited = 0;
  for (const [at, x, y] of early) { await sleep(Math.max(0, at - waited)); waited = at; await click(x, y); }
  for (const spec of seq) { await press(spec); }
  if (touchSpec.length) touchBad = await runTouch();
  await sleep(Math.ceil(ms / 2));
  for (const t of timers.concat(muteTimers, probeTimers)) clearTimeout(t);
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
  let failed = false;                 /* function scope: the exit at the end of main honours it */
  if (process.env.NIGHT_CHECK) {
    let st = null;
    try { st = typeof stateValue === 'string' ? JSON.parse(stateValue) : stateValue; } catch (e) { /* keep going */ }
    const bad = [];
    for (const p of touchBad) bad.push(p);        /* a stuck d-pad is a failure of the machine, not a taste */
    const netOfflineWindow = /offline=\d+-\d+/.test(process.env.NIGHT_NET || '');
    /* `cdda: fed nothing ...` is the host saying the disc feed stopped while the page was still asking for
     * bytes. It is the one line whose appearance means the music went dead for the rest of the session, so
     * it fails the run instead of being something a human has to notice in a wall of pump lines (docs/462). */
    const stuck = logs.filter((l) => /cdda: fed nothing/.test(l));
    if (stuck.length) bad.push('the CD-DA feed stopped: ' + stuck[0]);
    /* And the other half: if the run muted and unmuted the page, the reader has to have consumed CD-DA
     * afterwards. Every symptom of the trap in docs/462 looked healthy in every counter except this one. */
    if (mutes.length >= 2 && st && st.audio && st.audio.pulled && st.audio.pulled.cdda < 10000 && !netOfflineWindow)
      bad.push('no CD-DA came through after the mute cycle (pulled.cdda ' +
               (st.audio.pulled && st.audio.pulled.cdda) + ')');
    const errs = logs.filter((l) => /^EXCEPTION|^aborted|Aborted|SegmentationError|out of memory/i.test(l));
    if (errs.length) bad.push('console threw: ' + errs[0]);
    if (!st) bad.push('no state object read from the page');
    else {
      const fps = st.frames / (ms / 1000);
      const a = st.audio || {};
      if (st.exit !== 0) bad.push('the game exited with state ' + st.exit);
      if (st.gate && st.gate.started === false)
        bad.push('the game never started -- the gate was never pressed (gate ' + JSON.stringify(st.gate) + ')');
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
      /* A voice mixed under another one is invisible to every counter that counts frames: while the mixer was
       * scaling one ring twice, effects reached the output at 1/32768 of their level and pulled.sfx, inPeak,
       * drop and underrun all still looked healthy. So measure the OUTPUT of both voices in the blocks where
       * they are supposed to overlap -- which is the whole point of the game's mixer. */
      if (a.aud && a.pulled && a.pulled.sfx > 1000 && a.pulled.cdda > 1000 && !netOfflineWindow) {
        if (!(a.aud.bothBlocks > 0))
          bad.push('the two voices never shared an output block (bothBlocks 0) -- one is being swallowed');
        else if (!(a.aud.ringPeak[0] > 0.02 && a.aud.ringPeak[1] > 0.02))
          bad.push('a voice is inaudible under the other (output peak ' + JSON.stringify(a.aud.ringPeak) +
                   ' during ' + a.aud.bothBlocks + ' shared blocks)');
      }
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
    if (bad.length) { console.log('CHECK: FAIL\n  ' + bad.join('\n  ')); failed = true; }
    else console.log('CHECK: PASS (' + st.frames + ' frames, ' + (st.frames / (ms / 1000)).toFixed(1) +
                     ' fps, ' + st.keys + ' keys, ' + st.tracks + ' CD tracks played)');
  }
  const logLines = process.env.NIGHT_LOG_LINES ? parseInt(process.env.NIGHT_LOG_LINES, 10) : 60;
  console.log('--- console (last ' + logLines + ' lines) ---');
  for (const l of logs.slice(logLines > 0 ? -logLines : 0)) console.log(l);
  ws.close();
  chrome.kill('SIGKILL');
  /* A failure has to reach the shell: this used to end in process.exit(0) unconditionally, which threw away
   * the exitCode the branch above had just set -- so a CHECK: FAIL came back green from `make check`. */
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.error('FAIL ' + e.message); chrome.kill('SIGKILL'); process.exit(1); });
