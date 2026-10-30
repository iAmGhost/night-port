# 237 — Item 2b, second look: the entity-type table *is* loaded and matches the shipped row for row; what never happens is that `fn_01441e`'s settle walk sees a record carrying the settle flag

Continues `fix-exp-values`.  This document retracts two claims made one document ago in docs/236 and replaces them
with measurements.  Nothing here changes the goal's open items: item 2b is still unfixed.

## 1. RETRACTED (docs/236 §2): "the port never fills the entity-type table"

The port does fill it, in `fn_037b03`, and the values match the shipped's runtime table exactly.

Shipped loader, read out of the bytes (capstone, `fn_037b03` body 0x37b03-0x3801a):

```
0x37e5c  push [DS 0x63370]            ; the stage index
0x37e62  push 0x159e ; 0x37e68 lea eax,[ebp-0x4c] ; 0x37e6c call 0x428f8   ; open "night%d.chr"
0x37e77  call 0x367c9 ; 0x37e7c xor ebx,ebx ; mov edx,2 ; 0x37e86 call 0x36853
0x37e8b  [ebp-0x14] = 0                                       ; row index
0x37e94  loop: eax = idx ; inc idx
0x37e9a  cmp idx, 0x65 ; jge 0x38008                          ; 101 rows  (0x65)
0x37eb4  eax = idx*0x88 ; 0x37ebb esi = 0x3e9c ; add          ; dest = DS 0x53e9c + idx*0x88
0x37ec2  call 0x4192e                                         ; column +0x00
0x37ec7  eax = [DS 0x639b8] ; 0x37ecc call 0x41c8f ; 0x37ed8 [idx*0x88 + 0x3ea0] = eax   ; +0x04 (dword)
0x37ef5  dest +8  ; call 0x4192e                              ; +0x08
0x37f1b  dest +0xc; call 0x4192e                              ; +0x0c   <- the EXP award column
0x37f2a  call 0x41c8f ; 0x37f3b [.. + 0x3eac] = eax           ; +0x10 (dword)
0x37f58  dest +0x14 ; 0x37f7e dest +0x18 ; 0x37fa4 dest +0x1c (EDX=EBX=0xa) ; 0x37fca +0x80 ; 0x37ff2 +0x84
0x38003  jmp 0x37e94
```

Port model, `port/src/f/f.c:5296-5307`: the same ten accesses in the same order (`fn_04192e(e+0x00/+0x08/+0x0c/
+0x14/+0x18, 1, 2, io)`, `fn_04192e(e+0x1c, 10, 10, io)`, `fn_04192e(e+0x80/+0x84, 1, 2, io)`, and the two
`fn_041c8f` dword reads for `+0x04` and `+0x10`), with `for (i < 0x65)` matching 0x37e9a ✓

New witness `NIGHT_CHRDBG=1` (added at `f.c:5309+`, prints the io node state and three table rows after the loop),
`./port/test/night --script scripts/rig/drives/ep1_kill3.txt`, pinned clock:

```
chrload: io=0x102878b40 flags=49 cursor=20140 remain=572 buf=16616 handle=1 size=4096
chrrow  0: +0=100 +4=0  +8=1000 +c=0   +14=0   +18=0   +80=0 +84=0
chrrow  1: +0=200 +4=1  +8=2000 +c=250 +14=200 +18=20  +80=1 +84=0
chrrow 31: +0=35  +4=2  +8=0    +c=10  +14=300 +18=150 +80=2 +84=50
```

Row 31 is `(35, 2, 0, 10, …)` = the shipped's `(0x23, 0x2, 0, 0xa, …)` measured in 972 of 974 shipped states
(docs/199 §2) ✓✓ The table is loaded, the award column carries 10, and `g_dat_53ea8 = g_dat_53e9c + 0xc`
(`f.c:8158`) addresses it as the shipped does.  docs/236 §2's "8 unmodelled fills / BSS-zero table" is void.

(Accident worth recording: while adding that witness one `fn_04192e(e + 0x80, …)` line was briefly deleted by the
edit and then restored -- the comment at `f.c:5306` is the marker.  An edit that rewrites a block must be diffed,
not trusted.)

## 2. RETRACTED (docs/236 §3): "the flat-5 store at 0x15054 is unmodelled"

It is modelled, inside `fn_01441e`'s flag-2 branch:

```c
g_settle_flag2++;
*(uint8_t *)(cur + 0x5a) = 1;                    /* 0x1503c */
*(uint8_t *)(cur + 0x31) = 1;                    /* 0x15046 */
*(uint8_t *)(cur + 0x30) = 2;                    /* 0x1504d */
((struct ua32 *)((cur + 0x32)))->v = 5;          /* 0x15054 -- present, just uncited */
```

The only real complaint is the missing citation ✓ Add `0x00015054` to that line when the area is next touched.

## 3. Where the chain actually breaks: `fn_01441e`'s walk never meets a settle flag

New census counters `g_1441e_gate_set[8]` / `g_1441e_gate_kind[8]` (`f.c:1241`, incremented at the 0x15003 gate,
printed by the `NIGHT_ACTTRACE` census block, `main.c:1617-1622`), canonical 150 s dumped route
(`ep1_kill3.txt`, `--rig-hz 10`, 9000 frames, `NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_SEED=13579`,
`/tmp/rig/x2`):

