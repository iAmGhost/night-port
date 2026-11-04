# docs/153 — the command-list unlink path is faithful; the port enters it a few extra times

docs/139 filed "the port has 14 `rec[0]=0` stores where the shipped function has 12" together with the
`DAT_00063e14` counter wandering 67 → 66 → 65 → 64 → 62 → 61 while the shipped counter holds 67. A call witness
on the counter's only writer shows the mechanism is not the defect, and narrows what is left.

## 1. What the counter's writer looks like, and how faithfully it is ported

`DAT_00063e14` has exactly one decrement in the shipped binary (`0x0003e1f8`, inside `fn_03e14c`) and one
comparison (`0x0003e169`, `cmp dword [DAT_63e14], 1`). Every path in `fn_03e14c` jumps to 0x3e1f8 — 0x3e190,
0x3e1b4, 0x3e1d8 and the fall-through all reach the DEC — so "decrement on every call" is the shipped
behaviour, and the port's single unconditional `g_dat_63e14--` (`f.c:2006`, comment `DEC [0x63e14] (always)`)
matches it.

Callers: exactly two in the shipped binary (`call_sites.py --target 0x3e14c`): `fn_03ed6f` (0x3edd2) and
`fn_03ee0a` (0x3ee53), both feeding `a0 = DAT_00063e20` / the node's `+0x51` neighbour and both guarded by
`cmp [DAT_63e20], 0 ; je skip`. The port has exactly three references, all matching those two call sites
(`f.c:2029` -> `0x3ee4e/53/58`, `f.c:26400` -> `0x0003edd2 / 0x0003edd7`) — **no fabricated call site**.

## 2. Witness: the drain is a faithful, identical 21-call round

`NIGHT_WALKTRACE` now also traces `fn_03e14c` (`g_cmd_unlink_calls`):

```
UNLINK #1  a0=1631896 counter_before=88 e18=1631896 e1c=3831160 e20=1631896
UNLINK #2  a0=1631992 counter_before=87 e18=1631992 e1c=3831160 e20=3831160
...  (a0 advances by exactly 0x60 = 96 B per call: the command records)
UNLINK #21 counter_before=68
WALK unlink: fn_03e14c calls=21 counter_final=67
```

Run: `ep1_kill3`, `--warp-stage 1`, 4000 frames.

The shipped side of the same drive, read out of its DS cells (`/tmp/rig/g12`, 424 samples):

```
g_dat_63e14:  ORIG {67: 397, 88: 24, 0: 3}        transitions: 0 -> 88 (x1), 88 -> 67 (x1)
              PORT {0: 544, 61: 494, 67: 341, 64: 135, 88: 107}
g_dat_63e18:  ORIG 3683288 (x397)                 PORT 1632952 (x1058)     (both live heap offsets; different arenas)
g_dat_63e1c:  ORIG 5723656 (x397)                 PORT 3831064 (x1058)
g_dat_63e20:  ORIG 0 in 424/424                   PORT 0 in 1709/1709      (non-zero only inside the drain)
```

So the shipped game *also* loads 88 and drains to 67 — one round of 21 unlink calls over stride-96 records —
and the port's witness reproduces it exactly: 21 calls, 88 → 67, ending at 67. docs/139's framing of the
`rec[0]` store count as the cause of the counter excursion is superseded: the drain round is equivalent.

## 3. What is actually left open

The residual is **frequency, not mechanism**: on the 150 s anchored route the port holds 61 in 494 samples and
64 in 135, i.e. its counter ends up ~6 unlink calls below the shipped game's resting 67 and stays there, while
the shipped counter is never observed below 67 in 424 samples at 4 Hz. Both sides' `DAT_00063e20` reads 0 at
every sample, so those extra calls are transient (`e20` is non-zero only inside a drain — the witness shows
that directly). The open question is therefore which room/stage event makes the port re-enter the unlink path:
the two shipped call sites are `fn_03ed6f` (the room command pump) and `fn_03ee0a` (list teardown), and the
port reaches them ~6 more times per route.

That also reframes docs/137/142's population divergence: the command list is the thing that spawns room
enemies, and the port consuming ~6 extra records per route is the same order as the extra type-1 enemy
population measured per sample (0.97 shipped vs 2.33 port, docs/152 §4b).

## 4. Reproduce

```sh
make app
NIGHT_WALKTRACE=1 ./build/night --res-dir ./NIGHT --script scripts/rig/drives/ep1_kill3.txt \
    --warp-stage 1 --frames 4000 2>&1 | grep -E "^UNLINK|^WALK unlink"
uv run --with capstone scripts/audit/call_sites.py --target 0x3e14c --before 8
uv run --with capstone scripts/audit/disasm.py --entry 0x3e14c --end 0x3e20f
```
