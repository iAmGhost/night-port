# 145 — The player walks through a clamp the shipped player never passes

docs/144 §7 put the chain's head at "the port advances the room, the shipped game sits still". This turn
found a hard number for *why*: the clamp bounds are identical on both sides, and the port's player crosses a
bound the shipped player never crosses.

## 1. The clamp bounds, read out of the clamp-bounds struct on both sides

```
shipped (g11)                     port (g10)
  room[+0x0c] = 200  (976/976)      room[+0x0c] = 200  (1574/1574)
  room[+0x12] = 139  (976/976)      room[+0x12] = 139  (1574/1574)
  room[+0x16] = 195  (976/976)      room[+0x16] = 195  (1574/1574)
  room[+0x1a] = 1600 (976/976)      room[+0x1a] = 1600 (1574/1574)
```

Constant on both sides, byte-identical values. `fn_016a60` / `fn_017637` (f.c:9117 / f.c:9140) are exactly the
functions that consume them:

```c
int32_t g12 = ((struct sa32 *)((g_dat_63354 + 0x12)))->v;   /* 139 */
int32_t g16 = ((struct sa32 *)((g_dat_63354 + 0x16)))->v;   /* 195 */
...
    if (param2 + F >= X)   { *out = 0; return 1; }     /* stop at the target */
    if (param2 + F >= g16) { *out = 0; return 1; }     /* stop at the room's clamp ceiling */
```

so `g16 = 195` is a ceiling on an entity's **camera-relative** x, and `g12 = 139` is the matching floor on the
other branch.

## 2. Measured: the shipped player respects 195, the port's player does not

Player camera-relative x (`player[+0x01] - DAT_000639dc`) over every dump sample:

| | samples | above the 195 ceiling | max | min |
|---|---|---|---|---|
| shipped | 980 | **0** | **191** | -30 |
| port | 2047 | **374 (18.3%)** | **305** | -30 |

The shipped player's furthest reach is 191 -- four pixels short of the 195 ceiling, in 980 samples. The port's
player is past that ceiling in 374 samples and gets to 305, 110 px beyond it.

That closes docs/144 §1's open question quantitatively. The scroll-right precondition is
`player[+1] - cam > 0xbe` = 190, and the clamp ceiling is 195: on the shipped side those two numbers sit five
pixels apart, and the clamp wins (0 samples over 195, 6 samples over 190) -- the player is held inside the
screen and the camera never scrolls. On the port side the clamp loses (374 samples over 195) and the camera
scrolls 298 px.

So the ordering of the chain is now:

1. **the port's player crosses the `room[+0x16] = 195` clamp** (374 samples, out to 305);
2. camera scroll precondition fires 24% of samples instead of 0.6% (docs/144 §1);
3. camera moves 298 px instead of 27 (docs/142), and is then itself stopped by the port-only
   `room[+0x36] = 617` stop bound at exactly `617 - 0x140 = 297` (docs/144 §6);
4. camera-relative spawn gates open for records the shipped run leaves pending (docs/139),
   `DAT_00063e14` walks 67 -> 61;
5. population rises (docs/137: 24% at 6+ vs 99% at <=4), the extra `enb-1` attacker appears,
   hit code 11 lands on a zero-countdown player, the dying flag is set (docs/136/137);
6. CONTINUE countdown runs (`g_dat_53100` 3 -> 2, docs/134) and an extra CD track is requested (docs/135).

## 3. A transcription detail worth checking against the guest

`fn_016a60` as transcribed has a commented latent defect on its right-moving branch (f.c:9130-9131):

```c
        if (param2 + F >= X)  return 1;      /* latent: no *out write */
        if (param2 + F >= g16) return 1;     /* latent: no *out write */
```

while `fn_017637`, the sibling variant, *does* write `*out = 0` in the same two positions (f.c:9152-9153). If
the guest really omits the write in `fn_016a60`, then every caller of `fn_016a60` that tests
`fn_016a60(e, v_spd, &mv_b) != 0` and then uses `mv_b` is reading a value the callee never assigned -- and a
host `mv_b` that happens to hold a stale non-zero step keeps moving the entity even though the clamp said
stop. That is exactly the shape of the divergence measured in §2: the clamp branch is entered, and the entity
moves anyway.

The 22 call sites all look like `if (fn_016a60(e, v_spd, &mv_b) != 0) { ... }` /
`if (fn_0174a2(e, v_spd, &mv_a) != 0) { ... }` pairs (f.c:15845, 16197, 16542, 16888, 17240, 17607, 17959, ...),
i.e. the per-type movement controllers.

## 4. Queued, in order

1. Disassemble `0x016a60` and `0x017637` bodies from `NIGHT.EXE` and confirm whether the guest really omits the
   `*out = 0` store in `fn_016a60`'s two ceiling branches -- the port already documents them as latent, and the
   divergence in §2 is what a latent un-written output looks like at the caller.
2. Instrument one controller call site on the port side: log `F` (camera-relative x), `param2` (the step), the
   returned code, and the `mv_a`/`mv_b` values actually applied, and check whether a *stopped* movement still
   applies a non-zero step. Compare the per-frame applied step against the shipped speed census (60-69 px/s,
   docs/144 §2).
3. Only after (1) and (2): re-run the state oracle and the position census. If the clamp holds, the camera
   census, `DAT_00063e14`, the population census, the hit-code-11 death, the CONTINUE countdown and the extra
   CD selection are all expected to collapse together -- they are one chain, not six bugs.

## 5. Retraction: 195 is the *vertical* clamp, and neither side breaks it -- §2's headline is void

§2 attached `room[+0x16] = 195` to the player's horizontal position. It does not belong to that axis. The two
accessors the clamp functions use are one line apart in the port (f.c:7943-7944):

