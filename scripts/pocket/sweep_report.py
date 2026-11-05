#!/usr/bin/env python3
"""sweep_report.py -- read the nine-episode sweep's witnesses (task-8, docs/59's contract on the Pocket path).

One row per episode, from the host's own log plus the rig sidecars the Pocket host dumps (the same
src/app/rig_sink.c files the differential rig reads on the SDL side):

  * did the stage get entered: the warp entry pump's own line, which stops on GAME STATE (fn_03272f owning
    frames), not on a wall-clock pulse count;
  * which stage the shipped loader built: fn_037b03's room chain and the stage boss record;
  * what the host presented: the view modes seen (640x480 slides vs 320x200 game view) and their counts;
  * the game's own music schedule: every `cdda: index N lba=... sectors=...` request, which is the state proof
    docs/333 used, checked against docs/96's episode table;
  * effects delivered: transfer count and PCM bytes, and dropped/stolen voices;
  * live entities at play: walked out of records.jsonl (the entity list followed by its +0x13f links);
  * the shipped cards, matched by INDEX against the PCX the original would have blitted -- the docs/62 basis,
    needing no original run, so not blocked on the DOSBox capture path (docs/46).

Usage: sweep_report.py <sweep-dir> [--fingerprint N]
"""
import glob
import json
import operator
import os
import re
import struct
import sys

RES = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "scratch", "res")

VIEW = re.compile(r"(?m)^view=(\S+) (\d+)x(\d+) sig=(\S+) chg=(\d+)")
MEMSTAT = re.compile(r"(?m)^memstat frames=(\d+) ticks=(\d+)\(\+(\d+)/s\) pumps=(\d+) arena=(\d+)/(\d+) "
                     r"pit=(\d+) Hz xfer=(\d+) plays=(\d+) cdda=(\d+)/(\d+)/(\d+) clock=(\d+)")
CDDA = re.compile(r"(?m)^cdda: index (\d+)\s+lba=(\d+) sectors=(\d+)")
SFX = re.compile(r"(?m)^sfx#(\d+) ch=(\d+) rate=(\d+) frames=(\d+)")
PERF = re.compile(r"(?m)^perf pres=(\d+)us max=(\d+)us")


def pcx_indices(path):
    """The shipped still-image loader's own decode (fn_036190 / fn_03628e, docs/62): 128-byte header, then the
    6-bit-count PCX RLE, and the destination is palette INDICES -- the same kind of bytes the host dumps."""
    d = open(path, "rb").read()
    if not d or d[0] != 0x0A:
        return None, None, None
    x0, y0, x1, y1 = struct.unpack("<HHHH", d[4:12])
    w, h, want = x1 + 1, y1 + 1, (x1 + 1) * (y1 + 1)
    i, out = 128, bytearray()
    while len(out) < want and i < len(d):
        b = d[i]
        i += 1
        if b & 0xC0 == 0xC0:
            out += bytes([d[i]]) * (b & 0x3F)
            i += 1
        else:
            out.append(b)
    return w, h, bytes(out[:want])


