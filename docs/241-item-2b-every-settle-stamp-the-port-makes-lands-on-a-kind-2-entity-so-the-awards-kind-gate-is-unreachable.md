# 241 — Item 2b closes on a hard number: every settle stamp the port makes lands on a kind-2 entity, so the award's `kind == 1` gate can never be reached

One witness run, no code change (`NIGHT_SET30=1`, whose `set30_trace` already prints site + kind + type_idx + hp +
state for each `+0x30` store, `f.c:24823`) on the canonical drive, 300 s, pinned clock, `/tmp/rig/x6`:

```
83 set30 stamp lines in total
   76  site=f.c:24786   kind=2 type_idx=201        (fn_013c4d's stamp at shipped 0x141ca, on the hit victim)
    6  site=f.c:25088   kind=0 type_idx=0 hp=997/989/999 state=0xb/0xc  player=1   (fn_01441e 0x1499f, the player)
    1  site=f.c:25183   kind=0 type_idx=0 hp=989 state=0x15 player=1               (fn_01441e 0x14c1a, the player)
```

## Why that is the end of the road for the award path

* `fn_013c4d` resolves contacts and stamps the **victim's** `+0x30 = 1` (0x141c7-ca ✓) ✓ every stamp this run made,
  all 76, is on a record with **kind = 2, type_idx = 201** ✓ — the player's own reaction/hit entities ✓ not on a
  kind-1 enemy ✓
* `fn_01441e`'s hit walk explicitly skips kind-2 records (`if (e[0] == cur[0] || cur[0] == 2) goto hitnext` ✓
  0x146d? ✓ modelled `f.c:25165` ✓ faithful ✓) — so a stamp on a kind-2 record is invisible to the settle stage ✓
* the award's own kind gate (`cmp byte [a0+0x00], 1` at 0x15315-0x1531f ✓ `f.c:8255` ✓) therefore has no candidate ✓
  `fn_015295`'s census (`+0x30!=1 952 / +0x25==ff 231 / +5!=0 51 / +0x11<=0x15 5 / +0x00!=1 1 / reached 0`) is
  consistent: the calls that do arrive are player-side ✓
* the only stamps on a kind-1 victim would come from contacts the gates reject: docs/195's `GATE4` census
  (352 at 150 s, 599 on the long hit-trace run, `dl = 7` in every case) and docs/238 §2 (the port's enemies occupy
  32 Y values across 139-193 while the player sits at 172 on both sides ✓ shipped enemies caught at 193/150 ✓) ✓

So the six-step chain for item 2b is now measured end to end, with the break in one place:

`enemy Y placement (docs/238)` → `fn_013c4d` Y gate rejects every enemy pair (docs/195 §1a, docs/238 §1: the gate
itself is byte-faithful ✓) → no kind-1 record is ever stamped `+0x30 = 1` (this document ✓) → `fn_01441e`'s settle
block never runs for a victim (docs/240 ✓) → neither award writer (0x15054 flat 5, 0x15067/0x15070 per-type ✓) runs
→ EXP stays 0 and the per-type award is untestable ✓

## Fix site, and the measurement that must precede it

The fix belongs in the enemy's vertical placement, i.e. docs/238 §4 candidate (1): `fn_01194c` (the per-entity step
dispatch, `f.c:26146`) and `fn_011827` (0x11827, whose callers push the dy — `0x119b6 push 4; 0x119bd mov ebx,-4;
0x119c2 mov edx,-10` ✓) — specifically the shipped's `add [e + 9], ...` updates and any ground clamp the shipped
applies that the port does not ✓ (`scripts/audit/field_writers.py --disp 0x9` **with today's operand-0 fix** ✓
docs/239 §1) is the census that lists those writers ✓

Green basis (repeatable, dump-side only — docs/239 §4 forbids single-run in-process counters):
`scripts/rig/exp_award.py --orig /tmp/rig/ents-orig/orig --port <new run>` must show a port dying record with
`type_idx = 31` carrying `+0x32 = 10` (shipped: 31 such samples ✓ port today: `[0]` ✓), and `NIGHT_SET30`'s stamp
lines must start landing on `kind=1` records ✓ at least two runs per side ✓
