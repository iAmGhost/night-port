#!/usr/bin/env python3
"""Drive and capture the ORIGINAL game through the playtest route, headless.

The port side is scripted and deterministic (`--script`, `--shots`), so the only way to compare *the same moments*
is to anchor the original's timeline on content rather than on wall-clock time: boot timing under cycles=max is not
reproducible to the second, but which card is on screen is. This tool polls the dosbox-automation REST frame API,
classifies every frame against reference images (port shots already proven byte-identical to the shipped cards,
docs/58/docs/63), and runs a small drive file of `wait_label` / `key` / `shot` steps. Frames are stored as
`NNNN_<event>.png` next to an events.tsv carrying the detected label, similarity and emulated video mode, so the
compare tool can pair port shots and original frames by event name.

  usage: playtest_orig_drive.py --out DIR [--drive FILE] [--refs DIR] [--token T] [--base URL] [--hz 4]

Drive file syntax (one step per line, '#' comments):
  wait_label <label> [timeout_s]   block until a frame matches <label> (refs/<label>.*), or time out
  wait_mode <height> [timeout_s]   block until the captured frame's pixel height equals <height>; in this game
                                   480 means a VESA 640x480 card (logo/slides/stage card) and 200 means the
                                   mode-13h world (.fli intro, gameplay) -- a far steadier anchor than image
                                   matching, since animation frames never match a single reference well
  label_at_most <max>              continue when the best match is at or below a threshold
  sleep <ms>                       wait
  key <KBD_name> [press_ms]        press then release (PIC-timer scheduled, cycle accurate)
  type <text>                      typing helper (/api/v1/input/type)
  key_until_not_label <key> <label> <maxsim> <tries> <gap_ms>
                                   press the key until the picture stops matching <label>, so a title card
                                   that needs the press to land after its animation finishes still starts
  record <label> <ms>              sample frames for <ms>, saving them as <label>_NN
  frame <event>                    save the next frame as event <event>
  stop                             end the drive
"""
import argparse, base64, json, os, struct, sys, time, urllib.error, urllib.request

KEY_DEFAULTS = {"enter": "KBD_enter", "esc": "KBD_esc"}   # the emulator spells it KBD_esc; a wrong
                                          # spelling is rejected with HTTP 400 "Unknown key: KBD_escape"


def api(base, token, path, method="GET", body=None, raw_body=None, ctype="application/json", retries=8):
    data = None
    if body is not None:
        data = json.dumps(body).encode()
    elif raw_body is not None:
        data = raw_body.encode() if isinstance(raw_body, str) else raw_body
    for attempt in range(retries + 1):
        req = urllib.request.Request(base + path, data=data, method=method,
                                     headers={"Authorization": "Bearer " + token,
                                              "Content-Type": ctype})
        try:
            with urllib.request.urlopen(req, timeout=20) as r:
                return r.read()
        except urllib.error.HTTPError as ex:
            # 409 on /api/v1/input/sequence means another injected sequence is still in flight (measured: a manual
            # single POST succeeds, the driver's back-to-back posts do not), so wait for the previous one to drain.
            if ex.code == 409 and attempt < retries:
                time.sleep(0.35)
                continue
            detail = ex.read().decode(errors="replace")[:300]
            raise RuntimeError("HTTP %d on %s %s: %s" % (ex.code, method, path, detail)) from ex


def parse_raw(b):
    w, h, pitch = struct.unpack_from("<IIi", b, 0)
    roww = abs(pitch) // 4                     # authoritative: header width is the DOS mode width
    return w, h, roww, pitch, b[15:15 + abs(pitch) * h]


def lum(rgb):
    return rgb[:, :, 0] * 0.299 + rgb[:, :, 1] * 0.587 + rgb[:, :, 2] * 0.114


def down(g, w=160, h=120):
    import numpy as np
    H, W = g.shape
    iy = np.clip(((np.arange(h) + 0.5) * H / h - 0.5), 0, H - 1).astype(int)
    ix = np.clip(((np.arange(w) + 0.5) * W / w - 0.5), 0, W - 1).astype(int)
    return g[np.ix_(iy, ix)]


def ncc(a, b):
    import numpy as np
    a = a - a.mean()
    b = b - b.mean()
    return float((a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-9))


