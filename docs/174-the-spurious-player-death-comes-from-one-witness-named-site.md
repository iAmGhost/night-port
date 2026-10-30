# 174 — The spurious player death is written at a named site, and it is not a reused block

docs/135 left the port's player dying at 998/1000 HP as the trigger for the CONTINUE credit (`DS 0x53100`), the
extra CD selection, and part of the entity-bookkeeping divergence -- and proposed that the dying flag
`entity[+0x30] == 1` had arrived through a dangling/reused block (docs/128's family).  Two witnesses settle it.

## 1. No double allocation

A check added at the entity constructor, before the block is registered live:

```c
if (arena_dbg() && arena_live_has((uint32_t)(block - (const uint8_t *)g_heap_base)))
    fprintf(stderr, "arena double-alloc off=%u block=%p kind=%u\n", ...);
```

On `ep1_kill3`, 12 000 frames, calibrated cadence: **`double-alloc` lines: 0**.  `fn_00041bdc` never hands out a
block that a live entity still owns on this drive.  The reused-block hypothesis is not what happens here.

## 2. The writer is named, and it is in the damage code

The arena witness prints the site that sets the flag on the player block:

```
arena: fn_01441e 0x1498d debt settled, hp>0 set the dying flag (+0x30) on the PLAYER block
arena: PLAYER +0x30 0 -> 2 at tick 7556 (tickcount 6205 state 0xc  hp 1000)
arena: fn_01441e 0x14cd2 set the dying flag (+0x30) on the PLAYER block off=0x3a75d8 tick 7925
arena: PLAYER +0x30 2 -> 1 at tick 7925 (tickcount 6574 state 0x15 hp 992)
```

Two distinct writes, both from `fn_01441e`:

* `0x1498d` -- the settle path (docs/162 §9's settle at `0x14984`), which sets `+0x30 = 2`.  Shipped does this too:
  docs/167 §9 measured the shipped player holding `+0x30` values 0 and 2 only, across 1842 samples.
* `0x14cd2` -- sets `+0x30 = 1` **while the player is in state `0x15` with hp 992**.  This is the transition that
  ships the player into the death path, and `+0x30 == 1` is a value the shipped player never takes.

## 3. Where the chain starts

`0x15` is the reaction state entered by hit code 11 (docs/137's stride-4 decoding: code 11 -> state 0x15), and
docs/162 §12 measured the port's player receiving `atype=11` three times (`atk type=36 st=0x29 slot=0 atype=11
dmg=8`) while docs/155 showed **hit code 11 never landing on the shipped player**.  docs/144 §7 already flagged
state 0x15 as "the only route by which the player is ever marked dying".

So the causal chain for a whole cluster of the state residual is:

```
atype 11 lands on the player (3 times; never in shipped)
   -> player enters state 0x15
      -> fn_01441e at 0x14cd2 sets entity[+0x30] = 1
         -> the entity step retires the player
            -> CONTINUE credit spent (DS 0x53100 3 -> 2)
            -> extra CD-DA selection (docs/135's idx12)
            -> entity-list bookkeeping rows (DS 0x53c64 / 0x53c68 / 0x53c6c)
```

The fix therefore belongs in the hit selection that lets `atype 11` reach the player at all, not in the death
path.  The named next measurement: read the shipped action-record census for the attacker that produced it
(type 36, state 0x29, hit slot 0) and ask whether the shipped record for that combination carries code 11 --
the docs/156 `NIGHT_ACTCENSUS` data already holds the container list, and docs/155 already shows the shipped side
never applies 11 to the player.

## 4. The extra population is the same spawn family that carries the dying flag

`pop_align.py` on the calibrated route (`/tmp/rig/g20`, anchored on the room-script drain):

```
FIRST population divergence at t=167445 ms: shipped count=3 (3 live records), port count=4 (4 live records)
   PORT  ent00: type=0x01 state=0xc  hp=0    flag30=0  x=188 y=166
   PORT  ent01: type=0x00 state=0x1  hp=1000 flag30=0  x=228 y=173      <- the player
   PORT  ent02: type=0x01 state=0x2  hp=0    flag30=0  x=259 y=176
   PORT  ent03: type=0x02 state=0x1  hp=0    flag30=1  x=191 y=182      <- the extra record
   ORIG  ent00: type=0x01 state=0x1  hp=0    flag30=0  x=86  y=141
   ORIG  ent01: type=0x00 state=0x36 hp=1000 flag30=0  x=90  y=167      <- shipped player in a rope/climb state
   ORIG  ent02: type=0x01 state=0x1  hp=0    flag30=0  x=161 y=193
aligned buckets where port count > shipped count: 44 of 171
```

The extra record is `type=0x02` with **`flag30 = 1`** -- the "retire me at the end of the step" flag, on a record
with `hp = 0`.  That is the same transient spawn family docs/135 counted (146 of 148 deaths were type 2) and the
same family whose writers are `fn_011827` and the three-way spawn at guest `0x1164b-0x11675` -- the two places
that write `+0x30 = 1`.  The port therefore both **creates more** of these one-shot records than shipped and
**leaves the flag set on the player** through `fn_01441e @0x14cd2`; both symptoms sit on the spawn/hit-selection
boundary rather than on the entity list mechanics.

Shipped's own state at that instant is worth noting too: its player is in `0x36` (the rope/climb family,
docs/144 §7) while the port's is in `0x01` -- the population and the state-family divergences are visible in the
same sample.
