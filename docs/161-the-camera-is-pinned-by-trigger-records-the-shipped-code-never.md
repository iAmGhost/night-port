# docs/161 — the camera is pinned by trigger records the shipped code never accepts: the population divergence is upstream of it

docs/142 measured the symptom ("shipped `DAT_000639dc` never leaves 27, the port reaches 298"), docs/159 §5
retracted the window statistics, and docs/160 identified the policy's six room fields.  This checkpoint
measures the policy's *inputs* on both sides from the dumps, and finds that the camera is not the divergence —
it is the consequence of one.

## 1. The camera moves about the same number of times on both sides

`cam_policy.py` (travel census, `/tmp/rig/g15`, ep1_kill3, 387 s shipped / 300 s port):

```
ORIG  player travel per sample: |dpx| p50=0 max=55   total |dpx|=3069   net dpx=15
      camera: nonzero moves 58/1845  max |dcam| = 10  total |dcam| = 344  net dcam = 0
PORT  player travel per sample: |dpx| p50=0 (max dominated by garbage, see §5)
      camera: nonzero moves 56/2168  max |dcam| = 298 total |dcam| = 608  net dcam = 0
```

Same number of camera moves (58 vs 56), similar per-move size, **net camera displacement zero on both sides**.
`cam_jump.py --min 12` then says it exactly:

```
== ORIG: 1846 samples, jumps >= 12 px      (none)
== PORT: 2169 samples, jumps >= 12 px
   t=223005 cam 298 -> 0 (d=-298)  cnt=3 cmd=67 room[0]=0 room[8]=1600
```

The shipped camera never jumps; the port has one 298-px snap.  The docs/159 theory that 298 was "26 scroll
quanta of +3" is the wrong model for the *range*: the range is reached by ~40 ordinary moves and abandoned by
one reset.

## 2. The steady-state difference is the player's lead over the camera

```
ORIG player lead px-cam: n=1846  min=-30  p10=15  median=21  p90=162  max=209
     past the right gate (>190): 16    past the left gate (<130): 1397   dead band: 433
PORT player lead px-cam: n=2169  min=-1414813221  p10=0  median=117  p90=190  max=305
     past the right gate (>190): 212   past the left gate (<130): 1176   dead band: 781
```

The policy arms are pure geometry (`px - cam > 0xbe` right, `px - cam < 0x82` left, f.c:14128/14143).  The
shipped camera keeps the player ~21 px ahead; the port's camera trails ~117 px, so its right arm opens 212
times against the shipped side's 16.  At room entry both cameras are 0, but the player spawn differs: shipped
`px = -30` (lead −30), port `px = 0` (lead 0).  The entry-nudge table is *not* the cause — the shipped image's
`g_tab_52b84` (file offset 0x4b584) holds 1 only at indices 0x17, 0x18 and 0x3a, i.e. stage 3 room 4, stage 3
room 5 and stage 4 room 9; stage 1's ten entries are all zero, so no nudge runs on this route on either side.

## 3. The room record's stop bounds are written only by the spawn dispatcher — on the port

`room_record.py` follows the pointer cell (`0x3ab18: mov eax, [DS 0x63354]` → `call 0x0003d4e4`, mirrored by
`uint8_t *room = g_dat_63354` at f.c:23128 and passed at f.c:23193) and byte-censuses the record on both
sides.  Across 0x00..0x47 only three offsets differ, and they are one family:

```
  +0x28 PARTIAL  shipped [0] (1846 samples)  | port [0, 6553600, 15204352] (2169 samples)
  +0x34 PARTIAL  shipped [0] (1846 samples)  | port [0, 40435712] (2169 samples)
  +0x44 PARTIAL  shipped [0, 3679108]        | port [0, 1628748]
```

6553600 = 100<<16, 15204352 = 232<<16, 40435712 = 617<<16: these are **dword stores at +0x2a and +0x36** whose
low halves land in the dword at +0x28 / +0x34.  When they happen (port dump, the only side that ever writes
them on this route):

```
t=172414  room +0x2a=100  +0x2e=0  +0x32=0  +0x36=0    cam=103  camy=0  cnt=3
t=190473  room +0x2a=232  +0x2e=0  +0x32=0  +0x36=0    cam=233  camy=0  cnt=7
t=192984  room +0x2a=232  +0x2e=0  +0x32=0  +0x36=617  cam=298  camy=0  cnt=6
```

The camera's maximum on this route (298) is set at the third write, and each bound lands 3 px behind the
camera.  The camera's range is therefore a **function of which trigger records get consumed**, not of the
scroll arithmetic.

## 4. The shipped writer of those bounds, decoded verbatim — and the port is faithful to it

