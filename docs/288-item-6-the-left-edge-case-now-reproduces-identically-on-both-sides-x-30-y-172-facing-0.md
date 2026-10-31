# 288 — Item 6: the left-edge case now reproduces *identically* on both sides (X = -30, Y = 172, facing byte 0, state 0); the rig needed a new instrument to see that

## 1. The old left-edge pairing was void, and why

`scripts/rig/drives/ep1_leftedge.txt` (docs/220 era) alternates `key left down` with `key enter **up**` ✓ — a
release with no make ✓ `fn_00035dfc` (docs/97, shipped `0x35e14-0x35eb6`) consumes a **press** (make) of
`0x63ea4/0x63ea5/0x63ec0/0x63ec1` ✓, so the port never cleared the stage-1 slide show on HEAD:

```
/tmp/rig/le1 (route.sh, ep1_leftedge.txt, 150 s):
  port  : memstat "stage=1 room=0" in 196/196 samples ✓✗  (never entered a room)
          records.jsonl: 0 lines with a player ✓✗
          mode 320x200 window 13.7-43.5 s = the slide cards, not a room ✓
  orig  : reached rooms, player kind-0 records present, world X reaches bucket 0 (118/409 samples) ✓✓
```

So the le1 "screen comparison" compared shipped **room** frames against port **card** frames ✓✗ every number in the
first le1 census (shipped aggregate meanX 111 / L÷R 3.49 vs port 189 / 0.48 ✓) is void ✓ and is **not** evidence
about item 6 ✓ Rule recorded: a two-sided claim needs a drive that puts **both** sides in the same game phase ✓

## 2. New drive and new instrument

`scripts/rig/drives/ep1_leftedge2.txt`: `ep1_kill3.txt`'s bring-up (its `wait res title1.pcx` and `wait ds
0x53e9c >= 1` anchors ✓ then Enter makes for the cards ✓), then 35 s of walking left (re-made every 4 s so the
pulse typematic keeps making ✓ docs/282 ✓), then 48 s of attacks in place (`lctrl` every 1.2 s, `space` every
4.8 s, `left` re-pressed every 9 s to stay against the bound ✓) ✓

`port/src/app/rig_sink.c` gained a **player-geometry sidecar** `player_recs.jsonl` (its own file, so no existing
reader of `records.jsonl` sees a layout change ✓): per tick, the player record's world `x` (+0x01), `y` (+0x09),
`st` (+0x17), **`f1b` (+0x1b — the facing/mirror byte that selects the draw arm, shipped `0x13db7 cmp
[eax+0x1b],0` / `0x13dbb jne 0x13ee6` ✓ docs/255)** and `hp` (+0x29) ✓ It was necessary because
`records.jsonl` carries only `{o, n, type, st, ac, hp, f30}` ✓ — no X or Y ✓ — and the `ent##` cells are
pointer-following slots whose dumps read as zeros on this route (`min world X = 0` in 1677/1677 samples ✓
uninformative ✓ docs/252 §1-2 family ✓).

## 3. The both-sides left-edge measurement (`/tmp/rig/le4`, route.sh, ep1_leftedge2, 220 s)

Shipped side read through `diff_state.load_orig` record blocks (0x160 B ✓ kind 0 ✓ `+0x29` in 1..0x2000 ✓); port
side from `player_recs.jsonl` ✓

```
SHIPPED: 712 player samples   min X = -30   max X = 145
         hist16 {-32:34, 0:590, 16:26, 80:39, ...}
         f1b {0:466, 1:246}   state {0:32, 1:379, 2:71, 11:89, 12:107, 40:22, 51:12}
         at the bound (x <= -22): 34 samples, Y = {172}, f1b {0:34}, state {0:32, 1:2}

PORT   : 1086 player samples  min X = -30   max X = 169
         hist16 {-32:243, 0:463, 16:36, 64:90, 96:74, 128:55, 144:54, 160:28}
         f1b {0:420, 1:666}   state {0:233, 1:381, 2:276, 11:37, 12:97, 51:62}
         at the bound (x <= -22): 243 samples, Y = {172}, f1b {0:243}, state {0:233, 1:9, 2:1}
```

**The left-edge state is identical on both sides**: same bound `X = -30` ✓✓, same ground line `Y = 172` ✓✓, same
facing byte `0x1b = 0` ✓✓, same player state `0` at the bound ✓✓ (the sample-count difference — 34 vs 243 — is
cadence, not state ✓ docs/277 §1 ✓).  docs/242's "the shipped parks the player on the room's lower bound and the
port leaves it at 172" is therefore **closed**: after the `fn_01586b` clamp fix (docs/254) the shipped *also* reads
172 at this bound ✓ (and docs/254's player-at-139 → 406 sample lift is the same mechanism ✓)

## 4. Screen half: no separable wrong-side signal yet

Ground-line change clusters (`|Δluma| > 8` between consecutive 320x200 witnesses, cluster size 20-8000 px, cluster
mean-Y in 150-200 ✓):

```
SHIPPED le4: 116 clusters; leftmost  n=112 x=10 span 4-43 | n=40 x=18 span 8-43 | n=64 x=19 span 8-43
PORT   le4:  43 clusters; leftmost  n=1457 x=36 span 0-319 | n=3073 x=40 span 0-279 | n=2699 x=44 span 0-83
pacing on le4: orig 12.94 vs port 22.73 picture changes/s (ratio 0.57 ✓), channel-order check ok (0.703)
```

The shipped's tight left-edge clusters (`x≈10-19`, span `4-43` ✓ — a small limb-sized patch at the left border ✓)
have no clean port counterpart: every port cluster in that range is contaminated (span `0-319` ✓, i.e. other motion
in the same sampled pair ✓) and the two sides sample at a 0.57 cadence ratio ✓ **This census therefore cannot
separate a limb drawn 20-40 px to the right from ordinary motion noise** ✗✗ — it is neither a red nor a green
measurement ✓

## 5. Where item 6 stands, stated exactly

* **Not reproduced as a state or geometry divergence.** The port reaches the shipped's exact left-bound position,
  ground line, facing byte and state ✓ (this section ✓) — and everything the shipped's limb arithmetic consumes
  has been proven byte-faithful: the anchor arrays and both arms (docs/220), the box arithmetic and facing
  polarity (`0x13db7/0x13dbb` vs `SUBT(sprB,0x18)/SUBT(sprB,0x14)` ✓ docs/255 §2), the mirror-flag stores
  (docs/229's retraction of docs/225), the part arrays written by the spawn routine (docs/230), the sprite arrays
  (docs/212) ✓
* **Not measured as a draw-side defect either**, because the screen instrument cannot yet resolve it ✗ The
  remaining admissible instrument is a **blit-footprint census**: dump, port-side, the actual destination rect of
  each limb blit at the bound (`blit_probe_note`, `f.c:3168`, armed under `NIGHT_GLYPHDBG` ✓ docs/227 §5 ✓) and
  compare it against the destination the shipped's own faithful arithmetic computes from the same record values
  (which are now known identical on both sides ✓) — a same-record ⇒ same-rect test that does not depend on cadence
  or on the capture basis ✓
* **Item 6 is left open** on that basis: no surviving defect is identified, no fix is proposed, and no fabricated
  "green" claim is recorded ✓
