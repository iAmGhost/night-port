# 306 — The duplicate-hit report: the oracle, the shipped's row-selection structure, and the verdict that docs/263 was an over-relaxation

Playtest item (docs/manual-playtest/04-2026-11-02.md item B): one attack make lands several hits on the same
enemy at once.  Suspect named in the goal: docs/263's relaxation of `fn_01441e`'s action-row selection to
"either threshold".

## 1. The oracle and the red measurement

`scripts/rig/hits_per_attack.py` counts hits **from the dump only**: a "make" is a transition of the player
record's `+0x17` into an attack state (0x33..0x36) from outside that set, the window runs while the player
stays inside it, and a "hit event" is a fall of a victim's `+0x23` between two consecutive samples in which
that victim is present.  Identity is `(record offset, kind, span)`; a change of kind or span at the same offset
starts a new identity, so a re-used slot (docs/247, docs/251) is not mistaken for the same enemy.

`+0x29` (hp) is unusable for this: it never changes on the canonical route -- 0 decreases in 1500 samples of
`/tmp/rig/rb15` -- so `port/src/app/rig_sink.c` now also dumps `+0x23` (the term the port's own `DMG` witness
at `f.c:25457` calls `debt`) and `+0x56` (the damage pool).  The dump reads them as 32-bit; the script
normalises `+0x23` back to the word width the shipped uses (`f.c:25457` reads it as `ua16`).

