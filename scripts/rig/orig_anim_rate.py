#!/usr/bin/env python3
"""How many times per second does the ORIGINAL actually put a new picture on the screen?

Docs/449 follow-up.  The port's stage-mode draw rate measured 29/s against a DESIGN rate of 70 fps
(fn_038098 gates on 7 PIT ticks), so the port looked like it was running the game in slow motion --
every animation step advances per DRAWN frame, so fewer drawn frames is slow motion, not dropped frames.

But 70 fps is what the code ASKS for.  It is not what the original's hardware delivered, and the port
must match the original, not the spec.  This measures the original.

METHOD.  dosbox-automation's ZMBV capture is lossless and its recorder runs at the emulated display
refresh -- 70.09 fps in VGA mode 19h -- so the recording is a 70 Hz census of what the screen showed.
Decoding it and diffing consecutive frames counts how often the picture ACTUALLY changed.  The threshold
is swept rather than chosen, because a 1-pixel scroll and a full-page composite are different beasts;
what the sweep showed on the attract demo is that the answer is threshold-INSENSITIVE between 4 and 16
grey levels, because the game writes whole pages (10 450 px median per changing frame).

Why ZMBV and not frame polling: polling through the API on this machine runs at ~8 Hz, which would
"measure" 8 fps and call it the game's rate.  The recorder rate IS the measurement ceiling, so the
script prints it.

    TOKEN=$(docker inspect dbga --format '{{range .Config.Env}}{{println .}}{{end}}' | grep DOSBOX_API_TOKEN | cut -d= -f2)
    python3 scripts/rig/orig_anim_rate.py --token "$TOKEN" --base-url http://localhost:8387 \
        --secs 12 --out scratch/pocket/orig_anim

The drive is deliberately dumb: press Enter whenever the picture has sat still for >=1.2 s and the
screen is 640x480 (a slide or a stage card -- docs/447 SS6 shows the cards are key-gated in the
original too), plus a right-arrow nudge once gameplay starts.  Nothing else is pressed.
"""
import argparse, json, os, subprocess, sys, time

def api(tok, base, path, method='GET', body=None, timeout=60, binary=False, out=None):
    cmd = ['curl', '-s', '-m', str(int(timeout)), '-X', method, '-H', 'Authorization: Bearer ' + tok]
    if body is not None:
        cmd += ['-H', 'Content-Type: application/json', '-d', json.dumps(body)]
    if binary:
        cmd += ['-o', out, base + path]
        r = subprocess.run(cmd, capture_output=True)
        if r.returncode:
            raise RuntimeError('binary fetch failed on %s: %s' % (path, r.stderr.decode()[:200]))
        return out
    cmd.append(base + path)
    r = subprocess.run(cmd, capture_output=True, text=True)
    if not r.stdout.strip():
        raise RuntimeError('empty api response on ' + path)
    return r.stdout

