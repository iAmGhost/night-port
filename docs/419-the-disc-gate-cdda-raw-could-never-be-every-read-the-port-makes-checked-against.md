# 419 — the disc gate `cdda.raw` could never be: every read the port makes, checked against the shipped disc

docs/410 retired `cdda.raw` as a regression gate — three runs of one binary with one seed gave two different
md5s and two lengths, because the file is a wall-clock sampling log of the mixer ring, not a record of the read
path. Since then, every audio decision has had nothing to be judged against: the seven fictional
`sys_cd_audio_stop()` sites (docs/413), the `DISC_MIN_WANT` clamp, docs/407's causal re-derivation. This is the
gate that replaces it.

## What it is

`src/cd/cd_audit.c`, compiled into **both** hosts, records every read the port asks the disc for:

```
kind        who                        offset meaning
IMG         cd_iso.c (SDL host)        byte offset in TG_NIGHT.IMG
IMG_USER    cd_iso.c (SDL host)        byte offset of the 2048-byte user slice
TRACKnn     of_disc.c (pocket, both)   byte offset inside that track's slot file
```

…plus the byte count and FNV-1a 64 of what came back. `scripts/pocket/audit_disc.py` maps each request onto
`TG_NIGHT.IMG` and compares the bytes. For `TRACKnn` the mapping comes from `scratch/pocket/manifest.tsv`
(which already carries `img_first_sector`, PREGAP-corrected), so the track→IMG rule lives in **one** language —
if it lived in C as well, a drift between the two would show up as a false failure and get ignored.

Inert unless `NIGHT_CD_AUDIT=<file>` is set, in both hosts. Nothing on the device path ever sets it.

```sh
# the pocket path (slot reads)
NIGHT_CD_DIR=scratch/pocket/cd NIGHT_CD_AUDIT=/tmp/audrun/pocket.tsv \
  ./pocket/src/night/app_pc --warp-stage 1 --frames 2000 --rig-witness 0
# the SDL host (direct IMG reads)
NIGHT_CD_DIR=NIGHT/TG_NIGHT.CUE NIGHT_CD_AUDIT=/tmp/audrun/sdl.tsv ./build/night --warp-stage 1 --frames 2000
python3 scripts/pocket/audit_disc.py /tmp/audrun/pocket.tsv
```

## First run: both paths, byte for byte

```
POCKET shim : requests 236   TRACK02=13 TRACK03=176 TRACK10=16 TRACK12=31    OK
  track 02 coverage: 150 / 3153  sectors (5%)      <-- 150 = the 27-sector batch x5 plus the 150-sector prefill
  track 03 coverage: 2106 / 19639 sectors (11%)
SDL host    : requests 2365  IMG=2365                                            OK
```

Two hosts, two entirely different read paths (per-track slot files through `of_file` vs the raw IMG through
`cd_iso`), one disc, 2601 requests, zero mismatches. The coverage column is printed on purpose: 5% of a track
is what a 2000-frame run actually touches, and a gate whose reach nobody states is not a gate.

## A gate that only says OK proves nothing — so it was tested against failure

Feeding the verifier a copy of a real request with its offset shifted by one sector, and one with a wrong hash:

```
  TRACK03 2352 28224 eaf1eb4c…: BYTES DIFFER at IMG offset 94705632 len 28224
  TRACK03 0    2352 00000000…: BYTES DIFFER at IMG offset 94703280 len 2352
FAIL 2 of 3 requests do not match the shipped disc      (the unmodified line passed)
```

A one-sector slip in the read path — the class of bug this port has actually had (docs/331 §7, docs/409) — is
caught at the sector, with the IMG offset named for dumping.

## `--diff`: the A/B form, and a determinism fact worth having

```sh
python3 scripts/pocket/audit_disc.py /tmp/audrun/pocket.tsv /tmp/audrun/pocket2.tsv --diff
A ...: 236 requests -> 2808 sector-granular
B ...: 236 requests -> 2808 sector-granular
only in A: 0   only in B: 0   shared: 2808
OK the two audits request the identical byte-identical set
```

Same binary, same seed, same command, two runs. The request sets are **identical at sector granularity** (2808
sectors each), even though the same two runs would have produced two different `cdda.raw` md5s. So the
non-reproducibility docs/410 measured was entirely in the *sampling log*, not in the read path.

That matters for how the A/B is read: `--diff` canonicalises to per-sector requests (so batch sizes -- the
prefill, `DISC_MIN_WANT`, how far the voice ran ahead -- do not count as a difference), and within that
canonicalisation **there is no timing noise left to excuse**. A change that removes a read, adds one, shifts a
start sector, or returns different bytes shows up as an exact, named difference. If a future build's diff is
not empty, it is a real change, and no "maybe it was just scheduling" is available.

## What this unblocks

Each of these changes the bytes the port reads or when, and each can now be A/B'd against a reproducible
predicate instead of an audio waveform sampled in wall time:

- removing the seven fictional `sys_cd_audio_stop()` call sites (docs/413 proved they are INT 10h mode sets);
- lifting `DISC_MIN_WANT` past the `CD_MIN_WANT_MAX` clamp (docs/407 §3);
- re-deriving docs/407's causal claim, whose bisection rested on the retired md5.

Neither `make test` (3089 differential checks) nor `pocket make test` changes behaviour here: the audit is
compiled in and inert, and both suites still pass.

## One thing that nearly went wrong

The FNV-1a 64 offset basis in the C was typed as `1469598103934665603` — one digit short of
`0xcbf29ce484222325`. Caught by writing the python verifier from the hex constant rather than from the C source;
had it gone uncaught, the gate would have failed every request (loudly, which is the survivable direction) and
the first instinct would have been to blame the read path.
