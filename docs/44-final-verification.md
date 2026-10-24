# Final verification sweep

## Build + tests

* `make -C port test` → **2792 checks, 2792 PASS, 0 FAIL** at the time of this sweep; re-run later in the
  playtest work and currently **2873 checks, ALL PASS** (the extra checks come from docs/55/57/62 and the
  game-over `.spr` table invariants of docs/61). The same tree under ASan
  (`cc -fsanitize=address -std=c11 -O1 -g -Isrc test/test_f1.c src/f/f.c src/f/fx_04794a.c src/sys/sys.c src/cd/cd_iso.c`,
  i.e. exactly `$(SRC_F) $(SRC_SYS) $(SRC_CD)` from the Makefile -- note the test link needs no SDL3) reports
  `== ALL PASS ==` with **no ASan report** (re-confirmed after the fn_038445 composite fix).
* `make -C port selftest` → **ALL PASS (0 failures)** (re-confirmed).

## Byte-exact asset checks (previously silently skipped)

The selftest default wrote/compared into `/tmp/night-shots`, so its two reference comparisons printed
`[skip]` instead of running. The Makefile target now points at the checked-in references:

```make
selftest: $(T)/selftest
	SHOTS=../scratch/shots ./$(T)/selftest   # byte-exact refs (python FLI frame, CDDA wav) live in scratch
```

With that, the reference comparisons actually execute and pass:

```
[PASS] frame-39 fb matches python reference     # FLI decoder, 64000 B index buffer vs the python model
[PASS] ../scratch/cdda/track02.wav / 44100 Hz stereo 16-bit / 1831893 frames (41.5s)
[PASS] ../scratch/cdda/track10.wav / 44100 Hz stereo 16-bit / 16091957 frames (364.9s)
```

plus the existing exact checks (resource container index, PCX/WAL blob sizes and palette heads, FLI
1168-chunk stream ending exactly at EOF).

## Inventory

`python scripts/status_report.py`: **526 [x] + 2 [-] = 528**, `Next 0 pending`,
`Ordering: OK (no done/skipped function has a pending callee)`. The two `[-]` are the bundled-8087 pair
`0x0461cc` / `0x0473bf`, ruled out with evidence in `docs/36-fpu-engine-0461cc-ruled-out.md`.

## Ghidra naming sweep (last open contract item)

Renamed every remaining auto-named function: 24 in the semantic pass plus 391 in the address pass
(**415 renames**, zero failures).

```
functions whose name still starts with FUN_ : 0
functions with a custom name               : 522   (was 106 before this pass)
```

Names follow the port: semantic names where STATUS documents one (`heap_malloc`, `prng_next`,
`fx_normalize`, `board_out`, …) and `fn_<6 hex>` where the port keeps the address (`fn_03a829`,
`fn_010efc`, …). Address snapshot: `docs/12-functions/ghidra_function_addrs.txt` (522 lines).

Reconciling the 528 tracked rows against Ghidra's 522 functions:

* **8 STATUS rows have no Ghidra function object** — all are thunks / call-site aliases / non-functions:
  `0x041e94` (JMP heap_malloc), `0x042958` (JMP delay_units), `0x041fa1`, `0x0429e5`, `0x044c1e`
  (thunks), `0x042b7d` (the epilogue block of heap_free), `0x042044` (`_entry`), `0x046d00` (data
  mis-disassembled as code). Each is ported or documented in its STATUS row.
* **2 Ghidra functions are not tracked rows** — `0x0493c3`, `0x04a751`: Ghidra sub-blocks inside larger
  canonical functions, the same class as `0x044848`/`0x042438`/`0x04a84e` noted in the 2026-07-08 journal.

## Playable state

Headless run (`SDL_VIDEODRIVER=dummy ./test/night --frames 2100 --autokey`): the attract chain (logo →
opening FLI → title wipe → menu) plays through, `display mode` switches 640x480 (slides) → 320x200
(room), the stage background reproduces `stage1.wal` at 0.973 correlation (docs/41), and the run now ends
only because the harness kills it (`frames=2124`) instead of crashing near frame 600. With
`NIGHT_BOARD=1 NIGHT_SOUND=1 NIGHT_PLAYVOC=1` the shipped `.voc` slot reaches the DMA/DSP model
(`pushed=3607 xfer=1`, docs/43).
