# 142 — The camera: shipped `DAT_000639dc` never exceeds 27, the port reaches 298

Following docs/139's "the port consumes room-script commands the shipped run leaves pending" backwards, the
gates that decide whether a spawn command fires are all written in terms of the **camera** (`DAT_000639dc` =
camera x, `DAT_000639e0` = camera y): the four window tests in `fn_03e20f` compare the record's `rx/ry`
against `camx`, `camx + 0x172`, `camx - 0x32` and `camy + 0xc8`, and the spawn anchor itself is
`camx - 0x1e` or `camx + 0x15e`. So camera position decides spawning, and camera motion decides it over time.

Measured on the same pair of traces (`/tmp/rig/g11` shipped at the emulator's ~3.2 Hz ceiling,
`/tmp/rig/g10` port at 7.2 Hz), signed dwords straight out of the dumps:

```
window 150-260 s
  shipped DAT_000639dc: 611 samples, min 0   max 27,  7 distinct values
  port    DAT_000639dc: 1055 samples, min 0   max 298, 67 distinct values
whole run
  shipped DAT_000639dc: 980 samples, values {0 x590, 27 x179, 24 x174, 12 x30, 26 x3, 25 x2, ...}
  port    DAT_000639dc: 2047 samples, min 0, max 298, 74 distinct values
  DAT_000639e0 (camera y): 0 on both sides for every sample
```

The shipped camera is effectively **static** (it moves by at most 27 pixels and parks at 0/24/27), while the
port's camera scrolls through 74 distinct positions out to 298. Camera y never moves on either side.

## 1. Why this is the head of the chain

Every divergence collected since docs/135 is a consequence of gates that read `camx`:

* docs/137 §4 / docs/138 §1: the extra `enb-1` (type 0x24) spawns -- both enb-1 records sit exactly on a
  camera-relative window edge (`rx = 540 = camx + 0x172`, `rx = 134 = camx - 0x32`), i.e. they only ever pass
  because the port has scrolled the camera to that place;
* docs/139: the port consumes room-script commands (its `DAT_00063e14` walks 67 -> 66 -> 65 -> 64 -> 62 -> 61)
  while the shipped counter never leaves 67 -- consumption happens only when a gate accepts and clears the
  opcode byte;
* docs/139 §3 / docs/137 §5: the port's live-entity population (24% of samples at 6 or more) sits above the
  shipped one (99% at 4 or fewer);
* docs/137 §2: the extra enemies are what resolve hit code 11 against the player, set the dying flag, and
  trigger the CONTINUE countdown (`g_dat_53100` 3 -> 2) and the extra CD track selection (idx12);
* and the long-standing pacing flag (`picture changes per second` orig 4.44 vs port 29.76) is exactly what a
  camera that scrolls 300 pixels while the original's moves 27 would produce on screen.

## 2. The two sides' camera logic

Guest writes to `DAT_000639dc` come from three functions only (`scripts/audit/mem_access.json`):

```
fn_03cad3  0x3cb02 / 0x3cb14   MOV [0x639dc], EAX          (room load / re-point)
fn_03cb38  0x3cb84 ADD [0x639dc], 3 ; 0x3cb98 MOV ; 0x3cbb2 INC   -- THE SCROLL-RIGHT STEP
fn_03cbd9  0x3cc22 SUB [0x639dc], 3 ; 0x3cc30 MOV ; 0x3cc44 DEC   -- THE SCROLL-LEFT STEP
```

and the port's driver of them (f.c:13860-13901, transcribed from the shipped caller) is

```
if (room[+0x36] == 0 || cam + 0x140 <= room[+0x36]) fn_03cb38(DAT_63354);   /* scroll right */
else if (DAT_00053c68 == 1)                          fn_03cb38(DAT_63354);
if (room[+0x32] == 0 || cam >= room[+0x32])          fn_03cbd9(DAT_63354);   /* scroll left  */
else if (DAT_00053c68 == 1)                          fn_03cbd9(DAT_63354);
```

`room[+0x32]` / `room[+0x36]` are the room's scroll limits, and the shipped camera census -- 0, 12, 24, 25,
26, 27 -- is the signature of a room whose scroll limits clamp it almost immediately. The port scrolling to
298 means either those two limit fields are being read from the wrong place in the room record, or the clamp
inside `fn_03cb38` / `fn_03cbd9` (the `MOV [0x639dc], EAX` forms, which are the clamp writes) is being skipped
or computed from the wrong value.

## 3. Queued, in order

1. Dump `room[+0x32]` and `room[+0x36]` -- the room record is `g_dat_63354`, a pointer cell already in the rig
   table, so its record bytes are available on the port side, and the shipped side's `state_*.heap.bin` holds
   the same block: compare the two limit fields directly.
2. Read the clamp logic in the guest `fn_03cb38` / `fn_03cbd9` bodies (capstone, `--entry 0x3cb38` / `0x3cbd9`)
   and compare the clamp bounds against f.c:13793 / f.c:13830.
3. Make `DAT_639dc` a first-class oracle row with its *trace*, not just its value -- a camera that scrolls
   where the original stands still is a whole-family divergence, and every screen/pacing/spawn flag downstream
   of it is expected to close once it does.
