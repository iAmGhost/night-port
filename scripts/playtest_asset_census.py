#!/usr/bin/env python3
"""Census of which shipped .spr assets the port is actually SEEN to draw (feature sweep, docs/75).

For a set of headless runs (nine warped stages, pause menu, help, level-up, game-over, ending) the port's
env-gated sprite traces print every sprite composite it composites: the cell's raw row-RLE stream plus the bytes the
blit wrote (NIGHT_ENTSPR=1 for entities, NIGHT_MENUBG=1 / NIGHT_HUDDBG=1 for screens and the HUD). This script takes
each composite's stream and finds which extracted shipped asset contains those exact bytes, then buckets the assets
into the families the playtest contract names. Attribution is by content, so it does not just re-read the port's own
name tables.

    scripts/playtest_asset_census.py            # runs everything, prints the census
"""
from __future__ import annotations

import collections
import glob
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(ROOT, "port", "test", "night")
RES = os.path.join(ROOT, "scratch", "res")
ROUTE_SHOW = "../scripts/playtest/route_show_keyed.txt"
ROUTE_PAUSE = "../scripts/playtest/route_pause_menu.txt"
ROUTE_TITLE = "../scripts/playtest/route_attract_wait.txt"

cell_re = re.compile(r"menucell tag=(\S+) k=(\d+) dest=(?:0x)?([0-9a-f]+) srow=(-?\d+) col=(\d+) rows=(\d+)"
                     r"(?: type=(-?\d+) spr=(\S+) act=(\S+))?.*? stream=([0-9a-f]+)")

RUNS = [("stage%d" % n, ["--warp-stage", str(n), "--script", ROUTE_SHOW],
         {"NIGHT_ENTSPR": "1", "NIGHT_HUDDBG": "1"}) for n in range(1, 10)]
ROUTE_WALK = "../scripts/playtest/route_walk_fight.txt"
RUNS += [("setup", ["--script", "../scripts/playtest/route_setup_menu.txt"], {"NIGHT_MENUBG": "1"}),
         ("pausemenu", ["--script", "../scripts/playtest/route_pause_menu.txt"], {"NIGHT_MENUBG": "1"})]

RUNS += [("walk%d" % n, ["--warp-stage", str(n), "--script", ROUTE_WALK],
          {"NIGHT_ENTSPR": "1", "NIGHT_HUDDBG": "1"}) for n in range(1, 10)]
ROUTE_LU = "../scripts/playtest/route_levelup_warp.txt"
RUNS += [
    ("levelup2", ["--script", ROUTE_TITLE, "NIGHT_SHOWSCREEN=levelup", "NIGHT_SHOWSCREEN_AT=2777"],
     {"NIGHT_MENUBG": "1"}),
]
RUNS += [
    ("pause", ["--script", ROUTE_PAUSE], {"NIGHT_MENUBG": "1"}),
    ("help", ["--script", ROUTE_SHOW, "NIGHT_SHOWSCREEN=help"], {"NIGHT_MENUBG": "1"}),
    ("levelup", ["--script", ROUTE_SHOW, "NIGHT_SHOWSCREEN=levelup"], {"NIGHT_MENUBG": "1"}),
    ("gameover", ["--script", ROUTE_SHOW, "NIGHT_SHOWSCREEN=gameover"], {"NIGHT_MENUBG": "1"}),
    ("ending", ["--script", ROUTE_SHOW, "NIGHT_SHOWSCREEN=ending"], {"NIGHT_MENUBG": "1"}),
]

FAMILIES = ([f"ena{i}" if False else f"en{c}" for c in "abcdefghij"] +
            [f"st{i}s" for i in range(1, 10)] + ["mboss1", "mboss2", "mboss3", "mboss4", "mboss5", "mboss6",
                                                 "boss1", "boss2", "boss3", "boss4", "boss5"] +
            ["dust", "mark", "exp", "face", "gage", "gun", "levelup", "over", "pause", "system"])


def family_of(name: str) -> str:
    n = name.lower()
    for f in sorted(FAMILIES, key=len, reverse=True):
        if n.startswith(f):
            return f
    return "(other)"


