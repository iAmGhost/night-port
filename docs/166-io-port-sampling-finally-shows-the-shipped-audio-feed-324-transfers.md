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

## 7. Reading 1 fails its test, and the failure points at a specific port path

First the semantics were verified rather than assumed.  The port's own transcription of the shipped programming
burst (`port/src/f/f.c:8472-8480`, carrying the guest addresses) writes:

```c
outb(0x0a, 5);                        /* 0x40c1a: mask ch        */
outb(0x0c, 0);                        /* 0x40c26: clear flip-flop*/
outb(0x0b, 0x49);                     /* 0x40c35: preset         */
outb(0x02, g_dat_63fac & 0xff);       /* 0x40c4b: addr low       */
outb(0x02, (g_dat_63fac >> 8) & 0xff);/* 0x40c64: addr mid       */
outb(0x83, (g_dat_63fac >> 0x10)&0xff;/* 0x40c7d: page register  */
outb(0x03, g_dat_63f98 & 0xff);       /* 0x40c93: count low      */
outb(0x03, (g_dat_63f98 >> 8) & 0xff);/* 0x40cac: count high     */
outb(0x0a, 1);                        /* 0x40cbb: unmask ch      */
```

So `0x02` really is the channel-1 address (written twice through the flip-flop) and `0x03` really is the count --
the census is measuring what it claims.

**The forward-walk test, run against the census already collected:** pair each programming event with the next one
and ask whether the address delta equals the previous transfer's size.

```
aligned programming events (both registers within 400 ms): 327
address delta == previous transfer size:  44        mismatched: 282
residues scatter: +39564, -59971, -557, +10302, -41672, -22178, ...
```

Reading 1 (a contiguous stream walked forward) **fails**: 44 of 326 steps fit, 282 do not, with residues tens of
kilobytes wide.  The shipped driver does not walk -- it **seeks**: each transfer is programmed at a fresh offset
inside the one fixed page (`0x83` constant at 1), 313 distinct addresses across 324 events.

That reframes the divergence, and it lands on a specific place in the port:

* Both feeders in the port come from the **same walker**: `fn_040839` decodes a device command stream and
  dispatches `kind == 1 -> fn_04097e(cursor, len)` (arm and start a transfer, `f.c:8514`) and
  `kind == 2 -> fn_040b83(cursor, len)` (**one DMA chunk** of a device transfer, `f.c:8515`).
* On the measured drive the port ran `fn_04097e` **73 times and `fn_040b83` zero times** (the `sfxcopy` witness in
  docs/164 §6).  The shipped side's shape -- many transfers, sizes on a 257-byte ladder, addresses varying while
  the page stays fixed -- is the **chunk** shape, i.e. the kind-2 path, repeatedly.
* So the open question is now concrete and narrow: *why does the port's command stream never contain a kind-2
  chunk on this route*, when the shipped side's DMA traffic looks like nothing else.  That is a claim about the
  command stream the port feeds the walker, testable by censusing the `kind` values `fn_040839` actually sees.

Next measurement named: a `kind` census witness at `fn_040839`'s dispatch (counts per kind, plus the cursor and
stream length), compared against 324 shipped programming events.  If the port only ever decodes kind 1, the audio
cluster's root cause is the command stream it is handing the walker, not the DMA model, not the mixer, and not the
sample assets.

## 8. The `kind` census: the port's command stream is 81 kind-1 commands and no kind-2 at all

A witness at `fn_040839`'s dispatch (`NIGHT_AUDIODUMP`, `port/src/f/f.c:8512`) prints every command the device
command stream actually carries.  On `ep1_kill3`, 6000 frames:

```
decoded commands: 81      kind=1 x81        kind=2 x0
lengths:          3609 x79,  5348 x1,  5031 x1
```

That is the shape mismatch, localised.  The port's stream is **one kind-1 command per sound, carrying the whole
block** (3 609 bytes 79 times).  The shipped side programs **324 transfers**, sizes on a 257-byte ladder, 313
different addresses, one fixed page.  The walker's own dispatch explains why the two cannot agree:

```c
if (kind == 1) { fn_04097e(cursor, len); return; }   /* 0x4089a -- and RETURN */
if (kind == 2) { fn_040b83(cursor, len); return; }   /* 0x408ad -- and RETURN */
```

