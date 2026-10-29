# docs/152 — the port's player barely *steps*: 602 px from the mover against 70 px/s in the census

docs/149 read the walk census as "the port's step quantum is one size too big". A witness inside the mover
says the opposite, and the two statements are reconciled by a third fact: most of the port's horizontal
displacement never comes from a step at all.

## 1. The mover census (`NIGHT_WALKTRACE`, `fn_01545e` entry)

`port/src/f/f.c` now counts every entry to `fn_01545e` (guest 0x0001545e, the mover that adds `d` to the
camera-x axis field `+0x01`), plus calls into the two chain-driven player movers `fn_034295` / `fn_0342ff`,
and prints a shutdown summary:

```
WALK summary: all_calls=12792 player_calls=3715 sum|d|=602 per_call=0.162
   hist: -5:6 -4:4 -1:196 0:3167 1:337 4:2 5:3 tick=4
WALK movers: fn_034295 calls=0 sum=0 per=0.00 | fn_0342ff calls=0 sum=0 per=0.00
```

Run: `ep1_kill3`, `--warp-stage 1`, `--frames 6000` (≈117 s, ~51 frames/s).

* the mover is entered for the player **3715 times** but with `d == 0` **3167** of them (the shipped code has
  the same early return at 0x1547b-7f, so those are faithful no-ops);
* only **548** calls carry a step, almost all `d = ±1` (337 positive, 196 negative);
* total stepped distance: **602 px over 6000 frames = ~0.10 px/frame ≈ 5 px/s**.

But `walk_rate.py` measures the port's player moving in **±70 px/s** bands (docs/149) on the anchored route.
Those cannot come from the mover: the mover's entire contribution is ~5 px/s. The distance is coming from the
places that **store** the player's x from the camera:

```c
/* f.c:5391, 5512-5515, 26279, 27189-27190 */
((struct sa32 *)((g_player_ent + 1)))->v = (int32_t)g_dat_639dc - 0x1e;    /* 0x3765c-6d */
((struct sa32 *)((g_player_ent + 1)))->v = (int32_t)g_dat_639dc + 0x15e;
/* f.c:26283 */
((struct sa32 *)((P + 0x01)))->v += (g_dat_63398 == 0 ? 0x78 : -0x78);     /* 0x0003d31c-32c: 120 px */
```

An anchor store moves the player by up to 120 px in one instruction. At 4 Hz / 10 Hz sampling those show up as
the 70-80 px/s bands and the 140/230/250 px/s outliers in state `0x15`. **The walk census in docs/149 is
measuring teleport-like anchor stores, not steps.** That is a correction to docs/149's interpretation, not to
its numbers.

The shipped signature is the opposite shape: 60.7 px/s is very nearly `1 px per frame at 60 fps`, i.e. a
per-frame `d = 1` step. The port steps `d = 1` on ~9% of its frames. So the port's defect is a **deficit of
step calls** (548 stepped frames in 6000) coupled with an excess of anchor stores, not a step that is too big.

## 2. Two hypotheses closed by measurement

* **The `.act`-chain movers are not the walk source — on either side.** `fn_03272f`'s arms call the player
  movers only at exact counter values and only when `player[+0x3e] ∈ {0x83, 0x84, 0x85}`
  (`0x32f1e`, `0x32f4a`, `0x32f64`, `0x331ea`, `0x33364`). Measured on `/tmp/rig/g12`,
  `player[+0x3e]` is 0 in **424/424** shipped samples and in **1630/1709** port samples (the 79 exceptions are
  the ASCII/garbage block reads docs/148 §4 already refuses to interpret), and `player[+0x42]` (the mover's
  speed field, set to 2 by `fn_01441e` at 0x14a5f) is 0 in 424/424 and 1630/1709. So `fn_034295` / `fn_0342ff`
  being unreachable in the port **is equivalent behaviour**, not a defect.
* **The wall/trigger table count agrees.** The mover's blocking pre-pass scans `g_tab_5c8bc[0 .. g_dat_62a8c)`;
  `g_dat_62a8c` is 9 in 397/424 shipped samples and 9 in 1058/1709 port samples (the zeros are the
  slide-show phase), and `g_dat_62a90` is 8 on both. "The port has no walls" is not the explanation.

## 3. Axis audit (this one matters for every future record claim)

`fn_01545e` is labelled "THE Y-AXIS MOVE" in `f.h:1168` and `fn_01586b` clamps `a0[9]`, but the shipped uses
show the axes the other way round for the camera:

* `fn_03d4e4` compares `player[+0x01]` against **camera x** `DAT_000639dc` (`0x3d512-25`, `0x3d585-98`) and
  `player[+0x09]` against **camera y** `DAT_000639e0`;
