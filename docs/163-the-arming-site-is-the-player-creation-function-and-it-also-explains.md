# docs/163 — the arming site is the player-creation function, and it also explains the −30 spawn

docs/162 §13 queued "why does the port re-arm `+0x56` a second time".  The write is at guest `0x377a1`, and the
function that owns it is **`fn_0376c8`** — the player-entity creation function (`src/f/f.c:5494`).  Read in
full it answers two queued questions at once.

## 1. What fn_0376c8 actually does

```c
uint32_t blk = fn_00041bdc(0x14f);                                  /* 0x376f9: allocate one entity block */
int32_t tbl = g_tab_52c3c[room[+0x40] + (stage - 1) * 0xa];         /* 0x3770d-29: the room's walk line */
player = fn_010efc(hb + blk, 0xffffffe2, 0, tbl, 0, 0);             /* 0x37733: x = 0xffffffe2, type 0 */
g_player_ent = player;                                              /* 0x3774c */
(player + 0x23) = 0;                                               /* 0x37756: debt cleared */
player[0x28] = g_dat_63384;                                        /* 0x37761-67: the "ship" index */
if (player[0x28] == 0)
    (player + 0x56) = (int16)g_dat_53e9c[0x00];                     /* 0x377a1: the 100 */
else
    (player + 0x56) = g_tab_52738[player[0x28]] & 0xffff;           /* 0x3778b-3779c */
(player + 0x29) = (int16)g_dat_53e9c[0x08];                         /* 0x377ba-c1: hp = 1000 */
```

Two consequences:

* **`+0x56` is not a damage accumulator in the abstract — it is the player's starting shield, and its value
  comes from the per-ship table.**  `g_tab_52738` (`DS 0x52738`, the whole 0x400-byte image constant is present
  in the port at `f.c:5381`-5493) is indexed by `player[+0x28]`, which is copied from `g_dat_63384`; the default
  path reads `g_dat_53e9c[0x00]`, and the dumps show those bytes as `64 00 00 00` → 100 on both sides
  (docs/162 §9).  The table body is visible in the same listing and is mostly `0x3e8` (1000) with `0x19` (25),
  `4`, `10`, `12`, `15` and a run of `9`s — so a different ship index arms a different shield.
* **A re-arm of `+0x56` means `fn_0376c8` ran again, i.e. the player was re-created.**  That is the whole of
  docs/162 §13's "at least twice on the port, once on the shipped side": it is not a stray write, it is a second
  player creation, and the port's own death trace (docs/135: one PLAYER death at tick 7066 on this route) is the
  matching cause.  Every creation resets the debt to zero *and* refills the shield, which is exactly the state in
  which `fn_01441e` refuses to touch hp.

## 2. It also settles the spawn-position question from docs/161 §2

docs/161 measured, at room entry: `shipped px = -30`, `port px = 0`, and ruled out the entry-nudge table.  The
creation call above passes **`0xffffffe2` = −30** as the player's X.  So the shipped spawn X is a constant in the
construction call, and the port's own construction call uses the same constant — the `px = 0` reading was the
port's dump catching the player *after* it had already been walked on, not a different spawn position.  The
comparison in docs/161 §2 has to be taken at the creation event, which is now identifiable from the re-arm
signature in `debt_trace.py` (`acc 0 -> 100`): shipped `t=48093`, port `t=61739`.

## 3. What to measure next

1. **How many times does each side call `fn_0376c8` per route?**  On the port this is directly countable with a
   one-line witness at the top of the function; on the shipped side the same count is readable from the dump as
   `+0x56` re-arms plus `+0x29` jumps back to 1000 (`debt_trace.py`'s event kinds already separate both).
   docs/162 §6 counted 3 player identities shipped vs 2 port by `ent##` proximity — this is the independent
   count that comparison needs.
2. **What is `g_dat_63384` on both sides at creation time?**  It selects the ship index and therefore the shield
   value; if the two sides differ there, the shields differ and every damage comparison in docs/162 is scaled
   wrong.  `g_dat_63384` is a rig cell (docs/133 fixed its width to 1 byte), so this is one census away.
3. **The `player[0x28] != 0` branch**: `g_tab_52738[idx] & 0xffff` masks a dword to 16 bits.  The table holds
   1000s, so the masked shield is 1000 rather than 100 for those indices — a ten-fold difference in how long
   damage is absorbed, and a candidate explanation for routes where the port's debt sits high for a long time.

## 4. §3's item 2 and 3, measured immediately -- and they come back as a negative plus a new open cell

```
ORIG player[+0x28]: 0 x1846            PORT player[+0x28]: 0 x2146, 170 x23
DS 0x53384 (g_dat_63384): ORIG 160 x1846   PORT 0 x2169
DS 0x53380 (g_dat_63380): ORIG 160 x1846   PORT 0 x2169
g_tab_52738[160] in the image (DS 0x529b8): 09 00 00 00  -> 9, not 1000
```

* **The shield branch is equivalent.**  The shipped player's `+0x28` is `0` in every sample, so shipped also
  takes the `+0x56 = g_dat_53e9c[0] = 100` arm, and both sides arm 100.  The `g_tab_52738` "1000 vs 100" worry in
  §3 item 3 is dead for this route, and even if it fired, index 160 holds **9** in the image, not 1000.
* **The port's 23 samples with `player[+0x28] = 170 (0xaa)` are stale-block reads**, the same `0xaa` filler class
  docs/125 §6 and docs/162 §13 already caught, not a ship index.
* **New open item, and it touches an oracle classification.**  `DS 0x53384` and `DS 0x53380` read `160` in every
  shipped sample and `0` in every port sample, yet `fn_03a829` mirrors `pl[0x28]` (which is 0 on both sides) into
  `0x53384`, and `fn_0376c8` reads that cell back.  So either the shipped mirror uses different source offsets
  than the port's transcription, or the port's 1-byte cell (width set from the c9 access-width check in docs/133)
  sits at the wrong offset within the block.  docs/134 §1 filed `0x53384` under `NON-GAMEPLAY: device_plumbing` --
  that classification is now suspect, because the port's own code uses this byte as the player's ship index at
  creation time.  This is the docs/134 §4 "odd sub-dword offsets need struct-layout calibration" family, with a
  concrete member.