def main() -> int:
    assets = {}
    for p in sorted(glob.glob(os.path.join(RES, "*.spr"))):
        assets[os.path.basename(p)] = open(p, "rb").read()

    seen: dict[str, dict[str, int]] = collections.defaultdict(lambda: collections.Counter())
    loaded: dict[str, set] = {}    # docs/80: the shipped assets each run LOADS (resource-lookup trace)
    unattributed: collections.Counter = collections.Counter()
    want = sys.argv[1:] or ["all"]
    runs = [(l, a, e) for (l, a, e) in RUNS
            if "all" in want or l in want or any(l.startswith(w) for w in want)]
    for label, argv, env in runs:
        e = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", NIGHT_SHOTLIMIT="4",
                 NIGHT_RESLOG="1", **env)
        out = tempfile.mkdtemp(prefix="census_")
        cmd = [APP] + [a for a in argv if not "=" in a] + ["--shots", out, "--no-audio", "--frames", os.environ.get("CENSUS_FRAMES", "9000"),
                                                          "--shot-limit", "4"]
        # env-carrying items in argv (NIGHT_SHOWSCREEN=...) are real environment settings, not CLI args
        for item in argv:
            if "=" in item:
                k, v = item.split("=", 1)
                e[k] = v
        try:
            r = subprocess.run(cmd, env=e, capture_output=True, timeout=420, cwd=os.path.join(ROOT, "port"))
            log = r.stderr.decode("utf-8", "replace")
        except subprocess.TimeoutExpired as ex:
            log = (ex.stderr or b"").decode("utf-8", "replace")
        hits = 0
        for m in cell_re.finditer(log):
            tag, typ, named, stream = m.group(1), m.group(7), m.group(8), m.group(10)
            raw = bytes.fromhex(stream)
            # Short streams (the level-up digits are 7-row glyphs) are used whole; anything under 4 bytes is too
            # ambiguous to attribute by content.
            probe = raw[:48] if len(raw) >= 8 else raw
            if len(probe) < 4:
                continue
            match = named if named and named != "?" else None
            if not match:
                for name, data in assets.items():
                    if probe in data:
                        match = name
                        break
            if match:
                seen[label][match] += 1
                hits += 1
            else:
                unattributed[tag] += 1
        print(f"{label:9s} composites attributed to a shipped .spr: {hits:5d}   assets: "
              f"{len(seen[label]):3d}  {sorted(seen[label])[:8]}{' ...' if len(seen[label]) > 8 else ''}")
        # docs/80: the lookup trace. An asset that is loaded but never composited is present in the
        # episode's data but not on screen in the sampled window -- a different fact from "not used".
        got = {m.group(0) for m in re.finditer(r"[A-Za-z0-9_-]+\.spr", log)}
        loaded[label] = got
        only_load = sorted(got - set(seen[label]))
        print(f"{label:9s} loaded by the game's own loader: {len(got):5d}   loaded but not drawn: "
              f"{len(only_load):3d}  {only_load[:10]}{' ...' if len(only_load) > 10 else ''}")

    all_assets: dict[str, int] = collections.Counter()
    where: dict[str, set[str]] = collections.defaultdict(set)
    for label, per in seen.items():
        for a, n in per.items():
            all_assets[a] += n
            where[a].add(label)

    fam: dict[str, list[str]] = collections.defaultdict(list)
    for a in sorted(all_assets):
        fam[family_of(a)].append(a)
    print("\n== observed draws by family ==")
    for f in FAMILIES:
        got = fam.get(f)
        print(f"  {f:8s} {'DRAWN: ' + ', '.join(got) if got else 'not observed'}")
    other = {k: v for k, v in fam.items() if k == "(other)"}
    if other:
        print(f"  other assets drawn: {', '.join(other['(other)'])}")
    print(f"\ntotal distinct .spr assets seen drawn: {len(all_assets)} of {len(assets)} extracted "
          f"({len(glob.glob(os.path.join(RES, '*.spr')))} files)")
    never = sorted(set(assets) - set(all_assets))
    print(f"extracted but never drawn in these runs: {len(never)}: {', '.join(never[:40])}"
          f"{' ...' if len(never) > 40 else ''}")
    # docs/80: loaded-vs-drawn matrix by family, the evidence that an unobserved family is a sampling
    # limitation (its assets ARE resident in the episode) rather than a port omission.
    fam_loaded: dict[str, set] = collections.defaultdict(set)
    for label, assets_l in loaded.items():
        for a in assets_l:
            fam_loaded[family_of(a)].add(label)
    print("\n== family matrix: drawn / only loaded / never referenced in these runs ==")
    for f in FAMILIES + ["mboss1", "mboss2", "mboss1-1", "mboss1-2", "number1", "number2", "number3"]:
        d = "drawn" if fam.get(f) else ("loaded only" if fam_loaded.get(f) else "never referenced")
        runs = sorted(fam_loaded.get(f, set()) | {r for r in loaded if any(family_of(a) == f for a in seen[r])})
        print(f"  {f:9s} {d:17s} episodes: {','.join(x.replace('walk','w').replace('stage','s') for x in runs) or '-'}")
    if unattributed:
        print("composites whose stream was not found in an extracted .spr, by trace tag: "
              + ", ".join(f"{t}={n}" for t, n in unattributed.most_common()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
