# 56 — Game-anchored playtest script, and a key-handler crash it exposed

## Why the wall-clock script was not deterministic

`--script` events used to fire at absolute host wall-clock times. Any change in host speed moved the game
to a different state when a key arrived: with an audio device live the same route stopped reaching the
stage at all (docs/55), and `--frames` never terminated the run because the limit is only checked between
game steps while one attract step keeps flipping frames.

## Anchored timeline (`wait res NAME`)

`scripts/playtest/route_episode1_anchored.txt` uses a new directive:

    <deadline_ms> wait res <substring>

`pt_drive` polls it every pass (the leading number is only a deadline, so a missing anchor cannot hang a
run). When it fires, every remaining event is rebased by `now - deadline`, so gaps after an anchor are read
*relative to that anchor*. Matching cannot sample the old `g_res_trace` "last lookup" global — the
boot-time lookups are long gone by the time the script polls — so the game now announces each lookup as it
happens: `sys_res_notify()` (src/sys) → host ring `g_reslog[256][24]`, installed with `sys_set_res_hook`.
Slots must stay monotone, otherwise a later anchor can be evaluated before an earlier one.

Supporting host change: when the script is exhausted the app exits normally after 2.5 s, so `manifest.tsv`
is flushed instead of losing its tail to a `kill`.

## Measured determinism (the harness contract's clause)

Same anchored route, two host configurations — no audio at all (`--no-audio`, every CoreAudio device on this
machine refuses/hangs) vs the SDL null driver (`SDL_AUDIODRIVER=dummy`, device opens at 48 kHz):

| event | no-audio ms | audio ms | delta |
|---|---:|---:|---:|
| logo.pcx | 5 | 5 | +0 |
| opening.fli | 1454 | 1454 | +0 |
| title1.pcx | 42009 | 42010 | +1 |
| 1s1.pcx | 42945 | 42944 | −1 |
| 1s2.pcx | 43679 | 43677 | −2 |
| 1s3.pcx | 44414 | 44412 | −2 |
| stage1.pcx | 45148 | 45146 | −2 |
| episode_start | 47131 | 47133 | +2 |
| stage_play | 55130 | 55132 | +2 |
| playing | 60130 | 60133 | +3 |

Every event present in both runs, all within ±3 ms, with the full sequence logo → intro movie → title →
slides 1s1/1s2/1s3 → stage card → episode start → stage play → playing, each with its fade hold shot.

## The crash the anchored route exposed (fixed)

`EXC_BAD_ACCESS` at `address=0x1b` in `fn_034635` (f.c:8625):

    if (state == 1) g_dat_62a94 = *(int32_t *)(g_dat_53c7c + 0x1b);

`DAT_53c7c` is the player-entity pointer and is only valid after a stage is loaded; a key *make* during the
attract/menu screens read DOS heap offset 0 on the original (mapped, so it picked up garbage and carried
on) and faults on the host. The read is now skipped when the pointer is null, reported once, mirroring the
NULL room-chain link treatment in docs/52. Two regression checks added (`fn_034635` with no player: the key
counter still advances, no value is invented for `DAT_62a94`); suite **2821 checks ALL PASS**.

## Also this turn: `--no-audio`

Opening a host audio device can block outright on this machine (the call never returns, rather than
failing), which used to freeze a scripted run before its first frame, so the host gained `--no-audio`; the
board's DMA ring is still drained every frame in that mode (docs/55).
