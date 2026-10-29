# 136 — Localized: an atype-11 hit with zero debt marks the player dying at 994/1000 hp

docs/135 left one question: who sets the dying flag (`entity+0x30`) on the player. Answered with witnesses,
not inference.

## 1. Instruments added

- `NIGHT_ARENADBG=1` arena lifetime witness (`port/src/f/f.c`, before `fn_00041bdc`): an open-addressed set of
  the entity blocks the constructor actually initialised (added in `fn_010efc`, removed in `heap_free_guard`),
  plus `arena_alloc_check`, which reports when an allocation hands back a block a live entity still owns.
  This is the arena-level sanitizer docs/128 asked for; AddressSanitizer cannot see a lifetime error inside the
  port's own flat heap because every entity block is one arena.
- `NIGHT_DEATHTRACE=1` now reports `live=` for the dying block, and every store of the dying flag in the port
  (11 sites) is wrapped in `arena_flag_check`, which fires if the target is the player block.
- `fn_03203d`'s sweep watches the player's `+0x30` byte and prints every flip with a hex window 0x28..0x38.

## 2. The lifetime hypothesis is negative

- 0 reports from `arena_alloc_check` over a full 16000-frame run: the allocator never hands back a block a live
  entity owns.
- The dying player block reports `live=1`: it is not a freed block.
- No `+0x30` writer fires on the player at the spawn sites (`fn_011827`, the 0x1164b three-way spawn) or in
  `fn_00035c5c`.

So the flag arrives through a *legitimate-looking* write inside the hit application, and the disassembly says
exactly which one.

## 3. The shipped rule (0x00014cba-0x00014cd9, capstone over NIGHT.EXE)

```
0x14cba  movsx eax, word ptr [e + 0x23]     ; the debt word
0x14cbe  test  eax, eax
0x14cc0  jne   0x14cc8                      ; debt != 0 -> cannot take this arm
0x14cc2  cmp   dword ptr [ebp - 0x24], 0xb  ; atype >= 11 ?
0x14cc6  jge   0x14cca
0x14cca  cmp   dword ptr [ebp - 0x24], 0x14 ; atype <= 20 ?
0x14cce  jle   0x14cd2
0x14cd2  mov   byte ptr [eax + 0x30], 1     ; DYING, independent of hit points
```

i.e. **a victim with no outstanding debt that is hit by a reaction code 0x0b..0x14 is marked dying no matter
what its hit points say.** The port transcribes this faithfully (f.c:24763-24766, `0x00014cd2`).

## 4. The event, measured (ep1_kill3, 16000 frames, NIGHT_HITTRACE=1 + NIGHT_DEATHTRACE=1)

```
arena: fn_01441e 0x1498d debt settled, hp>0 set the dying flag (+0x30) on the PLAYER off=0x3a75d8 tick 6513
HIT  atk(type=36 st=29) vic(type=0 PLAYER st=01) slot=0 atype=11 dmg=8 hp 1000->994 debt 0 ovs 0
arena: fn_01441e 0x14cd2 set the dying flag (+0x30) on the PLAYER off=0x3a75d8 tick 6513
arena: PLAYER +0x30 0 -> 1 at tick 6513 (tickcount 5426 state 0x15 hp 994) window 0x28..0x38: 00 e2 03 00 00 00 00 00 01 00 05 00
deathtrace: tick=6801 off=0x3a75d8 type=0 state=0x1 hp=994 exp=5 live=1 PLAYER
```

An attacker of **type 36** in **state 0x1d**, resolved at **slot 0**, produces **atype 11** (`0x0b`, the first
member of the lethal band), 8 damage, zero debt -- and by the shipped rule that instantly flags the player as
dying. Everything in docs/135 (the CONTINUE countdown decrement, the extra CD track idx12 selection, the
"quit/fatal" unwind) is downstream of this single hit resolution.

## 5. What is therefore the real open question

The code path is not wrong; the **resolved reaction code** is what differs. The open item "reaction volume:
port 227 reaction-band samples vs shipped 101" (docs/120 era) is the same signal. The differential question is
now concrete and measurable on both sides:

> For an attacker of type 36 in state 0x1d, what is `byte[record + slot + 0x38]` at slot 0 -- on the shipped
> side, from the action containers captured in the state dumps (docs/118's heap census route:
> `*(e+0x123) -> [+4] -> +4*e[+0x17] -> record`), and on the port side, from `NIGHT_HITTRACE`?

If the shipped byte is not in 0x0b..0x14, the port is resolving the chain to the wrong record or the wrong
slot (docs/110's "port consumes slots more often, per-slot level field is large"), and that -- not the CD
layer, not the reaction table, not the death dispatch -- is the source of the player-death cluster.

## 6. Note on run-to-run variation

Three runs of the same drive flagged the player at ticks 5832 / 6513 / 7643 / 7732 with hp 992..1000: the
trigger is reachable on this route in every run, at a different phase (docs/101's entropy cells), so it is not
a boundary case of one seed.

## 7. Queued measurement, written down so the next run does not have to rediscover it

The open question is a single byte, and both sides can produce it:

- Port side: the drive-language verb `actdump <type> [bytes]` (implemented as `rig_dump_act()` in
  `port/src/app/rig_sink.c`) captures the *loaded* action container for an entity type. For type 36 the chain
  the game walks is `lvl2 = *(hb + *(e+0x123) + 4)`, `rec_e = *(hb + lvl2 + 4*state)`, and the reaction code is
  `byte[rec_e + slot + 0x38]` -- with `state = 0x1d` and `slot = 0` for the hit measured in §4. The port
  already reports the value it uses through `NIGHT_HITTRACE` (`atype=11`).
- Shipped side: the same container is on the CD image under `NIGHT/` (the `.act` archives read by the master
  loader); the shipped value for the same (type, state, slot) triple is therefore readable from the asset
  without the emulator, and independently from the entity/action records captured in `state_*.heap.bin`
  (docs/118's census route).

Decision rule: if the shipped byte is outside 0x0b..0x14, the port is resolving the chain to the wrong record
or the wrong slot (docs/110 §"port consumes slots more often, per-slot level field is large"), and that
resolution -- not the CD layer, not the reaction table, not the death dispatch -- is the root of the
player-death cluster and, through it, the CONTINUE-countdown DIFF cell (docs/134 §7) and the extra CD idx12
selection (docs/135 §2). If the shipped byte is inside 0x0b..0x14, then the shipped game really does kill the
player on this drive and the divergence is elsewhere in the timeline (docs/131's phase offset), which would
retire the "spurious death" reading of docs/135.
