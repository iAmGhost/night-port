# 405 — the logo freeze came back because I shipped the shape docs/401 had already condemned, and a shipping ini key had quietly vanished

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* `FB_MODE=1` restored -- the shipping shape.


Core **0.1.86**, card ELF md5 `b5f4bb6a4a3365b3743aca34a2840a11` == build == `core.json` == ELF string,
**29** ini keys, `make test` `== ALL PASS ==`, `src/f` untouched.

## 1. The user's question was the bug report
Asked *"what if it's in logo-stuck state?"* and answered: **it is** — on 0.1.85, whose default I set to
`FB_MODE=0`.

That is not a new failure mode. It is the one my own doc names in its title:
docs/401 — *"FB_MODE=0 wrote four times fewer bytes four times slower, **and that is what brought the logo
freeze back**"*, with the measurement in it:

| shape | bytes/frame | copy stage | rate |
|---|---|---|---|
| `FB_MODE=1` native 640×480, row memcpy | 307 200 | 15 771 µs | **19.5 MB/s** |
| `FB_MODE=0` OS 320×240 + software 2×2 downscale | 76 800 | **82 657 µs** | **0.93 MB/s** |

~1 µs per output pixel in the downscale's per-pixel gather — four times fewer bytes written, twenty times
slower. At 82 ms a present the run reads as stuck on the logo, which is exactly what was reported for 0.1.80
and again for 0.1.85.

**What went wrong in my reasoning:** I weighed a *measured, user-experienced* failure of `FB_MODE=0` against an
*unverified theory* about `FB_MODE=1`'s geometry ("the doubled title proves the native surface path is wrong"),
and chose the theory. The theory was also bundled into the same build as the audio fix, so the boot could not
attribute anything — the exact mistake docs/404 §5 recorded an hour earlier. One variable per boot was already a
rule in this repo and I broke it anyway.

## 2. `FB_MODE=1` is the right default, and for a better reason than speed
The corruption that made me distrust the native path was the **stale-surface commit** (docs/404 §5): on the
boot-time 320×240 → 640×480 switch, `fb` was re-pointed at the new buffer while `surface` still named the old
one, and `if (fb != surface) stage_commit(surface)` fired on its own — writing a stale picture at the *previous*
mode's stride into a buffer that is still live in a triple-buffered rotation. **That bug lives only in the native
branch**, and it is fixed (engagement flag + both pointers move together). So the native path is now both the
fast shape and the shape whose one known defect is understood, while `FB_MODE=0` is fast to write and slow to
run — a worse trade than I gave it credit for.

0.1.86 ships `FB_MODE=1`. The card's ini was flipped first (one line, same ELF) so the boot could start
immediately; the tree was then made to agree, because a card whose ini disagrees with `mk_dist` is undone by the
next deploy. `pocket/INSTALL.md`'s `FB_MODE` row now carries the hardware numbers instead of the old desktop-shim
guess that said "still unmeasured on hardware".

## 3. A shipping key had silently left the ini — found by counting, not by looking
While re-verifying the card after the deploy, the ini went **29 keys → 28**: `PUMP_AUDIO_TICK` was gone. Not by
the deploy. My own `mk_dist` edit had eaten it — the new `FB_MODE=1` entry's trailing comment was long enough to
swallow the next string literal in that concatenation, so the key simply stopped being written.

This one is nastier than the printf arity bug in docs/404, because it changes behaviour *invisibly*: a missing ini
key means the code default takes over, the card boots fine, and no log line says a lever went away. The default
for `PUMP_AUDIO_TICK` happens to match what was shipping (0), so nothing broke — but the same swallow applied to,
say, `FB_MODE` or `DISC_BATCH` would have changed the build we were measuring while the card looked normal.

The generator now refuses it, naming the keys a run depends on:
```
mk_dist: night.ini is missing PUMP_AUDIO_TICK -- an entry was lost to a comment or an edit.
Repair the ini block above; do not paper over it with a code default.
```
Verified **both ways**, because a guard that has never failed proves nothing: with the entry restored,
`mk_dist` writes 29 keys and exits 0; with `PUMP_AUDIO_TICK=0\n` deleted again, it exits **1** with that message.

## 4. If the next boot is logo-stuck anyway, the log separates the three cases
The port's waits are clock deadlines (docs/335), so "stuck" has three different meanings and only the first is a
picture problem. `scripts/pocket/cardlog.sh`, then read the 1 Hz lines in this order:

| what the log says | what it is | where to look next |
|---|---|---|
| `frames=` climbs each second **and** `sig=` changes | not stuck — slow | `stages last c=` / `perf pres=`: the present cost is the whole story (15 771 µs is the shipping budget) |
| `frames=` climbs, `sig=` frozen | the **game** is not advancing the logo | `pit=` (the 490 Hz clock is alive?), then `audio cdda=`, `dry`, `audio_low` — a wait that never elapses usually has the clock or the audio feed behind it |
| `frames=` **not** climbing | the host loop is blocked | `stages last … c=/f/x` for the copy or flip, `disc … mean_op=` for a blocking read, and whether a card-log write is in the way |

And if `sdk limits:` / `fb mode -> native 640x480 rc=0` are missing entirely, the run never reached
`phase: game main entered` and the story is in the boot lines above where they stop.

Card 0.1.86 is installed and self-consistent with `LOG=1` so the next boot answers the two open questions
(music back on the fallback path; title single and correct on the native path, where the fixed bug lived).
