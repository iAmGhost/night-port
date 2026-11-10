# 434 — the stall has a name: the port stops presenting while it loads a stage, and the load is ~6 MB at ~2.5 MB/s

`0.2.10`'s witness line fired on real hardware, and it answered the question in six lines:

```
STALL 0.6s | res=opening.fli  seq=138 | res_calls=530  res_kb=2252 | disc ops=7   sectors=162  | frames=2149 ticks=32875 pumps=2149
STALL 1.4s | res=title1.pcx   seq=141 | res_calls=808  res_kb=3509 | disc ops=40  sectors=558  | frames=2348 ticks=35536 pumps=3520
STALL 3.0s | res=1s1.pcx      seq=174 | res_calls=1203 res_kb=5872 | disc ops=88  sectors=1290 | frames=2428 ticks=37731 pumps=3600
STALL 0.5s | res=stage1.pcx   seq=175 | res_calls=1274 res_kb=6156 | disc ops=107 sectors=1518 | frames=2486 ticks=38956 pumps=3658
```

## 1. What the numbers say

**The game's clock keeps running while the screen does not.** Between the 1.4 s and the following 0.5 s
witness: `frames +20`, `ticks +1007`. A thousand ticks of a 490 Hz clock is **2.05 seconds** of game time in
which 20 frames reached the panel. So this is not the loop dying and it is not the `delay_units` hold of
docs/416 (which presented ~57 times a second precisely because it calls the host pump). The loop is alive, the
pump is being entered, and nothing presents.

**The traffic during the gap is file reads.** Across the stall window: `res_calls 808 -> 1274` and
`res_kb 3509 -> 6156`, i.e. **~2.6 MB in ~466 calls**, and `disc ops 40 -> 107`. At the batch-level cost this
route measures for disc operations (mean 26 ms/op, docs/425/429), 88 ops is ~2.3 s -- which is essentially the
whole 3.0 s stall. The largest witness sits exactly between `res=1s1.pcx` (the stage-1 slide) and
`res=stage1.pcx` (the stage card): the port loads a stage, and while it loads, no present happens.

So the 2.998 ms... 2.998 **second** gap in docs/433's p99/max is not a scheduling accident, it is stage entry.

## 2. The two things that follow, and why I want to do them in the other order

**(a) Is this even a bug?** The original also had to read the same ~6 MB from the same era of optical drive,
and docs/415 already established the original *holds* for 3.91 s on its stage card. A port that spends ~3 s
loading stage 1 may be faithful. Before I spend another turn optimizing, the fair question is what the
original's gap looks like when measured the same way -- screen-change timeline during stage entry, on the
calibrated container route (docs/414: factor 1.000). If the original also blanks for seconds, the work is
cosmetic; if it doesn't, I have a throughput bug and should chase it.

**(b) Regardless of (a), the freeze can be made not to freeze.** The load path never yields to the host, so
the panel shows a still frame for 3 s while the mixer ring and the clock carry on. Presenting during the load
would be cheap and, with `PRESENT_SKIP=1` on, would re-present the *same* frame -- a no-op for the picture, not
a half-drawn card. That is a small change in the host's resource-read path with the same re-entry guards docs/426
established (`g_in_present`, one present per blocking read, never during a draw). It does not shorten the 3 s;
it removes the "is it hung?" feeling, which is what people actually report.

## 3. Also in this run, for the record

`ROW_SKIP=0` confirmed inert (`rows 0/0`, and the phase table is back to the pre-experiment numbers: 640x480
33 fps at 30.0 ms copy, gameplay 44 fps at 6.5 ms). Choppiness unchanged (median 38, p90 65, p99 1 376, max
2 978 ms) -- as expected, since nothing behavioural changed. The row-skip line stays closed (docs/433).
