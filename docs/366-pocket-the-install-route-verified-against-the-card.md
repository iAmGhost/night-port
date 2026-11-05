# 366 — the install route, verified against the card: what `make copy` fills, what `make deploy` fills, and what a card must match

Task-10's packaging half.  `pocket/INSTALL.md` is now the human-facing install README; this is the measurement that
says it is true.

## 1. The two copy steps are not the same step

| command | what it actually writes |
|---|---|
| `make -C pocket/src/night copy` | the SDK's `platforms/pocket/copy.sh`: **core tree only** (`Cores/`, `Assets/` core files, `Platforms/`).  It has no notion of the ~600 MB of game data, and it writes macOS AppleDouble sidecars (`._night.elf`, `._os.bin`) on the way. |
| `make -C pocket/src/night deploy` | `scripts/pocket/deploy.sh`: the core tree **and** the slot files, with a md5/size manifest so a core-only re-deploy does not re-push 600 MB of audio that never changed, plus the save-window placeholders handled so they do not generate sidecars, and a final sweep that strips any `._` files older deploys left. |

Consequence learned the hard way this session: run `deploy` **last**.  A `copy` after a `deploy` put `._night.elf` and
`._os.bin` back on the card; they were removed by hand and the rule is now written into `pocket/INSTALL.md`.

`deploy` also overwrites `night.ini` with the shipping defaults, so a diagnostic cycle has to re-set `LOG`/`DIAG`
after every deploy (docs/362 already said this; it bit again when `make copy` reset `LOG=1` to `LOG=0`).

## 2. Card-vs-README check, run against the deployed card

```
README lists 15 data-slot files (slots [4, 5, 6, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31])
missing on card      : []
card extras          : ['night.elf', 'os.bin']            (core files, not data slots -- expected)
names over 23 chars  : []
data.json declares 28 slots, ids 0..31, count<=32: True
save range 10-19 reserved for saves: [10, 11, 12, 13, 14, 15, 16, 17]
```

`missing in build tree` is `NIGHT.RES` + the 14 track files, and that is the design: the build tree
(`pocket/build/pocket/night/`) holds the core, and the disc data lives in the regenerable `scratch/pocket/` that
`deploy` copies from.  A build artifact must not be 600 MB.

`scripts/pocket/check_slots.py` is the durable version of this check and it passes, including the line that matters
most after the docs/339 fault (audio tracks sitting in the 256 KB save window, where every read past 262 144 bytes
came back short):

```
[PASS] disc_table.h slot ids and NIGHT.json bindings agree for all 14 audio tracks (ids 5..31, none in the save range 10-19)
[PASS] track09: 23973 sectors, LBA 161147..185119 byte-identical to IMG at 4 sampled boundaries
...
== ALL PASS ==
```

## 3. Version identity on the card, checked rather than assumed

```
core.json                 "version": "0.1.54"
strings night.elf | grep ^0.1.   -> 0.1.54
md5 night.elf (card)   a4c88a1fd49b098f62b3b36b5c73dae5
md5 app.elf (build)    a4c88a1fd49b098f62b3b36b5c73dae5
```

This check is not ceremony: docs/346 records reading a 0.1.35 card log as a 0.1.34 one because a version bump lived
only in a generated header.  It also caught a real ordering trap this session -- bumping `VERSION` in `mk_dist.py`
and then running `deploy` put `core.json` 0.1.54 over an ELF that still said 0.1.53, which is exactly the mismatch
the check exists to catch.  The fix is to rebuild after the bump (`make` in `pocket/src/night`, whose `-MMD -MP`
dependencies make the regenerated `of_interact_idx.h` force the recompile) and then deploy.

## 4. What task-10 still needs

The contract's second half -- "user hardware run report folded back into docs" -- needs a boot.  The card is
prepared for it: core 0.1.54 (the CDDA pump tick-lock plus the budget instruments, docs/364), `night.ini` with
`LOG=1 DIAG=0`, no `._` sidecars, all 15 slot files present.  What the next boot is asked to produce, in priority
order:

1. the `budget` lines, which are the only source of the device's own `disc reads=… mean/max us` and of `dry` /
   `audio_low` on hardware (docs/364 §5 shows why this machine cannot measure them from the mounted card);
2. one `FB_MODE=0` boot against one `FB_MODE=1` boot, comparing the `stages`/`perf` lines, to close the 640×480
   present-cost question on real hardware (docs/343);
3. one `SCREEN=1` boot, which is task-12a's contract line -- Terminal text visible on the Pocket LCD during a run.

## Bump-and-deploy is verified on the binary, not on core.json (0.1.68)

Version lives in exactly one place in the code -- `OF_CORE_VERSION` in the generated `pocket/src/night/of_interact_idx.h`,
written by `mk_dist.py` from its `VERSION` -- and `$(OBJ_DIR)/app.elf: | dist` orders the regeneration before the link.
On the 0.1.68 bump that ordering did not hold in practice for the first `make deploy`: the card came back with
`core.json` saying `0.1.68` while the ELF still carried the `0.1.67` string:

```
$ strings /Volumes/Pocket/Assets/night/common/night.elf | grep -E '^0\.1\.'
0.1.67
$ grep -o '"version": *"[^"]*"' /Volumes/Pocket/Cores/iamghost.night/core.json
"version": "0.1.68"
```

`make dist` and then `make deploy` produced a consistent pair (ELF string `0.1.68`, card and build ELF md5
`8b7d01d3d4039e48c5a82ae362bea56b` identical).  The rule is therefore the check that caught it, and it belongs in the
install route rather than in a Makefile promise:

```sh
make -C pocket/src/night test          # builds the desktop shim AND the RISC-V ELF: the shim alone is not evidence
make -C pocket/src/night dist && make -C pocket/src/night deploy
strings /Volumes/Pocket/Assets/night/common/night.elf | grep -E '^0\.1\.'    # must equal core.json's version
md5 -q /Volumes/Pocket/Assets/night/common/night.elf pocket/build/pocket/night/Assets/night/common/night.elf
```

`deploy.sh` skips slot files whose bytes already match (all 14 tracks reported "unchanged -- skipped"), so a
version-only redeploy costs seconds, not the 600 MB.
