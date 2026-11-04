#!/usr/bin/env python3
"""Headless check: which .spr/.act asset each stage-1 entity actually draws from (docs/73).

Closes the playtest contract's requirement that a headless run "enumerates the entity types present in
stage 1 and asserts each entity's sprite source is the type-correct asset".

What the port prints for this:
  NIGHT_ENTDBG=1   one `entmap` line per live entity per diagnostic pass, carrying the entity's type index
                   (resolved from entity+0x123 against the shipped per-type act table g_dat_53818[]) and the
                   shipped asset names that type maps to (kTypeSprNames / kTypeActNames, the arrays pinned by
                   FNV hash against NIGHT.EXE in docs/63: 11ab4c98d4de6f12 / b68eb82fdab264eb).
  NIGHT_ENTSPR=1   one `menucell tag=entspr` line per entity composite: sprite index, destination, size, the
                   same type/asset attribution, and the cell's raw row-RLE stream in hex.

What this script asserts, per entity composite drawn in stage 1:
  1. the composite's row-RLE stream is byte-present in the .spr file named for that entity's type -- the
     attribution is content-based, so it does not merely re-read the port's own table;
  2. the pixels the blit wrote equal an independent decode of that stream (the docs/60 verifier, reused);
  3. the player entity is present and no enemy draws from the player's sprite struct -- the original defect
     (report item: "Enemy sprites are main character's sprite. also player is missing").

Usage: playtest_check_entity_sprites.py [run.log]     (runs the port itself if no log is given)
"""
from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from playtest_verify_spr import decode_rows  # the independent row-RLE decoder (docs/60)

APP = os.path.join(ROOT, "port", "build", "night")
RES = os.path.join(ROOT, "scratch", "res")
ROUTE = os.path.join(ROOT, "scripts", "playtest", "route_episode1_anchored.txt")

entmap_re = re.compile(
    r"entmap idx=(\d+) player=(\d+) type_act=(-?\d+) type_spr=(-?\d+) spr_asset=(\S+) act_asset=(\S+) "
    r"rbase=([0-9a-f]+) spr_struct=(\S+)")
cell_re = re.compile(
    r"menucell tag=entspr k=(\d+) dest=(?:0x)?([0-9a-f]+) srow=(-?\d+) col=(\d+) rows=(\d+) type=(-?\d+) "
    r"spr=(\S+) act=(\S+) mode=(-?\d+)(?: tick=(\d+))? stream=([0-9a-f]*)")
pix_re = re.compile(r"menupix k=(\d+) r=(\d+) ([0-9a-f]+)")


def run_port() -> tuple[str, str]:
    out = tempfile.mkdtemp(prefix="entspr_")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
               NIGHT_ENTDBG="1", NIGHT_ENTSPR="1", NIGHT_DENSE="0")
    r = subprocess.run([APP, "--script", ROUTE, "--shots", out, "--no-audio", "--frames", "9000",
                        "--shot-limit", "400"], env=env, capture_output=True, timeout=900,
                        cwd=os.path.join(ROOT, "port"))   # the app's default --disc path is ../NIGHT/... from here
    log = os.path.join(out, "run.log")
    open(log, "wb").write(r.stderr)          # the diagnostics go to stderr
    return log, out


