# 190 — Item 2b from the port's own award witness: the EXP award never fires on the canonical route

The port already carries the award witness docs/126 introduced (`g_exp_awards`, `g_exp_added`, plus the display-fold
counters), printed at the end of a bounded run.  Run: `port/test/night --script drives/ep1_kill3.txt --frames 9000`
(the 150 s canonical route, `NIGHT_SEED=13579`, `NIGHT_ACTTRACE=1`, same binary and cadence as `g33`):

```
exp award path: adds=0 added=0 commits=0 committed=0
```

`ep1_kill3` kills enemies (it is the route whose 1 500 samples / 413 shipped states every other oracle uses), and the
award path contributes **zero** awards and **zero** EXP through the whole 150 s.  Combined with docs/186 §2 (the
victims' award operand `+0x32` holds `0` or `5` in the port's records, 174 and 927 samples) and docs/189 §1 (the
shipped award adds the victim's `+0x32`, which in shipped records holds `0x33..0x36` = 51..54 in 327 samples), item
2's EXP half is now detected *and* localized to a gate chain rather than to a data table.

## 1. A gate census for the award (and two corrections to what I wrote before this)

`fn_015295` (`f.c:8173-8192`) mirrors five shipped tests before the `add [player+0x52], victim[+0x32]` at `0x15453`.
Counters were added at each of them (`g_exp_gate_hits[6]`, `f.c:23226`, printed by the act-census block in
`main.c`), and the route was run with the **canonical rig environment** (`SDL_AUDIODRIVER=dummy NIGHT_SEED=13579
NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, the same `PORT_ENV` `route.sh` uses for `g33`):

```
exp award path: adds=0 added=0 commits=0 committed=0
exp award gates (fn_015295, docs/190): +0x30!=1 1108  +0x25==ff 231  +5!=0 51  +0x11<=0x15 5  +0x00!=1 1  reached 0
```

`fn_015295` is entered 1 396 times on this route and **79 % of those calls (1 108) are rejected by the very first
test**, `victim[+0x30] != 1` (guest `0x152d1/152dc`, the dying/settle flag).  Nothing reaches the award.  The
display-fold counters are 0 as well (`commits=0 committed=0`, the `0x00038fff` path that moves `+0x52` into the HUD
field `+0x2b`), so on this route EXP neither accumulates nor is committed.

Correction to the first draft of this document, which I wrote from the record dumps before running the census: it
claimed the `victim[+0x00] == 1` test (guest `0x1531c`) was the blocker because every admissible record in the dumps
has `+0x00 == 0`.  The census says that gate rejects **one** call out of 1 396.  The records the dumper admits and the
records `fn_015295` is called with are not the same set -- exactly the identity warning docs/188 §3 records.  A
gate census in-process beats a dump-side inference here.

Second hygiene note, learned the hard way in the same minute: the same command *without* `NIGHT_RIG_STEP_CALLS` and
`NIGHT_RIG_FPS` gives a completely different census --

```
exp award path: adds=1 added=10 ...   +0x30!=1 0  +0x25==ff 150  +5!=0 51  +0x11<=0x15 5  +0x00!=1 0  reached 1
```

so the unpinned host clock changes which entities are mid-action when the award is attempted.  Award-path numbers
are only quotable with the pinned environment (the docs/168 clock basis), and `route.sh` pins it; ad-hoc invocations
must repeat it.

## 2. What the shipped side contributes to the same question

Shipped cannot be counted this way (no in-process witness, no debugger capability), but its record census is the
other half: on `g33` the award operand `+0x32` holds `0x33..0x36` (51..54) in 327 of 711 admissible shipped samples
and `0` in 138, while the port's records hold `0` (927) or `5` (174) in 1 101 samples (docs/186 §2).  Combined with
docs/189 §1 -- the award is `player[+0x52] += victim[+0x32]`, a saved-state id, plus the self-award
`player[+0x52] += player[+0xbb]` -- the item-2 statement is now: **the port neither fills the award operand with the
values shipped carries nor reaches the award at all on the canonical route.**

The next step is the dying-flag gate itself: `+0x30` is set to 1 by `fn_035c5c` (init pass) and to 2 by `fn_01441e`
(`0x1504d`), and docs/125/117 already carry the fallen-state question.  Measuring the `+0x30` write sequence at the
kill step (witness, not dump) on the port side, against the shipped `+0x30` census (`1` in 46 of 711 admissible
samples), is the concrete next measurement.
