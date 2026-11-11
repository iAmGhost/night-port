# 365 — the two hosts, same shared `src/f`, diffed on the same route: what a Pocket-path run and an SDL-path run agree on

Task-11's differential half: *PC-shim frame dumps diffed against the existing SDL host dumps on the same scripted
route*.  `scripts/pocket/diff_hosts.py` does it.  Both hosts write the same `src/app/rig_sink.c` sidecars, so this is
a port-vs-port diff through one shared game layer, which is a sharper test than a port-vs-emulator diff.

## 1. Two witness gaps had to be closed first, or the diff was blind

| witness | SDL host (`src/app/main.c`) | Pocket host before | Pocket host now |
|---|---|---|---|
| CD select markers | `rig_cdda_marker()` on every change, *and* a `stop` record for the track being left | `play` only | `play` **and** `stop` (`of_cdda.c`'s `rig_stop()`, index `-1` with the outgoing lba/sectors, exactly main.c's shape) |
| delivered-audio series | `rig_cdda_sample(t, raw, 2352)` per sector -> `cdda.raw` + the running `bytes` column in `cdda.jsonl` | **absent** -- `cdda.jsonl` had no audio half at all | per sector, same call |
| resource-load anchors | `sys_set_res_hook(reslog_add)` -> `markers.jsonl` `res <name>` (docs/56) | `sys_set_res_hook(0)` -- an empty `markers.jsonl` | hook installed **only when `--rig-dump` is given** (on the device it would run on every resource lookup) |

The last one is the interesting host-policy difference: the rig anchors are a desktop diagnostic, and the Pocket host
must not pay for a diagnostic on hardware.  Same for the `#ifdef OF_PC` around the arena witness (docs/363) and the
`DIAG=` gating (docs/347).

## 2. The comparison has to be paced-independent, and that is a finding by itself

`--warp-stage 3 --frames 2500` presented **2 501** frames on the Pocket shim and **5 243** on the SDL host in the same
stage with the same seed.  Frame index is therefore *not* a shared coordinate, and neither is wall time: `rig_poll`
samples every 100 ms, so the two runs offered 417 and 459 state samples for the same route.  What the hosts do agree
on is the game's own state, so every test below compares sets, ordered sequences, or samples keyed by the rig's own
sample index -- never "frame k against frame k".  (docs/333 hit the same wall and compared mean RGB instead.)

## 3. Result, three routes (`NIGHT_SEED=1234`, `--warp-stage N --frames 2500`, Pocket shim reading `scratch/pocket/cd`, SDL host reading `TG_NIGHT.IMG`)

```
python3 scripts/pocket/diff_hosts.py /tmp/xpoc/s1 /tmp/xsdl/s1  /tmp/xpoc/s3 /tmp/xsdl/s3  /tmp/xpoc/s8 /tmp/xsdl/s8
```

| route | CD selections, in order | pictures drawn by both hosts | entity state samples equal | RES loads |
|---|---|---|---|---|
| stage 1 | `1 → 9 → 11 → 2` identical index/LBA/sector on both | 1 606 of 1 681 pocket pictures (95.5 %) | 368 / 417 (88.2 %) | 157 vs 155, 155 names in common |
| stage 3 | `1 → 9 → 11 → 9 → 12` identical | 1 394 of 1 520 (91.7 %) | 304 / 417 (72.9 %) | 160 vs 158, 158 in common |
| stage 8 | `1 → 9 → 11 → 8 → 12` identical | 810 of 879 (92.2 %) | 349 / 422 (82.7 %) | 146 vs 144, 144 in common |

Plus, on every route: identical view-geometry sequence (`640x480` then `320x200`), and `meta.json` (the rig's DS cell
manifest -- the map of which guest address each host is watching) **byte-identical**.

**The CD half is the strongest line in the table.**  The game asked for the same tracks, at the same LBAs, with the
same sector counts, in the same order -- and the two hosts read the disc through completely different backends
(`src/cd/cd_iso.c` seeking a CUE/IMG pair on the Mac, versus `of_disc.c` opening `slot:18..31` APF data slots).  The
per-episode track is the discriminator (`2`, `9`, `8`) and the fixed selectors (`1` opening, `9` title, `11`
stage-start slides, `12` stage-clear slides) land in the same places.  docs/363 checked those selects a second way,
against `g_tab_52da4` read out of `src/f/f.c`.

