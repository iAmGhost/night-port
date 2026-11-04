# 263 — Item 2b: the slot-selection rule in `fn_01441e` was invented, not transcribed — fixing it nearly triples the landed hits and drops the enemy debt floor

## 1. The shipped's rule (`fn_01441e`, 0x0001465b-0x000146a7)

```
0x1466a  cmp [ebp-0x30], 6 ; jge 0x146ab          ; i < 6
0x14673  eax = i*4 + [ebp-0x10]                   ; rec_e (state-indexed, docs/260)
0x1467b  dx  = word [eax + 8]                     ; row start threshold
0x14682  cmp edx, [e + 0x11] ; jg 0x146a1         ; start > frame  -> ACCEPT i
0x14693  dx  = word [eax + 0xa]                   ; row end threshold
0x1469a  cmp edx, [e + 0x11] ; jg 0x146a1         ; end   > frame  -> ACCEPT i
0x146a9  -> 0x14664  inc [ebp-0x30]               ; otherwise next row
0x146ab  cmp [ebp-0x30], 6 ; je 0x15196           ; no row accepted -> leave the pass
```

So a row is skipped **only when both of its thresholds are at or below the frame counter** — accept ⇔
`start > frame || end > frame` ✓ There is **no lower-bound test** ✓

## 2. What the port had

```c
if ((int32_t)w1 <= fr && (int32_t)w2 > fr) { slot = i; break; }     /* f.c:25207, before this doc */
```

i.e. "the row whose window *contains* the frame" ✓ — an invented `w1 <= fr` conjunct, the same failure mode as
docs/254's clamp guard ✓ With the player's frame counter (`+0x11`) sitting at 0 in 276-295 of 800 samples ✓ and a row
whose start threshold is above 0 ✓ (the common case ✓), the port **rejected row 0** and picked a later row ✓ — and
since the reaction code is read as `byte[rec_e + slot + 0x38]` ✓, the wrong row means the wrong reaction code, which
means the wrong `g_tab_51b60` entry, which means the wrong victim state ✓

Fix applied: `if ((int32_t)w1 > fr || (int32_t)w2 > fr) { slot = i; break; }` ✓ with the disassembly quoted in the
comment ✓ (`f.c:25201-25215` ✓) ✓

## 3. Before / after, same 150 s canonical route, same env, same build apart from this line

| measure | before (`/tmp/rig/rb3`) | after (`/tmp/rig/ch2`) |
|---|---|---|
| landed hits on enemies (`HIT` witness) | 30 (atype 1×7, 2×13, 0x0b×10) | **79** (atype 1×37, 2×40, 0x0b×2) |
| lowest enemy `+0x23` (pending-damage term) seen | 23 | **15** |
| player state census (`+0x17`) | {0:240, 1:330, 2:117, 0x0b:38, 0x0c:37, **0x33:41**} | {0:262, 1:415, 2:168, **0x33:65**, 0x0b:10, 0x0c:15, 0x15:18, 5:2} |
| `make test` | ALL PASS | **ALL PASS** ✓ |

The direction is the shipped's: more hits land, the enemy's pending-damage term descends further toward 0 ✓ (the
shipped reaches 0 on this route ✓ docs/199 ✓) ✓

## 4. Test fixture corrected rather than the code bent around it

Two unit tests failed and both encoded the invented rule ✓ The fixture asserted that windows `(20,30)` at frame 3
"miss the frame" and the pass returns ✓ — under the shipped's rule `20 > 3` **accepts** row 0 ✓ The fixture now sets
all six rows to `(1,2)` at frame 3 ✓ the genuinely "nothing unfinished" case ✓ and restores `(0,10)`/`(20,30)` for the
hit case ✓ (`test/test_f1.c` ✓ the label is now "an attack animation with no unfinished slot window returns" ✓)
✓ Suite: **ALL PASS** ✓

## 5. New facts about the reaction-state band (they change what "missing 0x34/0x35" means)

Reading all 256 dwords at DS `0x51b60` in NIGHT.EXE (`f.c` and the binary agree ✓ docs/259 ✓):

```
value 0x01 -> index 0x4e          value 0x0b -> indices 0x1, 0x38
value 0x0c -> index 0x2           value 0x14 -> indices 0xa, 0x60, 0x64, 0x77
value 0x33 -> index 0x2e          value 0x34 -> (none)      value 0x35 -> (none)
213 distinct values, max 0xfffffffb (the table is mixed data, as docs/258 already said)
```

**`0x34` and `0x35` do not occur anywhere in that table** ✓ so docs/257's "the port never enters 0x34/0x35" cannot be
closed by making the table lookup fire ✓ Those shipped states (measured dump-side: 0x33 ×26, 0x34 ×20, 0x35 ×20,
0x36 ×1 samples ✓) must come from a writer other than `fn_01441e`'s table store ✓ Checked the obvious ones:

* the only `+0x17` increments in the shipped are `0x12764`, `0x127a0`, `0x127dc` in `fn_012616` ✓ and their gates are
  `0xab..0xae`, `0xb5..0xb8`, `0xbf..0xc7` ✓ — the port's copies (`0xaa < s < 0xae`, `0xb4 < s < 0xb9`,
  `0xbe < s < 200` ✓ `f.c:26441-26455` ✓) are **equivalent** ✓ not the source ✓
* `fn_03272f` contains only *comparisons* against 0x33/0x34 (`0x32ed7 cmp [eax+0x11],0x34`, `0x33803` and `0x33c87
  cmp [eax+0x17],0x33` ✓) ✓ no stores ✓

Remaining open half of item 2b, narrowed honestly: the port reaches `0x33` ✓ (65 samples ✓) but never advances past
it, and no `kind==1` enemy ever reaches `+0x30 != 0` in the port ✓ (port `+0x30` census all-zero in 955 samples ✓ vs
the shipped's `{0:233, 2:104}` ✓✓) — the settle path is still the gate on the award ✓ The writer that carries
0x33 → 0x34 → 0x35 → 0x36 in the shipped is the next thing to name ✓

## 6. Basis checks that must be re-run before this fix is called green

* `exp_award.py` on the after-run: type-31 `+0x32` still `[0]` ✗ (only type 0 shows `[0, 5]` ✓) → the per-type award
  gate is **not yet met** ✓
* nine-episode soak: last full soak is docs/262 (pre-fix ✓ 9/9 rc=0 ✓ 0 ASan/SEGV ✓ 1 standing UBSan report per
  episode ✓) → must be re-run after this change ✓
* state-oracle residual tally: docs/254's post-clamp-fix tally was 34 DIFF / 24 PTR / 19 REPR / 14 DIFF-SIZE /
  3 SAME ✓ must be re-tallied so the 25-row attributed set does not grow ✓
