#!/usr/bin/env python3
"""Replay a fixed input sequence against the original and record its state sequence.

Input goes in through one /api/v1/input/sequence POST: the emulator schedules the key edges on its
own PIC timeline, so every run sees the *same* input stream, independent of polling jitter. Polling
only decides when we look, so the recorded object is the sequence of *distinct states* rather than a
wall-clock trace (docs/56: align on content, not on frame numbers).

State per sample = the whole DS group (.object2, 0x50000..0x650bf) read from physical memory at the
calibrated DS base, plus the framebuffer hash and video mode from /video/frame/info. A new state is
recorded whenever the DS digest or the frame hash changes.

  python3 scripts/rig/replay_original.py --drive DRIVE --out DIR [--hz 12] [--seconds 90]

Drive file:
  <t_ms> key <KBD_name> down|up        input edge on the replay timeline
  anchor static <stable_s> <max_wait_s>   wait until the framebuffer stops changing, then start
  anchor frame <sha1-prefix> <max_wait_s> wait for a specific frame, then start
The anchor is what makes a replay repeatable: the input timeline is released at a game-defined
moment, not at whatever wall-clock the host happened to reach (docs/56).
"""
import argparse
import hashlib
import json
import os
import re
import sys
import time
import urllib.request
import urllib.error

HERE = os.path.dirname(os.path.abspath(__file__))
DS_LO, DS_HI = 0x50000, 0x66000
MASK = os.path.join(HERE, "..", "audit", "gen", "ds_mask.bin")


def masked_digest(ds, mask):
    """Digest over the cells the guest itself touches: stack padding and untouched holes must not
    make a state look different (scripts/audit/gen_headers.py builds the mask)."""
    return hashlib.sha1(bytes(b for b, m in zip(ds, mask) if m)).hexdigest()
CHUNK = 0x4000


