# 230 — Item 6, candidate (b) closed statically: the part record's own W/X arrays are written by `fn_010efc`, and the port writes them the same way

Follow-up to docs/229 (which re-opened item 6).  The next candidate named there was the attack-part record's
geometry arrays at spawn.  Audited, closed.

## 1. There is exactly one writer of each array field, and it is the spawn routine

`scripts/audit/field_writers.py --disp 0x12b --disp 0x137` over 523 modelled bodies:

* **+0x12b: 11 sites.**  Nine are *reads* inside the draw routine `fn_012efa` (0x1303a, 0x13104, 0x131cc, 0x13231,
  0x132a9, 0x132e3, 0x13373, 0x1341a, 0x13454), one is a read in `fn_013908` (0x13a6e), and the only store is
  **0x000110e4 `mov [eax + 0x12b], edx` in `fn_010efc`** (spawn-or-get entity).
* **+0x137: 9 sites.**  Seven reads in `fn_012efa` (0x130a1, 0x1313e, 0x13242, 0x132f4, 0x133ad, 0x13465), two reads
  in `fn_013908` (0x139e1, 0x13a82), and the only store is **0x00011135 `mov [eax + 0x137], edx` in `fn_010efc`**.
* Zero indexed (`base + index*scale`) forms: the field itself holds a *pointer*, and the per-sprite index is applied
  by the reader (the shipped reads `[eax+0x12b]` then dereferences with the frame index; the port does
  `woff = [e+0x12b]` then `hb + woff + 4*spr`, `f.c:14909-14914`) ✓ the two models agree on that structure.

## 2. What the port stores there

```c
/* src/f/f.c, fn_010efc */
1328:  ((struct ua32 *)((block + 0x12b)))->v = ((struct ua32 *)((spr + 0x04)))->v;   /* 0x110d8-e4 */
1331:  ((struct ua32 *)((block + 0x137)))->v = ((struct ua32 *)((spr + 0x0c)))->v;   /* 0x11129-35 */
```

i.e. each entity's array pointer is copied from its sprite control block (+0x04 = widths, +0x0c = x-hotspots),
which is the shipped's source too (the store at 0x110e4 / 0x11135 is fed from the same block fields) ✓ and the
readers use it identically (`f.c:14848` in the other draw path, `f.c:14912-14914` and `f.c:15261` in `fn_012efa`'s
two arms).

So docs/229 §4 candidate (1) -- "the limb is a part record whose arrays are never filled" -- is **closed**: they
are filled, by the same routine, from the same source field, with the same index structure.

## 3. Item 6's state, and what the next step must be

Static side, all closed: the two arms and the row anchor (docs/225 §1), the three clamp candidates (§2), the
mirror-flag stores (docs/229), the array writers and readers (this doc).  Runtime side, port only: 11 976
composites in `/tmp/rig/le6.log`, arms self-consistent with the flag in 11 976/11 976.

What is *not* available is any shipped-side observation of the left-edge attack -- not a frame (the two dumps
share no presentation basis, docs/202/docs/205), not a record field (the shipped-side `records.jsonl` snapshots
carry only `{o, n, type, st, ac, hp, f30}`; `+0x1b`, `+0x12b`, `+0x12f`, `+0x133`, `+0x137`, `+0x13b` are not
captured), not an in-process witness.  Item 6 therefore cannot be localized further without a **new oracle**:
extend both dumpers to capture the per-entity geometry fields (`+0x1b`, `+0x12b/0x12f/0x133/0x137/0x13b`
dereferenced at the current frame index) and the blit destination, on the shipped side as well as the port side,
then re-measure `scripts/rig/drives/ep1_leftedge.txt` on both sides at the same event anchor.

`fix-side-glitch` stays open with that as its next action.
