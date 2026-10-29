# 144 — Position census: the shipped player stays in one screen, the port's walks across the level

docs/142 established the camera divergence but left its cause open. This turn measured the two things the
camera logic consumes -- the player's own position, and the block the camera functions are handed -- plus a
speed census. All numbers are from existing routes (`/tmp/rig/g11` shipped, `/tmp/rig/g10` port) plus two
witness runs.

## 1. Player x and camera x, per 20 s window

```
shipped (g11)                                    port (g10)
  40- 60s  px max=  0  cam max=  0                0- 20s  px max=  0  cam max=  0
  60- 80s  px max= 90  cam max=  0               60- 80s  px max= 90  cam max=  0
 160-180s  px max=154  cam max=  0               80-100s  px max= 98  cam max=  0
 180-200s  px max=154  cam max=  0              140-160s  px max=256  cam max= 66
 200-220s  px max=135  cam max=  0              160-180s  px max=311  cam max=121
 220-240s  px max=154  cam max=  0              180-200s  px max=442  cam max=252
 240-260s  px max=217  cam max= 27              200-220s  px max=603  cam max=298
 260-300s  px max=217  cam max= 27              220-240s  px max=603  cam max=298
```

Whole-run signed ranges: shipped player x `-30 .. 217` (camera `0 .. 27`); port player x `-30 .. 603` (camera
`0 .. 298`). Both start at the same place (the `camx - 0x1e` spawn anchor = -30) -- the shipped player never
gets past x=217, about two-thirds of one 0x140-pixel screen; the port's travels 603, roughly two screens.

The gate that lets the camera scroll right is `player[+1] - cam > 0xbe` (190):

| | samples | px-cam > 0xbe | px-cam < 0x82 | mean px-cam |
|---|---|---|---|---|
| shipped | 976 | **6** (0.6%) | 440 (45%) | 109.6 |
| port | 1574 | **378** (24%) | 548 (35%) | 157.5 |

The shipped run almost never satisfies the scroll-right precondition; the port satisfies it a quarter of the
time. docs/142's camera divergence is therefore downstream of the **player** travelling further, not of a
different camera rule: the shipped player is confined to the first screen and the camera parks at 27.

## 2. Speed census (cadence-normalised, per-transition dx/dt)

```
shipped: 979 transitions, |v| > 1 px/s in 328 (33.5%); speed histogram peaks at 60-69 px/s (153 samples)
port   : 2046 transitions, |v| > 1 px/s in 426 (20.8%); speed histogram peaks at 70-79 px/s (146 samples)
```

The shipped side's dominant walking speed is 60-69 px/s, the port's 70-79 px/s, and the port has a 80-89 band
(20 samples) the shipped run has none of. So the port's player also moves *faster*, not just further --
consistent with the long-standing pacing flag (4.44 vs 29.76 picture changes/s) and with docs/137's finding
that the fixed-rate countdown clocks agree (43.3 vs 42.7 units/s): the logic clock is fine, the per-frame
displacement is not.

## 3. What the camera is actually handed: `fn_03d4e4(room)` gets `g_dat_63354`

`f.c:22876` is `uint8_t *room = g_dat_63354;` and the camera policy call is `fn_03d4e4(room)` (f.c:22941),
so the camera guard's `room[+0x2a] / +0x2e / +0x32 / +0x36` **are** fields of the same block the clamp
functions receive. docs/143's worry that the camera reads a different object is resolved for the camera path:
it is the same block, the one the port's own header calls the clamp-bounds struct.

Live witness (`NIGHT_ROOMHEX` / extended `NIGHT_SPAWNTRACE`), printed from the running pointer:

```
ROOM off=0018d6ec b00=0 d00=0 b08@1=64 d08=1600 d11=35584 d32=0 d36=0 d40=0
```

and the dump's record block for the same cell, first 0x48 bytes (port, /tmp/rig/sw2):

```
00000000 00000000 40060000 c8000000 00008b00 0000c300 00004006 0000018c
05fe7104 09a50100 00000000 x7  004cda18 00
```

The block is consistent between the live read and the dump: `dword[+0x08] = 1600` with `byte[+0x08] = 64` (the
low byte of 0x640) and `dword[+0x40] = 0`, and the tail `0x18da4c` is a heap offset, matching the double-deref
clamp fields `f.h:1146` describes. What is **not** consistent is offset `+0x11`: the byte read there (the value
`fn_03e20f`'s spawn arm branches on) prints 0 live, while a dword at the same address reads 0x8B40, whose low
byte is 0x40. A byte and a dword cannot disagree at one address, so one of the two prints is reading a stale or
rebased pointer -- most likely `fn_03e20f`'s room *parameter* versus the global `g_dat_63354`, since `fn_03e20f`
takes the room as a parameter from `fn_03ed6f` while `fn_03cb38`/`fn_03cbd9`/`fn_03d4e4` use the global.
docs/143 §5's identity caveat therefore stands for the *spawn* path and is settled for the *camera* path.

## 4. Consequence for the chain

The chain assembled since docs/135 now has a cleaner head:

1. the port's player travels 603 px where the shipped player never passes 217, at 70-79 px/s against the
   shipped 60-69 px/s;
2. because it travels further, it satisfies `px - cam > 0xbe` in 24% of samples against 0.6%, so the camera
   scrolls 298 px instead of 27;
3. because the camera scrolls, the camera-relative spawn gates in `fn_03e20f` open for records the shipped run
   leaves pending (docs/139's `DAT_00063e14` 67 -> 61 walk, the two `enb-1` records sitting exactly on
   `camx+0x172` and `camx-0x32`);
4. the extra population (docs/137: 24% of port samples at 6+ live entities vs shipped 99% at <=4) is what
   resolves hit code 11 against the player, sets the dying flag, and cascades into the CONTINUE countdown and
   the extra CD selection (docs/135).

So the item to fix is at the top of that list: the player's per-frame displacement. Everything else collected
so far is downstream of it.

## 5. Queued

1. Census the player's per-frame x step in the *idle-walk* case only (state 2, one direction held) on both
   sides: the shipped side's per-frame dx against the port's, cadence-independent, to turn "70-79 vs 60-69 px/s"
   into a per-frame step comparison and locate the speed table or scaling factor.
2. Read the guest's player-walk step: the sites that write `player[+0x01]` and the speed source
   (`fn_017637`/`fn_0174a2`/`fn_01519f` family, and the `0x140`-relative clamp), then diff against f.c's copies.
3. Finish the identity check on the spawn path: print `fn_03ed6f`'s room *parameter* pointer alongside the
   global, so the byte-vs-dword disagreement at `+0x11` is attributed to a specific object.
