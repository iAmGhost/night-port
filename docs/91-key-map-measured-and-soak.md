# 91 — The key map, measured on the original: the host had the arrows crossed, and F1 was never mapped

Goal task `input-map-and-soak` (part 1: the scripted key-mapping check). Part 2 (the ASan soak over all nine
episodes with the arena / RSS growth table) is appended below once the run finishes.

## What was wrong

The host wrote the game's held-arrow bytes from a guess made in docs/60. Measured on the original, the guess
crossed the pairs:

| key | scancode | DS byte the ORIGINAL sets (measured) | port before this change |
|---|---|---|---|
| Up | 0x48 | **DAT_00063ed0** | 0x63ed3 |
| Down | 0x50 | **DAT_00063ed8** | 0x63ed5 |
| Left | 0x4b | **DAT_00063ed3** | 0x63ed0 |
| Right | 0x4d | **DAT_00063ed5** | 0x63ed8 |
| Space | 0x39 | DAT_00063ec1 (a plain key press) | same ✓ |
| Enter | 0x1c | DAT_00063ea4 (confirm) | same ✓ |
| Esc | 0x01 | DAT_00063e89 (abort) | same ✓ (measured in docs/87: `key esc` took 0x63e89 from 0 to 1) |
| **F1** | 0x3b | **DAT_00063ec3** (the level-up "help pages are being shown" mode byte) | **not mapped at all** |

The consumers are all ported at the shipped addresses, so the host assignment was the only wrong part. Read in
the shipped code's own terms, the corrected map says:

- the setup menu walks its entry index with **Up/Down** (`0x00039d2a` reads DAT_63ed0, `0x00039d61` reads
  DAT_63ed8) and changes the selected value with **Left/Right** (`0x00039de2` / `0x00039ebb` read DAT_63ed3 /
  DAT_63ed5);
- the level-up screen's help scroll uses **Left/Right** (`0x000392af` / `0x00039334`);
- F1 arms the byte that the level-up screen tests at `0x0003922a` before its help-page branch, which is what
  makes that screen's `EXIT:ESC HELP:F1` footer real -- in the port the byte had **no writer at all**, so the
  branch was unreachable.

## How it was measured (headless, scripted)

New ops in `scripts/playtest_orig_drive.py`: `key_down <name>` / `key_up <name>` (hold a key across a memory
read -- the plain `key` op releases before the next sample, which is why a naive census showed only zeros) and
`readbytes <DAT> <count>` (one hex-dump line). Drive: `scripts/playtest/probe_orig_keybytes.txt` -- in a live
episode-1 room, for each key: `key_down`, 400 ms, dump DS `0x63e80..0x63ee8`, `key_up`.

Baseline non-zero bytes in that window (not key state): `0x63e80=0x49, 0x63e81=0x0d, 0x63e84=0x80, 0x63e86=0x01`.
While each key is held, **exactly one** further byte appears:

```
KBD_f1     set bytes: [('0x63ec3', 1)]
KBD_up     set bytes: [('0x63ed0', 1)]
KBD_down   set bytes: [('0x63ed8', 1)]
KBD_left   set bytes: [('0x63ed3', 1)]
KBD_right  set bytes: [('0x63ed5', 1)]
KBD_space  set bytes: [('0x63ec1', 1)]
KBD_enter  set bytes: [('0x63ea4', 1)]
```

**The name -> scancode anchor.** The emulator's `KBD_*` names address the standard DOS scancodes, shown by the
game's own scan-code slot array `DAT_00062c58`: dumping the 0x58-byte array while holding each key arms slots
`+0x1c` (Enter), `+0x39` (Space) and `+0x3b` (F1) -- indices 0x1c / 0x39 / 0x3b, i.e. exactly those scan codes.
The arrow slots read 0 while held because the pump (`fn_0347e3`) walks and clears those slots through
`fn_034635`, which the suite test now pins separately.

**Who writes F1's byte.** Ghidra: `DAT_00063ec3` has exactly two xrefs, both READ (`0x0003b3b1` in `fn_03af18`,
`0x0003922a` in `fn_038f3d`) -- its writer is not inside any analyzed function. The only WRITE xrefs anywhere in
the key-byte window (`0x63ea5`, `0x63eb2`, `0x63ec0`) are the three at `0x0003fe06`, inside the unanalyzed
`0x0003fdxx-0x0003fexx` block: the game's installed keyboard handler. So F1 is handled by code the analyzer
never bounded, which is why a static reading missed it and the host guessed wrong.

