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

## 5. Closed by measurement and by the player: `DELAY_US` is not moved

`0.2.21` on the card, long run (`...020111-0.2.21-hold-kinds.log`). Every long hold classified itself, and
**none of them were stalls**:

```
HOLD  4.77 s CARD SITTING | presents=91  noflip=40  | res=opening.fli | res_calls=+1   | disc ops=+6
HOLD  4.55 s CARD SITTING | presents=30  noflip=4   | res=1s1.pcx     | res_calls=+395 res_kb=+2362
HOLD 22.34 s CARD SITTING | presents=530 noflip=508 | res=1s2.pcx     | res_calls=+85  | disc ops=+138
HOLD  6.81 s CARD SITTING | presents=145 noflip=121 | res=1s3.pcx     | res_calls=+86  | disc ops=+39
HOLD  2.44 s CARD SITTING | presents=39  noflip=17  | res=stage1.pcx
HOLD  5.46 s CARD SITTING | presents=112 noflip=64  | res=stage1.pcx
docs/446 invariant: OK -- flipnw=0, noflip reached 874
```

The middle row is the one that settles the comparison: our `1s2.pcx` card sits **22.34 s**, and the original
holds a static card **20.62 s** at the same place in the route (docs/435) — about 8 % long on a 22-second
quantity, on a card the original itself parks on. `1s3` is 6.81 s against the original's 6.0–6.5 s (docs/416).
The `1s1` row shows what the reads look like when they are *not* the problem: 395 calls and 2 362 KB moved
inside a hold during which the game kept presenting.

> *"i think stage card animation speed is acceptable state."*

That is the player's verdict and the numbers do not argue with it. **`DELAY_US` stays as it ships: 500 on the
dev card, 0 in the release ini. The hold thread is closed, and the fade thread with it** (docs/444's table:
612–892 ms against the original's 600–830 ms). No trim, no rebuild, no further boots spent on it.

**The witness's own caveat, since I quoted it.** `presents` runs 20–50 ahead of `noflip` in every long row.
That is not a flip happening inside a window that is defined as flip-free; `g_present_visits` counts visits
that returned from `present()` at the `OFF_PRESENT` gate (`main_of.c:976`) or the empty-view gate (`:990`) and
so never reached the flip decision. The classification rule is unaffected — a true stall has `presents=0`,
and nothing in this log looked like that — but the `presents` column is "visits that got past the cap", not
"frames the panel was offered". I have not counted which of the two gates dominates, and this thread does not
need it.