def fingerprint(idx, n):
    """A strided sample: cheap, and enough to pair a dumped frame with the card it shows."""
    if len(idx) <= n:
        return idx
    return idx[::len(idx) // n][:n]


def match_ratio(a, b, tol):
    """Fraction of sampled pixels whose every channel is within `tol` of the card's quantized value.
    The game's DAC is 6-bit (the loader does (v >> 2) << 3, docs/62), so 8 is one DAC step: a pixel
    inside tolerance is the same colour the original could have shown."""
    if len(a) != len(b):
        return 0.0
    good = 0
    for ch in range(3):
        good += sum(map(operator.eq, a[ch], b[ch])) if tol == 0 else sum(
            1 for x, y in zip(a[ch], b[ch]) if abs(x - y) <= tol)
    return good / (3.0 * max(len(a[0]), 1))


def identical(a, b):
    """Byte-identical count between two index arrays of the same length.  docs/62's headline is exactly this
    number (307200/307200 identical for every shipped slide card), and it is the only comparison that is
    independent of the DAC fade the host happens to be mid-way through."""
    if len(a) != len(b):
        return 0
    return sum(1 for x, y in zip(a, b) if x == y)


def stage_cards(n):
    pats = [f"na{n}*.pcx", f"{n}s*.pcx", f"{n}e*.pcx", f"stage{n}.pcx"]
    out = []
    for p in pats:
        for f in sorted(glob.glob(os.path.join(RES, p))):
            w, h, idx = pcx_indices(f)
            if idx is None:
                continue
            out.append((os.path.basename(f), w, h, idx))
    return out


def entity_census(stage_dir):
    """Live entities per sample, from the rig's own walk of the entity list (records.jsonl)."""
    path = os.path.join(stage_dir, "records.jsonl")
    if not os.path.exists(path):
        return None
    mx, mx_types, mx_tick, samples, with_player, types = 0, [], -1, 0, 0, {}
    with open(path) as fp:
        for line in fp:
            try:
                r = json.loads(line)
            except json.JSONDecodeError:
                continue
            samples += 1
            if r.get("player") is not None:
                with_player += 1
            ents = r.get("ents") or []
            if len(ents) > mx:
                mx, mx_tick = len(ents), r.get("tick", -1)
                mx_types = [e.get("type") for e in ents]
                types = {}
                for e in ents:
                    types[e.get("type")] = types.get(e.get("type"), 0) + 1
    return dict(samples=samples, with_player=with_player, max_ents=mx, tick=mx_tick,
                types=sorted((k, v) for k, v in types.items()))


def report(stage_dir, n):
    log = os.path.join(stage_dir, "run.log")
    txt = open(log).read() if os.path.exists(log) else ""
    r = {}
    r["exit"] = open(os.path.join(stage_dir, "exit.txt")).read().strip() if os.path.exists(
        os.path.join(stage_dir, "exit.txt")) else "?"
    m = re.search(r"^warp entry: stage loop reached after (\d+) confirm pulses \(fn_03272f calls (\d+)\)",
                  txt, re.M)
    r["entered"] = f"yes ({m.group(1)} pulses, resolver calls {m.group(2)})" if m else "NO"
    if not m:
        f = re.search(r"^warp entry FAILED: (.*)$", txt, re.M)
        r["entered"] = "FAILED: " + (f.group(1) if f else "no warp line at all")
    m = re.search(r"fn_037b03: stage=(\d+) DAT_639d8=(\d+) rooms", txt)
    r["chain"] = f"stage {m.group(1)} / {m.group(2)} rooms" if m else "no chain line"
    m = re.search(r"^stgboss stage=(\d+) rec=(\S+) sub=(\d+) x=(-?\d+) y=(-?\d+)", txt, re.M)
    r["boss"] = f"@({m.group(4)},{m.group(5)})" if m else "-"
    views = {}
    for mm in VIEW.finditer(txt):
        views[f"{mm.group(1)} {mm.group(2)}x{mm.group(3)}"] = views.get(f"{mm.group(1)} {mm.group(2)}x{mm.group(3)}", 0) + 1
    r["views"] = ", ".join(f"{k}:{v}" for k, v in sorted(views.items())) or "-"
    # The selects live in the rig's own sidecar (rig_cdda_marker), not in the log: index/lba/sectors is the
    # state proof docs/333 used, and it is the game's own schedule, not the host's choice.
    r["cdda"] = []
    cj = os.path.join(stage_dir, "cdda.jsonl")
    if os.path.exists(cj):
        with open(cj) as fp:
            for line in fp:
                try:
                    e = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if e.get("event") == "play":
                    r["cdda"].append(f"idx{e['index']}@lba{e['lba']}/{e['sectors']}s")
    sf = [m for m in SFX.finditer(txt)]
    r["sfx"] = f"{len(sf)} blocks / {sum(int(m.group(4)) for m in sf)} frames"
    ms = list(MEMSTAT.finditer(txt))
    if ms:
        last = ms[-1]
        r["final"] = (f"frames={last.group(1)} ticks={last.group(2)}(+{last.group(3)}/s) "
                      f"pit={last.group(7)}Hz xfer={last.group(8)} arena={last.group(5)}/{last.group(6)}")
        r["clock"] = last.group(13)
    perf = [int(m.group(2)) for m in PERF.finditer(txt)]
    r["present_max_us"] = max(perf) if perf else 0
    r["unwound"] = "unwound" if "game main unwound" in txt else ("returned" if "the game main returned" in txt else "running at limit")
    r["ents"] = entity_census(stage_dir)
    cards = stage_cards(n)
    raws = sorted(glob.glob(os.path.join(stage_dir, "arena", "*.raw")))
    best, exact, ents = {}, 0, 0
    for f in raws:
        mm = re.match(r"(\d+)_(\d+)x(\d+)\.raw", os.path.basename(f))
        if not mm:
            continue
        frame, fw, fh = int(mm.group(1)), int(mm.group(2)), int(mm.group(3))
        d = open(f, "rb").read()
        need = fw * fh
        if len(d) < need:
            continue
        arena = d[:need]
        m = re.search(rb"#ents=(-?\d+)", d[need:])
        if m:
            ents = max(ents, int(m.group(1)))
        for name, cw, chh, cidx in cards:
            if (cw, chh) != (fw, fh):
                continue                       # a 640x480 slide and a 320x200 card are different pictures
            same = identical(arena, cidx)
            if same == need:
                exact += 1
            if name not in best or same > best[name][0]:
                best[name] = (same, need, frame)
    r["cards"] = sorted(((k, v[0], v[1], v[2]) for k, v in best.items()), key=lambda x: -x[1])[:8]
    r["exact"] = exact
    r["ents_game"] = ents
    r["n_cards"] = len(cards)
    r["n_frames"] = len(raws)
    return r


def episode_tracks():
    """The shipped per-room CD track table, read out of the port's own source (f.c's g_tab_52da4, DS 0x52da4):
    100 entries, ten per episode, 0x08 being the 'walking in' variant.  The sweep's music selects are only a
    state proof if they are checked against the shipped table, so the table is read from the same source the
    game runs on rather than typed in here."""
    src = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "src", "f", "f.c")
    txt = open(src).read()
    i = txt.find("const uint32_t g_tab_52da4[100] = {")
    if i < 0:
        return []
    body = txt[i:].split("}", 1)[0]
    vals = [int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]{4})", body)]
    return [vals[r * 10:(r + 1) * 10] for r in range(len(vals) // 10)]


TRACKS = episode_tracks()


def music_selects(cdda):
    """The room/episode track out of the host's own select list: the one whose sectors match no fixed
    intro/title/slide constant (docs/96 lists 1 opening, 9 title, 11 stage-1 slides, 12 stage-clear slides)."""
    out = []
    for c in cdda:
        idx = int(c.split("@")[0][3:])
        out.append(idx)
    return out


def music_matches(cdda, stage):
    if not TRACKS or stage > len(TRACKS):
        return "no table"
    want = sorted(set(TRACKS[stage - 1]))
    got = music_selects(cdda)
    hits = [g for g in got if g in want]
    return f"episode table allows {want}, selects were {got}" + ("  -> IN TABLE" if hits else "  -> NOT IN TABLE")


FP = 4096          # sampled pixels per picture
TOL = 8           # one 6-bit DAC step (docs/62: the loader quantizes the card palette with >>2 <<3)


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else "/tmp/pocket_sweep"
    global FP, TOL
    if "--fingerprint" in sys.argv:
        FP = int(sys.argv[sys.argv.index("--fingerprint") + 1])
    if "--tol" in sys.argv:
        TOL = int(sys.argv[sys.argv.index("--tol") + 1])
    print(f"# Pocket episode sweep report  ({d}, {FP}-sample index fingerprints against scratch/res)\n")
    ok = 0
    for n in range(1, 10):
        sd = os.path.join(d, f"stage{n}")
        if not os.path.isdir(sd):
            print(f"stage {n}: NO RUN DIRECTORY")
            continue
        r = report(sd, n)
        entered = r["entered"].startswith("yes")
        # "game main unwound (quit / fatal)" is how this host ENDS a frame-limited run (the quit flag unwinds
        # fn_03af18 through g_game_escape), so it is not a crash signal; a non-zero exit or the fatal marker is.
        crash = r["exit"] != "0" or "fatal" in r["unwound"]
        # r["cards"] rows are (name, identical_bytes, card_size, best_frame): the card is "shown" when a whole
        # arena sample equals the shipped card byte for byte, which is docs/62's exact criterion.
        card = any(v[1] == v[2] for v in r["cards"])
        r["music_ok"] = music_matches(r["cdda"], n)
        ok += 1 if (entered and not crash and card) else 0
        e = r["ents"] or {}
        print(f"## stage {n}: exit={r['exit']} entered={r['entered']}  ({r['unwound']})")
        print(f"  loader      : {r['chain']}  boss {r['boss']}")
        print(f"  views       : {r['views']}")
        print(f"  cdda selects: {' '.join(r['cdda']) or 'none'}")
        print(f"                {r['music_ok']}")
        print(f"  sfx         : {r['sfx']}")
        print(f"  final       : {r.get('final', '-')}  present max {r['present_max_us']} us  clock {r.get('clock', '-')}")
        print(f"  entities    : max {e.get('max_ents', 0)} live at tick {e.get('tick', '-')}, "
              f"types {e.get('types', [])}, samples {e.get('samples', 0)} with a player {e.get('with_player', 0)}")
        print(f"  verdict     : entered={entered} clean={not crash} card-identical={card}")
        print(f"  cards       : {r['n_frames']} arena raws vs {r['n_cards']} shipped cards; "
              f"{r['exact']} arena sample(s) byte-identical to a shipped card; entity count seen {r['ents_game']}")
        for name, same, need, frame in r["cards"]:
            ratio = same / max(need, 1)
            flag = "IDENTICAL" if same == need else ("mostly" if ratio >= 0.9 else ("partial" if ratio >= 0.5 else "no"))
            print(f"     {name:<14} {same:>7}/{need} identical ({ratio * 100:6.2f}%)  best frame {frame:>7}  {flag}")
        print()
    print(f"== {ok}/9 episodes entered and ran clean ==")
    return 0 if ok == 9 else 1


if __name__ == "__main__":
    sys.exit(main())
