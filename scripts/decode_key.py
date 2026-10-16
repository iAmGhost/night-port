#!/usr/bin/env python3
"""Decode NIGHT .key technique files and verify against binary findings.

Binary basis (NIGHT.EXE, see docs/10-gameplay.md §8.8 / §9.1):
- .key parser: FUN_00032464 (0x32464-0x326bf). fscanf order per record:
    action(u16) n(u8) key*n("%c") "\\n" timings*n(u32) x(u8) y(u8) z(u8)
- In-memory table (heap, 0x4b4 B, FUN_00041bdc(0x4b4)):
    +0x000 u32 count
    +0x004 u8  n[60]            (stride 1)
    +0x068 ptr keys[60]         (FUN_00041af8(n), chars mapped via 0x5302c)
    +0x1f8 ptr timings[60]      (FUN_00041af8(n))
    +0x388 u16 action[60]
    +0x450 u8  z[60]
- Global per-action-ID arrays (0..0xff):
    0x62ac8[action] = x   (combo group)
    0x62b90[action] = y   (combo step)
- Pointer array (set by FUN_0003690b, 0x3690b):
    0x62aa0 gun.key  0x62aa4 gun1  0x62aa8 gun2  0x62aac gun3  0x62ab0 gun4
    0x62ab4 catch.key 0x62ab8 catch1 0x62abc catch2 0x62ac0 catch3 0x62ac4 catch4
    Active: 0x62a98 = 0x62aa0[char*4] (gun), 0x62a9c = 0x62ab4[char*4] (catch)
- Key-char map 0x5302c (ascii -> runtime input code):
    a=56 b=102 c=29 d=80 e=105 f=100 l=75 p=57 q=103 r=77 s=42 u=72 w=104 z=44
- exptab.dat: 6 lines x 5 ints, fscanf("%d") into 0x53c94[col*0x14 + level*4],
  loaded in FUN_0003690b (file open at 0x37251, "exptab.dat" string @ 0x5156b).
"""
import re, sys, os

KEYMAP = {0x61:56,0x62:102,0x63:29,0x64:80,0x65:105,0x66:100,0x6c:75,
          0x70:57,0x71:103,0x72:77,0x73:42,0x75:72,0x77:104,0x7a:44}
KEYCHARS = set(KEYMAP)

def parse_key(path):
    toks = open(path, 'rb').read().decode('latin1').split()
    it = iter(toks)
    count = int(next(it))
    recs = []
    for i in range(count):
        action = int(next(it)); n = int(next(it))
        keys = [next(it) for _ in range(n)]
        timings = [int(next(it)) for _ in range(n)]
        x = int(next(it)); y = int(next(it)); z = int(next(it))
        recs.append(dict(action=action, n=n, keys=keys, timings=timings,
                         x=x, y=y, z=z))
    # no trailing tokens expected
    tail = list(it)
    return count, recs, tail

def check(name, count, recs, tail):
    errs = []
    if count > 60: errs.append(f"count {count} > 60 slots")
    if tail: errs.append(f"trailing tokens {tail[:8]}")
    for i, r in enumerate(recs):
        if r['n'] > 8: errs.append(f"rec{i}: n={r['n']} > 8")
        if not (0 <= r['action'] < 256): errs.append(f"rec{i}: action {r['action']}")
        for k in r['keys']:
            if ord(k) not in KEYCHARS: errs.append(f"rec{i}: unknown key char {k!r}")
        if not (0 <= r['x'] < 256 and 0 <= r['y'] < 256 and 0 <= r['z'] < 256):
            errs.append(f"rec{i}: x/y/z out of range")
        # memory footprint check
    used = 0x450 + count
    if used > 0x4b4: errs.append(f"table overflow: {used:#x} > 0x4b4")
    print(f"== {name}: count={count}, used={used:#x} of 0x4b4")
    for i, r in enumerate(recs):
        keys = ''.join(r['keys'])
        tm = ' '.join(str(t) for t in r['timings'])
        print(f"   {i:2d} act={r['action']:3d} n={r['n']} seq={keys:<8s} win=[{tm}] grp={r['x']} step={r['y']} z={r['z']}")
    if errs:
        print("   !! " + "; ".join(errs)); return False
    print("   OK"); return True

def check_exptab(path):
    vals = [int(v) for v in open(path, 'rb').read().decode('latin1').split()]
    ok = len(vals) == 30
    print(f"== {path}: {len(vals)} ints (expect 30 = 6 cols x 5 levels) {'OK' if ok else '!! BAD'}")
    for col in range(6):
        row = vals[col*5:(col+1)*5]
        addr = 0x53c94 + col*0x14
        print(f"   col{col} @ {addr:#x}: {row}")
    return ok

if __name__ == '__main__':
    keydir = sys.argv[1] if len(sys.argv) > 1 else 'scratch/key'
    allok = True
    for name in sorted(os.listdir(keydir)):
        if name.endswith('.key'):
            c, r, t = parse_key(os.path.join(keydir, name))
            allok &= check(name, c, r, t)
    exp = sys.argv[2] if len(sys.argv) > 2 else 'scratch/exptab.dat'
    if os.path.exists(exp):
        allok &= check_exptab(exp)
    sys.exit(0 if allok else 1)
