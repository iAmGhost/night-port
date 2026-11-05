# 390 — a save write failed if its directory was missing, and four "load" runs proved nothing

Docs/374 fixed the *restore mask* default.  This closes the rest of the save question -- writing a save, loading it
back, and loading it somewhere else -- and starts with a defect that invalidated measurements I had already made.

## 1. The write: `open ... failed` whenever `NIGHT_SAVE_DIR` did not exist

`of_save_write()` did `fopen(path, "wb")` with no directory creation, so the first desktop save attempt of any new
save directory fails:

```
save: open /tmp/svrt3/night_3.sav failed
```

and the four "cross-stage load" runs I had just launched loaded nothing at all -- their `exit=0` was a run that never
touched a save.  The card is unaffected (openfpgaOS creates `Saves/night/common` itself); this is the desktop path,
where `NIGHT_SAVE_DIR` is whatever the operator names.  `of_save_write()` now does `mkdir(save_dir(), 0775)` first on
the `OF_PC` path.  Rebuilt with `make -C pocket/src/night test` (both halves, per docs/386 5):

```
save: slot 1 -> /tmp/svrt5/night_1.sav (192 B) stage=1 gauge=77/1000 state=00/00/00 resolver_calls=0
```

`make dist` afterwards produced an ELF md5 identical to the one on the card (`817131e7041de58be7f3bf6857a3f016`): an
`OF_PC`-only change does not touch the core, so **the installed 0.1.69 stays valid** and was not redeployed.

## 2. Does a load actually change the game? Only with a discriminating value

Loading a save whose HP is 1000 into a game whose HP is also 1000 proves nothing, and that is the test I ran first:

```
load: restore mask=1
load: restored player record (stage=1 gauge=77/1000 state=00/00/00)
live hp after load: {1000: 100}      control, no load: {1000: 100}
```

The discriminating version, following docs/374's method -- patch the blob's HP (`u16 == 1000` at blob offset 47, which
is index 41 of the 64-byte player window that starts at offset 6), recompute the FNV-1a CRC over the first 188 bytes,
then load:

| run | load reported | live player record (`player_recs.jsonl`) |
|---|---|---|
| stage-1 blob, `RESTORE=1` | `gauge=78/37` | **hp 37 for 19 samples**, then 1000 for 81 |
| stage-1 blob inside stage 5, `RESTORE=1` | `stage=5 gauge=78/37` | **hp 37 for 50 samples**, then 1000 for 50 |
| stage-1 blob inside stage 5, `RESTORE=7` | `stage=5 gauge=78/37` | **hp 37 for 50 samples**, then 1000 for 50 |

So the apply path works: the patched 37 is what the game holds immediately after the load.  Two things follow.

**The game re-fills the gauge.**  The restored 37 survives 1.9 s inside the saved stage and 5.0 s in the foreign one
(rig samples are 10 Hz), then the player is back at 1000.  That is the game's own entry/recovery behaviour, not the
port discarding the save -- but it bounds what a save means for a player: a load changes the gauge *at entry*, and the
game then does what the game does.

**A save does not choose the stage.**  The blob recorded `stage=1` and both cross-stage runs reported `stage=5`, which
is where `--warp-stage 5` put them: nothing in the restore path moves the game to the saved stage.  What a save carries
across stages is the gauge pair (and, under mask 4, the key table), not stage identity -- consistent with docs/374's
finding that the stage-selector words are the risky part to force.

## 3. What is now claimed about saves

- A slot **writes** on desktop whether or not the save directory exists (192 B blob, `NGSV`, CRC over 188 bytes).
- A load **applies** live values, proven with an out-of-band patch rather than a matching value.
- Cross-stage loads (mask 1 and mask 7) run clean -- 800 frames, `exit=0`, warp entry reached, no crash observed.
- A load **cannot** be used to jump to the saved stage, and a restored gauge is temporary because the game restores it.
- The card's 0.1.69 build is untouched by this fix; its ELF md5 is byte-identical to a fresh `make dist`.
