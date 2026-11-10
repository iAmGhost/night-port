#!/usr/bin/env python3
"""How long does the ORIGINAL sit on the stage card, in real seconds? Docs/413/414.

The port holds 3.29 s on the desktop and 7.95 s on the Pocket for the same `0xc00 delay_units(1)` loop. Only the
original can say which is faithful. It is measurable now that the container route is known to run at factor 1.000
(docs/414) -- provided the route reaches the card, which every previous attempt failed to do.

The mistake being corrected: the established drive presses Enter four times, and its own header warns an extra
Enter *skips* the slide show. So this route presses NOTHING and watches. Title, slides and card are all 640x480
(mode 101h) and only gameplay is 320x200, so the card cannot be found by height (docs/410's geometry trap); it
is found by picture, correlating each sampled frame against the shipped PCXes decoded by the game's own loader
(`sweep_report.pcx_indices`), luminance taken through the PCX's trailer palette rather than gray=index.

    TOKEN=$(docker exec dbga printenv DOSBOX_API_TOKEN)
    python3 scripts/rig/orig_card_hold2.py --token "$TOKEN" --out scratch/pocket/orig_hold2 --secs 110

Transport is `docker exec dbga curl`: the API port is unreachable from the host on this machine (docs/377).
"""
import argparse, csv, glob, hashlib, json, os, subprocess, sys, time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "pocket"))

import numpy as np
from PIL import Image

from sweep_report import pcx_indices

REFS = ("title1", "title3", "1s1", "1s2", "1s3", "stage1")
RES = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "scratch", "res")


def api(tok, path, method="GET", body=None, timeout=60, binary=False):
    cmd = ["docker", "exec", "dbga", "curl", "-s", "-m", str(int(timeout)), "-X", method,
           "-H", "Authorization: Bearer " + tok]
    if body is not None:
        cmd += ["-H", "Content-Type: application/json", "-d", json.dumps(body)]
    if binary:
        cmd += ["-o", "/work/_hold2.bin", "http://127.0.0.1:8386" + path]
        r = subprocess.run(cmd, capture_output=True)
        if r.returncode:
            raise RuntimeError("frame fetch failed: " + r.stderr.decode()[:200])
        return open("/tmp/dbgauto/_hold2.bin", "rb").read()
    cmd.append("http://127.0.0.1:8386" + path)
    r = subprocess.run(cmd, capture_output=True, text=True)
    if not r.stdout.strip():
        raise RuntimeError("empty api response on " + path)
    return r.stdout


def pcx_luma(path):
    """The shipped PCX as a luminance grid, using the palette the file itself carries."""
    w, h, idx = pcx_indices(path)          # the loader's order: w, h, indices
    idx = np.frombuffer(idx, dtype=np.uint8).reshape(h, w)
    d = open(path, "rb").read()
    pal = None
    if d[-769] == 0x12:
        pal = np.frombuffer(d[-768:], dtype=np.uint8).reshape(256, 3)
    if pal is None:                                    # no v2 trailer: fall back to a gray ramp
        lum = idx.astype(np.float32)
    else:
        p = pal[:, 0] * 0.299 + pal[:, 1] * 0.587 + pal[:, 2] * 0.114
        lum = p[idx]
    return np.asarray(Image.fromarray(lum).resize((48, 48), Image.BILINEAR), dtype=np.float64)