* `fn_01545e` moves `+0x01` and clamps it against camera-**x**-relative windows (`cam+0xf..cam+305`,
  `cam-0x100? no: cam-0x64..cam+400`, `cam+0xa..cam+302`), gating its wall bands on the `+0x09` field;
* `fn_01586b` is the mirror: it moves `+0x09`, clamps it against `*(p+0x12) + DAT_000639e0` (camera y), and
  gates its wall bands on the `+0x01` field.

So `+0x01` is the axis the camera-x policy follows and `+0x09` is the camera-y axis; the "Y-AXIS" label on
`fn_01545e` is a naming error in the port's own comment and should not be trusted when reading record claims.
The code is right; the prose was wrong.

## 4. The next probe (narrower than anything before it)

Count, per frame, whether the player is in the walking state `0x02` and whether the walk arm that supplies
`d = 1` (`0x12af0: mov edx, 1` and the `mv_a != 0` arms of `fn_01194c`, which owns 25 of the 26 mover call
sites) is reached. The shipped side's reference number is now a single scalar: about one `d = 1` step per
frame while state `0x02` holds (60.7 px/s at ~60 fps); the port's is 548 stepped frames in 6000. If the port's
`0x02` frames are *reached* but the `d = 1` arm is skipped, the defect is in the gate that feeds it; if the
port spends fewer frames in `0x02`, the defect is upstream in the chain that selects the state (docs/110's
`.act` chain question), and the camera divergence in docs/142/148 is downstream of that.

## 5. Reproduce

```sh
make -C port app
NIGHT_WALKTRACE=1 ./port/test/night --res-dir ./NIGHT --script scripts/rig/drives/ep1_kill3.txt \
    --warp-stage 1 --frames 6000 2>&1 | grep '^WALK'
python3 scripts/rig/walk_rate.py --orig /tmp/rig/g12/orig --port /tmp/rig/g12/port   # docs/149
uv run --with capstone scripts/audit/call_sites.py --target 0x1545e --before 6       # 26 callers, feeds of d
uv run --with capstone scripts/audit/disasm.py --entry 0x34295 --end 0x34353         # the chain movers
```
## 4b. The probe ran: 0.155 steps per player dispatch

The two dispatch counters (`g_step_player_01194c`, `g_step_player_012616`, both printed by
`NIGHT_WALKTRACE`) give the denominator the question needed, on the same 6000-frame run:

```
WALK steps: fn_01194c_player=3253 fn_012616_player=3253 | stepped mover calls=3610
```

* the player's step dispatch runs **3253** times in 6000 frames (the other ~45% is the slide-show/intro, where
  `DAT_00053c68 == 0`), and `fn_01194c` and `fn_012616` run exactly once per dispatch each — so the dispatch
  path is intact and not being skipped;
* nonzero mover steps are **504**, i.e. **0.155 steps per dispatch**. Against the shipped reference of ~1 step
  per walking frame, the port moves the player on roughly one in seven of its player frames.

The state shares explain why, and they diverge (same route `/tmp/rig/g12`):

| player state | shipped share | port share |
|---|---|---|
| `0x02` (walk) | 123 / 424 = **29%** | 164 / 1709 = **9.6%** |
| `0x01` | 180 / 424 = 42% | 466 / 1709 = 27% |
| `0x00` | 27 / 424 = 6% | 599 / 1709 = 35% (mostly the 544 slide-show samples) |
| `0x15` (hit code 11) | **absent** | 215 / 1709 = 12.6% |

So the port spends a third of the shipped time in the walking state and yet still travels further, because
distance is dominated by the anchor stores of §1. Two follow-ups are now measurable rather than speculative:
why the port leaves state `0x02` so early (the chain that selects it — docs/110), and why the player is put
into `0x15` (hit code 11) 215 times when the shipped run of the same route never puts the player there at all.

One more good piece of news, and one rig bug caught while measuring it. On the fixed build the type census of
the 12 entity slots is `ORIG {0: 4644, 1: 413, 2: 31}` and `PORT {0: 16271, 1: 3981, 2: 256}` — normalised,
0.97 vs 2.33 type-1 enemies per sample, which is docs/142's population divergence, but **type `0x24`
(`enb-1.spr`), the attacker docs/137 blamed for the code-11 hit, no longer appears on either side**: docs/147's
trip-count fix removed the extra population as well. The rig bug: the first version of that census reused one
`collections.Counter` for both sides and printed identical numbers (`16271, 3981, 256` twice); the "both sides
agree" reading was an artifact of the script, caught by the implausibility of exact equality, and fixed before
the numbers above were quoted.
