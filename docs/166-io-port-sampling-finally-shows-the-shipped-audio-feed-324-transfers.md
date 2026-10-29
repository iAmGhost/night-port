# 166 — I/O-port sampling finally shows the shipped audio feed: 324 transfers in 257-byte quanta, against the port's 73 one-shot blocks

The audio cluster has been measured from the *output* side for a long time (container mixer capture vs the port's
PCM witness), and docs/165 §5 concluded that the capture gives the board almost nothing to compare against.  The
missing half was the objective's "I/O-port sampling for DSP/mixer state".  It exists: dosbox-automation exposes
`GET /api/v1/io/port?port=0x02`, and the game's own audio driver programs the 8237 DMA controller by hand
(`fn_040b83`: clear the flip-flop on `0x0c`, address low/mid on `0x02`, page on `0x83`, byte count as *n-1*
low/high on `0x03`, then unmask on `0x0a`).  So the *changes* of those four registers are the shipped side's
transfer programming events, censusable without a debugger.

`capture_original.py --io 0x02,0x03,0x83,0x0a` (wired into `route.sh` as `RIG_IO`) collected them on
`/tmp/rig/g18` (`ep1_kill3`, 150 s, 25 Hz).  `scripts/rig/io_census.py` turns them into a census.

## 1. What was measured

```
samples: 2678   register changes: {0x02: 328, 0x03: 325, 0x83: 1, 0x0a: 1}
count-word pairings: 324   distinct sizes: 132   min 3   max 65433
most common: 2316 x18, 2830 x18, 3087 x17, 2573 x16, 2059 x12, 1802 x12, 3344 x10, 1031 x9, 3858 x9, 3601 x8
size ladder: 260, 261, 517, 774, 775, 1031, 1287, 1288, 1289, 1544, 1545, 1546, 1802, 1933, 2059,
             2316, 2572, 2573, 2574, 2830, 3087, 3344, 3601, 3858 ...
programming events per minute: {0: 9, 1: 126, 2: 189}
bytes programmed over 150.0 s: 2 932 694  (19 557 B/s of DMA feed)
```

* **The size ladder is `257·k + 3`.**  260, 517, 774, 1031, 1288, 1545, 1802, 2059, 2316, 2573, 2830, 3087, 3344,
  3601, 3858 are 257·1…15 plus a small residue.  A count register programmed as *n-1* with a 256-byte quantum is
  exactly that, and the residue is the same on every rung.  The shipped audio driver **streams**: it hands the
  board a fixed-size chunk repeatedly, and the chunk count grows when a longer sound is playing.
* **The DMA page is programmed once** (`0x83` changed once, `0x0a` changed once): the transfer *address* varies
  328 times while the page stays put — consistent with a fixed buffer whose contents are refilled, which is what
  `fn_04097e`'s cursor (`g_dat_63fa8 += len`, `g_dat_63f94 -= len`) does.
* **Feed volume**: 2.93 MB of programmed DMA in 150 s ≈ **19.5 kB/s**.

## 2. The port on the same drive

docs/164 §6 measured the port's feed on the same script: **73 feeds, 70 of them the same 3609-byte block**
(≈ 253 kB programmed), and docs/164 §2's PCM accounting measured 389 471 samples reaching the witness.  Placed
side by side:

| | shipped (`/tmp/rig/g18` I/O census) | port (`NIGHT_AUDIODUMP` census) |
|---|---|---|
| transfer programming events | 324 in 150 s | 73 in 150 s |
| distinct sizes | 132, on a 257-byte ladder | 4 (3609 ×70, 5348, 5031, 4916) |
| bytes programmed | 2 932 694 (19.5 kB/s) | ≈ 253 000 (1.7 kB/s) |
| page/mask reprogramming | once each | every feed |

The port is not streaming.  It hands the board one whole sample block per sound, roughly **an order of magnitude
less DMA traffic** than the shipped driver, with a size alphabet of 4 instead of 132.  That is the first
mechanism-level statement about the audio cluster that does not depend on the mixer capture, and it is consistent
with everything the capture kept saying: the shipped mixer is active 96 % of the time, the port's board stream is
active in 14 % of record windows.

## 3. What the measurement is *not*

* **Event counts are an upper bound, single sizes are weak.**  Sampling is at 25 Hz, not per instruction, so a
  burst can be missed or two bursts merged; the pairing of consecutive `0x03` changes into count words is a
  heuristic.  The *shape* (a 257-byte ladder, 132 distinct sizes, 9 → 126 → 189 events per minute) is the
  dependable part; `max 65433` is almost certainly a mis-pairing and is not used.
* **Port 0x22e is deliberately never read.**  It is the Sound Blaster read port, and reading it pops the DSP
  acknowledge byte the game is waiting on: sampling it would change the thing being measured.  `0x22c` was left
  out for the same class of reason.
