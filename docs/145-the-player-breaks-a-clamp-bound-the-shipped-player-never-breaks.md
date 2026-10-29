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