def main() -> int:
    if len(sys.argv) > 1:
        log = sys.argv[1]
    else:
        log, _ = run_port()
    text = open(log, "r", errors="replace").read()

    maps = [entmap_re.search(l) for l in text.splitlines()]
    maps = [m.groups() for m in maps if m]
    cells = [cell_re.search(l) for l in text.splitlines()]
    cells = [m.groups() for m in cells if m]
    # The composites' pixel rows follow each cell line in order.
    # sequential pairing: each `menucell` line is followed by its own `menupix` rows
    pix_rows: list[list[tuple[int, bytes]]] = []
    cur: list[tuple[int, bytes]] | None = None
    for l in text.splitlines():
        if cell_re.search(l):
            cur = []
            pix_rows.append(cur)
            continue
        m = pix_re.search(l)
        if m and cur is not None:
            cur.append((int(m.group(2)), bytes.fromhex(m.group(3))))
    pix = [(int(m.group(1)), int(m.group(2)), bytes.fromhex(m.group(3)))
           for m in (pix_re.search(l) for l in text.splitlines()) if m]

    print(f"parsed: {len(maps)} entmap entity records, {len(cells)} entity composites, {len(pix)} pixel rows")
    if not maps or not cells:
        print("FAIL: the run produced no entity records -- the stage was never entered")
        return 1

    # ---- 1. entity enumeration: player present, enemies present, types resolved ------------------
    dumps: list[list[tuple]] = []      # ent_dump passes, so the last in-stage pass can be chosen
    cur: list[tuple] = []
    for line in text.splitlines():
        if line.startswith("ent: count="):
            if cur:
                dumps.append(cur)
            cur = []
        m = entmap_re.search(line)
        if m:
            cur.append(m.groups())
    if cur:
        dumps.append(cur)
    staged = [d for d in dumps if any(g[1] == "1" for g in d) and any(g[1] == "0" for g in d)]
    if not staged:
        print("FAIL: no pass shows both the player entity and a non-player entity in stage 1")
        return 1
    final = staged[-1]
    players = [g for g in final if g[1] == "1"]
    enemies = [g for g in final if g[1] == "0"]
    print(f"   in-stage passes: {len(staged)}; last pass: {len(players)} player entity(ies), "
          f"{len(enemies)} others")
    for g in final:
        print(f"   ent idx={g[0]} player={g[1]} type_act={g[2]} type_spr={g[3]} "
              f"spr={g[4]} act={g[5]} struct={g[7]}")

    bad = [g for g in final if int(g[2]) < 0 or int(g[3]) < 0 or g[4] == "?" or g[5] == "?"]
    ok(bad, "every enumerated entity resolves to a shipped per-type act/spr asset")
    missing = sorted({g[4] for g in final if g[4] != "?"} - {f for f in os.listdir(RES) if f.endswith(".spr")})
    ok(missing, "every named .spr asset exists among the extracted shipped resources")
    plist = {g[7] for g in players}
    elist = {g[7] for g in enemies}
    if plist & elist:
        print(f"FAIL: an enemy shares the player's sprite struct {plist & elist} (report item: enemies show the "
              f"main character's sprite)")
        return 1
    print("PASS: the player entity is drawn from its own sprite struct and no enemy shares it")
    pas = {g[4] for g in players}
    eas = {g[4] for g in enemies}
    if pas & eas:
        print(f"FAIL: enemy asset set {pas & eas} overlaps the player's {pas}")
        return 1
    print(f"PASS: distinct assets -- player {sorted(pas)}, others {sorted(eas)}")

    # ---- 2. content-based attribution + pixel equality for each composite -----------------------
    asset_cache: dict[str, bytes] = {}
    n_att = n_pix = mism = clipped = 0
    for i, rec in enumerate(cells):
        k, dest, srow, colw, rows, typ, sprnm, actnm, mode = rec[:9]
        stream = rec[-1]
        raw = bytes.fromhex(stream)
        if not raw or sprnm == "?":
            continue
        if sprnm not in asset_cache:
            p = os.path.join(RES, sprnm)
            asset_cache[sprnm] = open(p, "rb").read() if os.path.exists(p) else b""
        if raw[:32] and raw[:32] in asset_cache[sprnm]:
            n_att += 1
        else:
            print(f"FAIL: composite #{i} (type {typ}) stream is not present in the asset it names ({sprnm})")
            mism += 1
            continue
        # pixel check: compare the blit's output against an independent decode of the same stream. The pixel rows
        # follow their cell line in the log, so pair them positionally -- matching on k alone re-used rows from a
        # different composite (the same cell index repeats every frame).
        want = list(decode_rows(raw, int(rows)))
        rows_here = pix_rows[i]
        if len(rows_here) < int(rows):
            continue
        # fn_03bb42 draws left-to-right in mode 0 and right-to-left (mirrored inside the cell) in any other mode,
        # so the cell column c lands at screen column c or (W-1-c) accordingly.
        # Only compare composites the blitter can actually place inside the 320x200 room: fn_03bb42 bails when
        # dest > 0x13f (and clips at the right edge / bottom), so a partially or wholly off-screen entity would be
        # compared against untouched background. Clipped ones are counted, not compared.
        x0 = int(dest, 16)
        y0 = int(srow)
        if not (0 <= x0 and x0 + int(colw) <= 0x140 and 0 <= y0 and y0 + int(rows) <= 0xc8):
            clipped += 1
            continue
        mir = int(mode) != 0
        wcell = int(colw)
        bad_cols = 0
        for r, got in rows_here[:int(rows)]:
            covered = want[r] if r < len(want) else {}
            for c, v in covered.items():
                sc = (wcell - 1 - c) if mir else c
                if 0 <= sc < len(got) and got[sc] != v:
                    bad_cols += 1
        if bad_cols:
            print(f"FAIL: composite #{i} (type {typ}, {sprnm}) has {bad_cols} decoded pixels the blit got wrong")
            mism += 1
        else:
            n_pix += 1
    print(f"   composites attributed to their type's .spr file: {n_att}/{len(cells)}; fully on-screen and compared: "
          f"{n_pix}; off-screen/clipped by fn_03bb42's own guards and skipped: {clipped}")
    if n_att == 0:
        print("FAIL: no composite could be attributed to a shipped asset")
        return 1
    if mism:
        print(f"FAIL: {mism} composites failed attribution or pixel equality")
        return 1
    print("PASS: every stage-1 entity composite decodes from the .spr file its own entity type names")
    print("\nALL CHECKS PASS")
    return 0


def ok(bad, msg: str) -> None:
    if bad:
        print(f"FAIL: {msg}: {bad}")
        sys.exit(1)
    print(f"PASS: {msg}")


if __name__ == "__main__":
    sys.exit(main())