## The port change

`key_apply` moved out of `main.c` into **`port/src/app/keymap.c` / `keymap.h`**, compiled into both the SDL host
and the test binary, so the map is asserted rather than assumed:

- the four held-arrow assignments corrected to the measured table;
- `SDL_SCANCODE_F1` -> scancode `0x3b` -> `DAT_00063ec3`, and F1 deliberately excluded from the "any key" bytes
  (`DAT_00063eb2` / `0x63ebe`), because measured F1 sets only `0x63ec3`; setting the any-key bytes on F1 would
  leave the level-up screen instead of paging help;
- the any-key pair keeps the host behaviour the shipped key-waits and the setup menu need (docs/60); the
  shipped writer's exact trigger stays unresolved and is recorded as such in the file header.

Makefile: new `SRC_KEY = src/app/keymap.c`, added to both the `test_f1` and `night` targets (`-Isrc` added to the
test target for the shared header).

## The suite check

`test_key_map_measured` in `port/test/test_f1.c` -- for each of the eight keys: the measured DS byte goes to 1,
the scan-code slot at the scancode index is armed, exactly one key byte moves, and everything clears on break;
then F1's exclusivity; then the pump dispatch:

```
key map: measured scancode -> DS key bytes, and the fn_0347e3 dispatch
  [PASS] Up 0x48    -> DAT_00063ed0                     [PASS] Down 0x50  -> DAT_00063ed8
  [PASS]   and the scan-code slot is armed at the scancode index
  [PASS]   exactly one key byte moves for this key      [PASS]   released on break
  ... (same four checks for Left 0x4b, Right 0x4d, Space 0x39, Enter 0x1c, Esc 0x01, F1 0x3b)
  [PASS] F1 arms only the help-page mode byte (measured), so the level-up screen pages help instead of leaving
  [PASS]   the pump walks the arrow slot through fn_034635 (2 -> 3)
  [PASS]   a non-arrow slot stays put (no handler in the switch)
  [PASS]   DAT_00052ff8 == 0 gates the whole pump (0x000347fb-0x000347cf)
```

## Reproduce

```sh
# original side (dosbox-automation up, container running NIGHT in an episode-1 room)
export DOSBOX_API_TOKEN=...
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_keyprobe --drive scripts/playtest/drive_orig_keymap.txt --hz 2      # get into a room
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_keyprobe2 --drive scripts/playtest/probe_orig_keybytes.txt --hz 1   # the hold-and-dump census
# port side
cd port && ./test/test_f1 | grep -A24 "key map:"
```

## Consequence to keep an eye on

Route captures taken before this change drove the port's arrows through the crossed bytes. Comparisons whose
routes used only Enter/Space (boot chain, slides, shows, the credit-less branch, the game-over card) are
unaffected; anything that *walked* now presses the byte the shipped game assigns to that arrow, so port-side
walk routes move the way the original's do. Re-verified guards: see the soak table below plus
`scripts/playtest_ep8_show.py` (the >= 99.5 % episode-8 show floor).

---

# Part 2 -- the long soak found a real crash: DAT_00053eb8 is the entity-type table's +0x1c view

`scripts/playtest_soak.sh` runs `--warp-stage 1..9` under a hard `timeout`. Stage 3 came back **rc=139
(SIGSEGV)** at frame 3720, a few frames after an Enter press inside a live room, with no unwind message --
the process just died. Same build, same route, AddressSanitizer:

```
ERROR: AddressSanitizer: global-buffer-overflow on address ... READ of size 1
    #0 fn_015a1f f.c:8920
    #1 fn_018e88 f.c:15599
    #2 fn_0322f5 f.c:23702
    #3 fn_03a829 f.c:22485
    #4 fn_03af18 f.c:26440        <- the game main
0x... is located 1610 bytes after global variable 'g_dat_53eb8' defined in 'src/f/f.c' of size 8192
SUMMARY: AddressSanitizer: global-buffer-overflow f.c:8920 in fn_015a1f
```

The offending line is the distance check `abs32(...) < g_dat_53eb8[idx]` with

