#!/usr/bin/env python3
"""Check the interact menu against the host's decoding of it.

WHAT THIS PROVES: the Pocket's Core Options menu and the host agree about where a knob lives and what its
option values mean.  Two bugs this catches, both of which happened:

  * the host reading a different interact INDEX than the JSON declares (docs/336 -- of_interact_get addresses
    variables by POSITION, and five variables share the word 0xF7000000),
  * two menu elements writing the SAME bits of one word.  Music Volume was about to sit at bits 10-13, which is
    exactly where "Analogizer Video Out"'s field is (its mask 0xFFFFC3FF is zero at 10-13); it now sits at 20-23.

Run:  python3 scripts/pocket/check_interact.py
"""
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
JSON = REPO / "pocket/dist/night/Cores/iamghost.night/interact.json"
HDR = REPO / "pocket/src/night/of_interact_idx.h"
SAVE = REPO / "pocket/src/night/of_save.c"

fails = 0


def fail(msg):
    global fails
    fails += 1
    print(f"  [FAIL] {msg}")


def main():
    variables = json.loads(JSON.read_text())["interact"]["variables"]
    hdr = HDR.read_text()
    src = SAVE.read_text()

    def mask_bits(v):
        """The bits a mask ALLOWS the element to write: the complement of its mask.  An element with no mask
        owns the whole word -- which is only safe at an address no other element shares."""
        if v.get("mask") is None:
            return set(range(32))
        m = int(v["mask"], 16) if isinstance(v["mask"], str) else v["mask"]
        return {b for b in range(32) if not (m >> b) & 1}

    # 1. the host's index constants match the JSON order
    idx = {v["name"]: i for i, v in enumerate(variables)}
    for name, const in (("Music Volume", "OF_IDX_MUSIC"), ("Pocket LCD", "OF_IDX_PLCD")):
        want = int(re.search(rf"#define {const}\s+(\d+)", hdr).group(1))
        if idx.get(name) != want:
            fail(f"{name}: interact.json position {idx.get(name)} != host index {want}")
        else:
            print(f"  [PASS] {name}: json position {want} == host index")

    # 2. no two elements write the same bits of the same word
    by_word = {}
    for v in variables:
        by_word.setdefault(v["address"], []).append(v)
    for addr, group in by_word.items():
        if len(group) < 2:
            continue
        seen = {}
        for v in group:
            for b in mask_bits(v):
                if b in seen:
                    fail(f"{addr}: bits {b} written by both {seen[b]!r} and {v['name']!r}")
                seen[b] = v["name"]
        if not fails:
            def runs(bs):
                bs = sorted(bs)
                if not bs:
                    return "(no mask: whole word)"
                out, a, b = [], bs[0], bs[0]
                for x in bs[1:]:
                    if x == b + 1:
                        b = x
                        continue
                    out.append(f"{a}-{b}" if a != b else f"{a}")
                    a = b = x
                out.append(f"{a}-{b}" if a != b else f"{a}")
                return ", ".join(out)
            print(f"  [PASS] {addr}: {len(group)} elements, no shared bits -- " +
                  "; ".join(f"{v['name']} {runs(mask_bits(v))}" for v in group))
            free = set(range(32)) - set(seen)
            print(f"         still unwritten at {addr}: {runs(free) if free else 'none'}")

    # 3. the rule 0.1.27 broke: an app knob must not be mounted on a word the core's own reserved entries use.
    #    0xF7000000 is openfpgaOS's control word (Analogizer / SNAC / Pocket LCD are decoded from it), and the
    #    firmware rewrites every declared element's field each frame.  Putting a knob there pokes core state
    #    60 times a second -- and the core stopped booting.
    reserved_addrs = {v["address"] for v in variables if v.get("mask") is not None or v["address"] == "0xF7000000"}
    for v in variables:
        if v["name"] in ("Music Volume", "Screen Fit") and v["address"] in reserved_addrs:
            fail(f"{v['name']} is mounted on {v['address']}, a word the core's reserved entries already write")
    app = [v for v in variables if v["name"] in ("Music Volume", "Screen Fit")]
    if len({v["address"] for v in app}) != len(app):
        fail("two app knobs share one bridge address")
    else:
        print(f"  [PASS] app knobs on their own addresses: " +
              ", ".join(f"{v['name']}@{v['address']}" for v in app))

    # 4. each app knob's option values are exactly the 1..N the host maps onto its own enum
    consts = {"Music Volume": ("OF_IDX_MUSIC", "MUSIC_SHIFT", "MUSIC_MASK"),
              "Screen Fit":   ("OF_IDX_FIT",   "FIT_SHIFT",   "FIT_MASK")}
    for v in app:
        vals = [int(o["value"], 16) if isinstance(o["value"], str) else o["value"] for o in v["options"]]
        if vals != list(range(1, len(vals) + 1)):
            fail(f"{v['name']} options are {vals}, not the 1..{len(vals)} the host maps")
        else:
            print(f"  [PASS] {v['name']}: options {vals} map 1:1 onto the host's choice table "
                  f"(0 stays 'nothing was written', never a choice)")
        if v["defaultval"] not in vals:
            fail(f"{v['name']} defaultval {v['defaultval']} is not one of its own options")
        if v.get("mask") is not None:
            fail(f"{v['name']} declares a mask -- masked fields belong to the reserved word, not an app knob")
        const, sh, msk = consts[v["name"]]
        if not re.search(rf"#define {const}\s+\d+", hdr):
            fail(f"host has no {const} index constant for {v['name']}")
        want = idx.get(v["name"])
        got = int(re.search(rf"#define {const}\s+(\d+)", hdr).group(1))
        if want != got:
            fail(f"{v['name']}: json position {want} != host index {got}")
        else:
            print(f"  [PASS] {v['name']}: host index {got} matches its position in interact.json")

    print("== ALL PASS ==" if fails == 0 else f"== {fails} FAIL ==")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
