# 139 — The port consumes room-script commands the shipped run leaves pending

docs/138 §4/§5 left one question with a discriminator: is the `DAT_00063e14` gap (shipped pinned at 67, port
wandering down to 61/62) a real behavioural divergence or a sampling-cadence artifact? A dedicated route was
run for that.

## 1. Cadence ceiling of the shipped dumper (rig capability, worth recording)

`RIG_HZ=10` on the shipped side yields **3.2 Hz** in practice (980 samples over 304 s): each sample costs a
DS read + a heap-block read + a frame read through the REST API, and the loop cannot go faster. The port's
dumper reaches 7.2 Hz. So "matched cadence" is not achievable at 10 Hz on this machine; comparisons have to be
robust to a ~2x cadence factor rather than sample-for-sample.

## 2. Result: the counter gap is not cadence

```
shipped  DAT_00063e14: 67 x920, 88 x56, 0 x4        (980 samples, 3.2 Hz)  -- no excursion below 67
port     DAT_00063e14: 67 x752, 0 x473, 61 x427, 66 x231, 88 x109, 62 x27, 64 x26
                                                        (2047 samples, 7.2 Hz)  -- 35% of samples below 67
shipped  DAT_00063e18 (command-list head) nonzero: 976 / 980
port     DAT_00063e18 nonzero: 1574 / 2047
```

If `fn_03e14c`'s `DEC [0x63e14]` were executing at the same rate on both sides, a 3.2 Hz sampler would still
catch some intermediate values out of 980 samples; the shipped trace shows **none** in 980 samples while the
port shows 711 in 2047. Combined with docs/138 §5 (all four guest accesses to that cell are placed faithfully,
no missing increment), the reading is behavioural: **the port unlinks and frees room-script command records the
shipped run leaves in the list.**

## 3. The same route says the port also runs a bigger entity population

Live entity records per sample (the `+0x13f` chain walk, capped at 12 slots):

```
shipped: 3 x452, 1 x308, 2 x117, 4 x90, 5 x6, 7 x2, 6 x1, 0 x4      -- 99% at 4 or fewer
port   : 3 x866, 0 x473, 6 x330, 4 x114, 1 x109, 7 x62, 5 x61, 9 x28 -- 24% at 6 or more
```

Two independent signals -- more command consumption and a larger live entity population -- point at the same
mechanism, and it is the mechanism docs/137 §4 and docs/138 §1 needed: extra `enb-1` (type 0x24) enemies exist
because the port let their spawn commands through, and their hits are what resolve hit code 11 against the
player (docs/137 §2) and set the dying flag (docs/136 §3).

## 4. Fix target, named

The consumption test lives in `fn_03ed6f` (f.c:26328-26345): a command is unlinked and freed when the
interpreter has cleared its opcode byte (`*(hb + g_dat_63e20) == 0`), via `fn_03e14c` + `fn_041c28`. The
interpreter clears the opcode byte in six arms of `fn_03e20f` after a spawn. Since docs/138 §2 verified those
arms' *window tests* against the shipped disassembly, what is left is the **retry/pending semantics**: the
port's arms must be clearing the opcode byte (and thus consuming the command) in cases where the shipped arms
leave it set, e.g. after a spawn attempt whose anchor computation took the `camx + 0x15e`/`camx - 0x1e` form
versus the `cgate == 1` form, or after the `stamp_gate` variant (`0x0003e486` / `0x0003e583`). The audit is a
per-arm comparison of *where each arm clears `rec[0]`* against the shipped instruction sequence, not of the
window arithmetic -- and the oracle for it is now available: `DAT_63e14 / 63e18 / 63e1c / 63e20` on both sides
plus the entity population census.
