#!/usr/bin/env python3
"""How long does the ORIGINAL sit on the stage card, in real seconds? Docs/413's open question.

The port holds ~3.3 s on the desktop and ~8.0 s on the Pocket for the same `0xc00 delay_units(1)` loop, and the
one number that would say which is right -- the original's own duration -- has never been measured. Frames can
answer it: the card is a 640x480 picture and gameplay is 320x200 (mode 13h, captured as a 640x200 payload), so
"last slide change" -> "first 320x200 frame" brackets the hold exactly, with no key press in between.

Emulation speed is measured, not assumed. `[cpu] cycles=auto` settled at `current_max=3000` with
`auto_adjust=false` on this build, which is an assumption until you check it, so the DOS clock is read twice
across a known host interval first (`TIME` prints hundredths of a second, and /api/v1/video/text reads it back).
If the factor is off, the hold number below is still usable -- it just needs dividing by that factor.

Transport goes through `docker exec … curl`: the container's API port is unreachable from the host on this
machine (docs/377; scripts/rig/capture_container.py notes the same).

    TOKEN=$(docker exec dbga printenv DOSBOX_API_TOKEN)
    python3 scripts/rig/orig_card_hold.py --token "$TOKEN" --out scratch/pocket/orig_card_hold.jsonl

Writes JSONL (t_ms, host_t, w, h, sig, presses) and prints the mode timeline plus the hold.
"""
import argparse, hashlib, json, re, subprocess, sys, time

KEY = {"enter": "KBD_enter", "esc": "KBD_escape"}


def api(tok, path, method="GET", body=None, timeout=60, binary=False):
    cmd = ["docker", "exec", "dbga", "curl", "-s", "-m", str(int(timeout)), "-X", method,
           "-H", "Authorization: Bearer " + tok]
    if body is not None:
        cmd += ["-H", "Content-Type: application/json", "-d", json.dumps(body)]
    if binary:
        cmd += ["-o", "/work/_hold.bin", "http://127.0.0.1:8386" + path]
        r = subprocess.run(cmd, capture_output=True)
        if r.returncode:
            raise RuntimeError("frame fetch failed: " + r.stderr.decode()[:200])
        return open("/tmp/dbgauto/_hold.bin", "rb").read()
    cmd.append("http://127.0.0.1:8386" + path)
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r.stdout


def press(tok, key):
    api(tok, "/api/v1/input/sequence", "POST",
        {"events": [{"t": 0, "type": "key", "key": KEY[key], "pressed": True},
                    {"t": 60, "type": "key", "key": KEY[key], "pressed": False}]})
    return time.time()