```
settle-flag writers : fn_035c5c calls=0 nodes=0 sets(+0x30=1)=0   fn_01441e sets(+0x30=2)=0
exp award gates     : +0x30!=1 1224  +0x25==ff 0  +5!=0 0  +0x11<=0x15 0  +0x00!=1 0  reached 0
fn_01441e walk      : kind 0..7 = 20 6 0 0 0 0 0 0
fn_01441e walk      : +0x30 seen 0..7 = 26 0 0 0 0 0 0 0
exp award path      : adds=0 added=0 commits=0 committed=0
```

Read together:

* the walk covers **26 record slots** -- 20 of kind 0, 6 of kind 1, **none of kind 2** -- and **every visited slot
  has `+0x30 == 0`** ✓✓ so neither award branch (flat 5 at 0x15054, per-type table at 0x1505d-0x15072) and neither
  `fn_035c5c` (0x150f5) is reachable;
* `fn_015295` is called 1224 times and every call leaves at the `+0x30 == 1` gate (0x152d1-0x152dc) ✓
* yet the same run's *record dumps* contain **178 records with `+0x30 == 1`** (`scripts/rig/exp_award.py
  --port /tmp/rig/x2/port`, window 150-300 s): `kind=2 type_idx=250 state=0x3/0x2/0x4`, `kind=2 type_idx=201
  state=0x1/0x4`, `kind=0 type_idx=0 award=5 state=0x15 hp=999` ×31 ✓✓

So the settle-carrying records exist in the port's heap and are stamped by the modelled spawners
(`fn_0113e7` 0x1164b-0x11675, `fn_011827` 0x1191a -- both modelled, both stamp `+0x30 = 1` ✓), **but they are not
among the slots `fn_01441e` walks.**  The shipped's dumps do show settle-carrying kind-2 records and settle-pass
signatures, and the shipped's award census carries the per-type value 10 (docs/199 §1, `/tmp/rig/ents-orig/orig`)
✓

That points item 2b at the same open defect as the camera cluster: the port's **live-entity population / list
membership** (docs/161 §, docs/162 §: population -> damage link established, population root cause open) ✓ The
walk is `cur = g_dat_53c70; for (i < g_dat_53c68)` in `fn_01441e`'s pass (`f.c:25091-25093`, citing
0x144d5/0x144dd-0x144f5) ✓ so the number 26 is the port's `g_dat_53c68`, and the kind-2 reaction records are
apparently not linked into that list ✓

## 4. Tool defect found while doing it: `imm_scan.py` missed every bare-displacement DS read

`--imm 0x3c80` and `--imm 0x3c68/0x3c70` returned zero hits, while the disassembly plainly contains
`0x15016 cmp eax, dword ptr [0x3c7c]` ✓ Cause: capstone keeps a memory displacement in `op.mem.disp`, not
`op.imm`, and in this capstone build a memory operand reports `type == 3` while the imported `CS_OPT_MEM` is 4 ✓
The scan now matches `getattr(o,'mem',None) is not None and o.mem.disp == want and base == 0 and index == 0` ✓
Re-running with the fix:

* `--imm 0x3c70` (DS 0x53c70, the list head `fn_01441e`/`fn_035c5c` walk): bare reads at 0x35c74 (in
  `fn_035c5c`), 0x380c6 (`fn_038098`), 0x382a4 (`fn_0381d7`)
* `--imm 0x3c68` (DS 0x53c68, the live count): `0x3d563/0x3d5ca cmp [0x3c68],1` in `fn_03d4e4`,
  `0x3e38a/0x3e6e5 mov edx,[0x3c68]` and `0x3ebec cmp [0x3c68],1` in `fn_03e20f`
* `--imm 0x3c7c` (DS 0x53c7c, the player-record cell): 3 reads in `fn_03d4e4`, 3 in `fn_03fcbd`

Consequence: every earlier "no instruction names that DS cell" verdict obtained with `imm_scan.py` for a cell the
game reaches as `[disp32]` is **void** and has to be re-run before being quoted ✓ (docs/232 §3's negative results
for the CD-age cell used `--imm 0x3e38`-style scans and must be re-checked the same way.)

## 5. Next measurement for item 2b

1. Census the *link* mechanism: every shipped write that adds a record to the `DS 0x53c70` / `DS 0x53c68` list
   (`fn_010efc`'s link sites, `fn_031f10` / `fn_031f5f` queue sites, `fn_03e20f`'s spawn tails) and the port's
   model of each ✓ then compare the walked-slot count (port: 26) against the shipped's live-record census taken
   from the same route's record dumps ✓
2. If the port under-links: the fix lands in the spawn/link path, and the green measurement is already defined --
   `scripts/rig/exp_award.py` must show per-type award values (10 for type 31 ✓) on the port's dying records
   instead of `type_idx=31: [0]`, with `fn_035c5c calls > 0` and `exp award path: adds > 0` ✓
3. Keep the docs/190 gate census as the accompanying witness (it names which gate the calls die at ✓).
