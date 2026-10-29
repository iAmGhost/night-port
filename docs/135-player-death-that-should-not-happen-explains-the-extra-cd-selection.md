# 135 — A player "death" at hp=992 explains the CONTINUE-countdown DIFF *and* the extra CD selection

## 1. Every shipped call site of fn_010b1d (select + start a CD-DA track), mapped

New audit tool `scripts/audit/call_sites.py` (`uv run --with capstone ... --target 0x10b1d`) walks every
caller in functions.json, disassembles the body from NIGHT.EXE, and prints the instructions feeding the call.

| call site | function | argument source |
|---|---|---|
| 0x10db9 | fn_010d47 (periodic CD refresh) | `dword[0x2da4 + 4*room[+0x40] + 0x28*(stage-1)]` (room track) |
| 0x39e8c | fn_039c03 | same table read |
| 0x3a878 | fn_03a829 (room controller) | same table read |
| 0x3b089 | fn_03af18 (game main) | `mov eax, 0xb` — **gated** by `cmp dword[eax*4 + 0x2f2c], 0` (slide count for the stage) |
| 0x3a442 | fn_03a224 | `mov eax, 9` |
| 0x38637, 0x38877 | fn_038445 (CONTINUE countdown screen) | `mov eax, 0xc` (second one behind the `idiv 0x1ea` / `cmp [0x3c48]` refresh test) |
| 0x391fb, 0x39a4c | fn_038f3d (status screen) | `mov eax, 0xb` |
| 0x38b02 | fn_038ae0 (boot step 2) | `mov eax, 1` |

The per-room table `g_tab_52da4` is byte-identical to the image (0 mismatches over its 100 dwords: 2,2,2,8,2,
… 3,3,3,8,3 … 9×10 … 22 once), and the per-stage slide table `g_tab_52f2c` matches too ({0x16,0x3,0x2,0x6,0,0,
0x3,0,0x1,0x3}). So the *data* the selection reads is faithful on the port side.

## 2. Selection residence times on the g9 route (DS 0x53c44 = selected start LBA)

```
SHIPPED (samples 47.2 s .. 283.6 s)      PORT (samples 0.0 s .. 263.3 s)
  idx9  47.2s  held   0.75 s               (none)  0.0s  held  3.6 s
  idx11 48.3s  held  23.72 s               idx1    3.7s  held 43.6 s
  idx2  72.4s  held 211.20 s               idx9   47.3s  held  2.8 s
                                           idx11  60.0s  held 10.4 s
                                           idx2   70.6s  held 165.8 s
                                           idx12 236.5s  held  2.5 s   <-- shipped shows no change here
                                           idx2  239.1s  held 24.2 s
```

Same order, same tracks, ~2 s phase offset (docs/131). The port's **idx12 selection (LBA 0x3ad6a = 241002) at
236.5 s is a real divergence**: the shipped sample window covers 47–283 s and holds idx2 through it. idx12 is
reachable in the shipped code only from `mov eax, 0xc` inside **fn_038445**, the CONTINUE countdown screen.

## 3. Correction to docs/134 §6: the shipped guard IS reproduced

docs/134 §6 suspected the port called fn_038445 without the shipped guard. It does not. Shipped dispatch at
0x3abae-0x3ac06 is `cmp [0x1336c],1` -> clear it -> `if ([0x53100]==0) {3, ret 0}` -> else `call 0x38445`,
with the other arms `==3` (ret 1), `==2` (exit rect), `==4` (quota). The port writes the same dispatch as
`if (g_dat_6336c == 1) break;` inside a `for (;;)` room loop whose only other exits are `return`s, and the
credits block sits after the loop. Control flow is equivalent; fn_038445 has exactly one call site on both
sides. So the port reached fn_038445 legitimately -- **which means the port's player entity died.**

## 4. The actual event: a new witness shows a player death at hp = 992

Added `NIGHT_DEATHTRACE=1` at the shipped death site (f.c, guest 0x000129c4 `if (e[0x30] == 1)`), printing
tick, heap offset, type, state, hp, exp and a PLAYER tag. Port run, ep1_kill3, 16000 frames: 148 deaths,

```
   146 x type 2   (the spawned one-shot copies made by fn_011827 / the 0x1164b three-way spawn: both set
                   entity+0x30 = 1 on the new block, so they die on purpose)
     1 x type 1
     1 x tick=7066 off=0x3a75d8 type=0 state=0x1 hp=992 exp=5 PLAYER      <-- the divergence
```

The player is flagged as dying while holding **992 of its 1000 hit points**. Everything downstream follows
from that one event: `g_dat_6336c = 1` (player-died request) -> room loop break -> credits block ->
fn_038445 runs -> `g_dat_53100` decrements 3 -> 2 (docs/134's DIFF cell 0x53100) and fn_038445's refresh arm
plays track idx12 (the extra 0x53c44 run above, docs/131's "idx12 blip at 234 s"). One spurious death, two
independently-observed divergences.

## 5. Where the flag can come from (measured, not guessed)

Entity byte +0x30 is the "this entity dies at the end of its step" flag. In the port the only writers are
`fn_011827` (f.c:13917, guest 0x01191a, spawns one copy) and the three-way spawn at f.c:14767-69 (guest
0x1164b-0x11675); the only clears are `g_player_ent[0x30] = 0` (guest 0x374ca) and `P[0x30] = 0` inside
fn_038f3d (guest 0x395e9). Both spawn sites write the flag onto the **newly allocated** block, and the dying
player here is type 0 while every spawn from those sites is type 2 -- so the flag did not come from a spawn
writing the player's own field. It came either from a stale/dangling node in the entity list whose base is not
a record start (docs/128's second fault: a read through a pointer outside the flat heap) or from
`g_player_ent` itself pointing at a block that has been freed and reused: the dying "player" sits at heap
offset 0x3a75d8 while 146 spawned blocks from the same window sit at 0x3a772c, 0x154 bytes away -- adjacent
slots in the same arena region.

That distinction is exactly what the arena sanitizer (docs/128 open item 2: poison freed blocks with a magic
word and check it at dereference sites) resolves, because AddressSanitizer cannot see lifetime errors inside
the port's own heap arena. It is the next measurement.

## 6. Consequence for the music cluster

docs/131's remaining music question ("why does the port change track where the original does not") no longer
needs a CD-layer explanation for this instance: the selection cells, the track table, the per-stage table, the
track count (15 on both sides) and the sound gate (DS 0x51a10 = 1 on both sides) all measure equal, and the
extra change is a downstream consequence of a spurious player death. The music oracle stays as the detector;
the bug it caught is a lifetime/combat bug, not a CD bug.