def dos_time(tok):
    """Ask the shell for the DOS clock and read it back off the text screen."""
    api(tok, "/api/v1/input/sequence", "POST",
        {"events": [{"t": 0, "type": "key", "key": KEY["enter"], "pressed": True},
                    {"t": 60, "type": "key", "key": KEY["enter"], "pressed": False}]})
    time.sleep(0.15)
    api(tok, "/api/v1/input/type", "POST", {"text": "TIME", "cps": 30})
    api(tok, "/api/v1/input/sequence", "POST",
        {"events": [{"t": 0, "type": "key", "key": KEY["enter"], "pressed": True},
                    {"t": 60, "type": "key", "key": KEY["enter"], "pressed": False}]})
    time.sleep(0.6)
    txt = json.loads(api(tok, "/api/v1/video/text"))
    m = re.search(r"(\d{1,2}):(\d{2}):(\d{2})(?:\.(\d{1,2}))?", json.dumps(txt))
    if not m:
        return None
    h, mi, s, cs = (int(x or 0) for x in m.groups())
    return ((h * 60 + mi) * 60 + s) * 100 + cs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--token", required=True)
    ap.add_argument("--out", default="scratch/pocket/orig_card_hold.jsonl")
    ap.add_argument("--calib", type=float, default=20.0, help="host seconds between the two TIME reads")
    ap.add_argument("--hz", type=float, default=5.0)
    ap.add_argument("--max", type=float, default=300.0)
    ap.add_argument("--skip-calib", action="store_true")
    a = ap.parse_args()
    tok = a.token
    ev_out = open(a.out, "w")

    # --- 1. is the emulation running at 1x? ---------------------------------------------------------------
    ratio = 1.0
    if not a.skip_calib:
        t0 = time.monotonic(); d0 = dos_time(tok)
        time.sleep(a.calib)
        d1 = dos_time(tok); t1 = time.monotonic()
        if d0 is not None and d1 is not None:
            ratio = ((d1 - d0) / 100.0) / (t1 - t0)
            print("calibration: DOS advanced %.2f s over %.2f s host -> realtime factor %.3f"
                  % ((d1 - d0) / 100.0, t1 - t0, ratio))
        else:
            print("calibration: no DOS clock on screen -- text-mode shell assumed; factor taken as 1.0")
    # clear the TIME prompt (accept unchanged) and get back to a shell prompt
    press(tok, "enter")

    # --- 2. launch ------------------------------------------------------------------------
    api(tok, "/api/v1/input/type", "POST", {"text": "NIGHT", "cps": 20})
    press(tok, "enter")
    print("NIGHT launched")

    # --- 3. watch the screen --------------------------------------------------------------
    t0 = time.monotonic()
    prev_sig, prev_sig_t, mode200_t, card_static_t, pressed_start = None, time.monotonic(), None, None, False
    log = []
    presses = [("launch_enter", 0.0)]
    while time.monotonic() - t0 < a.max:
        now = time.monotonic() - t0
        try:
            info = json.loads(api(tok, "/api/v1/video/frame/info", timeout=20))
            raw = api(tok, "/api/v1/video/frame?format=raw", timeout=30, binary=True)
        except Exception as e:
            print("  poll failed: %s" % e); time.sleep(1.0); continue
        h, w = info["height"], info["width"]
        sig = hashlib.md5(raw).hexdigest()[:12]
        changed = sig != prev_sig
        if changed:
            log.append((now, h, sig))
            ev_out.write(json.dumps({"t_ms": int(now * 1000), "w": w, "h": h, "sig": sig,
                                     "presses": [p[0] for p in presses]}) + "\n")
            prev_sig, prev_sig_t = sig, now
        # one Enter once the first long-static 640x480 card (the title) settles: that starts the episode
        if not pressed_start and h >= 400 and (now - prev_sig_t) > 2.5:
            presses.append(("title_enter", now)); press(tok, "enter"); pressed_start = True
            print("  t=%6.2fs  title card settled -> Enter (start the episode)" % now)
        if h <= 240 and mode200_t is None:
            mode200_t = now
            print("  t=%6.2fs  320x200 (gameplay) -- the hold ended" % now)
            break
        if int(now) % 10 == 0 and int(now) != getattr(main, "_last", -1):
            main._last = int(now)
            print("  t=%6.2fs  h=%s %s %s" % (now, h, sig, "" if changed else "(static)"))
        time.sleep(1.0 / a.hz)
    ev_out.close()

    # --- 4. the answer ---------------------------------------------------------------------
    last480 = max((t for t, hh, s in log if hh > 240), default=None)
    print("\n=== result ===")
    print("realtime factor            %.3f" % ratio)
    print("frames captured            %d changes" % len(log))
    print("last 640x480 change at     %s s" % ("%.2f" % last480 if last480 is not None else "never"))
    print("first 320x200 at           %s s" % ("%.2f" % mode200_t if mode200_t is not None else "never"))
    if last480 is not None and mode200_t is not None:
        print("CARD HOLD (host wall time) %.2f s   -> %.2f s at the measured realtime factor"
              % (mode200_t - last480, (mode200_t - last480) / ratio if ratio else 0))
    print("presses during the run     %s" % [p[0] for p in presses])
    print("detail                     %s" % a.out)


main()
