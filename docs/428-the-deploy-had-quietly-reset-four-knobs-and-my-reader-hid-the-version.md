# 428 — the deploy had quietly reset four knobs, and my log reader hid the version I said was missing

Preventing a boot that could tell me nothing beats waiting for it. Between the `0.2.5` deploy and the user's
boot, two of my own claims about the card log turned out to be about my tooling, not the build.

## 1. `night.ini` was back at the release defaults, so the next boot would have logged nothing

After `make deploy` for `0.2.5`, the card read:

```
DIAG=0   LOG=0   DISC_IDLE_HOOK=0   CDDA_STOP_FIX=1
```

Four knobs, all reverted:

- `LOG=0` — **no log at all.** I had been asking for a boot to read `split` lines from.
- `DIAG=0` — the diag block that emits the `budget` / `split` lines would not run.
- `DISC_IDLE_HOOK=0` — the hook counter would have read zero again, this time because I turned it off, and I
  would have written another paragraph about the OS not calling it.
- `CDDA_STOP_FIX=1` — the value I had measured as deleting 2.50 s + 0.26 s of music (docs/421), back on
  because that is the shipped default.

This is not new information — docs/362 already records that `deploy.sh` regenerates `night.ini` from the dist
tree and overwrites hand edits, and I had been re-setting the lines manually every deploy. What is new is the
failure mode: a manual step that is forgotten does not announce itself. The build is correct, the deploy is
correct, the card is correct, and the measurement is silently null.

So the fix is a script, not a habit:

- **`scripts/pocket/devini.sh`** — patches a deployed card into the instrumented state, idempotent, ini-only,
  and it *refuses* if a key it expects is absent (`mk_dist.py` changing shape should be an error, not a
  silent no-op). It prints the other knobs so the person ejecting can see the whole state, and says out loud
  that a test card must not ship this way.
- **`deploy.sh`** now ends with `Card ini is the QUIET release default (DIAG=0 LOG=0)` and points at the
  script. The release staying quiet is correct — a player's core should not do MMIO or write to the card — so
  the release default stays and the diagnosis gets its own tool.

`CDDA_STOP_FIX` stays at release default `1` in `mk_dist.py`. Docs/421's measurement points the other way, but
flipping a shipping audio behaviour inside a diagnosis is how unmeasured changes get out; that belongs in its
own documented decision.

## 2. My two "gaps" were both my reader's bugs

**The log does stamp the version.** It is the boot marker line: `core 0.2.4: boot marker`. I searched a
segment cut with `t.rfind('boot marker')`, which starts the string *after* the version prefix, and concluded
"the card log never records the core version". It always has. The real gap is that my ad-hoc parsers kept
making this class of error — docs/402's tail-of-a-longer-run, docs/422's summed cumulative counter, docs/427's
hash-inside-the-copy bucket, and now a substring slice eating a line prefix.

**The byte comparison I claimed had not run.** `cmp -s "$E" card.elf` with `$E` empty printed "DIFFERS" and I
reported it as a caveat rather than noticing the empty path. `verify_card.py` now does the check properly and
names the right source: the ELF is staged in `pocket/build/pocket/night/Assets/night/common/night.elf`
(`pocket/dist/night` holds only `night.ini` and the README), so that is what the card must match:

```
note: night.elf bytes match the build tree (7c037edd..., 939324 B)
OK card matches pocket/dist/night: core JSONs identical, 15 slot files, version 0.2.5, ini keys unique
```

The `7c037edd...` above is copied from the tool, because my first draft of this section filled it in from
memory and it was wrong (`d2c1...`) -- the same class of error the rest of this document is about, caught by
running the command instead of remembering its output.

That closes the claim honestly: **the card holds the binary I built**, by hash and not by string.

## 3. State as this turn ends

Card: `0.2.5`, ELF byte-identical to the build tree, ini instrumented by `devini.sh`
(`LOG=1 DIAG=160 DISC_IDLE_HOOK=1 CDDA_STOP_FIX=0`, plus `PRESENT_SKIP=1 DELAY_US=0 SCREEN=0 FDUMP=2
FB_MODE=1`), one AppleDouble sidecar on `nightlog.sav` (WARN — inert, the app asks slots by exact name).

The boot is still the user's, and that is the right division of labour: I can build the instrument, but only a
run on the hardware says whether the instrument is aimed at anything.
