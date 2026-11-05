#!/usr/bin/env python3
"""Check the repacked slot files against the disc, using the mapping the Pocket host uses at run time.

WHAT THIS PROVES (task-4c's evidence): the pair (of_disc.c slot math, scripts/pocket/repack.py cutting)
returns the same 2352 bytes src/cd/cd_iso.c returns for the same disc LBA.  The fixture suite already
pins cd_iso's side (disc LBA 37262 == IMG sector 37112, the 0xc51 gate, the refused lead-in read); this
closes the other half -- the slot files really carry those bytes at those offsets.

Mappings checked, for every audio track:
    disc_lba -> track file            (of_disc_track_of_lba: the last track starting at or below it)
    offset    = disc_lba - track.lba  (of_disc.c, no -150: the .bin starts at the track's INDEX 01)
    IMG       = disc_lba - 150        (cd_iso.c: the IMG carries no PREGAP)

Run:  python3 scripts/pocket/check_slots.py
"""
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts" / "pocket"))
import repack  # noqa: E402  (reuse its CUE parser and constants -- one source of truth)

SECTOR, PREGAP = repack.SECTOR, repack.PREGAP
CD = REPO / "scratch" / "pocket" / "cd"

fails = 0


def fail(msg):
    global fails
    fails += 1
    print(f"  [FAIL] {msg}")


def main():
    tracks, pregap = repack.parse_cue(repack.CUE)
    audio = [(t, lba) for t, _m, lba in tracks if t != tracks[0][0]]
    img = repack.IMG.open("rb")
    img_sectors = repack.IMG.stat().st_size // SECTOR

    if pregap != PREGAP:
        fail(f"PREGAP {pregap} != the {PREGAP} the scripts assume")

    # the gate value the shipped disc must carry (docs/67)
    if audio[1][1] - audio[0][1] != 3153:
        fail(f"track3-track2 = {audio[1][1] - audio[0][1]}, the gate wants 3153")
    else:
        print(f"  [PASS] track3 - track2 = {audio[1][1] - audio[0][1]} (0xc51), the value fn_010c3e compares")

    def owner(lba):
        best = None
        for t, lba0 in audio:
            if lba0 <= lba and (best is None or lba0 >= best[1]):
                best = (t, lba0)
        return best

    for track, lba in audio:
        path = CD / f"track{track:02d}.bin"
        if not path.is_file():
            fail(f"{path} missing -- run scripts/pocket/repack.py")
            continue
        n = path.stat().st_size // SECTOR
        tf = path.open("rb")
        if path.stat().st_size % SECTOR:
            fail(f"{path.name} is not a whole number of sectors")

        # every boundary sector of the track, plus one inside it
        probes = [0, 1, n // 2, n - 1]
        for off in probes:
            disc_lba = lba + off
            own = owner(disc_lba)
            if own is None or own[0] != track:
                fail(f"of_disc would send LBA {disc_lba} to track {own and own[0]}, not {track}")
                continue
            img_sector = disc_lba - PREGAP
            if not (0 <= img_sector < img_sectors):
                fail(f"LBA {disc_lba} maps outside the IMG ({img_sector})")
                continue
            img.seek(img_sector * SECTOR)
            want = img.read(SECTOR)
            tf.seek(off * SECTOR)
            got = tf.read(SECTOR)
            if got != want:
                fail(f"track{track:02d}+{off} (LBA {disc_lba}, IMG sector {img_sector}) differs")
        tf.close()
        print(f"  [PASS] track{track:02d}: {n} sectors, LBA {lba}..{lba + n - 1} byte-identical to IMG "
              f"at {len(probes)} sampled boundaries")

    # the lead-in must stay unreadable through a track file (the same refusal cd_iso has)
    own = owner(100)
    if own is not None:
        fail(f"LBA 100 (the 150-sector lead-in) was claimed by track {own[0]}")
    else:
        print("  [PASS] an LBA inside the lead-in belongs to no audio slot")

    # The host's runtime map and the deployed manifests must be the same map.  0.1.22 shipped C arithmetic
    # (19 + track) against a manifest built another way, so the title theme read slot 29 -- track13.bin -- and
    # track15 asked for a slot that does not exist.  docs/339.
    sys.path.insert(0, str(REPO / "scripts" / "pocket"))
    import re as _re
    from slot_ids import AUDIO_IDS, SAVE_IDS
    hdr = (REPO / "pocket" / "src" / "night" / "disc_table.h").read_text()
    rows = _re.findall(r"\{\s*(\d+),\s*\d+u,\s*(\d),\s*(-?\d+)\s*\}", hdr)
    runtime = {int(tr): int(slot) for tr, _audio, slot in rows if int(_audio)}
    inst = json.loads((REPO / "pocket" / "dist" / "night" / "Assets" / "night" / "iamghost.night" / "NIGHT.json")
                      .read_text())["instance"]["data_slots"]
    bound = {b["id"]: b["filename"] for b in inst}
    bad = 0
    for tr, slot in sorted(runtime.items()):
        want = f"track{tr:02d}.bin"
        if slot not in AUDIO_IDS:
            fail(f"track{tr:02d} runtime slot {slot} is outside the non-save ids"); bad += 1
        elif bound.get(slot) != want:
            fail(f"track{tr:02d}: runtime slot {slot} is bound to {bound.get(slot)!r} in NIGHT.json, wants {want!r}")
            bad += 1
    if bad == 0:
        print(f"  [PASS] disc_table.h slot ids and NIGHT.json bindings agree for all {len(runtime)} audio tracks "
              f"(ids {min(runtime.values())}..{max(runtime.values())}, none in the save range {SAVE_IDS[0]}-{SAVE_IDS[-1]})")

    # ---- night.ini hygiene (docs/370).  A careless edit to mk_dist.py's ini block put SCREEN twice and FB_MODE
    # twice into the shipped settings file; a duplicated key is a coin-flip about which value the parser honours, so
    # the generated file is checked for it rather than eyeballed.
    ini_path = REPO / "pocket" / "dist" / "night" / "Assets" / "night" / "common" / "night.ini"
    if ini_path.exists():
        ini = ini_path.read_text()
        keys = [l.split("=")[0] for l in ini.splitlines() if _re.match(r"^[A-Z_]+=", l)]
        dup = sorted({k for k in keys if keys.count(k) > 1})
        if dup:
            fail(f"night.ini names a key more than once: {dup} -- the parser cannot honour both")
        else:
            print(f"  [PASS] night.ini names each of its {len(keys)} keys exactly once "
                  f"({', '.join(k for k in keys if k in ('SCREEN', 'FB_MODE', 'LOG', 'DIAG', 'PUMP_AUDIO_TICK'))} tracked)")

    print("== ALL PASS ==" if fails == 0 else f"== {fails} FAIL ==")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
