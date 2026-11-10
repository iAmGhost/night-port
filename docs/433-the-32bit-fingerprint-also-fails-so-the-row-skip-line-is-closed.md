# 433 — the 32-bit fingerprint also fails, so the row-skip line is closed; the real problem is a 3-second stall

`docs/432` set a rule instead of an opinion: if `0.2.9` (32-bit row fingerprint) does not beat `ROW_SKIP=0`
on the static 640x480 phase, the feature goes off and stops being tried. `0.2.9` booted. The rule fired.

## 1. The measurement

```
perf pres=8005us max=95135us ... | rows c=307606 s=1302794 fs=2430 dry=0 f=0 inv=50 mm=0 chk=4096 | br n/g/d/o=2404/1902/0/0
```

| | 0.2.8 (64-bit hash) | 0.2.9 (32-bit hash) |
|---|---|---|
| rows skipped | 85.7% | **80.9%** |
| frames skipped whole | 2 584 | 2 430 |
| self-check | mm=0 / 4 096 | **mm=0 / 4 096** |
| copy-stage median, static 640x480 | 30.1 ms | **29.4 ms** |
| frames/s (static 640x480 phase) | 33 | 33 |

The skip is *correct* (4 096 byte-verified skips, zero lies) and *pointless*: skipping 81% of the writes
changed a 30 ms frame by 0.7 ms. So the pass that decides "is this row already there" costs essentially what
the write costs -- reading the emulated-A0000 arena row-by-row is around 11 MB/s, and I do not know why it is
that slow relative to the one linear 307 KB pass docs/422 measured at 34.8 MB/s. That is a fine thing to not
understand, as long as the consequence is decided by the number and not by hope.

**`ROW_SKIP=0` is on the card.** Behaviour is back to the good build. The code stays default-off; the rule is
in docs/432; there will not be a third fingerprint.

## 2. A reporting bug nearly condemned a correct build

The first run of the new log reporter printed:

```
self-check: mm=50 mismatches over chk=0 verified skips  <-- mm must stay 0
```

Which reads as "the row skip lied 50 times and never verified anything" -- the kind of result that ends a
feature. It was wrong: my `%` field unpacking read `inv` and `chk` as `mm` and `chk`. The raw log line says
`inv=50 mm=0 chk=4096`. Same class as the four parser bugs in docs/428, and now the reporter names its own
field order in a comment right where the mistake was, because this is the second time a *summary* lied to me
about a build's health.

## 3. What the log actually complains about is choppiness, and it is not the copy

```
choppiness (gap worst, ms/second, n=124): median 37.0  p90 64  p99 1380  max 2998
```

Half the run is smooth at ~37 ms. The tail is the problem: **a 1.4-second gap in the worst second of ~1%, and
a 2.998-second gap at the top.** Earlier builds could not see this at all -- every metric before
`gap worst=` was a rate or a sum.

That tail is where the user's "long wait, stutter" lives, and it is the thing to work on next: naming what the
port is doing for those 3 seconds (the log shows resource traffic climbing and individual presents costing
37-96 ms in that window, `max copy=95 ms`, but "presents cost 37-96 ms" is not yet an explanation).

Next instrument, therefore: attribute the stage-entry window per file/resource operation, not per second.
That is `res_calls/res_kb` per second, plus a budget-style line naming the largest single resource operation
in the window -- the same shape as the `disc ops=... mean_op=... max_op=...` line that settled the music
question, aimed at the load path instead.