Byte-pattern search (`disasm.py --search 89422a` → file 0x3c3a2 = guest 0x3e9a2; `894236` → guest 0x3eab2) put
the stores inside `fn_03e20f`, whose sub-case 1 body at guest 0x3e964-0x3e9a8 is:

```
0x3e967: mov eax, [eax+0x0d] ; cmp eax, [DS 0x639e0] ; jle skip   ; record[+0x0d] > camy
0x3e978: add edx, 0xc8       ; cmp edx, record[+0x0d] ; jg keep    ; record[+0x0d] < camy + 200
0x3e98b: mov eax, [eax+0x09] ; cmp eax, [DS 0x639dc] ; jge skip   ; record[+0x09] < camx   <- a Y-field
0x3e99f: mov eax, [eax+0x09]                                        ; compared against a camera X
0x3e9a2: mov [edx + 0x2a], eax                                      ; bound = record[+0x09]
0x3e9a8: mov byte ptr [eax], 0                                      ; consume the record
```

with cases 2/3/4 at 0x3e9ee (+0x2e), 0x3ea50 (+0x32), 0x3eaa6+ (+0x36) in the same shape (3/4 add a
`camx + 0x140 > record[+0x09]` test).  The shipped code uses **record[+0x0d] for the vertical band and
record[+0x09] for the horizontal test and for the stored value** — the pairing is a shipped quirk, not a
coordinate the port may "correct".  The port reproduces it exactly:

```c
const int32_t rx = ((struct sa32 *)((rec + 0x09)))->v, ry = ((struct sa32 *)((rec + 0x0d)))->v;   /* f.c */
...
case 1: if (camy < ry && ry < camy + 0xc8 && rx < camx) { room[+0x2a] = rx; rec[0] = 0; }  /* 0x0003e9a2 */
```

**Negative result, recorded deliberately:** the docs/94 field-role/heap-offset hypothesis is *not* what is wrong
here.  `rx`/`ry` in the port are the same fields shipped reads, the guards match instruction for instruction,
and the stored value is the same field.  The divergence is in what reaches this dispatcher.

## 5. What reaches it: the port keeps more live entities

Entity count (`DAT_00053c68`) histogram over the whole route, both sides:

```
ORIG  0:4   1:943  2:89  3:705  4:83  5:19  6:3                      max 6
PORT  0:494 1:108  2:381 3:696  4:117 5:79  6:227 7:53 8:5 9:9       max 9
samples with count > 5:  orig 3    port 294
```

and `ent_list.py` on the same run reports 2676 shipped entity-samples against 3167 port ones (18 % more), with
the *state* census comparable after docs/115's normalisation (hurt-band 0.080 shipped vs 0.086 port).  The port
is not behaving differently per entity; it has **more entities alive**, for longer, and the extra ones are the
trigger records that get as far as `fn_03e20f` sub-cases 1-4 and pin the camera.

A related witness: `NIGHT_SPAWNTRACE=1` on a 90 s port run emits only two spawn dispatch events
(`t=31 stamp=0 rec=00374594 sub=1 gate=0 cgate=0 rx=324 ry=150 room11=0 cnt=1 cap=9 anchor=350` and
`rx=368 ry=193 cnt=2`), i.e. the visible spawn traffic is small early and the bound writes happen after 170 s —
the census has to cover the later half of the route.

Garbage found in passing: 23 port samples have an absurd player x (≈ −1 414 813 000, e.g. sample 1467 with
`px = -1414812923`, `cam = 298`).  Same family as docs/128's dangling entity pointer.

## 6. Rig defect caught while making this measurement

`disasm.py --entry 0x10000 --end 0x50000` produced 10 560 lines covering only **0x10000..0x18dce** — it stops
at an undecodable byte rather than continuing.  Grepping that file for `mov dword ptr [reg + 0x2a]` returned
zero hits and briefly supported a false conclusion ("the shipped code never writes the camera bounds"), which
the `--search 89422a` pattern search contradicted within a minute.  Rule filed: **an absence claim must come
from a pattern search or from a per-function disassembly, never from one linear sweep of this disassembler.**

## 7. Next measurement (queued)

Both symptoms — extra live entities and camera bounds pinned by trigger records — are outputs of
`fn_03e20f`.  The measurement that separates "the port's gates are too permissive" from "the port's record
stream contains records shipped never builds" is a **per-`sub` spawn-gate census on the port** (extend
`NIGHT_SPAWNTRACE` to log every `fn_03e20f` dispatch including the ones that *reject*, with `rx/ry/camx/camy/
sub`) against the shipped side's **entity-count timeline aligned on the docs/158 transition anchor**, so the
first sample where the port's count exceeds the shipped count is identified and the records alive at that
moment are read from both dumps.
