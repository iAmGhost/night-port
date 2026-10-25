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