def key(tok, base, name):
    api(tok, base, '/api/v1/input/sequence', 'POST', {'events': [
        {'t': 0, 'type': 'key', 'key': name, 'pressed': True},
        {'t': 60, 'type': 'key', 'key': name, 'pressed': False}]})

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--token', required=True)
    ap.add_argument('--base-url', default='http://localhost:8386')
    ap.add_argument('--secs', type=float, default=12.0)
    ap.add_argument('--out', default='scratch/pocket/orig_anim')
    ap.add_argument('--hold', type=float, default=1.2, help='press Enter after this many s of still 480x480')
    ap.add_argument('--gameplay-warmup', type=float, default=6.0, help='s of 320x200 before recording starts')
    ap.add_argument('--walk', action='store_true', help='nudge right-arrow during gameplay')
    ap.add_argument('--drive-timeout', type=float, default=150.0)
    ap.add_argument('--cycles', default=None, help='e.g. "fixed 12000" or "max" -- set before the run')
    a = ap.parse_args()
    tok, base = a.token, a.base_url.rstrip('/')
    os.makedirs(a.out, exist_ok=True)

    if a.cycles:
        print('cycles -> %s' % api(tok, base, '/api/v1/cpu/cycles', 'PUT', {'cycles': a.cycles}))

    # One Enter at the attract screen starts an episode (orig_card_hold2.py's route).  The attract is
    # ALSO 320x200, so '320x200' cannot mean gameplay on its own -- gameplay is 320x200 SEEN AFTER a
    # 640x480 slide/card, which is the only thing that separates the two.
    t0 = time.monotonic()
    prev, static_since, g200_since, press_t = None, t0, None, 0.0
    started, saw480 = False, False
    while True:
        now = time.monotonic() - t0
        if not started and now > 2.0:
            key(tok, base, 'KBD_enter')
            started = True
            print('  t=%6.2fs  ONE Enter at the attract screen (starts the episode)' % now, flush=True)
        info = json.loads(api(tok, base, '/api/v1/video/frame/info', timeout=20))
        w, h = info['width'], info['height']
        api(tok, base, '/api/v1/video/frame?format=raw', timeout=40, binary=True, out=a.out + '/_probe.bin')
        raw = open(a.out + '/_probe.bin', 'rb').read()
        sig = hash(raw[-w * h:])
        if sig != prev:
            static_since = now
            prev = sig
        if h > 400 and started:      # the INTRO is 640x480 too; only a 480 after the Enter counts
            saw480 = True
        if h == 200 and saw480:
            if g200_since is None:
                g200_since = now
                print('  t=%6.2fs  320x200 (gameplay): warming up %.0f s' % (now, a.gameplay_warmup), flush=True)
        elif g200_since is not None:
            g200_since = None        # we left 320x200 (a card, a death): the warmup starts over
            print('  t=%6.2fs  left 320x200 -- warmup reset' % now, flush=True)
        if h > 400 and (now - static_since) > a.hold and now - press_t > a.hold:
            key(tok, base, 'KBD_enter')
            press_t = now
            print('  t=%6.2fs  Enter (%dx%d still %.1f s)' % (now, w, h, now - static_since), flush=True)
        if g200_since is not None and (now - g200_since) >= a.gameplay_warmup and h == 200:
            break
        if now > a.drive_timeout:
            sys.exit('never held 320x200 for %.0f s within %.0f s -- the drive did not reach gameplay'
                     % (a.gameplay_warmup, a.drive_timeout))
        time.sleep(0.05)

    print('  t=%6.2fs  recording %d s of 320x200' % (time.monotonic() - t0, a.secs), flush=True)
    print('  ' + api(tok, base, '/api/v1/capture/video/compression', 'PUT', {'raw': 0}))
    print('  ' + api(tok, base, '/api/v1/capture/video/start', 'POST', {'mode': 'raw'}))
    rec_t = time.monotonic()
    step = 0
    while time.monotonic() - rec_t < a.secs:
        if a.walk and step % 20 == 0:
            key(tok, base, 'KBD_right')
        step += 1
        time.sleep(0.05)
    print('  ' + api(tok, base, '/api/v1/capture/video/stop', 'POST'))

    # The capture dir is the app's own XDG path unless the conf's captures= is honoured.
    avi = subprocess.run(['docker', 'exec', 'dbga', 'bash', '-c',
                          'ls -t /root/.config/dosbox-automation/capture/*.avi /work/caps/*.avi 2>/dev/null | head -1'],
                         capture_output=True, text=True).stdout.strip()
    if not avi:
        sys.exit('no .avi found in the container -- check [dosbox] captures=')
    local = os.path.join(a.out, 'capture.avi')
    subprocess.run(['docker', 'cp', 'dbga:' + avi, local], check=True)
    print('capture: %s -> %s' % (avi, local))

    r = subprocess.run(['ffprobe', '-v', 'error', '-count_frames', '-select_streams', 'v',
                        '-show_entries', 'stream=nb_read_frames,duration,width,height', local],
                       capture_output=True, text=True)
    kv = dict(x.split('=', 1) for x in r.stdout.strip().splitlines() if '=' in x)
    n, dur = int(kv['nb_read_frames']), float(kv['duration'])
    W, H = int(kv['width']), int(kv['height'])
    print('recorder: %d frames / %.3f s = %.2f fps   (%dx%d)  <-- the measurement ceiling'
          % (n, dur, n / dur, W, H))

    rawp = os.path.join(a.out, 'frames.raw')
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', local, '-f', 'rawvideo', '-pix_fmt', 'rgb24', rawp], check=True)
    try:
        import numpy as np
    except ImportError:
        sys.exit('decoded to %s; run the diff under: uv run --with numpy python3 ...' % rawp)
    a3 = np.fromfile(rawp, dtype=np.uint8)[:n * W * H * 3].reshape(n, H, W, 3).astype(np.int16)
    d = np.abs(a3[1:] - a3[:-1])
    mx, npx = d.max(axis=(1, 2, 3)), (d.max(axis=3) > 0).sum(axis=(1, 2))
    print('\npicture changes per second, by how much a frame must differ:')
    for th, lab in [(4, '>=4 levels'), (16, '>=16 (visible)'), (64, '>=64 (large)')]:
        c = mx >= th
        print('   %-16s %4d changes  %5.1f/s   median %6.0f px change on a changing frame'
              % (lab, c.sum(), c.sum() / dur, np.median(npx[c]) if c.sum() else 0))
    idx = np.flatnonzero(mx >= 4)
    if len(idx) > 2:
        sp = np.diff(idx)
        print('   spacing: median %d recorder frames = %.1f updates/s while animating; '
              'longest still %.2f s' % (np.median(sp), (n / dur) / np.median(sp), sp.max() * dur / n))
    print('\nCompare with the port: the Pocket draws ~29 room frames/s (docs/449 follow-up), the design '
          'gate is 70.')

if __name__ == '__main__':
    main()