Both arms **return**, so one call to the walker handles exactly one command.  The port therefore needs one stream
command per transfer, and it only ever has kind-1 commands; the shipped side achieves 324 transfers without any
kind-2 command in sight, which points at the *continuation* machinery rather than the stream content:

* `fn_04097e` already models a chunked feed: `g_dat_63f94 = total - 2` (remaining), copy a chunk capped at
  `0xffff` into the buffer base, `g_dat_63f94 -= len`, `g_dat_63fa8 += len` (cursor advances), `g_dat_63f98--`
  (the device is told one byte fewer), then program DMA -- and `fn_040b83` is the *continuation* of exactly that
  block ("one DMA chunk" of the same remaining/ cursor state).
* In the shipped game the continuation is driven by the board's acknowledge / timer path: the driver re-programs
  the next chunk when the previous transfer completes.  That is where a 257-byte ladder and a shifting address
  with a fixed page come from naturally.
* **Hypothesis to test, not to assume: the port never services that continuation.**  It arms once per sound
  (81 arms, 81 transfers) and the remaining/cursor state it maintains is never advanced by a second chunk,
  because nothing re-enters `fn_040b83` -- which the `sfxcopy` witness already measured at **zero calls**.
  The test is the acknowledge traffic: count `inb` hits on the Sound Blaster read/status ports
  (`0x22a`, `0x22e`) and the ISR firings per transfer.  If the port reads the ack and still never continues, the
  defect is in the continuation logic; if it never reads the ack at all, the defect is that the handshake the
  shipped driver depends on is not modelled.

## 9. Patch defect caught while doing it

The first version of this witness printed nothing, and the reason was mine, not the game's: the `fprintf` was
written on the line *after* a block comment started, so it sat inside the comment.  The tell was
`strings port/test/night | grep -c cmdstream` returning 0 while `grep -c cmdstream port/src/f/f.c` returned 1 --
a witness that is not in the binary.  Added as a standing check: **confirm a new witness exists in the built
binary before believing its silence.**

## 10. No hidden continuation path: the gap is trigger frequency, not wiring

Three checks, all run against the shipped binary rather than assumed.

1. **`fn_040b83` has exactly one caller in the shipped code.**  `scripts/audit/call_sites.py --target 0x40b83`:

   ```
   callers of 0x40b83: 1     --> call at 0x408ad: call 0x40b83   (inside fn_040839, the walker)
   ```

   So the shipped game reaches the chunk feeder *only* through the same one-command-per-call walker the port has.
   There is no continuation routine the port failed to port.

2. **The port's streams really do end where the walker stops them.**  A peek witness at the dispatch prints the
   four bytes at `cursor + len` before returning: **73 out of 73 read `00,00,00,00`**, i.e. the kind-0 terminator.
   The assets the port hands the walker contain one command each; nothing is being left unread.

3. **The trigger wiring is present in the port.**  `fn_04185e` has **9 callers** in the shipped binary, and
   `fn_0113e7` alone contains **11 call sites** to it (`0x116c1, 0x116e4, 0x11707, 0x1172a, 0x1174d, 0x11770,
   0x11793, 0x117b6, 0x117d9, 0x117f9, 0x11819 -- 23 bytes apart, a dispatcher table shape`), with `fn_01441e`
   (the hit code) contributing another.  The port calls `fn_04185e` from `fn_03f6bc`, `fn_0113e7`, `fn_03a70e`,
   `fn_03a224`, `fn_03a829`, `fn_01441e`, `fn_038445` and `fn_038f3d`.

Put together: shipped programs 324 DMA transfers and the port 73-81 feeds on the same route, one transfer per
walker call on **both** sides, and neither side has an unread continuation.  The difference is therefore **how
often the game asks for a sample** -- roughly 4x on this route -- not how the ask is carried out.

Caveat stated: 324 DMA programming events are used as a proxy for shipped walker calls, valid because the port's
own census shows a strict 1:1 (73 feeds, 73 transfers).  If a single shipped arm programs more than one transfer,
the ratio overstates the trigger gap.

Next measurement named: a per-call-site trigger census in the port (count each `fn_04185e` call site
separately -- the 11 sites inside `fn_0113e7`, plus `fn_01441e`, `fn_03a829`, ...), compared against the shipped
programming events *grouped by time* (9 -> 126 -> 189 events per minute, docs/166 §1).  A site that never fires
in the port while the shipped minute-rate climbs is the specific missing trigger, and the docs/120 coverage
census can already say whether that caller was ever entered.

