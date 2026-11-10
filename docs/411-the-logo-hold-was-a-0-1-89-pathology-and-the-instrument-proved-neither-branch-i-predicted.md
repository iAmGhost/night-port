# 411 — The logo hold was a 0.1.89 pathology, not a defect; and the instrument proved neither branch I predicted

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.


Core 0.1.91, user's report: "Logo hold / longer intro loading hold fixed."

Before accepting that, the hold was measured the same way on every build that has a card log, using the
field that ends the phase — the game's first CDDA request, `cdda req idx=1 lba=37262 sectors=3153`, i.e.
the moment it asks for the intro's audio and switches the view from the 640x480 logo to the 320x200 video:

| core | tick of first track request | wall time | what the phase did |
|---|---|---|---|
| 0.1.88 | 4 460 | **9.1 s** | (no `resio` field yet) |
| 0.1.89 | 18 892 | **38.6 s** | (no `resio` field yet) |
| 0.1.91 | 3 972 | **8.1 s** | `reads=0 bytes=0`, `spin/s` 0…2 759 |

Two things fall out, and the first is uncomfortable:

## 1. The 35 s hold belonged to one build

0.1.88 took 9.1 s and 0.1.91 takes 8.1 s for the same phase. 0.1.89 took 38.6 s. So the quantity that
docs/410 §2 spent its whole analysis on — "13.4× longer on the device" — was mostly a comparison against
the one build in which the phase happened to be pathological, not against the port's steady state.

It is worth being precise about what went wrong in the reasoning, because it is a recurring trap: 0.1.89
was the *instrument-only* build, the one docs/408 added the voice probe to. When a build exists solely to
measure something, and it is also the build whose measurement looks worst, the temptation is to read the
number as the truth about the game. It is more likely to be the truth about that build. 0.1.89 consumed
4× more ticks in a phase whose per-frame present cost was unchanged (16 130-17 835 µs vs 0.1.88's
17 660-17 694 µs), so the phase was not slower per frame — some counter the wait exits on advanced about
4× more slowly in that build. Which one, and why, is still unknown; it is closed here only because no
shipping build exhibits it.

That also retires an earlier open question. "Longer logo/intro video loading is back", raised when 0.1.89
showed its first track request at frame 2112 against 0.1.88's frame 492, was the instrument.

## 2. Neither branch of the instrument's prediction happened

docs/410 set the trap correctly and the answer came in as neither option:

- **It is not an SD-card wait.** During the whole hold `resio` reads `reads=0 bytes=0`. All of the phase's
  reading — 2 302 464 B in 529 calls, largest single read 49 664 B, i.e. ~2.3 MB/s — happens in the *first*
  second, before the hold even begins. `opening.fli`'s 9.59 MB is never fetched as one object, and no
  megabytes move during the stretch the player sees as black.
- **It is not a keyboard spin either.** `spin` counts calls to the host's input pump, whose only caller is
  `sys_key_spin` (src/f/f.c:16410). On the device the hold runs at **2 759 spins/s**, which is simply the
  host poll rate (~363 µs/poll). For comparison, the ungated desktop build measured the same phase at
  **~10 000 000 spins/s**. A three-and-a-half-order difference means the two hosts are not sitting in the
  same wait at all: on the desktop the game is inside a keyboard busy-wait essentially all the time; on the
  hardware it is not inside one at all, and the host is just running its loop.

So the honest summary of the phase on shipping hardware is: **~8 s, no disc reads, no keyboard spins, a
static 640x480 grid with a palette fade, ~530 presents of the same picture.** The port can now prove what
the hold is *not*; what it *is* remains unexplained. The remaining 3× gap against the desktop (8.1 s vs
2.6 s) is a real difference between the two hosts in a shared-code phase, and it is now the open question,
not a regression. Settling it properly needs the original side (the container route in docs/377, never
completed for this phase) to say what the hold's true length is.

## 3. What the instrument is still good for

`resio` stays in the device build: it is the only thing that counts NIGHT.RES reads, and it has already
disproved two theories per hour of its cost (see the `.wal` tiles at up to 1.5 MB each and the 300 KB
`.pcx` cards, all of which pass through `sys_file_read` and were previously invisible). Its numbers also
put the device's file-read throughput at ~2.3 MB/s in bursts — an order of magnitude above the per-sector
CDDA cost that docs/396 measured, which is itself a useful fact for any future prefetch work.

## Reproduce

```sh
# first-track-request tick, identically, for any card log
python3 - <<'PY'
import re
raw = open('/Volumes/Pocket/Saves/night/common/nightlog.sav','rb').read()
st = [m.start() for m in re.finditer(rb'boot marker', raw)]
txt = (raw[st[-1]:] if st else raw).decode('utf-8','replace')   # docs/402: newest run forward
tick = 0
for l in txt.split('\n'):
    m = re.search(r'ticks=(\d+)', l)
    if m: tick = int(m.group(1))
    if 'cdda req' in l:
        print('hold ends at tick %d = %.1f s' % (tick, tick/490.0)); break
PY
```

Archived: `/tmp/run88.sav`, `/tmp/run89.sav` (0.1.88 / 0.1.89 card logs; the 38.6 s anomaly and its
9.1 s neighbour), and the shipping run on the card at `Saves/night/common/nightlog.sav`.
