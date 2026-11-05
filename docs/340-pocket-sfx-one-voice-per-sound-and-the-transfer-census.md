# 340 — what a sound actually looks like on the wire, and why long ones were heard half

Core 0.1.24.  Two reports from the card: a kick hit is inaudible, and a menu confirm cuts off early.  Both are
host-side plumbing, and this time the sizes are measured instead of guessed.

## 1. The census: what the game arms

A 10 000-frame pad-only run into stage 1 (docs/333's route: Start presses at polls 5600…7400, then right/a/b/x/
left/up/a) arms **124 DMA transfers** in **10 distinct sizes**, dumped with `NIGHT_AUDIODUMP=400`:

| frames | count | rate |
|---|---|---|
| 3607 | 32 | 10 989 Hz |
| 4037 | 28 | 10 989 Hz |
| 4914 | 26 | 10 989 Hz |
| 5087 | 20 | 10 989 Hz |
| 6144 | 4 | 11 111 Hz |
| 4508 | 4 | 10 989 Hz |
| 3792 | 4 | 11 111 Hz |
| 5346 | 2 | 11 111 Hz |
| 3088 | 2 | 10 989 Hz |
| **14674** | 2 | 10 989 Hz |

All 8-bit, and nine of the ten sizes are **longer than the port's `SFX_CHUNK` of 4096 frames**.  (docs/166
measured the shipped driver feeding the same sounds as a ladder of ~257-byte chunks at a clock-paced cadence;
the port hands the board one whole block per sound — different traffic shape, same audio.)

## 2. Why "cuts early"

`drain_audio()` capped one drain at `SFX_CHUNK`, and `audio_pump()`'s `while ((got = drain_audio()) != 0)` loop
then started **every piece in the same pump iteration** — on different channels, so they played *at the same
time*.  A 5346-frame sample at 11 111 Hz is 481 ms of audio: 4096 frames went out as voice one, and its tail of
1250 frames started as voice two 2 ms later, on top of it.  What the player hears is a sound that stops at
about 370 ms.  The comment above `SFX_CHUNK` already said a transfer "should reach the mixer as one voice"; the
code did not do it.

Blocks are now 16 384 frames (32 KB, 12 of them = 384 KB of the 8 MB mixer pool) — 1.5 s at 11 kHz, which
covers the largest transfer this route produces.  A drain that *still* fills a block is logged as `SPLIT`, so
if a longer sound exists it will show up in `nightlog.sav` rather than being reasoned about.

## 3. Why the kick was not heard

`SFX_BLOCKS` was 8, and when all eight slots were still sounding the port **dropped the new sound**:

```
if (b < 0) { g_of_sfx_drops++; continue; }   /* all eight still sounding: drop rather than tear one in half */
```

That rule is backwards for the sound the player just caused.  A kick hit lands while a footstep, an explosion
tail or a 14 674-frame sound is still playing, and the kick is the one thrown away.  The card log had already
said the slots were the limit — `plays=5` on hardware against `plays=193` on the desktop shim over a comparable
span (docs/337).  Now the port takes the block that will finish soonest — the sound least worth keeping — and
counts it (`steals`), the same order openfpgaOS-Doom uses when the mixer refuses a voice (`i_sdlsound.c:289`).

Two more mixer-side changes, both from the SDK's own text:

- SFX voices are allocated with `of_mixer_alloc_for_group(OF_MIXER_GROUP_SFX, …, priority 100, …)` instead of
  plain `of_mixer_play` (untagged).  "The steal path prefers a same-group victim before crossing groups, so a
  busy MUSIC track can't silence an in-flight SFX (and vice versa)" (`of_mixer.h:322-329`); music uses priority
  0 (`of_cdda.c:50`), so gameplay sounds outrank it, as Doom states at `i_sdlsound.c:253`.
- `of_setting_sfx_vol()` now reaches `of_mixer_set_group_volume(OF_MIXER_GROUP_SFX, …)`.  Until now the menu's
  SFX Volume appeared only in the settings log line.

## 4. The census nobody had: DSP commands the port ignores

`src/sys/sys.c` handles DSP 0x40 (set rate), 0x14/0x17 (8-bit block), 0x75/0x77 (16-bit block) and 0xd0/0xd1
(stop).  Everything else fell through `default: break;` **without leaving a trace**, so "a sound the port never
arms" and "a sound the mixer refused" were indistinguishable.  Both now count, and the host names them:

```
audio: dsp commands with no case in the port = 2: 0xe1=1 0xf2=1
```

One each on this route — not a pattern yet, but the number is the thing: the next time a sound is missing the
log either says an arm was ignored, or it doesn't, and that decides which half to fix.

## 5. Measured after the change (desktop shim, same route and seed)

```
sfx=124/0/0        plays / drops / steals
SPLIT              0 lines
```

124 transfers in, 124 voices out, nothing dropped, nothing split.  Fixture suite `ALL PASS`.

## 6. Still open

- Hardware confirmation for both symptoms.  If the kick is *still* silent, `nightlog.sav` now says whether a
  voice was allocated for it (`sfx#N ch=… rate=… frames=… voice=…`, every play, not the first eight) and how
  often a slot was stolen.
- Whether 0xe1 / 0xf2 belong to the game's DSP presence check or to a sound path, and whether the canonical
  *fight* route produces more unknown commands than the title route.
- 14 674 frames at 10 989 Hz is 1.34 s — the longest sound on this route.  Worth knowing what it is; a longer
  one would reintroduce the split, and the honest fix for that is a timed chain (start the tail when the head
  finishes) rather than a bigger block.
