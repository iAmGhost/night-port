#!/usr/bin/env python3
"""Is the ORIGINAL's stage animation rate clock-paced or CPU-paced? The follow-up to orig_anim_rate.py.

orig_anim_rate.py measured 46.3 picture updates/s in stage play with cycles=max -- an unlimited
emulated CPU, which is NOT the machine anyone ran NIGHT on.  So the rate has to be swept:

  * if the rate HOLDS as the emulated CPU is throttled, the animation is clock-paced and 46/s is what
    the original showed on period hardware -> the Pocket's 29/s is a real ~63% shortfall to fix;
  * if the rate FALLS with the CPU, then the original on a 486 drew fewer frames than the emulator's
    fast case, and the target moves down -- possibly to at or below the port's 29/s.

Guessing which, without running the sweep, would be exactly the docs/435 / docs/447-SS6 mistake: fixing
a number that was never the real comparison.

Cycles are DOSBox IPS.  The points below bracket the 1996 range: 8k ~ 386SX, 15k ~ 386DX-33,
40k ~ 486DX2-66, plus max.

    TOKEN=$(docker exec dbga printenv DOSBOX_API_TOKEN)
    python3 scripts/rig/orig_anim_cycles.py --token "$TOKEN" --base-url http://localhost:8387 \
        --values 8000,15000,40000,max --secs 8 --out scratch/pocket/orig_cycles
"""
import argparse, json, os, subprocess, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from orig_anim_rate import api, key   # same transport, same honesty about the recorder ceiling


def capture(tok, base, out, secs, walk=True):
    api(tok, base, '/api/v1/capture/video/compression', 'PUT', {'raw': 0})
    api(tok, base, '/api/v1/capture/video/start', 'POST', {'mode': 'raw'})
    t0 = time.monotonic(); step = 0
    while time.monotonic() - t0 < secs:
        if walk and step % 20 == 0:
            key(tok, base, 'KBD_right')
        step += 1
        time.sleep(0.05)
    api(tok, base, '/api/v1/capture/video/stop', 'POST')
    time.sleep(0.5)
    avi = subprocess.run(['docker', 'exec', 'dbga', 'bash', '-c',
                          'ls -t /root/.config/dosbox-automation/capture/*.avi 2>/dev/null | head -1'],
                         capture_output=True, text=True).stdout.strip()
    local = os.path.join(out, 'cyc.avi')
    subprocess.run(['docker', 'cp', 'dbga:' + avi, local], check=True)
    r = subprocess.run(['ffprobe', '-v', 'error', '-count_frames', '-select_streams', 'v',
                        '-show_entries', 'stream=nb_read_frames,duration,width,height', local],
                       capture_output=True, text=True)
    kv = dict(x.split('=', 1) for x in r.stdout.strip().splitlines() if '=' in x)
    n, dur = int(kv['nb_read_frames']), float(kv['duration'])
    W, H = int(kv['width']), int(kv['height'])
    raw = os.path.join(out, 'cyc.raw')
    subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', local, '-f', 'rawvideo', '-pix_fmt', 'rgb24', raw], check=True)
    import numpy as np
    a = np.fromfile(raw, dtype=np.uint8)[:n * W * H * 3].reshape(n, H, W, 3).astype(np.int16)
    mx = np.abs(a[1:] - a[:-1]).max(axis=(1, 2, 3))
    c = mx >= 16
    idx = np.flatnonzero(c); sp = np.diff(idx) if len(idx) > 2 else []
    return dict(n=n, dur=dur, W=W, H=H, rate=(c.sum() / dur),
                spacing=(float(np.median(sp)) if len(sp) else 0.0))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--token', required=True)
    ap.add_argument('--base-url', default='http://localhost:8387')
    ap.add_argument('--values', default='8000,15000,40000,max')
    ap.add_argument('--secs', type=float, default=8.0)
    ap.add_argument('--out', default='scratch/pocket/orig_cycles')
    a = ap.parse_args()
    tok, base = a.token, a.base_url.rstrip('/')
    os.makedirs(a.out, exist_ok=True)

    print('%-10s %-10s %-12s %-14s %s' % ('cycles', 'mode', 'updates/s', 'recorder fps', 'spacing'))
    for v in a.values.split(','):
        # The API wants {"cycles": <int>} -- NOT {"protected_mode": N}.  My first sweep sent
        # protected_mode, got it echoed back as null, and printed four rows of numbers from FOUR
        # IDENTICAL CONFIGURATIONS.  Read the value back rather than trust the PUT body.
        body = {'cycles': 0} if v == 'max' else {'cycles': int(v)}
        api(tok, base, '/api/v1/cpu/cycles', 'PUT', body)
        time.sleep(1.0)
        st = json.loads(api(tok, base, '/api/v1/cpu/cycles', 'GET'))
        got = st.get('protected_mode')
        if v != 'max' and got != int(v):
            print('cycles=%s: REFUSED -- the running value reads back %r, skipping' % (v, got)); continue
        h = json.loads(api(tok, base, '/api/v1/video/frame/info', timeout=20))
        if h['height'] != 200:
            print('cycles=%s: not in 320x200 (%dx%d) -- skipping; drive into a stage first'
                  % (v, h['width'], h['height']))
            continue
        r = capture(tok, base, a.out, a.secs)
        mode = '%s (max=%s)' % (got, st.get('current_max'))
        print('%-10s %-10s %-12s %-14s %s' % (
            v, mode, '%.1f' % r['rate'], '%.2f' % (r['n'] / r['dur']),
            '%.1f recorder frames' % r['spacing'] if r['spacing'] else '-'))
        print('   (%dx%d, %d frames / %.2f s)' % (r['W'], r['H'], r['n'], r['dur']))


if __name__ == '__main__':
    main()
