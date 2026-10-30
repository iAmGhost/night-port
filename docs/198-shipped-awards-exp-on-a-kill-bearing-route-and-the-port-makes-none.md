# 198 — Shipped *does* award EXP on this drive (7, then 8); the current port build awards none, and the route no longer kills

## 1. The shipped side's award, read out of its own player record

Run `/tmp/rig/ents-orig` + `/tmp/rig/ents-port` (docs/108's kill-bearing pair, drive `ep1_kill3.txt`, shipped
window covering t=[150,300] s, 974 shipped states, 1 292 port samples at 10 Hz).  Shipped's kill counter
`DS 0x639bc` increments twice: state 371 (0 -> 1) and state 398 (1 -> 2).  Reading the player record snapshot on
both sides with `scripts/rig/ent_fields.py --cell g_player_ent` (DS `0x53c7c`, 352 B snapshot):

```
phase                              +0x2b           +0x52           +0xbb           +0x32           +0x30
P213@265200                          0/0             0/0             0/0             0/0             0/0
P214@267200                        a/0             7/0             0/0             ...             ...
P215@268200                        a/0             0/0
   shipped field value sets over the run:  +0x2b {0, 0xa, 0x14}   +0x52 {0, 7, 8}   +0x32 {0, 0x36}   +0x30 {0, 2}
   port    field value sets over the run:  +0x2b {0}              +0x52 {0}         +0x32 {0, 0x34, 0x35}  +0x30 {0, 2}
```

Shipped's player record therefore *receives* EXP (`+0x52` becomes 7, later 8 — two awards for two kills) and its
display field `+0x2b` ramps 0 -> 0xa -> 0x14, which is exactly the `fn_0137c1` count-up (docs/125 §5) feeding the
number the player sees.  The port's player record holds **0 in both fields in every phase of the same drive**.

The award values 7 and 8 differ per kill, which is the shape item 2b claims ("EXP gain per enemy type"), and the
shipped award path is the one docs/189 §1 enumerated (`fn_015295` 0x15448-0x15453: `player[+0x52] += victim[+0x32]`,
and `fn_01441e` 0x15161-0x15186: `player[+0x52] += player[+0xbb]` for self-award).  `+0xbb` is 0 on both sides in
every phase, so the shipped awards came through `victim[+0x32]`, whose value set on the shipped side is {0, 0x36}.

## 2. The current build, same drive, 320 s

`./port/test/night --script ep1_kill3.txt --frames 19200` with the pinned environment
(`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, binary rebuilt through `make -C port app`):

```
exp award path: adds=0 added=0 commits=0 committed=0
integer-draw calls: fn_038c23=0 fn_0383a4=0
DS 0x639bc (kill counter): 0 in all 2 248 samples  (docs/108's build made 2 kills at t=184260 and t=211749 on this same drive)
```

Two things follow, and they are separate.

1. **The port no longer lands a kill on this drive at all.** docs/108 measured `0x639bc` going 0 -> 1 -> 2 on the
   port side of this same drive; today the same drive and seed give zero in 2 248 samples.  That is a regression to
   chase in its own right: `0x639bc` increments only for a victim whose `byte[+0x00] == 1` (shipped `0x12a24-0x12a33`,
   port `f.c:26234`), and the port's type census does contain type-1 records (770 samples on `ents-port`, 3 148 on
   `g37`), so the records exist and the lethal path that reaches that instruction does not.
2. **No EXP number is ever drawn**, consistent with docs/196 (neither `fn_0383a4` nor `fn_038c23` is called) and this
   run's `commits=0`.

## 3. A framing trap I fell into again while doing this (recorded so it is not repeated)

Reading the record snapshot by hand-computed offsets (`rec[+0x2b]`, `rec[+0x52]`) out of `cells.bin` returned
`+0x2b = 256000` and `+0x52 = 327680000` for *every* sample — which is `0x3e8` (the hp value 1000, `f.c:1306`
writes it at `+0x29`) seen three bytes late, i.e. a framing error of my own, not a port value.  Same family as
docs/124 and docs/167 §19.  Only `scripts/rig/ent_fields.py`, which uses the dump's own `rec_bytes`/`offset`
framing, produced values that are quotable; §1 above is quoted from that tool, §2's cell values from
`scripts/rig/cells_reader.py`.
