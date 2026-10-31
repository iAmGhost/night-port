# 283 — Item 2b, award half: `fn_015295`'s entry block and callers read from the shipped, and the `+5` accumulator agrees between the sides in value family

## 1. `fn_015295` head, shipped (`--entry 0x15295 --end 0x15340`)

```
0x152ac: mov [ebp-8], eax        ; arg1 in EAX      0x152af: mov [ebp-4], edx   ; arg2 in EDX
0x152b8: add  dword [edx + 5], eax                  ; accumulate arg1 into record+5 (unaligned dword)
0x152be: cmp  dword [eax + 5], 0 / 0x152c2 jle      ; 0x152c7 mov dword [eax+5], 0   (clear when > 0)
0x152d1: al = [eax + 0x30] / 0x152d9 cmp eax,1 / jne 0x152f0      ; dying flag
0x15300: cmp [eax + 0x11], 0x15 / jg 0x1530b -> 0x15306 jmp 0x15456 ; state-slot gate
0x1530e: mov byte [eax + 0x25], 0xff                              ; latch
0x15315: al = [eax] / 0x1531c cmp eax,1 / jne 0x15456            ; record kind == 1
0x15328: cmp eax, [0x3c80]   ; DS 0x53c80                        ; "is this the player record?"
```

The port's model matches every test (`f.c:8248+`: `a0[0x30]==1`, `a0->0x11 > 0x15`, `a0[0]==1`, `a0[0x25] = 0xff`, the
`+5` accumulator via `struct sa32 *p5 = (a0 + 5)` ✓ "unaligned in the shipped layout" ✓) ✓ So the award **gates** are
faithful ✓ — and that settles one confusion: the shipped's `kind=0 type_idx=0 award(+0x32)=54` dying records (docs/282
§3) **cannot** be award sources, because `0x1531c` demands `record[0] == 1` ✓ So `+0x32 = 54/53` on kind-0 records is the
chain-target id parked in that cell ✓ (docs/188 ✓), not EXP ✓

## 2. Who calls it (`call_xrefs.py`, 4503 resolved `E8 rel32` edges)

```
call 0x15295: 3 direct sites -- fn_012616 x2 (0x12d61, 0x12dd2), fn_01441e x1 (0x14bc7)
```

`fn_012616` is the per-type handler family ✓ (the same family that holds the `+0x17` state stores ✓ docs/249/258 ✓) and
`fn_01441e` is the reaction/damage step ✓ — so **the value passed as arg1 is set up at those three sites**, and that is
where "EXP per enemy type" lives if it lives anywhere ✓ Next measurement: dump the argument-setup window before each of
`0x12d61`, `0x12dd2`, `0x14bc7` and check whether arg1 comes from a table row (type-indexed ✓) or from a constant ✓
then compare with the port's call sites ✓

## 3. `+5` accumulator census, both sides, rb9 (`exp_by_type.py --run /tmp/rig/rb9 --award 5`)

```
type 0x00   port: 42/893 nonzero   values {0x0, 0xffffffc8, 0xffffffc9, 0xffffffca, 0xffffffcb, 0xffffffcc}
            shipped: 12/1184 nonzero  values {0x0, 0xffffffcb, ...}
```

Both sides put the *same family* of values in `record+5` — small negatives `-56 … -52` ✓ (i.e. negative target-ish
numbers ✓) — so `+5` is faithful in shape on this route ✓ and it is **not** an EXP counter ✓ It is also not the cell the
port's award census reads (`+0x32` ✓ docs/282 §4 item 1 ✓) ✓

## 4. Item 2b status after this step

Chain selection: **closed** (docs/282 ✓ targets `0x33/0x34/0x35/0x36` selected, `0x1d`-window parity, settled `kind==1`
award-bearing record present ✓). Award gates: **faithful** (this doc §1 ✓). Award *value* provenance: **open**, and now
narrowed to three instructions-of-interest — the argument setup at `0x12d61`, `0x12dd2`, `0x14bc7` ✓ Player-side EXP
accumulation still has no valid instrument (§1's `+5` is not it ✓ and `+0x52` read out of the record dumps is
pointer-shaped ✓) ✓