def load_refs(dirs):
    import numpy as np
    from PIL import Image
    refs = {}
    for d in dirs:
        if not os.path.isdir(d):
            continue
        for f in sorted(os.listdir(d)):
            if not f.lower().endswith((".bmp", ".png")):
                continue
            label = os.path.splitext(f)[0]
            for strip in ("_hold", "_pcx"):
                pass
            label = label.replace(".pcx", "").replace("_hold", "")
            im = np.array(Image.open(os.path.join(d, f)).convert("RGB")).astype(np.float64)
            refs.setdefault(label, []).append((os.path.join(d, f), down(lum(im)), im))
    return refs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--drive", required=True)
    ap.add_argument("--refs", action="append", default=["scratch/origcmp/refs"])
    ap.add_argument("--token", default=os.environ.get("DOSBOX_API_TOKEN", ""))
    ap.add_argument("--base", default=os.environ.get("DOSBOX_BASE", "http://localhost:8386"))
    ap.add_argument("--hz", type=float, default=4.0)
    ap.add_argument("--thresh", type=float, default=0.90)
    ap.add_argument("--swap13", type=int, default=1,
                    help="1 = decode mode-13h payloads as BGRX (measured, see the comment in poll()); 0 keeps them as-is")
    a = ap.parse_args()
    import numpy as np
    from PIL import Image
    if not a.token:
        sys.exit("no token")
    refs = load_refs(a.refs)
    if not refs:
        sys.exit("no reference images in %s" % a.refs)
    print("refs: %s" % ", ".join(sorted(refs)))
    steps = []
    for ln in open(a.drive):
        ln = ln.split("#")[0].strip()
        if ln:
            steps.append(ln.split())
    os.makedirs(a.out, exist_ok=True)
    man = open(os.path.join(a.out, "events.tsv"), "w")
    man.write("seq\tt_ms\tevent\tlabel\tsim\tmode\tbios\tfile\n")
    t0 = time.time()
    seq = 0
    state = {"label": "", "sim": 0.0, "px": None, "mode": "", "bios": "", "roww": 0, "h": 0}
    pending_shot = None

    def save(event, why=""):
        nonlocal seq
        if state["px"] is None:
            return
        name = "%04d_%s.png" % (seq, event)
        Image.fromarray(state["px"][:, :, :3].astype(np.uint8)).save(os.path.join(a.out, name))
        man.write("%d\t%d\t%s\t%s\t%.4f\t%s\t%s\t%s\n" % (
            seq, int((time.time() - t0) * 1000), event, state["label"], state["sim"],
            state["mode"], state["bios"], name))
        man.flush()
        print("  shot %-16s label=%-10s sim=%.4f mode=%s -> %s" % (event, state["label"], state["sim"], state["mode"], name))
        seq += 1

    def classify():
        best, bl = -2.0, ""
        for label, entries in refs.items():
            for _, g, _ in entries:
                s = ncc(g, down(lum(state["px"][:, :, :3].astype(np.float64))))
                if s > best:
                    best, bl = s, label
        state["label"], state["sim"] = bl, best

    def poll():
        it = time.time()
        info = json.loads(api(a.base, a.token, "/api/v1/video/frame/info"))
        raw = api(a.base, a.token, "/api/v1/video/frame?format=raw")
        w, h, roww, pitch, pix = parse_raw(raw)
        px = np.frombuffer(pix, dtype=np.uint8).reshape(h, roww, 4)
        # In mode 13h the payload is the emulator's double-width surface: frame/info reports the true image
        # width (320) with double_width=true, and the right half repeats the left half. Cropping to the true
        # width makes the original's frames directly comparable to the port's 320x200 shots.
        true_w = int(info.get("width", roww)) or roww
        if roww > true_w:
            px = px[:, :true_w, :]
        # Measured channel order, per mode (do not "unify" this): with the SAME decode the VESA 101h payloads
        # reproduce the shipped PCX palettes byte for byte (the title-card pair matched at 100.0000 % of pixels
        # within +-3), but the mode-13h payloads come out R/B exchanged -- proved from the stage-1 room frame,
        # whose colours equal the shipped stage1.wal palette at +0x4e21e exactly in the port (e.g. index 1 =
        # (182,97,97), 378 pixels) and exactly mirrored in the capture, and whose HUD portrait tint is warm
        # (149,119,100) in the port but cool (101,119,149) in the capture. So mode-13h payloads are BGRX.
        if a.swap13 and h == 200:
            px = px[:, :, (2, 1, 0, 3)]
        state["px"] = px
        state["h"] = h
        state["roww"] = px.shape[1]
        vm = info.get("video_mode", {})
        state["mode"] = "%dx%d" % (vm.get("width", 0), vm.get("height", 0))
        state["bios"] = str(vm.get("bios_mode_number", ""))
        classify()
        print("frame %s sim=%.4f best=%-12s mode=%s" % (state["mode"], state["sim"], state["label"], state["bios"]))
        return it

    def key(name, press_ms=150):
        # The API rejects unknown key names with a 400 ("Unknown key: down"), so bare direction/space names
        # are mapped to the emulator's KBD_ spelling.
        kn = KEY_DEFAULTS.get(name, name if name.startswith("KBD_") else "KBD_" + name)
        api(a.base, a.token, "/api/v1/input/sequence", "POST",
            {"events": [{"type": "key", "key": kn, "pressed": True},
                        {"type": "key", "key": kn, "pressed": False, "delay_ms": press_ms}]})

    # ---- guest memory (dosbox-automation webserver) -------------------------------------------
    # The image is relocated at load: code sits at (disassembler address + delta), and absolute data
    # references inside the code were fixed up to (DAT address - 0x50000 + R). Both are derived per session
    # instead of assumed: delta from a code pattern that must match NIGHT.EXE byte for byte, then R from the
    # relocated immediate of `mov dword [DAT_00063370], 1` at 0x365eb (docs/74).
    mem_state = {"delta": None, "R": None}

    def exe_slice(addr: int, n: int) -> bytes:
        with open(os.environ.get("NIGHT_EXE", "NIGHT/NIGHT.EXE"), "rb") as f:
            f.seek(addr - 0x2600)
            return f.read(n)

    def mem_read(phys: int, n: int) -> bytes:
        return api(a.base, a.token, f"/api/v1/memory/{phys}/{n}")

    def mem_search(value: int, width: int = 4) -> list[int]:
        return json.loads(api(a.base, a.token, "/api/v1/memory/search", "POST",
                              {"value": value, "width": width, "start": 0, "end": 1 << 24})).get("matches", [])

    def mem_calibrate() -> None:
        if mem_state["delta"] is not None:
            return
        # The search endpoint is unreliable (500s intermittently), so the constants can be supplied from a previous
        # calibration; they are then still verified against NIGHT.EXE byte for byte before anything is written.
        cd, cb = os.environ.get("DBA_CODE_DELTA"), os.environ.get("DBA_DATA_BASE")
        if cd and cb:
            delta, R = int(cd, 0), int(cb, 0)
            ok = all(mem_read(0x3bb42 + delta, 12) == exe_slice(0x3bb42, 12) for _ in (0,))
            if not ok:
                raise SystemExit("memory: supplied DBA_CODE_DELTA does not match NIGHT.EXE at the code anchor")
            b = mem_read(0x365eb + delta, 10)
            if b[:2] != b"\xc7\x05" or int.from_bytes(b[2:6], "little") != 0x13370 + R:
                raise SystemExit("memory: supplied DBA_DATA_BASE is not what the code references (bytes %s)" % b.hex())
            mem_state.update(delta=delta, R=R)
            print("memory: using supplied code delta=%#x data base=%#x (verified against the loaded image)"
                  % (delta, R))
            return
        deltas = {}
        for anchor_addr in (0x3bb42, 0x3a2d7, 0x12efa, 0x40660):
            want = exe_slice(anchor_addr, 12)
            for cand in mem_search(int.from_bytes(want[:4], "little")):
                if mem_read(cand, 12) == want:
                    deltas.setdefault(cand - anchor_addr, []).append(anchor_addr)
        if not deltas:
            raise SystemExit("memory: no code anchor matched NIGHT.EXE -- cannot calibrate")
        delta = max(deltas.items(), key=lambda kv: len(kv[1]))[0]
        b = mem_read(0x365eb + delta, 10)          # c7 05 <abs32> 01 00 00 00
        if b[:2] != b"\xc7\x05":
            raise SystemExit("memory: unexpected bytes at 0x365eb (%s), cannot derive the data base" % b.hex())
        R = int.from_bytes(b[2:6], "little") - 0x13370
        mem_state.update(delta=delta, R=R)
        print("memory: code delta=%#x data base=%#x (DAT_x -> phys x-0x50000%+#x)" % (delta, R, R))

    def mem_phys(dat: int) -> int:
        mem_calibrate()
        return (dat - 0x50000) + mem_state["R"]

    def mem_put(phys: int, payload: bytes) -> None:
        api(a.base, a.token, f"/api/v1/memory/{phys}", "PUT", raw_body=payload,
            ctype="application/octet-stream")

    si = 0
    period = 1.0 / a.hz
    last = None
    while si < len(steps):
        step = steps[si]
        op = step[0]
        if op in ("wait_label", "label_at_most", "wait_mode", "wait_mode_soft"):
            limit = float(step[2]) if len(step) > 2 else 120.0
            start = time.time()
            hit = False
            while time.time() - start < limit:
                it = poll()
                if pending_shot:
                    save(pending_shot)
                    pending_shot = None
                if op in ("wait_mode", "wait_mode_soft"):   # geometry is the reliable signal in this game: 480 = VESA card
                    hit = state["h"] == int(step[1])
                elif op == "wait_label":
                    hit = state["label"] == step[1] and state["sim"] >= a.thresh
                else:
                    hit = state["sim"] <= float(step[1])
                if hit:
                    break
                time.sleep(max(0.0, period - (time.time() - it)))
            print("%s %s: %s (%.1fs)" % (op, step[1] if len(step) > 1 else "", "hit" if hit else "TIMEOUT", time.time() - start))
            if not hit and op not in ("label_at_most", "wait_mode_soft"):
                print("  giving up on the drive")
                break
            si += 1
            continue
        if op == "key_until_not_label":     # key_until_not_label <key> <label> <maxsim> <tries> <gap_ms>
            kn, lab, mx = step[1], step[2], float(step[3])
            tries, gap = int(step[4]), float(step[5]) / 1000.0
            for n in range(tries):
                poll()
                if state["label"] != lab or state["sim"] <= mx:
                    print("  left %s after %d press(es) (sim %.3f)" % (lab, n, state["sim"]))
                    break
                key(kn, 150)
                time.sleep(gap)
            si += 1
            continue
        if op == "key_until_mode":      # key_until_mode <height> <key> <tries> <gap_ms>: the picture show
            want, kn = int(step[1]), step[2]        # waits a key per card, and the count per episode differs
            tries, gap = int(step[3]), float(step[4]) / 1000.0
            hit = False
            for n in range(tries):
                it = poll()
                if state["h"] == want:
                    hit = True
                    print("  reached mode %d after %d press(es)" % (want, n))
                    break
                key(kn, 150)
                time.sleep(gap)
            print("key_until_mode %d: %s" % (want, "hit" if hit else "TIMEOUT"))
            si += 1
            continue
        if op == "key_until_mode_stable":   # key_until_mode_stable <height> <key> <tries> <gap_ms> <stable>
            want, kn = int(step[1]), step[2]   # like key_until_mode, but the mode must hold for <stable>
            tries, gap = int(step[3]), float(step[4]) / 1000.0   # consecutive samples. Measured reason (docs/81):
            stable, run, hit, presses = int(step[5]), 0, False, 0  # a press landing as the picture show ends makes
            for _ in range(tries):            # the game flash mode 13h for one sample and fall back to a 640x480
                it = poll()                   # card, where it waits forever; accepting that flash ends the pacing
                if state["h"] == want:        # early and the stage never plays (two runs measured).
                    run += 1
                    if run >= stable:
                        hit = True
                        print("  mode %d held %d samples after %d press(es)" % (want, stable, presses))
                        break
                    time.sleep(max(0.0, period - (time.time() - it)))
                    continue
                run = 0
                key(kn, 150)
                presses += 1
                time.sleep(gap)
            print("key_until_mode_stable %d: %s (%d press(es))" % (want, "hit" if hit else "TIMEOUT", presses))
            si += 1
            continue
        if op == "shot":
            if state["px"] is not None:
                save(step[1])
            si += 1
            continue
        if op == "record":                       # record <label> <seconds>: sample the window so a stretch of
            label = step[1]                       # animation/slides is captured without knowing its content yet
            until = time.time() + float(step[2]) / 1000.0
            n = 0
            while time.time() < until:
                it = poll()
                n += 1
                save("%s_%02d" % (label, n))
                time.sleep(max(0.0, period - (time.time() - it)))
            si += 1
            continue
        if op == "frame":
            pending_shot = step[1]
            si += 1
            continue
        if op == "key":
            key(step[1], int(step[2]) if len(step) > 2 else 150)
            si += 1
            continue
        if op == "type":
            api(a.base, a.token, "/api/v1/input/type", "POST", {"text": step[1]})
            si += 1
            continue
        if op == "sleep":
            time.sleep(float(step[1]) / 1000.0)
            si += 1
            continue
        if op in ("writemem", "readmem"):        # writemem <DAT-hex> <value> / readmem <DAT-hex>
            dat = int(step[1], 0)
            if op == "readmem":
                v = int.from_bytes(mem_read(mem_phys(dat), 4), "little")
                print("readmem %s = %d (%#x) at phys %#x" % (step[1], v, v, mem_phys(dat)))
            else:
                val = int(step[2], 0)
                mem_put(mem_phys(dat), val.to_bytes(4, "little"))
                back = int.from_bytes(mem_read(mem_phys(dat), 4), "little")
                print("writemem %s <- %d : read back %d at phys %#x" % (step[1], val, back, mem_phys(dat)))
            si += 1
            continue
        if op == "poke_at":     # poke_at <DAT-hex holding a pointer> <offset-hex> <width> <value>
            # The game's own heap pointers are absolute (measured: DAT_00063354 = 0x38202c in a live room), so
            # a pointer read from DAT space addresses guest physical memory directly. Values below 0x100000 are
            # treated as DS-relative (the DAT_x convention) instead.
            dat = int(step[1], 0)
            ptr = int.from_bytes(mem_read(mem_phys(dat), 4), "little")
            off, width, val = int(step[2], 0), int(step[3]), int(step[4], 0)
            phys = ptr if ptr >= 0x100000 else mem_phys(ptr & 0xFFFFFF)
            mem_put(phys + off, val.to_bytes(width, "little"))
            back = mem_read(phys + off, width)
            print("poke_at %s(+%#x) ptr=%#x -> phys %#x <- %d : read back %s"
                  % (step[1], off, ptr, phys + off, val, back.hex()))
            si += 1
            continue
        if op == "readptr":     # readptr <DAT-hex>  (report the pointer and where it maps)
            dat = int(step[1], 0)
            ptr = int.from_bytes(mem_read(mem_phys(dat), 4), "little")
            print("readptr %s = %d (%#x)%s" % (step[1], ptr, ptr, "  [absolute]" if ptr >= 0x100000 else "  [DS-relative]"))
            si += 1
            continue
        if op == "recoff":      # recoff <DAT-hex holding a record pointer> <field-offset-hex>
            # Read one 4-byte field of the record the game's own pointer refers to. A cleared link reads as
            # DS offset 0 in DOS (physical data base), which is mapped memory -- not a fault.
            dat = int(step[1], 0)
            ptr = int.from_bytes(mem_read(mem_phys(dat), 4), "little")
            off = int(step[2], 0)
            phys = (ptr + off) if ptr >= 0x100000 else (mem_phys(ptr & 0xFFFFFF) + off)
            v = int.from_bytes(mem_read(phys, 4), "little")
            print("recoff %s+%s ptr=%#x -> phys %#x = %d (%#x)%s"
                  % (step[1], step[2], ptr, phys, v, v, "  [cleared link -> DS:0]" if ptr == 0 else ""))
            si += 1
            continue
        if op == "stop":
            break
        print("unknown step: %s" % " ".join(step))
        si += 1
    man.close()
    print("done: %d saved frames in %s" % (seq, a.out))


if __name__ == "__main__":
    main()
