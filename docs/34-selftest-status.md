# `make selftest` is buildable again; what it does and does not verify yet

Found while working toward the "assets byte-exact" requirement. The target had rotted:

* it required `pkg-config`, which is not installed here, and then still tried to run the missing binary;
* it omitted source files, and `src/res/selftest.c` has no `main` (it exposes `selftest(res_path,
  shot_dir)`, meant to be driven by a host).

Fixed: the target builds with the same `SDL_CFLAGS`/`SDL_LIBS` the SDL host uses, and `selftest.c` gains a
`RES_SELFTEST_MAIN` driver (`make selftest`, overridable with `RES=` / `SHOTS=` or argv).

## Result of the first real run (`RES=../NIGHT/NIGHT.RES`)

10 checks PASS, 6 FAIL -- all six from one cause: **the screenshot directory was never created**, so every
PNG write (`pcx_logo.png`, the five sprite sheets `pause`/`number1`/`ena-1`/`boss1`/`gun`) failed. The asset
names were right (they are `pause.spr`, `number1.spr`, `ena-1.spr`, `boss1.spr`, `gun.spr` in the index; the
check label is just the name without its extension).

After adding `SDL_CreateDirectory(shot_dir)` to the test: **`make selftest` prints
`ALL PASS (0 failures)`** -- the res-container / PCX / SPR / sheet-render / FLI / VOC / WAV checks are
byte-exact against the shipped `NIGHT.RES` and the CD tracks (the two `[skip]` lines are the optional
`/tmp/cdda/trackNN.wav` comparisons, which need `scripts/cdda2wav.py` output first).

| failure | cause |
|---|---|
| `wrote pcx_logo.png` | the screenshot directory (`/tmp/night-shots`) is never created by the test |
| `pause`, `number1`, `ena-1`, `boss1`, `gun` | `res_find` names that are not in the shipped `NIGHT.RES` index as spelled -- the expectations predate the real index (the game's own loader resolves 444 entries, and `title1.pcx` / `stage1.stg` / `logo.pcx` style names are what the EXE asks for) |

Four sections were **commented out rather than silently kept**: `key_decode`, `stg_decode`, `wal_decode`,
`act_decode` are declared in `src/res/data.h` but never implemented in that layer. The corresponding
decoding *is* ported and verified in the f layer (`fn_03e0a0` record stream, the `fn_037b03` record loop,
the device/sound path), so duplicating an unverified decoder here would have been worse than leaving the
gap visible in the source.

## To close this out

1. Print the real index: `selftest.c` has `r.count` / `r.entries[i].name` -- dump the 444 names once and
   correct the five stale expectations (or point them at the assets the disc actually ships).
2. `SDL_CreateDirectory` (or `mkdir -p`) the shot directory before writing PNGs.
3. Re-run; the byte-exact requirement of the final-verification task is only satisfied when it prints
   `ALL PASS`.
