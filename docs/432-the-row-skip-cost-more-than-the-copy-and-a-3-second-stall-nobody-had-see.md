# 432 — the row skip cost more than the copy, and a 3-second stall nobody had ever seen

Two findings from the `0.2.8` boot, and one of them is a new problem rather than an old one coming back.

## 1. `ROW_SKIP` measured as a net loss, and the arithmetic is embarrassing

`0.2.8` ran the row skip for real. It worked, in the sense that it worked:

```
perf ... skips=0(hash 0 smp 0) | rows c=214692 s=1295628 fs=2584 dry=0 f=0 inv=40 mm=0 chk=4096 | br n/g/d/o=2421/1451/0/0
```

**85.7% of rows skipped** (1 295 628 unchanged vs 214 692 copied), 2 584 of 3 872 frames skipped whole,
`mm=0` across all 4 096 self-checks, and `inv=40` where the broken rule had given 1 800. The logic is sound.

And the frame cost did not move:

```
copy-stage cost during the static 640x480 phase (t=15-65 s): n=49 median 30.1 ms  p90 30.1  max 30.1
```

Flat **30.1 ms**, identical to the builds without the row skip, with 85% of the writes gone. The reason is
mine: `row_hash` was 64-bit FNV-1a over 8-byte reads. On rv32imafc a 64-bit load is two loads and a 64-bit
multiply is several, so the pass over 307 200 bytes cost ~27 ms -- to avoid a 17.5 ms write. An optimization
slower than the thing it avoids, and the phase table proved it rather than my arithmetic.

`0.2.9` replaces it with 32-bit FNV over word reads -- the pass docs/422 measured at 34.8 MB/s (~8.8 ms for a
640x480 surface) -- with zero padding folded arithmetically (`prime^zeros` by squaring) because the game
branch pads every row it draws. A 32-bit per-row fingerprint is strictly stronger than the whole-**frame**
32-bit fingerprint `PRESENT_SKIP` has shipped with since 0.2.0: the same collision chance there would have
froze an entire frame rather than leaving one row stale until the next change. The 4 096 byte-verified
self-checks stay on, and `mm` is the tripwire.

Decision rule, so this cannot wander: **if `0.2.9` with `ROW_SKIP=1` does not beat `ROW_SKIP=0` on the static
640x480 phase, the feature is off permanently.** No third try at the fingerprint.

## 2. `gap worst=2983ms` -- a three-second stall at stage entry, invisible until this week

The new jitter metric reported, for the first time anywhere:

```
gap worst=2983ms since report (last frame 42ms ago)      ... at wall 84.5 s
cdda req idx=11 lba=223146 sectors=17856 (238.1 s) -> disc track 12 slot 28
cdda card stop: stage1.pcx on screen, no CD call since 0x03b130 -- quiet over the hold, per docs/413
stages last m=16 p=237 c=36972 f=217 x=243 | max all=96290 copy=95263
```

The five worst seconds of the run (2 983, 1 381, 634, 551, 502 ms) all fall in wall 70-88 s, which is exactly
when the port leaves the title and loads stage 1: `res_calls` climbing, `disc ops` climbing, and individual
presents costing 37-96 ms (`max all=96290 copy=95263`).

So what the user reports as a long wait and stutter is **not the logo card** -- the logo hold has been ~6 s
since 0.1.99 -- it is the **stage-entry window**, and it contains a single stall of about three seconds. No
earlier build could have seen this: every metric before `gap worst=` was a rate or a sum, and a run can
average 33 fps and still feel like this. This is the next thing to work on, and it is a different problem from
the surface copy.

Also worth naming: the 65 s of 640x480 before the stage is **player-gated** (the port sits on the title/attract
until you press). It is therefore not comparable across boots, and comparing it between builds is a mistake --
`docs/415` measured the original doing the same kind of thing (attract 27-40 s, title 49-50 s). The comparable
numbers are the game-gated holds (4-5 s cards) and the per-phase fps.

## 3. `logsave.sh`, because "did it get worse?" was unanswerable

`nightlog.sav` is one fixed 256 KB slot and a fresh boot writes from the beginning, so by the time the card is
mounted you have only the last run. Asked whether the logo wait had regressed, the only comparable measurement
was in a log that the build in question had already overwritten. So `scripts/pocket/logsave.sh` archives the
card's log -- named by the version the core stamped in its own boot marker (docs/428) -- before the next boot
eats it. Every "did it regress?" answer needs two logs, and one of them has to be saved before the next boot.

```
archived scratch/pocket/logs/nightlog-0.2.8-20261110-222616-pre-fix.log (206254 B, 1 boot marker(s), core 0.2.8)
```
