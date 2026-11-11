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

    check_log_open_name(fail)
    print("== ALL PASS ==" if fails == 0 else f"== {fails} FAIL ==")
    return 0 if fails == 0 else 1


def check_log_open_name(fail):
    """The C code and data.json must agree on where the diagnostic trail goes -- and every name in the
    chain must be a name the kernel can actually bind (docs/451, README.md's data-slot contract).

    Two bugs of this shape: 0.2.23 deleted seven save-slot bindings and the log's primary name was the
    ORDINAL alias `save:7`, so the trail went silent; and a later fix reached for `nightlive.txt`, a name
    nothing declares, which on a filesystem whose registry is populated by a directory scan is not an
    error but silence. Both were invisible to every check in this file, because each looked at one side.
    """
    import re as _re
    src = (REPO / "pocket" / "src" / "night" / "of_log.c").read_text()
    chain = _re.findall(r'static const char \*names\[\] = \{([^}]*)\}', src)
    if not chain:
        fail('of_log.c: cannot find the log open chain')
        return
    # The chain holds MACROS (LOG_NAME_0, ...), not literals -- resolve them, because the check has to
    # judge the actual strings the kernel will see.
    defs = dict(_re.findall(r'#define\s+(\w+)\s+"([^"]+)"', src))
    names = []
    for n in chain[0].replace('\n', ' ').split(','):
        n = _re.sub(r'/\*.*?\*/', '', n).strip()
        if not n:
            continue
        if n.startswith('"'):
            names.append(n)
        elif n in defs:
            names.append('"%s"' % defs[n])
        else:
            fail(f'log chain entry "{n}" is neither a literal nor a #define with a string value')
            return
    if not names:
        fail('of_log.c: the log open chain is empty')
        return
    m0 = _re.match(r'"slot:(\d+)"', names[0])
    if not m0:
        fail(f'of_log.c opens the log by "{names[0]}": the first name must be the explicit "slot:<id>" '
             f'form -- save:N is an ordinal and breaks when the save layout changes (docs/451)')
        return
    want = int(m0.group(1))
    if not (10 <= want <= 19):
        fail(f'slot {want} is outside the nonvolatile save range 10-19 (README.md data slot table)')
        return
    if want == 8:
        fail('slot 8 is SDK-owned shared config -- never an app file (README.md)')
        return
    dj = REPO / "pocket" / "dist" / "night" / "Cores" / "iamghost.night" / "data.json"
    slots = json.load(open(dj))["data"]["data_slots"]
    ids = [e.get("id") for e in slots]
    if len(ids) != len(set(ids)):
        fail(f'duplicate data-slot ids: {sorted(i for i in set(ids) if ids.count(i) > 1)}')
        return
    if len(ids) > 32:
        fail(f'{len(ids)} data slots declared; Analogue firmware allows at most 32')
        return
    if want not in ids:
        fail(f'data.json does not declare slot {want}, which of_log.c opens for the log')
        return
    e = [x for x in slots if x.get("id") == want][0]
    if not e.get("nonvolatile"):
        fail(f'slot {want} is not nonvolatile: the log cannot persist there'); return
    if e.get("required"):
        fail(f'slot {want} (the log) is required:true: a missing log file would fail the launch'); return
    exts = [x.lower() for x in e.get("extensions", [])]
    for n in names[1:]:
        bare = n.strip('"')
        mm = _re.match(r'slot:(\d+)$', bare)
        if mm and int(mm.group(1)) != want:
            fail(f'log chain mixes slot ids: {names[0]} and "{n}" name different slots')
            return
        if not bare.startswith('slot:') and not bare.startswith('save'):
            smap = REPO / "pocket" / "dist" / "night" / "slot_map.tsv"
            bound = {}
            if smap.exists():
                for line in smap.read_text().splitlines():
                    f = line.split('\t')
                    if len(f) >= 2 and f[0].isdigit():
                        bound[int(f[0])] = f[1].strip()
            if bound and bare not in bound.values():
                fail(f'log chain tries "{n}", which slot_map.tsv binds to nothing: the registry binds '
                     f'declared names, and a name that does not bind fails silently (docs/451)')
                return
            if not bare.lower().endswith('.' + (exts[0] if exts else 'sav')):
                fail(f'log chain tries undeclared/extension-mismatched name "{n}": the registry binds '
                     f'declared names, and a name that does not bind fails silently (docs/451)')
                return
    print(f'  [PASS] log chain {names} all bind slot {want} '
          f'(nonvolatile, not required, {e.get("size_maximum")}); {len(ids)} slots, cap 32')


if __name__ == '__main__':
    sys.exit(main())