Canonical drive `scripts/rig/drives/ep1_kill3.txt`, `--frames 9000`, `--rig-hz 10`, pinned env
`SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, two runs per side:

| run | build | makes | hit events | (make,victim) pairs | makes that hit one victim >=2x | max hits on one victim in one make |
|---|---|---|---|---|---|---|
| `/tmp/rig/h306/head1` | HEAD `4560c79` | 99 | 4 | 3 | **1** | **2** |
| `/tmp/rig/h306/head2` | HEAD `4560c79` | 114 | 7 | 5 | **1** | **3** |
| `/tmp/rig/h306/base1` | `e4a1f4e` (pre-docs/263) | 44 | 11 | 11 | 0 | 1 |
| `/tmp/rig/h306/base2` | `e4a1f4e` | 47 | 5 | 5 | 0 | 1 |

The witness instance, HEAD `/tmp/rig/h306/head2`: victim `o=1631896` (kind 1, span 352) inside a single make
(ticks 980..982, player states 52, 53, 53) absorbs three separate falls -- `35 -> 23`, `23 -> 9`, `9 -> 2`.
The pre-docs/263 build produced **no** multi-hit make in either run: every one of its 11 and 5 (make,victim)
pairs took exactly one hit.  The baseline tree is a `git worktree` at `27ff0d9^ = e4a1f4e` in `/tmp/base263`;
the only edit made to it is the identical two-field instrument in `rig_sink.c` (5 inserted lines, no game
logic), so both sides carry the same oracle.

Note the shape: HEAD has 2.2-2.5x **more** attack makes yet **fewer** hit events than the baseline, while
concentrating those events on a single victim repeatedly.  The report is about concentration, not volume.

## 2. The shipped's decision structure, instruction by instruction

`python3 scripts/audit/disasm.py --entry 0x14640 --end 0x14710` (run under `uv run --with capstone`):

```
0x1465b  mov  dword ptr [ebp - 0x30], 0            ; i = 0
0x14662  jmp  0x1466a
0x14664  mov  eax, [ebp - 0x30] ; inc [ebp - 0x30] ; i++
0x1466a  cmp  dword ptr [ebp - 0x30], 6 ; jge 0x146ab        ; i < 6
0x14670  mov  eax, [ebp - 0x30] ; shl eax, 2 ; add eax, [ebp - 0x10]   ; eax = rec_e + i*4
0x14679  xor  edx, edx
0x1467b  mov  dx, word ptr [eax + 8]               ; w1 = word[rec_e + i*4 + 0x08]
0x1467f  mov  eax, [ebp - 4]                       ; e (attacker)
0x14682  cmp  edx, dword ptr [eax + 0x11]          ; w1 vs e[+0x11] (action frame counter)
0x14685  jg   0x1469f                              ; w1 >  fr  ->  0x1469f
0x14687  mov  eax, [ebp - 0x30] ; add eax,eax ; add eax,eax ; add eax,[ebp-0x10]
0x14691  xor  edx, edx
0x14693  mov  dx, word ptr [eax + 0xa]             ; w2 = word[rec_e + i*4 + 0x0a]
0x14697  mov  eax, [ebp - 4]
0x1469a  cmp  edx, dword ptr [eax + 0x11]          ; w2 vs e[+0x11]
0x1469d  jg   0x146a1                              ; w2 >  fr  ->  0x146a1  = ACCEPT
0x1469f  jmp  0x146a9                              ; <-- where "w1 > fr" actually lands
0x146a1  mov  eax, [ebp - 0x30] ; mov [ebp - 0x2c], eax       ; slot = i
0x146a7  jmp  0x146ab
0x146a9  jmp  0x14664                              ; next row
0x146ab  cmp  dword ptr [ebp - 0x30], 6 ; je 0x15196          ; nothing accepted -> leave the pass
```

Read the branches by their **targets**, not by the comparison:

- `w1 > fr` jumps to `0x1469f`, and `0x1469f` is a two-byte `jmp 0x146a9`, which is the **next-row** path.  A
  row whose start threshold is above the frame counter is **skipped**.
- The accept block `0x146a1` is reachable only from `0x1469d`, which is evaluated only when `w1 <= fr`.

So the shipped accepts row `i` exactly when `w1 <= fr && w2 > fr` -- the row's window contains the current
action frame.  There is no "either threshold" rule anywhere in this loop.

## 3. Verdict on docs/263

**docs/263's "either threshold" is an over-relaxation, and it was produced by mis-reading one jump target.**
docs/263 §1 quotes the first test as `cmp edx, [e + 0x11] ; jg 0x146a1 -> ACCEPT i` -- the real target is
`0x1469f` (`jg 0x1469f`, and `0x1469f` is a stub `jmp` to the next-row path), and `0x146a1` is reached only
through the *second* test.  docs/263 therefore called the port's `w1 <= fr` test "an invented lower-bound
conjunct" when it was in fact the shipped's own test, and relaxed the selection to accept rows whose window
has not started yet.  docs/263 also rewrote two unit tests that encoded the faithful rule, so the suite
agreed with the wrong code (docs/265 §1's lesson inverted: here the disassembly and the test agreed, and the
source was the odd one out).

This is the mirror image of docs/267 §2.7: there the defect was an invented conjunct *added* to a guard; here
a genuine conjunct was *removed* on the strength of a mis-read target.  The rule it produces: **a conditional's
meaning is its target, and a target that is itself a `jmp` stub is a reject, not an accept.**

## 4. Why a wrong slot becomes several hits on one victim

The shipped deduplicates per victim.  `fn_01441e`'s pass-3 filter (`disasm.py --entry 0x14709 --end 0x147c0`):

```
0x14700  mov  dl, byte ptr [eax + 0x4e]            ; victim[+0x4e]
0x14706  cmp  edx, dword ptr [eax + 0x17]          ; vs attacker state (eax reloaded = attacker)
0x14709  jne  0x1471f                              ; different state -> proceed
0x1470b  mov  al, byte ptr [eax + 0x4f]            ; victim[+0x4f]
0x14716  cmp  eax, dword ptr [ebp - 0x2c]          ; vs the ACCEPTED row index
0x14719  je   0x15150                              ; same state AND same row -> this victim is skipped
0x1471f  ...  call 0x13c4d                          ; the hit test itself
```

The pair is written right after a landed hit, and only in three places each -- `field_writers.py` census:

| field | shipped writers |
|---|---|
| `+0x4e` | `0x11016 fn_010efc = 0`, `0x12710 fn_012616 = 0`, `0x14812 fn_01441e = dl` (attacker state) |
| `+0x4f` | `0x1101d fn_010efc = 0`, `0x12717 fn_012616 = 0`, `0x1481b fn_01441e = al` (accepted row) |

The port models both halves: the test at `f.c:25394-25395` (`cur[0x4e] == e[+0x17] && cur[0x4f] == slot ->
goto hitnext`) and the writes at `f.c:25433-25434` (`cur+0x4e = state`, `cur+0x4f = slot`) ✓ so the latch is
not missing -- it is keyed on `(attacker state, accepted row)`, and the **row half of the key is decided by the
selection rule under dispute**.  A selection that accepts rows whose window has not started hands `fn_01441e` a
different `slot` as `e[+0x11]` advances, so each pass presents a fresh `(state, slot)` key, the latch never
repeats, and the same victim absorbs one hit per pass inside one make -- the observed `35 -> 23 -> 9 -> 2`.
The writer census for the two compared fields backs the same reading from the other side: `+0x11` has 144
writes, of which `fn_012616` holds the raiser (`INC` at `0x12732`, `0x12b34`) and `fn_01441e` the resets
(`0x145d9`, `0x14632`, `0x14870`, `0x14aef`) and one assignment `0x14aa3 mov dword [edx+0x11], eax`; `+0x23`
has 8 writes, and the only subtractor is `0x14964 fn_01441e sub word[edx+0x23], ax` -- which is why a fall of
`+0x23` is a legitimate hit witness (the raiser at `0x138f6` belongs to `fn_0137c1` and, per docs/252 §3, only
ever touches the player).

## 5. What has not been claimed yet

Nothing has been fixed in this note.  The gate for the fix (task `hit-fix`) is: restore `w1 <= fr && w2 > fr`,
restore the two fixtures docs/263 rewrote so they assert the shipped's window rule, show those fixtures red on
the pre-fix line and green on the fixed line, and re-run `hits_per_attack.py` twice on HEAD with the phase
matched (player X, room, mirror) so the multi-hit makes disappear while the total hit volume stays in the
baseline's range.
