# 323 — The stage-1 overlay dialogue: which routine owns it, and the press-gate red measurement

Basis: `--warp-stage 1 --warp-column 3` (stage 1's last playarea: node #4, the only node whose `+0x34c` is
NULL) with `scripts/rig/drives/ep1_st1_room3_walk.txt` (entry chain + Right bursts, no pokes).
Runs: `/tmp/rig/e325r` (red), `/tmp/rig/w324d` (records/boss presence).  Continues docs/322.

## 1. The route the objective guessed is refuted: the room command interpreter has no overlay opcode

`fn_03ed6f` (0x0003ed6f) pumps the room's script-command list into `fn_03e20f` (0x0003e20f, 2862 B), whose
dispatch is `switch (rec[0])` at **0x0003ed40** (`dec al`, `>5` → default).  The full opcode census:

| opcode | offset | what it does |
|---|---|---|
| 1 | 0x0003e237 | spawn an entity for a trigger zone (`fn_00041bdc(0x147)` + `fn_010efc`) |
| 2 | 0x0003e94f | latch the room's edge triggers: dwords at room `+0x2a/+0x2e/+0x32/+0x36`, bytes at `+0x3a/+0x3b` |
| 3 | 0x0003eb8f | append a wall/clamp entry to `g_tab_5c8bc[g_dat_62a8c]` |
| 4 | 0x0003ebec | arm the `DAT_5d08c..98` box, only while `DAT_53c68 == 1` (the player alone) |
| 6 | 0x0003ec82 | arm `DAT_5d0a0..b0`, bumping the **same** counter as opcode 3 (an original quirk, kept) |
| 0, 5 | — | **hit the default and leave the record alone** |

So there is no text/overlay/dialogue opcode in the room command stream.  Item E is *not* reached through
`fn_03ed6f -> fn_03e20f`.

## 2. The routine that actually draws an overlay over the live stage

`fn_03a70e` (0x0003a70e, 282 B), armed from `fn_03af18`'s room loop.  The arm is three tests on the
**latched guardian record `DAT_00053c80`** (the record whose type index equals the stage counter — stage 1's
type 1, the record with hp 2000 seen in 84 of 121 samples in `/tmp/rig/w324d`):

```
0x3aa4c  mov eax, [0x53c80]                 ; the latch
0x3aa51  mov edx, [0x639e0]  ; camY
0x3aa57  add edx, 0xc8                       ; camY + 200
0x3aa5d  cmp edx, [eax+9]                    ; record Y
0x3aa60  jg  0x3aa64                         ; needs record Y < camY + 200
0x3aa69  mov edx, [0x639dc] + 0x32           ; camX + 50
0x3aa72  cmp edx, [eax+1]  ; jg 0x3aa77     ; needs record X >= camX + 50
0x3aa7c  mov edx, [0x639dc] + 0x10e          ; camX + 270
0x3aa88  cmp edx, [eax+1]  ; jge 0x3aa8f    ; needs record X <= camX + 270
0x3aa8f  call 0x3a70e                        ; the only call site of fn_03a70e in the whole image
```

Inside the show:

| offset | instruction | meaning |
|---|---|---|
| 0x3a73d | `DAT_52ff8 = 0` | the keyboard pump (`fn_0347e3`, gated at 0x347fb/cf) is OFF for the whole show |
| 0x3a7c5 | `count = [&DAT_52f08 + stage*4]` | stage 1 → **5 pictures** (`g_dat_52f08` = {2,5,6,6,6,4,3,9,3,…}) |
| 0x3a744-59 | `sprintf("na%d%04d.pcx", stage, index+1)` | `na1_0001.pcx` … `na1_0005.pcx` |
| **0x3a764-77** | `fn_03a594(0xaa8c0, DAT_633b0 + 0xa8c0, 0x5140)` | **the overlay composite**: `fn_03a594` is a filtered byte copy in which *bytes equal to 0xff are not copied*, so the previous screen survives wherever the new picture is transparent — that is the "overlay graphic over the actual stage" the playtester described |
| 0x3a77e-84 | `fn_036041(name, 0)` | load the picture into the A0000 window |
| **0x3a785-0x3a7a4** | press gate: `cmp [0x63ec1],0 / jne` … `cmp [0x63ea4],0 / je` … `cmp [0x63e89],0 / je → loop` | breaks when a latch is **already held**; it requires no rising edge and contains **no clearing store** |
| 0x3a7ad-bf | `if (index != count) call 0x4185e([0x53dd8])` | the show's device-command stream |
| **0x3a7cd-0x3a7df** | `while (DAT_63ec1 != 0 || DAT_63ea4 != 0) loop` | waits for both latches to drop to zero — again with no store |
| 0x3a7e7-f0 | `fn_03a5f7(DAT_633b0)` | the progressive odd/even-row reveal wipe (item F's effect) |
| 0x3a7f2-… | restore `DAT_63e24/63e28`, `DAT_52ff8 = 1` | pump back on |

The only writers of those latch bytes in the whole port are the keyboard-IRQ model (`src/app/keymap.c`,
make at **0x0003fe06**, break at **0x0003fe69-6e**) — the shipped has no clearing store in either wait loop,
so a picture is consumed without a press whenever a latch is still non-zero when the wait is entered.

## 3. Red measurement on the unfixed build (`/tmp/rig/e325r`, `NIGHT_SHOWDBG=1 NIGHT_KEYDBG=1`)

```
show: entry stage=1 count=5 latches esc=0 enter=0 punch=0 kick=0 space=0 pumpoff=1
show: pic 1/5 pre-latch enter=0 space=0 esc=0
show: pic 1 press taken by enter(63ea4)
show: pic 2/5 pre-latch enter=1 space=0 esc=0        -> press gate satisfied with NO press
show: pic 3/5 pre-latch enter=1                      -> NO press
show: pic 4/5 pre-latch enter=1                      -> NO press
show: pic 5/5 pre-latch enter=1                      -> NO press
```

Inspected counts (so the zero is admissible): `show: entry` runs = 1, `show: pic` pictures = 5 (14 witness
lines across the 5 pictures), gates satisfied with **no press = 4**, and the latch-edge witness
(`NIGHT_KEYDBG`, `latch1 = DAT_63ea4`) recorded **10 edges = 5 press+release pairs** for the whole run —
3 of them in the window containing the show.

**Red shape: 5 pictures consumed, 4 of them by a gate that needed no press at all** — the reported
"one Enter press skips the whole dialogue" ✓

## 4. The open half (must be closed before a fix)

The shipped's two wait loops contain no store, and the port's transcription matches them instruction for
instruction (docs/322 §3 basis).  Yet after pic 1's press is taken, pic 2 enters the gate with `DAT_63ea4 = 1`
even though the release wait at 0x3a7cd-0x3a7df should have spun until it reached 0.  Something in the port
re-asserts the Enter latch between pictures with no physical press.  The candidate to instrument is the spin
path itself — `sys_key_spin()` runs the SDL input pump inside both waits (and the shipped's keyboard pump is
OFF during the show at 0x3a73d), so the port may be feeding latch writes through a path the shipped has
closed.  A writer witness on `DAT_63ea4/63ec1` scoped to the show is the next measurement; a fix is not
writable until it names the site.
