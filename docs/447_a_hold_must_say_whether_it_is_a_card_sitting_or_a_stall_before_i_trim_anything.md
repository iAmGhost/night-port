# 447 — a hold must say whether it is a card sitting or a stall, before I trim anything

## 1. The trim I was about to make had a stale basis

The open item was "`DELAY_US` from 500 to ~430, because our holds run 7 018 ms against the original's
6.0–6.5 s" — about 10 % long. Two numbers from the `0.2.20` card log make that sentence unsafe to act on:

| | value | why it does not support the trim |
|---|---|---|
| the `7 018 ms` I quoted | `0.2.16` | measured on code where **a skip still flipped** — the exact behaviour docs/445 removed |
| longest hold in the current build | **5 298 ms** | already *inside* the original's 6.0–6.5 s window |

So the trim would have tuned a shared timing knob to a measurement of code that no longer exists, in the
direction of making holds shorter than something that is already not long. `DELAY_US` stays at 500 on the dev
card and 0 in the release ini.

## 2. The meter conflates two opposite causes

`hold=` means "time since a frame actually reached the display", which is the right quantity to *care about*
and the wrong one to *act on*, because two causes reach it identically:

- **CARD SITTING** — the game is presenting, and presenting the *same* thing. After `0.2.18` those presents
  skip both the write and the flip, so the panel legitimately holds one picture. The original holds a stage
  card 6.0–6.5 s (docs/416) and a static card 20.6 s at slides→gameplay (docs/435). Not a defect.
- **STALL** — nothing asked to be shown: the game is blocked on disc reads (docs/434). Also not a fidelity
  gap (docs/435 measured the original holding still at the same moment).

They call for opposite actions — one is trimmed with `DELAY_US`, the other must not be touched — and the old
meter could not tell them apart. A single number that names neither is how I nearly spent a boot making the
game run fast.

## 3. The witness (`0.2.21`)

At the end of any hold ≥ 1 500 ms, one line classifies it and shows its receipts:

```
HOLD 5.30 s = CARD SITTING (presented, nothing new to show) | presents=412 noflip=411
              | res=1s2.pcx seq=175 | res_calls=+1180 res_kb=+231 | disc ops=+7 sectors=+84
HOLD 3.00 s = STALL (nothing asked to be shown) | presents=0 noflip=0 | res=1s1.pcx ...
```

The classification is one comparison — `presents == 0` over the window, counted at `present_step` after the
`PRESENT_CAP_MS` return. Bounded: ≥ 1 500 ms only, 12 lines per boot, and it re-snapshots its counters on
every flip so the deltas are per-hold rather than cumulative.

A bug in the first cut, caught by reading the format string against its arguments: the `res_kb` delta
subtracted **call counts from byte counts** (`rb/1024 - g_hw_rc/1024`). Now `g_hw_rb` is snapshotted with the
rest.

## 4. What the next boot decides, and how

Boot `0.2.21` and let **2–3 stage cards finish on their own** — deliberately *not* ESC-skipped, or the sitting
case never appears and the run answers nothing.

| if the boot shows | then |
|---|---|
| `CARD SITTING` holds > ~6.5 s | trim `DELAY_US` by the ratio (target / observed), one command, no rebuild |
| `CARD SITTING` holds inside 6.0–6.5 s | the hold thread **closes**: we match, and there is nothing to tune |
| every long hold is `STALL` | `DELAY_US` is irrelevant to the complaint; leave it, and the reads are the thread (docs/434/435) |

The reads in the witness make the third row checkable rather than rhetorical: `res_calls=+N` over the window,
divided by its length, is directly comparable to the original's per-read pacing on the same card (docs/416).