def grid(a):
    v = np.asarray(Image.fromarray(a).resize((48, 48), Image.BILINEAR), dtype=np.float64)
    return (v - v.mean()) / (v.std() + 1e-9)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--token", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--secs", type=float, default=110.0)
    ap.add_argument("--hz", type=float, default=5.0)
    ap.add_argument("--press-after-title", action="store_true",
                    help="send ONE Enter the first time 320x200 returns after a 640x480 title sequence, then no keys")
    ap.add_argument("--max-presses", type=int, default=0,
                    help="with --advance-on-static: send at most this many presses. The show is press-gated but "
                         "the CARD HOLD IS NOT: 0x3b188 tests DAT_13e89 and 0x3b19a leaves the hold loop early "
                         "on any key. Press through the slides then stop, or you measure 'hold aborted by a press' "
                         "(measured: 1.25 s) instead of the hold. 0 = no cap.")
    ap.add_argument("--esc-once-on-static", type=float, default=0.0,
                    help="send exactly ONE Escape once a 640x480 picture has sat unchanged this long, then send "
                         "nothing ever again. I added this believing the card only appears when ESC unwinds the "
                         "slide show; orig_hold5 disproved that -- Enter presses reached the card normally "
                         "(docs/415). It also never fires usefully, because with no key the game's 640x480 title "
                         "card simply waits (44.7 s static) and static-detection lands there. Kept as the null "
                         "run it is, and as the reminder that 'a 640x480 still' is not 'a slide'.")
    ap.add_argument("--advance-on-static", type=float, default=0.0,
                    help="press Enter whenever a 640x480 picture has been unchanged for this many seconds "
                         "(the original's slides are gated on a press: 1s1 sat unchanged for 7.4 s with no key). "
                         "The last press before the mode leaves 640x480 is the entry into the card hold, so the "
                         "press-to-mode-change gap is the hold.""")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    tok = a.token

    refs = {}
    for n in REFS:
        p = os.path.join(RES, n + ".pcx")
        if os.path.exists(p):
            refs[n] = grid(pcx_luma(p))
    print("refs: %s" % ", ".join(refs))

    ev = open(os.path.join(a.out, "events.tsv"), "w", newline="")
    w = csv.writer(ev, delimiter="\t")
    w.writerow(["seq", "t_ms", "w", "h", "sig", "best", "sim", "file"])

    t0 = time.monotonic()
    seq, prev = 0, None
    static_since = 0.0
    saw480 = False
    pressed = not a.press_after_title            # when the flag is off, nothing is ever sent
    last_press, press_log = -1e9, []
    esc_sent = False
    esc_t, card_t, leave_t = None, None, None
    next_witness = 0.0
    while True:
        now = time.monotonic() - t0
        if now > a.secs:
            break
        try:
            info = json.loads(api(tok, "/api/v1/video/frame/info", timeout=20))
            raw = api(tok, "/api/v1/video/frame?format=raw", timeout=40, binary=True)
        except Exception as e:
            print("  t=%6.1f  poll failed: %s" % (now, e)); time.sleep(1.0); continue
        hh, ww = info["height"], info["width"]
        if a.press_after_title:
            if hh > 400:
                saw480 = True
            elif saw480 and not pressed:
                # the attract screen is back after the title sequence: this is the one moment a single Enter
                # starts an episode. Pressing twice skips the slide show (drive_orig_episode1.txt's warning).
                api(tok, "/api/v1/input/sequence", "POST",
                    {"events": [{"t": 0, "type": "key", "key": "KBD_enter", "pressed": True},
                                {"t": 60, "type": "key", "key": "KBD_enter", "pressed": False}]})
                pressed = True
                print("  t=%6.2fs  ONE Enter at the attract screen (the only key of this run)"
                      % (time.monotonic() - t0), flush=True)
        if now >= next_witness:
            next_witness = now + 4.0
            try:
                png = api(tok, "/api/v1/video/frame?format=png", timeout=40, binary=True)
                open(os.path.join(a.out, "witness_%04d_%dx%d.png" % (int(now), ww, hh)), "wb").write(png)
            except Exception as e:
                print("    (witness failed: %s)" % e)
        sig = hashlib.md5(raw).hexdigest()[:12]
        if a.esc_once_on_static > 0 and hh > 400 and not esc_sent and (now - static_since) > a.esc_once_on_static:
            api(tok, "/api/v1/input/sequence", "POST",
                {"events": [{"t": 0, "type": "key", "key": "KBD_escape", "pressed": True},
                            {"t": 60, "type": "key", "key": "KBD_escape", "pressed": False}]})
            esc_sent = True
            esc_t = now
            print("  t=%6.2fs  ONE Escape (480-static %.1f s); no keys after this" % (now, now - static_since),
                  flush=True)
        if a.advance_on_static > 0 and hh > 400 and (now - static_since) > a.advance_on_static \
                and now - last_press > a.advance_on_static \
                and (a.max_presses == 0 or len(press_log) < a.max_presses):
            api(tok, "/api/v1/input/sequence", "POST",
                {"events": [{"t": 0, "type": "key", "key": "KBD_enter", "pressed": True},
                            {"t": 60, "type": "key", "key": "KBD_enter", "pressed": False}]})
            last_press = now
            press_log.append(now)
            print("  t=%6.2fs  Enter (480-static %.1f s)  [press #%d]" % (now, now - static_since, len(press_log)),
                  flush=True)
        if sig != prev:
            # the raw payload is a header + index pixels or BGRX; take the pixel part and view it as gray
            px = np.frombuffer(raw[-ww * hh:], dtype=np.uint8) if len(raw) >= ww * hh else np.frombuffer(raw, dtype=np.uint8)
            side = int(np.sqrt(px.size))
            try:
                g = (px[:side * side].astype(np.float32).reshape(side, side))
            except ValueError:
                g = None
            best, bs = "-", -1.0
            if g is not None and g.shape[0] > 8:
                vg = grid(g)
                for n, rg in refs.items():
                    s = float((vg * rg).mean())
                    if s > bs:
                        bs, best = s, n
            png = os.path.join(a.out, "%04d_%dx%d.png" % (seq, ww, hh))
            try:
                if len(raw) >= ww * hh * 4:                       # BGRX32_ByteArray
                    im = Image.fromarray(np.frombuffer(raw[-ww * hh * 4:], dtype=np.uint8)
                                         .reshape(hh, ww, 4)[:, :, [2, 1, 0]])
                else:                                             # paletted: index bytes as gray
                    im = Image.fromarray(px[:ww * hh].astype(np.uint8).reshape(hh, ww))
                im.save(png)
            except Exception as e:
                print("    (no png: %s)" % e); png = "-"
            w.writerow([seq, int(now * 1000), ww, hh, sig, best, round(bs, 4), os.path.basename(png)])
            print("  t=%6.2fs  %dx%d %s best=%s sim=%.3f" % (now, ww, hh, sig, best, bs), flush=True)
            if esc_sent and card_t is None:
                card_t = now                # the card itself, replacing the last slide
            prev = sig
            static_since = now
            seq += 1
        time.sleep(1.0 / a.hz)
    ev.close()
    rows0 = list(csv.DictReader(open(os.path.join(a.out, "events.tsv")), delimiter="\t"))
    for r in rows0:
        if esc_sent and card_t and leave_t is None and int(r["h"]) < 400:
            leave_t = now
    print("done: %d change-frames in %s; presses sent: %s" % (seq, a.out, ["%.1f" % x for x in press_log]))
    if esc_sent:
        lv = None
        for r in rows0:
            if int(r["h"]) < 400:
                lv = int(r["t_ms"]) / 1000.0
                break
        print("CARD HOLD: Escape at %.2f s -> card drawn at %.2f s -> mode leaves 640x480 at %s = %s s real"
              % (esc_t if esc_t else -1, card_t if card_t else -1,
                 "%.2f" % lv if lv else "never", "%.2f" % (lv - card_t) if (lv and card_t) else "-"))
    # The hold: last press that left a 640x480 picture -> the frame where the mode leaves 640x480.
    rows = list(csv.DictReader(open(os.path.join(a.out, "events.tsv")), delimiter="	"))
    leave = None
    for r in rows:
        if int(r["h"]) < 400:
            leave = int(r["t_ms"]) / 1000.0
            break
    if press_log and leave:
        print("HOLD candidate: last press %.2f s -> mode leaves 640x480 at %.2f s = %.2f s real time"
              % (press_log[-1], leave, leave - press_log[-1]))


main()
