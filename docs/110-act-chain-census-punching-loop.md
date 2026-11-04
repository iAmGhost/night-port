# 110: the punching loop is a .act-chain question, not a damage question

Route: `ep1_kill3` on both sides (`/tmp/rig/ents-orig/orig`, `/tmp/rig/ents-port2`, `/tmp/rig/ht1`), port
witness logs `/tmp/rig/kt1.log`, `/tmp/rig/act1.log`, `/tmp/rig/act3.log`. This turn supersedes one label
from docs/107 and adds one witness.

## 1. Reader bug on my side (not the rig's): cells with no shipped size read as zero

`diff_state.py` already handles symbols the shipped image gives no size for (`gs = gsz or hsz`, verdict
`SIZE?`, diff_state.py:211-222). My ad-hoc entity scripts sliced `min(guest_size, n)` bytes instead, and
`g_dat_63f14/0x63f1e/0x63f28` have `guest_size == 0` in `gen/rig_cells.json` -> 0 bytes -> "the port's input
ring is all zeros", which was false. Ad-hoc readers must use the same `guest_size or host_size` fallback.

## 2. States 0x0b/0x0c are chain (technique) states, not hurt states -- docs/107 label superseded

Every entry into them on the shipped side is entered from idle/walk with no attacker anywhere near and the
player undamaged (the four measured entries, route `ents-orig`):

```
t=191105  0x2 -> 0x0b   enemies within 40px: 0    hp(+0x29)=1000  absorb(+0x23)=99  +0x32=54  DAT_00062d58=51
t=208560  0x1 -> 0x0b   enemies within 40px: 0    hp=1000         absorb=89                    DAT_00062d58=53
t=220364  0x1 -> 0x0b   enemies within 40px: 0    hp=1000         absorb=59                    DAT_00062d58=53
t=225575  0x1 -> 0x0b   enemies within 40px: 0    hp=1000         absorb=29                    DAT_00062d58=51
```

`DAT_00062d58` (the chain-state id, docs/92) is 0x33/0x35 just before, i.e. the player is coming out of the
punch chain. So "missing hurt-stagger + infinite punching" and "player stuck when hit mid-kick" are, on the
state level, *the port never leaves the punch chain into the follow-up states* -- the follow-ups are entered
through the `.act` chain matcher, and that matcher is driven by the input ring, not by damage.

## 3. Damage on the player is already equivalent (a false alarm retired)

`NIGHT_HITTRACE=1` (port): 91 fn_01441e hits in the route, **56 with victim=PLAYER**, and the damage lands in
the absorb word exactly like the shipped data does:

```
port   HIT atk(type=31 st=29) vic(PLAYER st=01) slot=0..2 atype=1/2 dmg=1/3/6   +0x23: 99->96->90->89->86->80, +0x29 stays 1000
shipped entity records:        +0x23: 99 -> 89 -> 59 -> 29 while +0x29 stays 1000, later (0,970)
```

So the earlier "port player HP never drops" reading was the two-word HP model (+0x23 absorbs first, docs/107
section on the HP words), not a defect.

## 4. New witness: NIGHT_ACTTRACE=1, a failure census of the .act chain matcher

Instrumented the matcher `LAB_000339c5` inside `fn_03272f` (the code that turns a key sequence into a
technique state) -- per-entry reason census plus a histogram of the `target` state ids of fully matched
entries (`src/f/f.c`, printed at shutdown from `main.c`). One run, ep1_kill3, 14000 frames:

```
matcher visits 10239 | code0 mismatch 80012 | slot consumed 15274 | codeK mismatch 6731
level too high 71 | len != k 6802 | face byte 0 | hurt target != 0x99 0 | chain MATCHED 150
```

The matcher is **not** dead: 150 entries match and get a target. The ring that feeds it is live on both sides
with the same code alphabet and a similar stamp cadence:

| DS cell | shipped (974 samples) | port (1302 samples) |
|---|---|---|
| `0x63f14` ring codes | 933 nonzero, top `1d481d1d1d481d661d1d` | 817 nonzero, top `1d661d1d1d481d661d48` |
| `0x63f1e` consumed flags | 973, top `01000101010001000101`; all-ones only 41 | 1302, top `01010101010101010101` (485 = every slot consumed) |
| `0x63f28` per-slot level | `1e 000000 27 000000 0e 00` (small time deltas) | `34 04 0000 0000 ...` (0x434 then zeros) |
| `0x63f50` ring index | advances ~0.62 stamps/s | ~0.81 stamps/s |

Two real divergences sit in that table: the port more often has **every** ring slot flagged consumed
(`0x63f1e`, which the matcher tests as `if (g_dat_63f1e[s2] == 1) continue;` -- 15274 rejections in the
census), and the port's per-slot **level** (`0x63f28`, written as `g_dat_63f08 - g_dat_63f0c` in fn_034353 /
`g_dat_63f08` in fn_0348e2) is a large number where the shipped game stores small ones.

## 5. What decides the next step (the target histogram)

`act3` re-runs the same route with the target histogram. Two outcomes, both decisive:

* the matched targets in the port never include 0x0b/0x0c/0x36 while the shipped `.act` container has
  entries with those targets -> the defect is in the port's parse/layout of the chain container
  (`+0x000` count, `+0x004+i` len, `+0x068+i*4` codes, `+0x1f8+i*4` lvls, `+0x388+i*2` target, `+0x450+i`
  face gate), i.e. a resource-layout bug;
* the targets do include them -> the defect is the consumed-flag / level state above, i.e. the matcher's
  bookkeeping (`g_dat_63f1e`, `g_dat_63f28`) diverges first.