def api(base, token, path, method="GET", body=None, timeout=30):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(base + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + token,
                                          "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


def load_ds_base():
    p = os.path.join(HERE, "gen", "ds_base.json")
    if not os.path.exists(p):
        sys.exit("run scripts/rig/calibrate_ds.py first")
    return json.load(open(p))["phys_ds_base"]


def read_at(base, token, phys_addr, n):
    return api(base, token, f"/api/v1/memory/{phys_addr}/{n}", timeout=60)


def read_ds(base, token, phys_base):
    out = b""
    for a in range(DS_LO, DS_HI, CHUNK):
        n = min(CHUNK, DS_HI - a)
        out += api(base, token, f"/api/v1/memory/{phys_base + a}/{n}", timeout=60)
    return out


def res_names(ds):
    """Resource lookups are the event anchors both sides can see: the guest leaves the name of the
    resource it is working on as ASCII somewhere in DS, and the port announces every lookup. Scan for
    name-shaped strings instead of assuming an address for the name buffer."""
    out = set()
    i = 0
    while i < len(ds):
        c = ds[i]
        if 0x20 <= c < 0x7f:
            j = i
            while j < len(ds) and 0x20 <= ds[j] < 0x7f:
                j += 1
            run = ds[i:j].decode("ascii", "replace").strip()
            if run.lower().endswith((".pcx", ".spr", ".fli", ".act", ".voc", ".wav", ".mid")):
                out.add(run.lower())
            i = j
        else:
            i += 1
    return out


def parse_drive(path):
    steps = []
    for line in open(path):
        line = line.split("#")[0].strip()
        if line:
            steps.append(line.split())
    return steps


# the drive files name keys the way the port's playtest scripts do; the emulator wants XT set-1
# break-code names.
KEYNAME = {"enter": "KBD_enter", "return": "KBD_enter", "space": "KBD_space", "esc": "KBD_esc",
           "tab": "KBD_tab", "up": "KBD_up", "down": "KBD_down", "left": "KBD_left",
           "right": "KBD_right", "ctrl": "KBD_leftctrl", "lctrl": "KBD_leftctrl",
           "rctrl": "KBD_rightctrl", "shift": "KBD_leftshift", "lshift": "KBD_leftshift",
           "rshift": "KBD_rightshift", "alt": "KBD_leftalt", "lalt": "KBD_leftalt",
           "ralt": "KBD_rightalt", "altgr": "KBD_rightalt", "backspace": "KBD_backspace"}
# The emulator answers 400 in the middle of a run for a name it does not know, which wastes a whole
# boot; check every name up front instead.
# The emulator rejects an unknown name with HTTP 400 *mid-run*, so validate against the set of names
# the emulator binary actually knows (gen/kbd_names.txt, `strings bin/dosbox | grep ^KBD_`). Guessing
# from KEYNAME is not validation: `esc` was mapped to KBD_escape, which does not exist.
def known_kbd():
    p = os.path.join(HERE, "gen", "kbd_names.txt")
    if not os.path.exists(p):
        return set(KEYNAME.values())
    return {l.strip() for l in open(p) if l.strip()}


def check_names(segs):
    known = known_kbd()
    bad = {ev["key"] for sg in segs for ev in sg["events"] if ev["key"] not in known}
    if bad:
        sys.exit(f"drive uses key names the emulator does not accept: {sorted(bad)} "
                 f"(fix KEYNAME in replay_original.py; known names: gen/kbd_names.txt)")


def parse_events(steps):
    """Drive steps -> segments: [{"events": [...], "wait": None|{"t","ds","op","val"}}].

    A `t wait ds 0x63354 !=0` line splits the timeline: the emulator posts the events before it, then
    waits for the *shipped DS event* (read through the memory API) before posting the rest, and rebases
    the remaining times by the slippage -- the same rule the port's playtest script applies, so both
    sides run the same keys in the same game state. Resource-name anchors do not work on the shipped
    side: all 435 resource-name strings are resident in DS the whole time (measured)."""
    segs = [{"events": [], "wait": None}]
    for f in steps:
        if f[1] == "key":
            press = f[3].startswith("d")
            kn = KEYNAME.get(f[2].lower(), f[2])
            segs[-1]["events"].append({"t": int(f[0]), "type": "key", "key": kn, "pressed": press})
        elif f[0] == "anchor":
            continue
        elif f[1] == "wait" and len(f) >= 4 and f[2] == "res":
            # a port-side anchor (the port logs every resource lookup). The shipped side cannot see the
            # lookup itself -- all 435 resource names are resident in DS all the time -- so the shipped
            # side anchors on `anchor static` plus the `wait ds` lines instead. Recorded, not executed.
            continue
        elif f[1] == "wait" and len(f) >= 5 and f[2] == "ds":
            m = re.match(r"(!=|==|>=|<=)(.+)$", f[4])
            if not m:
                sys.exit(f"bad wait ds condition: {f[4]} (want ==N or !=N)")
            segs[-1]["wait"] = {"t": int(f[0]), "ds": int(f[3], 0), "op": m.group(1),
                                "val": int(m.group(2), 0)}
            segs.append({"events": [], "wait": None})
        else:
            sys.exit(f"unknown drive step: {' '.join(f)}")
    return segs


def ds_cond(base, token, phys_base, w):
    raw = api(base, token, f"/api/v1/memory/{phys_base + w['ds']}/4", timeout=30)
    v = int.from_bytes(raw, "little")
    hit = {"==": v == w["val"], "!=": v != w["val"],
           ">=": v >= w["val"], "<=": v <= w["val"]}[w["op"]]
    return v, hit


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--drive", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--base", default="http://localhost:8386")
    ap.add_argument("--token-file", default="/tmp/rig/.token")
    ap.add_argument("--hz", type=float, default=4.0)
    ap.add_argument("--seconds", type=float, default=90.0)
    ap.add_argument("--audio-capture", action="store_true",
                    help="record the mixer inside the container (/capture/audio) for the audio oracle")
    ap.add_argument("--chain-cells", default="",
                    help="comma-separated DS cells whose action-record chain (0x80 B) is also dumped")
    ap.add_argument("--no-heap", action="store_true",
                    help="skip the 43 heap reads per sample (screen/activity runs do not need them)")
    ap.add_argument("--settle", type=float, default=3.0, help="seconds before the input timeline starts")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    token = open(a.token_file).read().strip()
    phys_base = load_ds_base()
    mask = open(MASK, "rb").read()
    if len(mask) != DS_HI - DS_LO:
        sys.exit("ds_mask.bin size mismatch")

    st = json.loads(api(a.base, token, "/api/v1/status"))
    if not st.get("running"):
        sys.exit("emulator not running")
    cyc = json.loads(api(a.base, token, "/api/v1/cpu/cycles"))
    if cyc.get("auto_adjust"):
        sys.exit("cycles are not pinned (auto_adjust=true) -> bringup with a pinned conf")

    steps = parse_drive(a.drive)
    segments = parse_events(steps)
    check_names(segments)
    events = [e for sg in segments for e in sg["events"]]

    # --- anchor: release the input timeline at a game-defined moment -------------
    anchor = None
    for f in steps:
        # the drive writes `anchor static 3.0 90`; accept the verb in either field so a refactor of the
        # line format cannot silently disable the anchor (it did once: the shipped side then released its
        # keys during the logo, which shifted its whole timeline by ~90 s)
        if f[0] != "anchor" and (len(f) < 2 or f[1] != "anchor"):
            continue
        kind = f[1] if f[0] == "anchor" else f[2]
        stable, limit = (float(f[2]), float(f[3])) if f[0] == "anchor" else (float(f[3]), float(f[4]))
        t_wait, last_h, last_change = time.time(), None, time.time()
        while time.time() - t_wait < limit:
            h = hashlib.sha1(api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)).hexdigest()
            if kind == "static" and last_h == h and time.time() - last_change >= stable:
                anchor = {"kind": kind, "frame_sha": h, "wait_s": round(time.time() - t_wait, 3)}
                break
            if kind == "frame" and h.startswith(f[2]):
                anchor = {"kind": kind, "frame_sha": h, "wait_s": round(time.time() - t_wait, 3)}
                break
            if h != last_h:
                last_h, last_change = h, time.time()
            time.sleep(0.2)
        if anchor is None:
            sys.exit(f"anchor {kind} not reached within {limit}s")
    if a.audio_capture:
        # The capture starts at the anchor, so its t=0 is the same origin as the input timeline and
        # the oracle can slice it by drive phase without a second alignment.
        try:
            api(a.base, token, "/api/v1/capture/audio/start", "POST", {})
        except Exception as e:
            print(f"audio capture unavailable: {e}", file=sys.stderr)
    t0 = time.time()
    seg_i, ds_waits = 0, []
    # The anchor is a rebase point with an implied nominal: the shipped side releases the timeline when
    # the title card is steady, so the first segment's nominal times collapse onto that moment (the port
    # rebases the same way, onto its own `wait res title1.pcx`). Without this the drive's 120000 ms
    # nominal for the title would be taken as wall-clock and the keys would land two minutes late.
    first = [ev["t"] for ev in segments[0]["events"]] or [segments[0]["wait"]["t"]] \
        if segments and (segments[0]["events"] or segments[0]["wait"]) else [0]
    shift = int((time.time() - t0) * 1000) - min(first)
    # Rebase points, so a state sample can be mapped back to the drive's nominal time: each one says
    # "emulator clock t_ms corresponds to drive time nominal". diff_state cuts its windows on nominal
    # times, which is the only clock the port's dump is also stamped with.
    rebases = [{"t_ms": 0, "nominal": min(first)}]

    def post_segment(i):
        if not segments[i]["events"]:
            return
        # The emulator answers 409 while an earlier sequence is still being played back, and segments
        # are long (stepping the episode's slides takes ~28 s), so recompute the offsets and retry
        # until it accepts -- the times are relative to the POST that succeeds, which is what the
        # rebase rule assumes.
        for attempt in range(120):
            now = int((time.time() - t0) * 1000)
            evs = []
            for ev in segments[i]["events"]:
                e2 = dict(ev)
                e2["t"] = max(0, ev["t"] + shift - now)
                evs.append(e2)
            try:
                api(a.base, token, "/api/v1/input/sequence", "POST", {"events": evs})
                return
            except urllib.error.HTTPError as e:
                if e.code != 409 or time.time() - t0 > a.seconds - 2:
                    raise RuntimeError(
                        f"input/sequence rejected segment {i} ({len(evs)} events, "
                        f"t {evs[0]['t']}..{evs[-1]['t']} ms, shift {shift}): HTTP {e.code} "
                        f"{e.read().decode('utf-8','replace')[:200]}") from e
                time.sleep(0.5)

    post_segment(0)

    # pointer cells: the guest keeps entity/record objects on the heap; a fixed-size block at each
    # pointer is dumped so record fields can be compared at a fixed record offset (no layout guess).
    cells = json.load(open(os.path.join(HERE, "..", "audit", "gen", "rig_cells.json")))
    ptr_cells = [c for c in cells if c["ptr"] and c["guest_size"] in (0, 4)]
    # 0x160 covers the whole entity record used by the state oracle: EXP total +0x2b, score +0x52,
    # position +0x56/+0x73, score award +0xbb. The link fields at +0x13f/+0x143 are width-adapted in
    # the port (docs/94 family) and are deliberately not compared.
    PTR_BYTES = 0x160

    # Second-level chain (docs/107): the action record an entity is running right now. The guest
    # resolves it at 0x00012cf9-0x00012d0f as *(e+0x123) -> [+4] -> +4*e[+0x17] -> record, and reads
    # byte[record+0x6c] as the gate for the knockback + EXP award (fn_012616 / fn_01441e). The rig
    # dumps 0x80 bytes of that record for the cells named by --chain-cells so the gate byte is
    # comparable on both sides instead of inferred.
    chain_cells = {int(x, 16) for x in a.chain_cells.split(",")} if a.chain_cells else set()
    CHAIN_BYTES = 0x80

    def chain_block(blk):
        if len(blk) < 0x127:
            return bytes(CHAIN_BYTES)
        p1 = int.from_bytes(blk[0x123:0x127], "little")
        if not p1:
            return bytes(CHAIN_BYTES)
        rarr = read_at(a.base, token, p1 + 4, 4)
        if len(rarr) < 4:
            return bytes(CHAIN_BYTES)
        rarr = int.from_bytes(rarr, "little")
        st = int.from_bytes(blk[0x17:0x1b], "little") & 0x7FFFFFFF
        rec = read_at(a.base, token, rarr + 4 * st, 4)
        if len(rec) < 4:
            return bytes(CHAIN_BYTES)
        rec = int.from_bytes(rec, "little")
        return read_at(a.base, token, rec, CHAIN_BYTES) if rec else bytes(CHAIN_BYTES)

    def read_ptrs(ds):
        if a.no_heap:
            return b""
        out = b""
        for c in ptr_cells:
            v = int.from_bytes(ds[c["ds"] - DS_LO: c["ds"] - DS_LO + 4], "little")
            blk = read_at(a.base, token, v, PTR_BYTES) if v else bytes(PTR_BYTES)
            out += blk
            if c["ds"] in chain_cells:
                out += chain_block(blk)
        return out

    states, prev = [], None
    last_ds = None
    all_res, res_events = set(), []
    while time.time() - t0 < a.seconds:
        ds = read_ds(a.base, token, phys_base)
        fr = api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)
        info = json.loads(api(a.base, token, "/api/v1/video/frame/info"))
        dsh, frh = masked_digest(ds, mask), hashlib.sha1(fr).hexdigest()
        now = time.time() - t0
        for nm in res_names(ds):
            if nm not in all_res:
                all_res.add(nm)
                res_events.append({"t_wall": round(now, 3), "event": "res", "name": nm})
        if prev != (dsh, frh):
            states.append({"t_wall": round(now, 3), "ds_sha": dsh, "frame_sha": frh,
                           "mode": info.get("video_mode"), "w": info.get("width"),
                           "h": info.get("height"), "changed_ds": last_ds != dsh})
            last_ds = dsh
            with open(os.path.join(a.out, f"state_{len(states) - 1:04d}.ds.bin"), "wb") as f:
                f.write(ds)
            with open(os.path.join(a.out, f"state_{len(states) - 1:04d}.frame.bin"), "wb") as f:
                f.write(fr)
            with open(os.path.join(a.out, f"state_{len(states) - 1:04d}.heap.bin"), "wb") as f:
                f.write(read_ptrs(ds))
            prev = (dsh, frh)
        now_ms = int((time.time() - t0) * 1000)
        while seg_i + 1 < len(segments):
            w = segments[seg_i]["wait"]
            if not w:
                break
            v, hit = ds_cond(a.base, token, phys_base, w)
            if not hit and now_ms < w["t"] + shift:
                break
            shift = now_ms - w["t"]                  # same rebase rule as the port's playtest script
            rebases.append({"t_ms": now_ms, "nominal": w["t"]})
            ds_waits.append({"t_ms": now_ms, "ds": hex(w["ds"]), "op": w["op"], "val": w["val"],
                             "observed": v, "hit": bool(hit), "shift_ms": shift})
            seg_i += 1
            post_segment(seg_i)
        time.sleep(1.0 / a.hz)

    if a.audio_capture:
        try:
            api(a.base, token, "/api/v1/capture/audio/stop", "POST", {})
        except Exception:
            pass
    # order must match read_ptrs: each chain block follows the entity block it was derived from
    pcs = []
    for c in ptr_cells:
        pcs.append({"ds": c["ds"], "name": c["name"], "rec_bytes": PTR_BYTES})
        if c["ds"] in chain_cells:
            pcs.append({"ds": c["ds"], "name": c["name"] + ".act", "rec_bytes": CHAIN_BYTES,
                        "chain": 1})
    meta = {"ptr_cells": pcs,
            "drive": os.path.abspath(a.drive), "events": events, "anchor": anchor,
            "ds_waits": ds_waits, "rebases": rebases, "hz": a.hz,
            "seconds": a.seconds, "states": len(states), "cycles": cyc,
            "res_events": res_events, "timeline": states}
    json.dump(meta, open(os.path.join(a.out, "timeline.json"), "w"), indent=1)
    print(f"{len(states)} distinct states over {a.seconds}s -> {a.out}/timeline.json")


if __name__ == "__main__":
    main()
