# 407 — My own deferral starved the ring, and the trickle's real cause was a ceiling smaller than the buffer

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* the deferral it fixes was itself the bug of 0.1.87; docs/409 records what actually caused the pause.


Core **0.1.88**, card md5 `9d9b5ecdbeadab968e68d44e146b13b6`, 30-key ini.
Supersedes the fix in docs/406. Read that one first: its diagnosis of *where* the time went is
still correct; its prescription is what this doc withdraws.

## 1. The card indicted the fix I had just shipped

The user's report after 0.1.87: the logo hang is gone, but the music pause remains. The log says why
it got worse, not better:

| measure | 0.1.86 | 0.1.87 |
|---|---|---|
| `sectors/op` | 1.3–1.7 | 12 ✓ |
| disc ops/s | ~50 | ~6 ✓ |
| blocked share | 60–65 % | 20–25 % ✓ |
| **`audio_low`** | 0 | **0 → 7 553** (≈118/s, never recovering) |
| **`dry`** (real underrun) | 0 | **353** |
| **`ahead_min`** | 0.35 s | **0.00 s** |
| `deferred=` | — | 10 657, never draining |

The three counters that used to be zero all went non-zero. 0.1.87 traded frame dips for silence —
the **third** time in this port that "read less" has underrun this ring (docs/397: priming cut →
`dry=33`; docs/402: ramped priming → `dry=49`; now the deferral → `dry=353`).

The mechanism is visible in the two depth measures disagreeing. I gated the deferral on
`of_cdda_head()`, the *mixer voice's* cursor, and that kept reporting "deep" while the pump's own
accounting of the ring (`ring_ahead()`, the very measure that raises `audio_low` and gates `room`)
ran down to zero. A pump that defers work because a number says "enough" while its own number says
"empty" produces exactly `ahead_min=0.00, dry>0`. So 0.1.88 requires **both** to agree before it
holds off a read.

## 2. The real reason the reads were tiny was a ceiling smaller than the buffer it feeds

docs/406 saw 1.3–1.7 sectors per op and called the pump dutiful. It was dutiful *and constrained*.
The fill loop's own limit:

```c
uint64_t ceiling = g_budget + CD_LOOKAHEAD_SECTORS;   /* CD_LOOKAHEAD_SECTORS was 120 = 1.6 s */
if ((uint64_t)g_pos >= ceiling) return;
```

and the ring is `CD_RING_FRAMES/CD_FRAMES_SECTOR` = **300 sectors = 4 s**. So the ring could hold
four seconds of audio while the pump was forbidden from reading more than **1.6 s** past the disc
clock. Once `g_pos` pressed on that ceiling — which happens within two seconds of any track start —
the only thing left to read per tick is the allowance the clock releases: 75 sectors/s over a
490 Hz tick = **0.15 sectors, which truncates to 1**. That is the trickle. Not pump behaviour, not
batch size, not the deferral: an arithmetic ceiling below the buffer's own depth.

0.1.88 raises the ceiling to **240 sectors (3.2 s)**, leaving headroom under the ring's 4 s for the
guard, and sets `DISC_MIN_WANT=27` so a read that *is* due is one full 63504 B op. With 3.2 s of
permitted read-ahead, `avail` can actually reach 27.

## 3. The bug this exposed in my own method: a shared setter moved the shim's audio

While making the ceiling tunable I wired it through `of_cdda_set_lookahead()`, called from the
normal boot path on **both** hosts. The desktop rig's audio md5 changed:

```
HEAD (0.1.87)                              ad81a3e0f5567c210c358123accfe2e9   <- the docs/365/382 basis
constant 120, setter not in the ceiling     ad81a3e0f5567c210c358123accfe2e9
setter feeding the ceiling                  2ae66c0318bcdf8ef5a767319b4f87cb
```

Same route, same seed, same command line, and a *tuning* decision aimed at the card had rewritten
the byte-exact cross-host basis — and `make test` stayed green the whole time, because the rig does
not assert on that md5. I nearly accepted 2ae66c03 as "the new baseline". The bisect that proved it
was three builds; the stash-and-compare against `HEAD` is the tool.

The fix is structural, not a value: the constant is now chosen **by host** (`#ifdef OF_PC 120 #else
240`), and `of_cdda_set_lookahead()` is a **no-op under `OF_PC`** so no ini or environment value can
reach the shim's ceiling. Verified: the shim is back on `ad81a3e0…`, the RISC-V ELF still carries
the `lookahead=%d sectors` policy line, `make test` in `pocket/src/night` builds both halves, and
the repo suite is `== ALL PASS ==`.

The general rule for this port: **a knob that exists to fix a device number must be inert on the
host that is the measurement basis.** Everything else in this file (`DISC_ASYNC`, the palette gate,
the staging, the deferral) is already `#ifndef OF_PC`; the lookahead was the one I threaded through
shared code, which is how the drift was possible at all.

## 4. Shipping state (0.1.88)

`DISC_BATCH=27 DISC_MIN_WANT=27 DISC_LOOKAHEAD=240 DISC_RAMP=0 DISC_PREFILL_FIRST=150
DISC_ASYNC=0 FB_MODE=1 BLIT_DIRTY=0 SCREEN=0 FIT=1 LOG=1 DIAG=0`, BRAM off.

## 5. What the next boot answers, in the order that matters

1. `audio_low`, `dry`, `ahead_min` — **first**, before anything else. They were 0 / 0 / 0.35 s in
   0.1.86 and 7553 / 353 / 0.00 s in 0.1.87. If 0.1.88 shows any of them non-zero, the deferral
   should be deleted outright rather than tuned again.
2. `sectors/op` — should sit at 12–27 with ops/s 3–6. If it is still under 3, the ceiling is not the
   limiter and `room_before_voice()` (docs/400 §5) is the thing clipping the ring on the device.
3. `deferred=` — should be small or zero now; a large value with `ahead_min` healthy means both
   measures agree we are deep, which is the design case.
4. `perf fill max=` — was 424 ms; the priming fix plus fewer, larger ops should bound it near the
   single 63504 B read time.
5. Whether the music pause is gone. If it survives *with healthy audio counters*, it is not the ring
   and the next suspect is the silence between a track re-request and the first sample out of the
   ring (docs/398: 315–418 ms), which no fill policy can reach — that needs priming under the game's
   volume fade.