```
idx = (int32_t)(prng % 10) * 10 + entity[0xd] * 0x88      /* no bound check anywhere in the shipped code */
```

and the faulting index is `8192 + 1610 = 9802 = 0x48 * 0x88 + 10`, i.e. **entity type 0x48** -- a type
`fn_037b03` does fill from `night%d.chr`. The port's own comment on the line already flagged the negative-index
case as a latent 1:1 bug; what was missing is that the *positive* side also walked off the port's array.

## What DAT_00053eb8 actually is

Ghidra: `DAT_00053eb8` has ~120 data xrefs, all from the per-type entity handlers (`fn_02c838`, `fn_01f064`,
`fn_01603e`, `fn_020170`, ...). And in DS, `0x53e9c + 0x1c == 0x53eb8` exactly, where `DAT_00053e9c` is the
**entity-type table** -- 0x104 entries of stride 0x88, whose entry layout the port already records as
`(+0, +4, +8, +0x1c 100 B, +0x80/+0x84)`. So `DAT_00053eb8` is not a table of its own: it is the **100-byte
per-entry script-byte block inside each entity-type entry**, addressed `type*0x88 + slot*10 + step`
(`slot, step < 10`), which is exactly how `ai_script_byte` and `fn_015a1f` index it.

The port modelled it as a separate `0x2000`-byte zeroed array, which was wrong in two ways:

1. **Bounds** -- 0x2000 / 0x88 = 60, so every entity of type >= 60 read out of bounds; the crash was type 0x48.
2. **Values** -- the array was always zero, while the shipped game reads `night%d.chr` table data through it.
   `fn_015a1f`'s latch test is `abs(dx) < table[idx]`, so with zeros the proximity latch could never fire for
   any entity: a shipped behaviour the port was silently missing.

## The fix

`g_dat_53eb8` becomes the view itself (`uint8_t *const = g_dat_53e9c + 0x1c`), `FN_015A1F_TBL` becomes the
reachable extent (`0x88*0x104 - 0x1c`), and all seven read sites go through `tbl53eb8_get()`, which clamps to
the real region -- the negative end lands on the table base, exactly as it does in DS -- and counts:
`g_tbl53eb8_max_idx` / `g_tbl53eb8_clamped`, printed in the pump line as `tbl53eb8=`.

After the fix, the stage-3 ASan repro:

```
rc=0   asan_reports=0   memstat stage=3 room=1 frames=18790 arena=4463232/50331648   tbl53eb8=10428/0
```

`tbl53eb8=10428/0` is the measurement that sizes the real table: the shipped reads reach 10428 bytes past the
view (legal DS, illegal in the old 0x2000 array) and **nothing** needed clamping, so the port now reproduces
the shipped reads instead of substituting zeros.

Suite check `test_dat53eb8_alias`: the view equals `g_dat_53e9c + 0x1c`; a byte written into the type table at
`0x48*0x88 + 0x1c + 10` is visible through the view at the soak's index 9802; a wild type index (`0x7fffffff`
in entity+0xd, which the shipped code multiplies by 0x88 unchecked) returns a defined byte and is counted by the
witness; a negative index from the signed `prng % 10` is bounded to the table base.

## Part 2b -- the pump diagnostic was printing labels against the wrong values

While reading the soak logs the pump line showed `vga_bytes=1` next to `sfx=1124889600`: the `SDL_Log` arg list
had drifted out of step with its format string, so `cdstops / sfx / keyb_* / vgaws / vga_bytes / wipe_keys_*`
were each printing another field's value. Re-ordered to match; the same stage-3 run now prints
`vga_bytes=1278003200` (18790 frames x 64000 bytes of A0000 window charge, matching docs/76's model) and
`sfx=<the sound table field>`. Rule of thumb: a diagnostic's numbers are only as good as a label-to-argument
check against an independent expectation (here: frames x 64000).

## New soak witnesses

* `memstat stage=N room=<0|1> frames=F arena=A/CAP rss_max=R KiB` once a second -- `room=` is
  `g_dat_63354 != 0`, so a run can be told to have soaked a live room rather than the title/attract path.
* `scripts/playtest_soak_report.py` -- per stage: arena first/last/peak, RSS first/last/peak, live-room samples,
  AddressSanitizer report count, and how the run ended (returned / unwound / killed by timeout).
