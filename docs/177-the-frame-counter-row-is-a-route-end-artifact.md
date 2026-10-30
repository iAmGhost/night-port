# 177 — The frame counter row is a route-end artifact, not a port defect

`DS 0x63360` (the frame counter whose `%10` gates the periodic board output, and whose ticks the idle timer of
docs/167 §11 couples to) has stayed in the state oracle's residual list.  Reading the whole series on the canonical
route `/tmp/rig/g27` (`ep1_kill3`, 150 s, `RIG_HZ=10`, 853 shipped samples / 1500 port samples) shows why the row
cannot be closed by changing the port.

## 1. Both sides' write sites are already reproduced

Everything the port writes to the counter maps to a shipped site (comments carry the guest addresses):

```c
g_dat_63360 = 1;                                              /* 0x0003a908  f.c:23251 */
g_dat_6336c = 0; g_dat_63360 = 0; g_dat_63e28 = 0; ...       /* 0x0003a95a-  f.c:23259 */
g_dat_6336c = 0; g_dat_63360 = 0; g_dat_63e28 = 0; ...       /* 0x0003a9e1-  f.c:23262 */
g_dat_63360 = g_dat_63360 + 1;                                /* 0x0003ab6c-  f.c:23324 */
```

No shipped *absolute* store to the address exists (`disasm.py --search 60330600 --all` → 0 hits), which is the
docs/133 §4 base-relative convention, not an absence of writes: the shipped writes are the ones the port cites.

## 2. What the two runs actually do

```
shipped:  counter is 0 until sample 61, then 1 -> ... -> 6986 by sample ~727, then FROZEN at 6986
          for the remaining ~126 samples (its timeline runs to t = 284.0 s)
port   :  counter still climbing at the end of the route -- samples 1450/1470/1499 read
          1102 / 1292 / 1567 at t = 222.9 .. 227.9 s -- with 3 regressions (resets) over 1500 samples
```

Shipped's counter stops being advanced near the end of its run; the port's is still live at the end of its run, and
the port reaches the reset sites three times while shipped's series shows one long uninterrupted climb.

## 3. Verdict

The row compares two runs that **stop in different phases**: the shipped timeline extends to 284 s while the port's
ends at 227.9 s, and the shipped counter freezes exactly where its drive content runs out.  With identical write
sites and identical increment coupling (docs/167 §11 verified the 1:1 relationship on the port), the difference is
which phase each side reaches, not what either side computes.  This belongs with docs/172's ordering cluster: it
closes when both sides are cut off at the same *event* rather than the same wall-clock duration.

That distinction matters for the residual count: of the 34 rows left, this is one where the rig -- not the port --
is the thing that has to change.  Named test: end the drive on a state predicate (the counter-freeze transition
itself, or the room-command drain) and re-measure, instead of running both sides for 150 s of clock.
