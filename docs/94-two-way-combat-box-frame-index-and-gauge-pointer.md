# 94 — Two-way combat: fn_013c4d/fn_01441e index the hit boxes by the FRAME NUMBER from `rec+0x6d`, not by the raw frame slot; and DAT_00053c88 (the "other" gauge) must stay in the entity-pointer representation

Playtest log item "Player cannot attack enemies, even enemy can't attack the player".

## 1. What the shipped code does (Ghidra, NIGHT.EXE, project ppk)

`fn_01441e` (0x0001441e) box pre-check, 0x0001445c-0x0001447b (raw disasm):

```
0001445f  MOV EDX,[EAX+0x11]      ; frame
00014467  MOV EBX,[rec]           ; rec = [EBP-0x10]
0001446a  IDIV dword [EBX]        ; signed slot = frame % rec[0]
0001446c  ADD EDX,EDX
00014471  MOV EAX,[rec+0x6d]      ; uint16 FRAME-NUMBER array
00014478  MOV AX,word [EDX+EAX]   ; fi = framearr[slot]      <- frame NUMBER
00014487  MOV EAX,[type*4+0x639ec] ; spr = g_dat_639ec[type]
00014493  MOV EAX,[EAX+0x1c]; box = arr[fi<<2]               ; indexed by fi
000144b2  ...          [EAX+0x20]
```

`fn_013c4d` (0x00013c4d) is identical in shape (0x00013d06-0x00013d47): both A and B box
indices are the FRAME NUMBERS mapped through `rec+0x6d` / `anim+0x6d` (locals EBP-0x34/EBP-0x30),
then the eight parallel dword arrays `g_dat_639ec[type] + 0x14/0x18/0x1c/0x20/0x24/0x28/0x2c/0x30`
are indexed by those numbers (0x00013dc1-0x00013ebd, 0x00013ee6+, SHL 2 addressing).
docs/19's "indexed by the frame slot (`A[0x11] % rec[0]`)" is only true when the record's frame
array is the identity; the player's punch/kick ACT records map their slots to high sprite frames
(measured: punch state 0x33 -> fi=55, kick 0x3d -> fi=70/71, with `rec[0]=11` slots 0..10).

Second deviation, the HUD gauge latch: `DAT_00053c88` is written with the RAW entity pointer
(`0x000147f8 MOV [0x53c88],EAX`), compared with the RAW walked pointer (`0x00012a10 CMP`), and
passed directly as the entity argument (`0x0003814a MOV EAX,[0x53c88]; CALL 0x1348f`). The port
stored `cur - hb` (a heap offset) and then dereferenced it as a host POINTER in fn_038098 --
`fn_01348f((uint8_t *)(uintptr_t)g_gauge_other_off, ...)` -- which faults at the bare offset
value (lldb: `EXC_BAD_ACCESS address=0x18e6bb`, stack fn_01348f <- fn_038098 <- fn_03a829 room
loop). That was the stage-1 SIGSEGV whenever a hit on an enemy was followed by another room
draw, i.e. the playtest log's "Segfault when keep stage-1 opened (maybe related with hit
detection)" -- it IS the hit path.

## 2. Port fixes (all address-annotated in src/f/f.c)

* fn_01441e pre-check: map the slot through `hb + *(rec_e+0x6d) + 2*slot` (uint16) before the
  +0x1c/+0x20 array reads (`fslot` -> `fi`).
* fn_013c4d box section: index all eight arrays by `v34`/`v30` (the mapped frame numbers), not
  `idx_a`/`idx_b`. (fn_013c4d already computed v34/v30; they were only used for the effect-spawn
  argument -- latent before because every earlier test used identity frame arrays.)
* fn_038098: `fn_01348f((uint8_t *)g_heap_base + g_gauge_other_off, base, 1)`;
  fn_012616: death compare in the same representation
  (`g_gauge_other_off == (uint32_t)(uintptr_t)(e - hb)`).