**"Pictures drawn by both hosts" is the video half done honestly.**  A presented frame is hashed to a 40×30 luma
signature (`frames.bin`, `src/app/rig_sink.c`); the diff counts *distinct* signatures each run produced and how many
are the same picture on both hosts.  91.7–95.5 % of the Pocket host's distinct pictures appear in the SDL host's run
of the same route.  They are not 100 % and the reason is visible in the same table: the runs are not the same length
in wall time and the two hosts enter a stage by different means (the SDL host uses `wait res`/`wait ds` anchors,
the Pocket shim uses the state-anchored warp entry pump, docs/333 §5), so each run also visits a few states the other
does not.  The SDL host's distinct-picture count is larger partly because it presented 2× the frames.

**The entity half is where the two routes genuinely differ.**  Counting the live entity records (type, x, y) at each
sample: 73–88 % equal.  The disagreement is not a shared-layer disagreement -- the max live entity count is identical
in every route (4, 7, 5, matching docs/363's sweep) -- it is that the warp entry scripts differ, so the player is in a
different place a few seconds in.  That is the honest limit of a same-route diff between two hosts with different
entry mechanisms, and it is why the strongest state evidence remains the byte-exact stuff: the CD selects above, and
the arena-vs-PCX identity below.

## 4. What is *not* a divergence, said plainly so it is not mistaken for one

- **Delivered-audio byte totals differ** (9.4/9.3/9.9 MB on the Pocket shim against 7.57/7.57/7.56 MB on the SDL
  host, 9.7–13.3 s of stereo CD-DA).  The Pocket engine reads *ahead*: a 300-sector (4 s) prefill plus a 120-sector
  (1.6 s) lookahead (`CD_LOOKAHEAD_SECTORS`), while `src/app/main.c` fetches on demand.  More sectors pulled from the
  disc is a prefetch policy, not more music played -- `audio_low=0`, `dry=0` and `ahead_min=1.59 s` (docs/364) say
  the ring was never close to empty in either direction.
- **`logo.pcx` and `opening.fli` are pocket-only markers.**  The Pocket host boots through the logo and the .fli
  intro and *then* warps; the SDL host's `--warp-stage` self-entry (docs/322) does not mark them.  It is a host boot
  difference, and on the Pocket path it is the good kind: the .fli intro really runs there.
- **`cells.bin` differs** while `meta.json` does not: the manifest of watched cells is the same, their contents at
  sample time are not, because the runs are at different points in their own timelines.

## 5. The asset-identity half of task-11, already measured from the other direction

docs/62 measured the SDL host's arena against the shipped PCX decode; docs/363 measured the Pocket host's arena the
same way (`NIGHT_VIDRAW=1`, index bytes) and got 307 200 / 307 200 identical bytes for `1s1-1s3`, `stage1`,
`2s1-2s2`, `stage2`, `3s1-3s6`, `stage3`, `stage4`, `stage5`, `6s1-6s3`, `stage6`, `stage7`, `8s1`, `stage8`,
`9s1-9s3`, `stage9`.  Two hosts, two disc backends, one shipped asset set, byte-exact at both ends.

## 6. Reproduce

```sh
make app                                                     # build/night  (SDL host)
(cd pocket/src/night && make test)                           # pocket/src/night/app_pc  (SDK SDL2 shim)
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234
for n in 1 3 8; do
  NIGHT_CD_DIR=scratch/pocket/cd ./pocket/src/night/app_pc --warp-stage $n --frames 2500 \
        --rig-dump /tmp/xpoc/s$n --rig-witness 0 > /tmp/xpoc/s$n.out 2>&1
  ./build/night                 --warp-stage $n --frames 2500 --rig-dump /tmp/xsdl/s$n > /tmp/xsdl/s$n.out 2>&1
done
python3 scripts/pocket/diff_hosts.py /tmp/xpoc/s1 /tmp/xsdl/s1 /tmp/xpoc/s3 /tmp/xsdl/s3 /tmp/xpoc/s8 /tmp/xsdl/s8
```

`repo-root make test` after these changes: `== ALL PASS ==`.  `src/f` untouched; the witness additions are
`#ifdef OF_PC` or dump-conditional, so the device ELF carries none of them.