```c
uint32_t fn_01519f(const uint8_t *p) { return p[+0x01] - g_dat_639dc; }   /* x minus camera x */
uint32_t fn_0151d5(const uint8_t *p) { return p[+0x09] - g_dat_639e0; }   /* y minus camera y */
```

`fn_016a60` and `fn_017637` call `fn_0151d5` -- so their `F` is **vertical** position relative to camera y, and
139 / 195 are a vertical band. The horizontal clamp is `fn_0174a2`, which calls `fn_01519f` and clamps to
`[0, 0x140)` = `[0, 320)`.

Measured against the right axes, from the same player-record dumps:

| | y[+0x09] range | y in [139,195] | y > 195 | y < 139 | px - cam range | px - cam >= 320 |
|---|---|---|---|---|---|---|
| shipped | 0 .. 173 | 976/980 | **0** | 4 | -30 .. 191 | **0** |
| port | 0 .. 177 | 1574/2047 | **0** | 473 | -30 .. 305 | **0** |

* vertical: neither side ever exceeds 195, and both sit overwhelmingly at 139 (shipped 794 samples, port 809).
  The port's 473 samples below 139 are all exactly `y = 0`, which is the count of *not-live* player records
  docs/137 §4 counted in the same run -- they are empty record blocks, not clamp violations.
* horizontal: the clamp is `[0, 320)` and the port's worst case is 305, inside it.

So the "191 vs 195, five pixels apart" argument in §2 was a coincidence between a horizontal measurement and a
vertical bound, and §2's conclusion (and with it §1's framing of the chain's head) does not stand. The
§1 measurements themselves -- the clamp fields being byte-identical on both sides, and the port's player being
110 px further right at 18% of samples -- remain true; what was wrong is the mechanism attached to them.

## 6. What is actually open

Nothing in the clamp path explains the travel difference. The remaining difference in the same dumps is what
the player is *doing*:

```
shipped states: 0x01 x362, 0x02 x240, 0x33 x81, 0x0c x62, 0x0b x59, 0x35 x58   (139 samples in 0x33/0x34/0x35)
port    states: 0x01 x601, 0x02 x283, 0x15 x168, 0x33 x116, 0x00 x109, 0x0b x94 (0 samples in 0x34/0x35)
```

The shipped player spends 139 samples in the `0x33 / 0x34 / 0x35` family (the rope / single-axis family named
at `f.c:20467`) and reaches x = 217; the port's player has no `0x34` or `0x35` samples at all, spends 168 in
the hit-code-11 reaction state `0x15`, and reaches x = 603. The travel difference is a *behaviour* difference,
so the queued question is who moves the player into `0x34` / `0x35` in the shipped game and what holds it
there -- not a clamp.

Rig hygiene, restated: a bound quoted in a differential claim has to be tied to the accessor that reads it,
not to the field that happens to have a convenient value. `room[+0x16]` looked like the answer because 195 sat
just above 191.

## 7. Second retraction: the port *does* enter 0x34 and 0x35 -- §6 was a top-6-truncation artifact

§6 said the port has "no `0x34` or `0x35` samples at all". That came from reading a `most_common(6)` state
census, where those states fell outside the top six. Listing the player's state *transitions* instead, from the
same dumps:

```
shipped (397 transitions in 304 s)          port (342 transitions in 170 s)
  0x01 -> 0x02   x64                          0x33 -> 0x01   x59
  0x33 -> 0x01   x64                          0x01 -> 0x02   x54
  0x01 -> 0x33   x53                          0x01 -> 0x33   x42
  0x02 -> 0x01   x50                          0x02 -> 0x01   x35
  0x01 -> 0x34   x30                          0x01 -> 0x34   x21
  0x34 -> 0x01   x29                          0x02 -> 0x33   x19
  0x01 -> 0x35   x26                          0x34 -> 0x01   x19
  0x35 -> 0x01   x25                          0x35 -> 0x01   x18
  0x0b -> 0x0c   x14                          0x01 -> 0x35   x17
  0x02 -> 0x33   x13                          0x0b -> 0x0c   x13
  0x0c -> 0x01   x7                           0x0c -> 0x0b    x7
  0x02 -> 0x0b    x4                          0x15 -> 0x05    x5
```

Both sides cycle `0x01 <-> 0x33`, `0x01 <-> 0x34` and `0x01 <-> 0x35` with in/out counts that match each other
to within one transition, and both run the `0x0b <-> 0x0c` reaction pair (the states docs/111 restored).
Normalising by run length (shipped 304 s, port 170 s -- the port's counts are 0.56 of the shipped counts while
its raw transition total is 0.86 of the shipped total) there is no missing behaviour family: the state machine
is populated on both sides.

Two things survive from §6 and one new observation replaces it:

* the port's 168 samples in `0x15` (docs/137's hit-code-11 reaction state) and the shipped side's zero `0x15`
  samples remain a real difference -- `0x15` never appears in the shipped transition list at all, and the port's
  `0x15 -> 0x05` transitions are the only path into state `0x05`;
* the shipped side's `0x35` residence (58 samples) versus the port's (46 by the transition tally) is a small
  quantitative difference, not a missing family;
* the surviving headline divergence is the one in docs/144 §1 -- position, not state: player x reaching 603
  against 217, camera reaching 298 against 27, `px - cam > 0xbe` satisfied in 24% of samples against 0.6% --
  with docs/145 §5 having removed the clamp as its explanation.

Rig hygiene, second instance: a census truncated to a top-N is not an absence. State-family claims have to come
from the transition list, which is what docs/137's `ent_list.py` census was for.
