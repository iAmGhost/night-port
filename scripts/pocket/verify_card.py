#!/usr/bin/env python3
"""Verify a deployed Pocket card against the dist tree that was supposed to land on it.

deploy.sh checks its own steps; this checks the card AFTER a deploy and after whatever hand-editing a session
does afterwards (the DIAG/LOG/SCREEN/PRESENT_SKIP lines get rewritten by hand every deploy, and deploy.sh
regenerates night.ini from the dist tree over those edits). It is the same handful of assertions I ran by hand
twice on 2026-11-11 and then wished I had typed once.

    python3 scripts/pocket/verify_card.py [--card /Volumes/Pocket] [--dist pocket/dist/night]

Exit 0 = the card matches the dist tree. Any FAIL names the file to look at; nothing here writes to the card.
"""
import argparse
import hashlib
import pathlib
import re
import sys

CORE = 'iamghost.night'
APP = 'night'


def md5(p):
    return hashlib.md5(p.read_bytes()).hexdigest() if p.exists() else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--card', default='/Volumes/Pocket')
    ap.add_argument('--dist', default='pocket/dist/night')
    ap.add_argument('--build', default='pocket/build/pocket/night')
    a = ap.parse_args()
    card, dist = pathlib.Path(a.card), pathlib.Path(a.dist)
    build = pathlib.Path(a.build)
    fails, notes = [], []
    if not card.is_dir():
        print('FAIL card not mounted at %s' % card); return 1
    if not dist.is_dir():
        print('FAIL dist tree missing at %s (make dist)' % dist); return 1

    # 1. the core tree: byte-identical JSONs
    d_core, c_core = dist / 'Cores' / CORE, card / 'Cores' / CORE
    for f in sorted(d_core.glob('*.json')) + sorted(d_core.glob('*.rbf_r')) + sorted(d_core.glob('*.bin')):
        c = c_core / f.name
        if not c.exists():
            fails.append('%s missing on card' % f.name)
        elif md5(c) != md5(f):
            fails.append('%s differs from dist (%s vs %s)' % (f.name, md5(c)[:8], md5(f)[:8]))

    # 1b. THE ELF BYTES. deploy.sh copies the core tree out of the build tree, so that is the file the
    # card must match -- pocket/dist/night holds night.ini/README only, no ELF.  On 2026-11-11 I told the
    # user a deployment was "deployed and verified" on the strength of a version string and a grep, and
    # then found my own byte-comparison had compared an empty path to the card and printed "DIFFERS".
    # A version string says what a binary claims; only the hash says what is on the card.
    d_elf = build / 'Assets' / APP / 'common' / 'night.elf'
    c_elf = card / 'Assets' / APP / 'common' / 'night.elf'
    if not d_elf.exists():
        fails.append('build-tree ELF missing at %s -- cannot byte-check the card' % d_elf)
    elif md5(c_elf) != md5(d_elf):
        fails.append('night.elf on card differs from build tree (%s vs %s) -- stale deploy or hand copy'
                     % ((md5(c_elf) or 'missing')[:8], md5(d_elf)[:8]))
    else:
        notes.append('night.elf bytes match the build tree (%s, %d B)' % (md5(d_elf)[:8], d_elf.stat().st_size))

    # 2. version agrees three ways: core.json, the ELF's own string, and mk_dist's VERSION
    j = (c_core / 'core.json').read_text()
    ver_core = re.search(r'"version"\s*:\s*"([0-9.]+)"', j)
    elf = card / 'Assets' / APP / 'common' / 'night.elf'
    ver_elf = None
    if elf.exists():
        blob = elf.read_bytes()
        # The string sits between NULs in .rodata, so line anchors do not find it (measured: None on a card that
        # plainly printed 0.1.96). Anchor on the delimiter the compiler actually uses.
        m = re.search(rb'\x00([0-9]+\.[0-9]+\.[0-9]+)\x00', blob)
        ver_elf = m.group(1).decode() if m else None
    mk = pathlib.Path('scripts/pocket/mk_dist.py').read_text()
    ver_src = re.search(r'VERSION = "([0-9.]+)"', mk)
    trio = {'core.json': ver_core and ver_core.group(1), 'night.elf': ver_elf,
            'mk_dist.py': ver_src and ver_src.group(1)}
    if len(set(trio.values())) != 1 or None in trio.values():
        fails.append('version mismatch %s -- run make dist, then make deploy' % trio)
    else:
        notes.append('version %s in core.json / night.elf / mk_dist.py' % ver_elf)

    # 3. the slot files that deploy is supposed to carry
    mp = dist / 'slot_map.tsv'
    rows = [l.split('\t') for l in mp.read_text().splitlines() if l.strip()]
    common = card / 'Assets' / APP / 'common'
    saves = card / 'Saves' / APP / 'common'
    missing, sizes = [], []
    for sid, name in ((int(r[0]), r[1].strip()) for r in rows):
        if name.endswith('.sav'):
            # save slots live in Saves/ and are created by the OS on first write: absent before a run is normal
            tgt = saves / name
            if not tgt.exists():
                notes.append('%s not yet created (save slot, appears after a run)' % name)
            continue
        tgt = common / name
        if not tgt.exists():
            missing.append('%s (slot %d)' % (name, sid))
        elif len(name) > 23:
            fails.append('%s is %d chars, over the 23-char filename limit' % (name, len(name)))
        else:
            src = None
            for cand in (dist / 'Assets' / APP / 'common' / name,):
                if cand.exists():
                    src = cand
            if src and src.stat().st_size != tgt.stat().st_size:
                sizes.append('%s size %d on card vs %d in dist' % (name, tgt.stat().st_size, src.stat().st_size))
    if missing:
        fails.append('slot files missing on card: %s' % ', '.join(missing))
    if sizes:
        fails.append('slot sizes differ: %s' % '; '.join(sizes))
    ids = [int(r[0]) for r in rows]
    if len(ids) > 32 or max(ids) > 31:
        fails.append('%d registered ids with max %d -- the firmware caps at 32 slots' % (len(ids), max(ids)))

    # 4. night.ini: present, every key named exactly once, and carrying the keys a run depends on
    ini = common / 'night.ini'
    if not ini.exists():
        fails.append('night.ini missing on card')
    else:
        txt = ini.read_text()
        keys = re.findall(r'(?m)^([A-Z_]+)=', txt)
        dup = sorted({k for k in keys if keys.count(k) > 1})
        if dup:
            fails.append('night.ini names %s more than once -- which parser honours it is a coin flip' % dup)
        need = set(re.findall(r'"([A-Z_]+)=', mk))
        vals = dict(re.findall(r'(?m)^([A-Z_]+)=(.*)$', txt))
        got = set(keys)
        miss = sorted(k for k in ('CDDA_CARD_STOP', 'PRESENT_SKIP', 'DELAY_US', 'DIAG', 'LOG', 'SCREEN', 'FIT')
                      if k not in got)
        if miss:
            fails.append('night.ini missing keys %s (deploy.sh regenerated it from the dist tree?)' % miss)
        notes.append('night.ini %d keys; %s' % (len(keys), ' '.join('%s=%s' % (k, vals[k]) for k in
                    sorted(vals) if k in ('DIAG', 'LOG', 'SCREEN', 'PRESENT_SKIP', 'DELAY_US', 'CDDA_CARD_STOP'))))
        _ = need

    # 5. no AppleDouble sidecars in our folders (exFAT re-creates them after COPYFILE_DISABLE)
    litter = []
    for d in (c_core, common, card / 'Platforms' / APP, saves):
        if d.is_dir():
            litter += [str(p) for p in d.rglob('._*')]
    # WARN, not FAIL: a hand-edit of night.ini (which every session does right after deploy) re-creates
    # ._night.ini on exFAT immediately, so failing on this just trains you to ignore it. They are inert here --
    # the app asks for its slots by exact name ("night_0.sav"), never "._nightlog.sav" -- but they are listed so
    # a stray ._data.json in a core folder is not missed either. Clean with:
    #   find /Volumes/Pocket/Cores/iamghost.night /Volumes/Pocket/Assets/night /Volumes/Pocket/Saves/night -name '._*' -delete
    if litter:
        notes.append('WARN %d AppleDouble sidecar(s): %s' % (len(litter), litter[:3]))

    print('\n'.join('  note: %s' % n for n in notes))
    for f in fails:
        print('FAIL %s' % f)
    if fails:
        return 1
    print('OK card matches %s: core JSONs identical, %d slot files, version %s, ini keys unique' %
          (dist, sum(1 for r in rows if not r[1].strip().endswith('.sav')), ver_elf))
    return 0


if __name__ == '__main__':
    sys.exit(main())