* The shipped side's `0x02` values are recorded but not yet interpreted (328 distinct observations of one address
  byte); the address story has to come from the page + address pair, not one byte.

## 4. Next measurement

1. **Chunk cadence against the sample clock**: 19 557 B/s of 8-bit feed against the board rate the port programs
   (10 989 Hz measured at the pump line).  If the shipped feed rate is a multiple of its own programmed rate, the
   chunking is clock-paced, and the port's single-block feed is not merely different but *unsynchronised*.
2. **Per-sound totals**: group the shipped events into sounds by their inter-event gaps (9 → 126 → 189 events per
   minute already suggests intro / fight / dense-fight regimes) and compare each group's total bytes with the
   port's per-play block sizes.  If a shipped sound totals the same bytes the port's 3609-byte block carries, the
   defect is pacing; if it totals several times more, the port is dropping material.
3. **Same census on a title route**, where docs/165 §7 showed the shipped side plays disc music and the port's
   board produced two 0.24 s bursts: the census will say whether the shipped board even feeds during the title.

## 5. Grouped by inter-event gap: the shipped board is a stream, the port's is a block

Splitting the 324 events wherever the gap exceeds 1.5 s gives six feed bursts on this route:

```
start t      chunks   programmed bytes   span        quanta of 257
108263 ms       77          890 839      4084 ms        3466
135131 ms      107          656 228      6342 ms        2553
117534 ms       59          518 685      3094 ms        2018
124935 ms       63          413 319      3955 ms        1608
145839 ms        9          367 776       471 ms        1431
 49945 ms        9           85 847       470 ms         334
```

Two regimes, and they are not the same thing:

* **Long streaming bursts** (59-107 chunks, 257-byte ladder, 3-6.3 s): the driver feeding a sustained sound
  continuously, chunk after chunk.
* **Short high-quantum bursts** (9 chunks each, 85-368 kB in 0.47 s, i.e. individual transfers up toward 65 kB --
  a whole 0x10000 buffer at once): the driver re-arming a full-buffer transfer rather than stepping through
  quanta.

Against the port's 3 609-byte block per play (≈ 0.33 s of 8-bit samples at the rate its board programs), the
shipped side puts 85-891 kB into the board *per burst*.  The audio divergence is therefore architectural: the
shipped driver keeps a **stream** running through the board and refills it in 256-byte quanta, while the port
models audio as **one block per sound event** (docs/164 §6: 70 identical feeds of one block).  That explains why
the mixer capture is active 96 % of the route while the port's board witness is silent in 86 % of record windows,
and it does it without trusting the capture at all.

Caveat kept on the record: the 1.5 s grouping is a heuristic.  A group may be several sounds, and a group's byte
total is the sum of programmed transfer sizes, not decoded audio length -- but 85 kB against 3.6 kB is a factor of
23, far outside what grouping ambiguity can produce.

## 6. The DMA address walks forward on the shipped side and never moves on the port's

Pairing consecutive `0x02` changes into 16-bit address words:

```
0x02 changes 328 -> address words 327; DISTINCT addresses: 313
most common adjacent pairs: (2053, 2568), (2568, 3082), (3082, 3596)   -- each step +514
0x83 (page) constant: {1: 2678}      0x0a (mask) reads {255: 2678}
```

* **The shipped feed consumes a region forward.**  Almost every transfer is programmed at a different address, and
  the recurring adjacent triples step by exactly **514 = 2 × 257** bytes -- the same 257 quantum the size ladder
  showed.  The page never changes, so all of it lives in one buffer above 0x10000.  That is a stream being walked
  through the board, chunk after chunk, not a set of separate one-shot sounds.
* **The port's address never moves.**  docs/164 §6 recorded `dma_addr` for 60 consecutive transfers on the same
  drive: `0x381c` in one boot, `0x781c` in another, **constant within a run**.  The port's model copies each sound
  to the buffer base (`fn_04097e`'s `mem_copy(g_dat_63fb4, cursor, len)`) and arms a transfer at that base, so its
  address has exactly one value per run while the shipped side has 313.
* The mask register reading `255` in every sample is the idle "all masked" state and carries no information; it is
  reported only to show the census is not silently missing a register.

Two readings remain open and they are distinguishable, so the next measurement is named rather than guessed:

1. **Stream reading**: the shipped side plays long samples in forward-stepped chunks, which would make 2.93 MB of
   programmed DMA over 150 s a *continuous* board channel, and the port's 73 discrete effects are the wrong
   architecture entirely.
2. **Re-arm reading**: the same buffer is re-armed repeatedly and my address pairing mixes low/mid bytes across
   bursts, inventing variety.

The discriminator is the port's own copy cursor: if the shipped driver is walking forward, the *sequence of
transfer sizes* and the *sequence of address deltas* must agree (delta == previous size + small residue).  That is
checkable on the census already collected -- a mismatch rate near zero supports reading 1.
