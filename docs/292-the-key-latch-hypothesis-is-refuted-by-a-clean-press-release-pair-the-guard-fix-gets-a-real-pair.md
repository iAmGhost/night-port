# 292 — The key-latch hypothesis for item 6 is refuted by a clean press/release pair; the walking-body guard fix gets a real before/after pair

## 1. What /tmp/rig/le4b appeared to show, and why it was not evidence

On `ep1_downonly` (walk to the left bound, then 118-161 s of Down-only makes ✓) the shipped's directional latch
cells looked asymmetric ✓✗:

```
SHIPPED le4b:  63ed3 (Left, scan 0x4b) = 1 from t=109690 to the END of the run ✗ never cleared
               63ed8 (Down, scan 0x50) toggles 1/0 cleanly on every press pair (125266, 127112, 129866, …) ✓
PORT  le4b:    63ed3 toggles 1/0 on every press pair (60 transitions ✓), returns to 0 after the last release ✓
```

That asymmetry explained the shipped's two-step descent (`172 → 182 → 195` at t=124964-125574 ✓ with state 2,
x=15 ✓ — `LAB_0003284e` admitted by the latched Left ✓, then the vertical arm at `0x328cc` runs `fn_01586b(pl,
+step)` ✓) and the port's **zero** Y-change events ✓ — i.e. "the shipped keeps walking with Left latched at the
left edge, so it keeps refreshing `+0x1b = 1` (`0x328ff`) and stands on the lower bound, while the port clears
Left and stays at 172 with `+0x1b = 0`" ✓ which would have been item 6's mechanism ✓

**It was an input-timeline artifact.** `scripts/rig/drives/ep1_keylatch.txt` (bring-up like `ep1_kill3`, one
Left press at 80.0 s, release at 83.0 s, then **no directional event at all** to 102 s; filler `key tab up`
releases only ✓) run through `route.sh` 110 s at `/tmp/rig/lk1` ✓:

```
SHIPPED lk1: 63ed3 0 -> 1 at t=82391 ✓ , 1 -> 0 at t=85744 ✓✓ (and stays 0 to the end)
PORT   lk1: 63ed3 0 -> 1 at tick 766 ✓ , 1 -> 0 at tick 796 ✓✓ (and stays 0 to the end)
SHIPPED lk1 80-105 s: 82 player samples, Y = {172} only ✓
PORT   lk1 ticks 760-1010: 251 player samples, Y = {172} only ✓✓ match
```

So the shipped **does** clear a directional latch on its release ✓ exactly like the port ✓✓ The le4b latch came
from the replay's scheduled-edge injection dropping the *last* release before a long tail of other presses ✓✗
(`replay_original.py` posts one `/api/v1/input/sequence` ✓) and the game then saw the emulator's own typematic
repeat makes ✓ — a **rig** defect, not a game behaviour ✓

**Rule recorded:** a latch claim needs a purpose-built run with an *isolated* press/release pair and a key-silent
tail ✓ A cell that "stays 1 for the rest of a long drive" is not evidence about the game ✓ it may be evidence
about the injector ✓ (this is the same family as docs/280's key-state-census warning ✓ and docs/234's
`--rig-dump` artifact ✓)

## 2. The walking-body guard fix now has a real pair

`LAB_0003284e` entry guard fixed to the shipped's three-key chain (docs/291 §3 ✓ `port/src/f/f.c` ✓). Same
drive (`ep1_downonly.txt` ✓), player Y:

| build | Y histogram, whole run | Y-change events |
|-------|------------------------|-----------------|
| port, **pre-fix** (`/tmp/rig/le4b_pre`) | `{172: 580, 178: 1, 188: 1, 195: 91}` ✗ | descent to 195 while only Down is pressed |
| port, **post-fix** (`/tmp/rig/le4b`) | `{172: 912}` ✓ | **0** ✓ |
| shipped (`/tmp/rig/lk1`, keys properly released) | `{172: 82}` in the key-silent window ✓ | 0 ✓ |

So a Down press alone no longer drags the player to the room's lower clamp (`195` = `fn_01586b`'s `mx` ✓),
which is what the shipped does with no horizontal key held (`0x32875 … je 0x3294d` exits the body ✓) ✓
`make -C port test` **ALL PASS** ✓

The le4b shipped row (`172: 23, 195: 117`) is **withdrawn as a shipped behaviour**: it was produced under the
latched-Left injection artifact ✓ and is not comparable ✓

## 3. Sweep after the fix (all green, docs/289 §2 basis)

* `make -C port test` → **ALL PASS** ✓ (post-fix ✓)
* nine-episode ASan/UBSan soak `/tmp/rig/soak291` → stages 1-9 `rc=0 ubsan=0 asan=0 segv=0` ✓ no sanitizer text in any log ✓
* fuzz (`--episodes 1..9 --seeds 12 --workers 3 --cap 120`) → **252/503 functions, never-entered 251, 0 crashes** ✓ (≥ 247 baseline ✓)
* canonical state route (`ep1_kill3`, 150 s, `RIG_HZ=10`, pinned clock) `/tmp/rig/rb11` → cells 346, phases 274,
  tally `SAME 71, DIFF 26, SIZE? 34, PTR 52, REPR 20, DIFF-SIZE 14` ✓ vs `/tmp/rig/rb10` (`SAME 70, DIFF 27,
  REPR 19`) → one row moved DIFF→SAME, no new unexplained class ✓; activity profiles all SAME ✓;
  channel-order `0.982 ok` ✓; pacing row `orig 2.72 vs port 19.51` unchanged attribution (docs/168 family ✓)

## 4. Item 6 status, stated exactly

Both candidate mechanisms are now closed:
* ~~"the port invents the proximity flip in five per-type handlers"~~ → retracted, the shipped has the tails
  (`0x18e51-0x18e78`, `0x19e11-0x19e38` ✓ docs/291 §1 ✓)
* ~~"the shipped latches the last directional key and the port does not"~~ → refuted on a clean pair ✓ (§1 ✓)

and every input to the limb draw is proven byte-faithful: the anchor arrays and both arms (docs/220 ✓), the box
arithmetic and the `0x13db7 cmp [eax+0x1b],0` arm polarity (docs/255 §2 ✓), the mirror-flag stores (docs/229 ✓),
the part arrays from the spawn routine (docs/230 ✓), the sprite arrays (docs/212 ✓) — while the record state at
the left bound is *identical* on both sides (`X=-30 ✓ Y=172 ✓ +0x1b=0 ✓ state 0 ✓ docs/288 §3, re-confirmed on
`/tmp/rig/le3`: shipped 35 samples, port 277 samples, all `Y=172, f1b=0` ✓).

**Item 6 therefore has no located port defect and is not claimed as fixed.** The one instrument that could
still settle it is a **draw-footprint census**: `blit_probe_note` (`port/src/f/f.c:3212 ✓`, armed under
`NIGHT_GLYPHDBG=1 ✓`) accumulates the blit bbox `(x0..x1, y0..y1)` on the 320-wide canvas from the composite
routine `fn_03bb42` (`f.c:7733 ✓`) — dumping that bbox during a left-edge attack and comparing it with the
destination the shipped's faithful arithmetic computes from the same record values (identical on both sides ✓)
would show a wrong-side limb directly, with no dependence on cadence, capture geometry, or colour convention ✓
If that census matches the arithmetic, item 6 closes as "not reproducible on HEAD" ✓ and the report should be
attributed to the pre-fix walking/clamp behaviour that docs/254 and docs/292 §2 fixed ✓