## 11. Per-call-site trigger census: 61 of 63 sample requests come from ONE site

`fn_04185e` now prints `__builtin_return_address(0)` under `NIGHT_AUDIODUMP` (a `playsite` line).  Grouping the
returns on `ep1_kill3`, 4000 frames:

```
   61 requests from one call site   (host return 0x104677418)
    1 from 0x10467b9f8
    1 from 0x10467b8ec
```

Which is the sharpest audio finding yet, and it is a *distribution* statement: the shipped side programs 324
transfers across the route while the port's 63 sample requests on a shorter run come overwhelmingly from a single
call site.  The 11 sites `fn_0113e7` carries in the shipped binary (`0x116c1..0x11819`) are wired in the port --
they are simply not being taken.

Two honest gaps, both filed rather than papered over:

* **The site is not yet named.**  The returns are host addresses in a PIE binary and `nm` gives file addresses;
  mapping needs the load slide (`atos -o port/test/night -l <slide>`, or printing the name from the generated
  `src/gen/fn_table.c`, which already stores function pointers *and* their names side by side).  The count is
  trustworthy; the attribution is not yet.
* **The witness trap repeated itself.**  The first version of this witness printed nothing because its `fprintf`
  again sat inside a trailing multi-line comment opened on the `if` line -- the exact defect recorded in §9.  The
  binary check (`strings port/test/night | grep -c playsite` returning 0, then 1 after rewriting the comment
  above the statement) is what caught it, which is why the rule exists.

## 12. Naming the site failed, and why -- recorded as a rig gap

Two attempts, both rejected:

* **`nm` mapping.**  The first symbol grab used `grep " t fn_"` and found **nothing**, because Mach-O prefixes C
  symbols with an underscore: the correct pattern is ` t _fn_`, which yields 410 `fn_*` symbols.  The addresses
  still did not line up.
* **PIE suppression.**  Building with `-Wl,-no_pie` did not take effect: the dominant return moved from
  `0x104677418` to `0x1049df418` between two runs, i.e. the binary is still being loaded at a randomized base.
* **Slide search.**  Trying every slide that aligns the dominant return onto an `fn_*` symbol, scored by how many
  returns land within 0x4000 after a symbol, gives several candidates that all resolve at **offset 0x0** in two
  different functions (`fn_showscreen_gate`/`fn_107b9`, `fn_0003fade`/`fn_03690b`, `fn_0450da`/`fn_0353d0`, ...).
  A call site at offset 0 in two unrelated functions is not a measurement, it is an under-determined fit: two
  returns cannot pin a slide.

Standing fix for the rig, named rather than guessed: **resolve the name in the process**, not offline.
`port/src/gen/fn_table.c` already stores the function pointers and their names side by side, so the witness can
binary-search that table against `__builtin_return_address(0)` and print the symbol directly -- no slide, no `nm`,
no PIE arithmetic.  Until that is done the census statement stays exactly as strong as the data supports:
**61 of 63 sample requests on this drive come from one call site**, and that site is unnamed.

## 13. The site, named in-process: `fn_03f6bc+0x208`

`dladdr()` resolves `__builtin_return_address(0)` to a symbol and offset inside the running port, which is what
§12 said to do and it works with no slide, no `nm`, no PIE arithmetic:

```
   61 playsite fn_03f6bc+0x208
    1 playsite fn_03a224+0x4b0
```

`fn_03f6bc` is the **timed-event tick walker** (the function whose 61-dword tick table at `DS 0x53200` had an
out-of-bounds walk, fixed in docs/126).  So on this drive the port asks for a sample from the *timed-event*
path 61 times and from one other site once -- while the shipped binary's biggest cluster of sample requests,
the 11 sites inside `fn_0113e7` (`0x116c1..0x11819`, described in the port's own header comment as "fn_0113e7's
ten combat-SFX slots"), contributes **nothing** here.

That is not a coverage gap: `fn_0113e7` is *not* in `scripts/audit/reports/gap_frontier.json`'s never-entered
set, so the fuzz census does enter it.  The divergence is that its sound-trigger arms are not reached on this
route -- which is the same gating family as docs/136 (a hit that never reaches the code that applies it) and
docs/162 §13 (the accumulator re-arm), and it is now measurable as a per-site count on both sides.
