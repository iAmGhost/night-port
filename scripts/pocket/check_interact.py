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
# mask semantics, straight from Analogue: "Bits set to '1' will be left untouched in the register.  Bits set to
# '0' may be modified by the UI element."  So the bits an element owns are the COMPLEMENT of its mask -- and an
# element with mask 0xFFFFFFFF owns nothing at all.
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
JSON = REPO / "pocket/dist/night/Cores/iamghost.night/interact.json"
BASE = REPO / "pocket/dist/sdk/Cores/ThinkElastic.openfpgaOS/interact.json"
HDR = REPO / "pocket/src/night/of_interact_idx.h"
SAVE = REPO / "pocket/src/night/of_save.c"
MAIN = REPO / "pocket/src/night/main_of.c"

fails = 0


def fail(msg):
    global fails
    fails += 1
    print(f"  [FAIL] {msg}")


def main():
    variables = json.loads(JSON.read_text())["interact"]["variables"]
    hdr = HDR.read_text()
    # The field decoders used to live in of_save.c; since 0.1.38 the host (main_of.c) owns them.  Rule 6 asks
    # "does the host extract the same field the JSON writes", so search whichever file carries the #define.
    src = SAVE.read_text() + "\n" + (MAIN.read_text() if MAIN.exists() else "")

    def mask_bits(v):
        """The bits a mask ALLOWS the element to write: the complement of its mask.  An element with no mask
        owns the whole word -- which is only safe at an address no other element shares."""
        if v.get("mask") is None:
            return set(range(32))
        m = int(v["mask"], 16) if isinstance(v["mask"], str) else v["mask"]
        return {b for b in range(32) if not (m >> b) & 1}

    # 1. the host's index constants match the JSON order
    idx = {v["name"]: i for i, v in enumerate(variables)}
    for const, name in (("OF_IDX_MUSIC", "Music Volume"), ("OF_IDX_FIT", "Screen Fit"),
                        ("OF_IDX_PLCD", "Pocket LCD")):
        m = re.search(rf"#define {const}\s+(\d+)", hdr)
        if not m:
            continue                      # absent is legal; section 4 checks the reverse direction
        got, want = int(m.group(1)), idx.get(name)
        if want is None:
            fail(f"the header defines {const} = {got} but interact.json declares no {name}")
        elif want != got:
            fail(f"{name}: interact.json position {want} != host index {got}")
        else:
            print(f"  [PASS] {name}: json position {got} == host index {const}")

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

    # 3. the base must BE the shipped core's file.  os25 is the bitstream this port runs, and its interact
    #    definition is the one known to work on it; a hand-written approximation is what put our sliders on words
    #    the core does not decode (docs/336) and left "Analogizer 15kHz Timing" out of a menu that describes the
    #    same word the core decodes.
    APP_NAMES = ("Music Volume", "Screen Fit")
    app = [v for v in variables if v["name"] in APP_NAMES]
    reserved = [v for v in variables if v["name"] not in APP_NAMES]
    if BASE.exists():
        base = json.loads(BASE.read_text())["interact"]["variables"]
        if len(reserved) != len(base):
            fail(f"{len(reserved)} reserved entries, the shipped openfpgaOS core declares {len(base)}")
        for i, (mine, theirs) in enumerate(zip(reserved, base)):
            for key in ("name", "id", "type", "address", "mask"):
                if mine.get(key) != theirs.get(key):
                    fail(f"entry {i} {mine.get('name')!r}: {key} is {mine.get(key)!r}, the shipped core has "
                         f"{theirs.get(key)!r}")
        if not fails:
            print(f"  [PASS] entries 0-{len(base) - 1} are dist/sdk openfpgaOS interact.json verbatim "
                  f"(names, ids, types, addresses, masks)")
    else:
        print(f"  [SKIP] {BASE} not vendored; reserved-prefix comparison skipped")

    # 4. the rule two dead cores wrote into this file.  An app element may NOT be declared at openfpgaOS's control
    #    word, whatever its mask and whatever its values:
    #      0.1.27  Music Volume at 0xF7000000 bits 20-23  -> the core would not boot (card kept the old log)
    #      0.1.34  Music Volume bits 24-26 + Screen Fit bits 27-28, masks and option values well formed (every
    #              value inside its own field, none zero, no overlap with a reserved field) -> the core would not
    #              boot again.
    #    Every build that put an app element at ANOTHER address booted: 0xF700000C / 0xF7000010 (0.1.25/26) and
    #    0x03FE0000 / 0x03FE0008 (0.1.32).  So the fatal variable is the address, not the bit position: the
    #    firmware rewrites each declared element's field of that word every frame, and openfpgaOS's core_top is the
    #    consumer.  An app element inside it is a periodic write at core-owned state.
    # The probe entry is an app element, and it must obey the same two rules the dead cores wrote into this file:
    # never the control word, and never a mask that lets the element write nothing.  Its address (0xF700000C) is a
    # word no openfpgaOS core decodes, which 0.1.25/26 proved harmless to declare.
    CONTROL_WORDS = {"0xF7000000"}
    for v in app:
        if v["address"] in CONTROL_WORDS:
            fail(f"{v['name']} is declared at {v['address']} -- 0.1.27 (bits 20-23) and 0.1.34 (bits 24-28) both "
                 f"produced a core that would not boot.  No mask makes this safe")
    if app and not any(v["address"] in CONTROL_WORDS for v in app):
        print("  [PASS] app elements are off the control word (they were the fatal variable twice: 0.1.27, 0.1.34)")
    # An element that writes nothing at all is a failure, not a style choice: mask 1-bits are "leave untouched".
    for v in app:
        bits = mask_bits(v)
        if not bits:
            fail(f"{v['name']} has mask {v.get('mask')} -- an all-ones mask leaves EVERY bit untouched, so the "
                 f"element can write nothing.  Doom's App Option 1/2/3 are inert for exactly this reason")
        else:
            print(f"  [PASS] {v['name']} writes bits {min(bits)}-{max(bits)} of {v['address']}")
    for a, b in [(x["address"], y["address"]) for x in app for y in app if x is not y]:
        if a == b and all(mask_bits(x) & mask_bits(y) for x in app for y in app if x is not y):
            fail("two app knobs write overlapping bits of one bridge word")

    # 6. the menu and the host must agree in BOTH directions: no knob in the JSON without an index constant in
    #    the header, and no index constant pointing at a variable that is not there.  The host reads the fields
    #    off the control word (of_save.c bridge_control_word()); the index constants now exist to cross-check the
    #    documented ecall against the bus, which is the measurement docs/344 could not make.
    if not app:
        stray = [c for c in ("OF_IDX_MUSIC", "OF_IDX_FIT") if re.search(rf"#define {c}\b", hdr)]
        if stray:
            fail(f"interact.json declares no app knobs but the header still defines {', '.join(stray)}")
        else:
            print("  [PASS] no app knobs declared, and no dangling OF_IDX_* in the generated header")
    else:
        print("  [PASS] app knobs declared with index constants for the cross-check: " +
              ", ".join(f"{v['name']}@{v['address']} idx {idx[v['name']]}" for v in app))

    # 5. every option value a knob offers must fit inside that knob's own field, and none of them may be 0.
    #    0.1.27 offered 0x00..0x80 for a field at bits 20-23: values outside the field they write, and the core
    #    refused to boot.  0 stays reserved for "nobody chose", because an untouched variable reads 0 on the wire
    #    and 0 is a legal volume (docs/341).
    consts = {"Music Volume": ("OF_IDX_MUSIC", "MUSIC_SHIFT", "MUSIC_MASK"),
              "Screen Fit":   ("OF_IDX_FIT",   "FIT_SHIFT",   "FIT_MASK"),
}
    for v in app:
        vals = [int(o["value"], 16) if isinstance(o["value"], str) else o["value"] for o in v["options"]]
        field = mask_bits(v)
        stray = sorted({val for val in vals if any((val >> b) & 1 for b in range(32) if b not in field)})
        if stray:
            fail(f"{v['name']} offers {stray} outside its own field "
                 f"(mask {v.get('mask')} writes only {sorted(field)}) -- 0.1.27's shape, and it bricked a boot")
        if 0 in vals:
            fail(f"{v['name']} offers 0 as a choice: an untouched variable reads 0, and the host needs 0 to mean "
                 f"'nobody chose'")
        fieldvals = sorted({(val >> min(field)) & ((1 << (max(field) - min(field) + 1)) - 1) for val in vals})
        if not stray and not fails:
            print(f"  [PASS] {v['name']}: option values {['0x%08X' % x for x in vals]} all inside bits "
                  f"{min(field)}-{max(field)}, field values {fieldvals}, none of them 0")
        dv = v["defaultval"]
        if isinstance(dv, str): dv = int(dv, 16)
        if dv not in vals:
            fail(f"{v['name']} defaultval {v['defaultval']} is not one of its own options")
        # and the host must extract the same field the JSON writes
        const, sh, msk = consts[v["name"]]
        for name, want in ((sh, min(field)), (msk, (1 << (max(field) - min(field) + 1)) - 1)):
            m = re.search(rf"#define {name}\s+0x[0-9A-Fa-f]+|(?:#define {name}\s+)(\d+)", src)
            got = None
            m2 = re.search(rf"#define {name}\s+(0x[0-9A-Fa-f]+|\d+)", src)
            if m2:
                got = int(m2.group(1), 0)
            if got is None:
                fail(f"host has no #{name} for {v['name']}")
            elif got != want:
                fail(f"{v['name']}: host {name}={got}, interact.json's field says {want}")
            else:
                print(f"  [PASS] host {name}={got} matches {v['name']}'s field in {v['address']}")
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