* Diagnostics (no behavior): `NIGHT_HITTRACE=1` prints one line per fn_01441e hit
  (`HIT atk(...) vic(...) slot atype dmg hp/debt before->after [LETHAL]`), the fn_013c4d gate
  rejects (`GATE2/4/5/6`, incl. measured `dl=7`, punch box `+3..+36`), and the fn_01441e pass-2/3
  state. `NIGHT_HUDDBG` (pre-existing) prints the gauge words.
* test_f1 harness bugs the corrected indexing exposed: `prec` in test_fn_013c4d was 0x220 bytes
  while the sub-record writes reach +0x263 (spilled into the `afr` frame array allocated right
  after); the fn_01441e test left the container's STATE 2 entry null although the bump-scan cases
  run the attacker in state 2 (the box pre-check dereferences rec+0x6d for every state). Both
  fixed in test_f1.c; suite now 3061 checks ALL PASS.

## 3. Measured run (port, scripted; scripts/playtest/route_fight8.txt)

`NIGHT_HITTRACE=1 SDL_VIDEODRIVER=dummy ./test/night --script ../scripts/playtest/route_fight8.txt
--shots /tmp/dbgauto/port_fight18 --no-audio --frames 21000`

19 hits both directions in one ~60 s fight:

```
HIT atk(type=31 st=29) vic(type=0 PLAYER st=3d) slot=0 atype=1 dmg=1 hp 1000->1000 debt 99 ovs 0
   ... debt 98, 97, 96, 95, 94, 93, 92, 91  (nine enemy hits, player gauge word +0x23 drains)
HIT atk(type=0 st=3d) vic(type=31 st=29) slot=0 atype=7 dmg=4 hp 0->0 debt 31 ovs 0
   ... debt 27, 23, 19, 15, 11, 7, 3 (eight punches/kicks, enemy gauge word drains)
HIT atk(type=0 st=3d) vic(type=31 st=29) slot=0 atype=7 dmg=4 hp 0->0 debt 0 ovs 0 LETHAL
```

The HUD (`[hud]` witness, NIGHT_HUDDBG) shows the same numbers as pixels: player gauge words
`a=91 b=1000` after 9x1 damage, enemy panel `gi=31 a=31 r1=15` (half-width) mid-fight, and the
crimson bar at y=31 x=23..122 sampled from a shot. Depth matters exactly as the shipped gates
say: every miss is `GATE4` (|ay-by9| > dl=7) or a >36 px x gap; the hits cluster at
dy in {3..7, -1}, dx <= 36 (box +3..+36 measured).

## 4. Reproduce

```sh
cd port && make -j8 test/night
NIGHT_HITTRACE=1 NIGHT_HUDDBG=1 SDL_VIDEODRIVER=dummy ./test/night \
  --script ../scripts/playtest/route_fight8.txt --shots /tmp/dbgauto/port_fight18 --no-audio --frames 21000
./test/test_f1            # 3061 checks, includes fn_013c4d/fn_01441e pinning
```

Ghidra dumps for this doc: scripts/ghidra_scripts/DumpHpDis.java (HP model 0x1488a-0x149c0 +
fn_01441e decompile), DumpBoxPre.java (0x14439-0x144bf), DumpHitBox.java (0x13d01-0x14026),
DumpGauge.java (0x38114-0x38160 / 0x147e6-0x14800 / 0x129c4-0x12a24); outputs
/tmp/dbgauto/{hp_dis,boxpre,hitbox_dis,gauge_dis}.txt.

## 5. Still open (tracked by segfault-stage1)

With the gauge crash fixed, a later crash remains: the floating-value number sweep
`fn_03b953` reads the `+0x11` link AFTER fn_03b718 freed the node (faithful latent bug,
documented in the port's fn_03b953 header). ASan reproduces it:
`AddressSanitizer: SEGV f.c:13811 in fn_03b718` during a combat+hold run -- the chain cursor
becomes garbage rather than a stale-but-valid link, which the next task must root-cause
against the guest's block layout (link survives free in the guest; find what differs here).
