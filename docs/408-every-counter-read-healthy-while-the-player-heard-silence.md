# 408 — Every counter read healthy while the player heard silence, so the counters were pointed at the wrong thing

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* instrument only; its probes are what found the bug docs/409 fixes.


Core **0.1.89**, card md5 `4f115293053b68cfd9ce580e0222c24a`. **No behaviour change** — this build
only adds an instrument. It is deliberately one variable: docs/406's lesson was that bundling a fix
with a revert with a toggle left the next boot unable to attribute anything.

## 1. The symptom is a shape, and 0.1.88's log cannot see it

> "It plays music 1~2 seconds then pause a while and continues to play."

That is periodic, and periodic means something with a period. The 0.1.88 card log (83 800 B, one run
plus the old run's tail — the counters drop at `n=2511 → 2317`, which is docs/402 §1's boundary, so
the region after it is the previous 0.1.87 boot and says nothing about this one) answers it in the
negative. Across the whole gameplay region of the new run:

| counter | what it would show if the ring were the problem | 0.1.88 measured |
|---|---|---|
| `audio_low` | climbing | **0** |
| `dry` (real underrun) | climbing | **0** |
| `room_hold` | climbing | **0** |
| `ahead_min` | collapsing | 24 504 frames ≈ 0.55 s, steady |
| `ring` | draining | 83 400 frames ≈ 1.9 s, pinned |

Four track requests, four plays, four stops — so the game is not re-requesting the track every
second, and the pause is not the game's own MSCDEX churn (docs/398's re-request theory does not
apply to this run).

So: **every counter green, the player hears silence.** That is not a marginal disagreement; it says
the instruments measure the port's own model of what *should* be audible, and not the mixer.

## 2. Two properties of this CDDA engine make that gap structural

**(a) The consumption counter clamps itself.** `clock_consume()` computes what the wall clock says
the voice must have eaten, and then:

```c
if (want > g_done_frames) { g_dry++; want = g_done_frames; }   /* never consume audio that is not there */
```

The clamp keeps the model sane and makes `dry` structurally incapable of reporting the failure it is
supposed to catch: if the mixer's cursor runs *ahead* of what the model credited, the model just
refuses to believe it. `dry=0` therefore does not mean "no underrun", it means "the model never
wanted more than it wrote".

**(b) The voices loop the whole ring, so they cannot underrun at all.** `voice_start()` gives two
MUSIC voices each a mono ring of `CD_RING_FRAMES` = 176 400 frames (4 s) and
`of_mixer_set_loop_h(v, 0, CD_RING_FRAMES)`. A looping DMA voice does not stop when the writer falls
behind; it reads **on into audio it has already played, or into samples nobody has written yet**.
The failure mode this design can produce is therefore not a dropout from starvation — it is
*stale or silent samples under the cursor*, and *ring-lap replay*. Neither of those has a counter.

The one place the code does read the mixer, `room_before_voice()`, throttles writes — and its result
was never printed. Its sibling `of_cdda_head()` was printed, and read against the model it exposes a
disagreement the last build shipped on silently:

```
head  = 170 500 frames  (mixer's view:   3.86 s buffered, ring 96.5 % full, 0.03 s free)
ring  =  83 400 frames  (model's view:   1.89 s buffered, ring 47 % full)
difference = 87 100 frames;  the prefill is 150 sectors x 588 = 88 200 frames = 2.0 s
```

Those numbers differ by about the prefill, and the two views imply different free space in front of
the writer (0.03 s vs 2.1 s) — which is what sets the op size: `allowed` is clamped to
`room / 588`, so the mixer's view is why 0.1.88 still measured 1.6 sectors per read even after
`DISC_MIN_WANT` went to 27. Which view is right is exactly the question the new probe answers.

## 3. What 0.1.89 measures

Once per log interval, straight from the mixer:

```
audio voice live=0/0 dpos=0/1001989us rate=0.000x behind=69031 vroom=73127 model=69031
      e_live=19707 e_wtail=15462 starts=4 stops=3 laps=0 vfull=0 sounding=1 silent=0ms
```

| field | question it answers |
|---|---|
| `sounding`, `live` | is a MUSIC voice actually running, and where is its cursor |
| `dpos/…us`, `rate` | how fast the mixer eats the ring. `rate=1.000` = the 44 100 frames/s the model credits. **1.088 = 48 000/44 100**, the rate-ignored case openfpgaOS-Doom documents (`i_sdlsound.c:295`), which would drain the ring 8.8 % faster than every counter assumes |
| `e_live` | **max \|sample\| in 8 192 frames under the voice cursor.** Non-zero while sounding means the voice is reading real audio; `e_live=0` with `sounding=1` is the audible pause caught in the act |
| `e_wtail` | the same just behind the write cursor — separates "we wrote silence" from "the cursor is somewhere we are not writing" |
| `behind` / `vroom` / `model` | the mixer's depth and free space against the model's; the 88 200-frame disagreement, tracked over the run instead of inferred once |
| `starts`, `stops` | *our* voice (re)starts including `cdda_resume()`, not the game's MSCDEX calls |
| `laps` | the voice cursor passed the write cursor: ring-loop replay (a skip, not a gap) |
| `vfull` | fills refused because the mixer's view had no room — a refusal that used to be invisible |
| `silent` | accumulated time the track was live with nothing sounding: **the symptom, in milliseconds** |

Desktop path is derived from the model and the ring, so the rig basis is untouched: shim `cdda.raw`
md5 still `ad81a3e0f5567c210c358123accfe2e9`, `pocket/src/night` `make test` builds both halves,
repo `make test` = `== ALL PASS ==`, `src/f` unmodified. Two notes on how it was checked: the
specifier/argument count of the new line is 17/17 and was counted rather than trusted (docs/404
found a 12/10 line quoting phantom values for five builds); and the first three "unchanged md5"
readings in this session came from a *stale binary* after a failed `make test`, so every number
above follows a fresh build.

## 4. How to read the next boot

`silent` is the headline: if it grows by roughly 1–2 s every few seconds, the probe is looking at the
symptom and the fields below say which mechanism.

1. `sounding=0` and `silent` climbing, `starts` climbing in step → **the voice stops by itself and
   gets restarted**. Then `stops` vs `starts` and the period tell whether the mixer retires a
   looping voice or `cdda_resume()`'s 1 s/2 s backoff (`g_retry_us`) is the period the ear hears.
2. `sounding=1`, `e_live=0` → the voice is running over **silence**. Then `laps>0` or `behind` near
   `CD_RING_FRAMES` means the cursor is outside the region the writer is filling — a phase problem
   between write cursor and voice origin, not a bandwidth problem.
3. `sounding=1`, `e_live` loud, `silent` flat → **the pause is not in this path**; it is downstream
   (MUSIC group volume, or the game ducking CD audio through a DSP command with no case in the port
   — the `audio: dsp commands with no case` line runs 65 times a boot and is worth reading at last).
4. `rate≈1.088` → the mixer is eating at 48 kHz, every budget in the engine is 8.8 % short, and the
   fix is a rate correction, not another fill policy.
5. `vfull` climbing with `vroom` tiny while `model` says 2 s free → the writer is being throttled by
   a mixer view the model disagrees with, and the 1.6-sector reads are explained.

Whatever comes back, the fill-policy knobs (docs/406 §2, docs/407 §2) are now known to be the wrong
axis for *this* symptom: 0.1.87's log had the reads batched and throttled and the pause still grew,
and 0.1.88's log has them batched and the pause persists with every health counter at zero.
