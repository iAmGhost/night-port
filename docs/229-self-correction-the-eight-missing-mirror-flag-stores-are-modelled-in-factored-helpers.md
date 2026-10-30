# 229 — Self-correction: the eight "+0x1b mirror-flag stores the port never makes" (docs/225 §3) are modelled in factored helpers

Triggered while starting `fix-side-glitch`.  docs/225 §3 claimed the port writes the arm-selecting field
`[ent+0x1b]` in 12 of the 14 shipped writer functions and **misses it in two** -- `fn_03e20f` (6 stores) and
`fn_03272f` (2 stores) -- and built item 6's mechanism on that.  Both halves of that claim are **wrong**: the
stores exist in the port, in helpers the two functions call.  Item 6's mechanism is therefore **re-opened**; the
anchor result of docs/220 and the arm/blit faithfulness of docs/225 §1-§2 stand.

## 1. What the census actually measured

`scripts/audit/field_writers.py --disp 0x1b` reports the shipped side correctly: **74** sites in 28 functions
(72 `MOV`, 2 `SUB`).  The comparison it performed was *per shipped function name -> per port function name*:
"does the port's function of that name contain a write to +0x1b".  The port factors large shipped bodies into
named helpers, so the answer is "no" for two functions whose stores live one call away.

Two further traps found while re-checking, both recorded so neither is used again:

* a **citation-coverage** test (is the shipped store address inside any port `+0x1b` write's cited guest range?)
  over-reports: 31 of 74 sites look uncovered, but most are the behaviour-handler pairs whose port lines carry
  **no citation at all** -- e.g. the twelve near-identical `fn_01519f(e) > p_x / < p_x` pairs at
  `f.c:16439/16440, 16791/16792, 17136/17137, 17482/17483, 17834/17835, 18201/18202, 18553/18554,
  18908/18909, 19247/19248, 19575/19576, 19916/19917, 20333/20334` ✓ they are modelled.
* the admissible test is per shipped function **and its call graph in the port** (follow factored helpers), not
  per name.

The port writes `+0x1b` on 80 lines.

## 2. `fn_03272f`'s pair is present, in `lab_0003284e`

Shipped (`fn_03272f`, 0x0003272f, 7 014 B -> body 0x3272f-0x342a5, so 0x3284e is inside it):

```
0x328e2  cmp  byte [0x63ed3], 0 ; je 0x32908
0x328eb  mov  edx, [ebp-0x18] ; 0x328ee neg edx
0x328f0  mov  eax, [0x53c7c]  ; player
0x328f5  call 0x1545e
0x328fa  mov  eax, [0x53c7c]
0x328ff  mov  dword [eax + 0x1b], 1                  ; <-- docs/225 called this missing
0x32906  jmp  0x3292a
0x32908  cmp  byte [0x63ed5], 0 ; je 0x3292a
0x32911  mov  edx, [ebp-0x18] ; 0x32914 mov eax,[0x53c7c] ; 0x32919 call 0x1545e
0x3291e  mov  eax, [0x53c7c]
0x32923  mov  dword [eax + 0x1b], 0                  ; <-- and this
```

Port (`port/src/f/f.c`), `lab_0003284e` = the shipped's own `LAB_0003284e` sub-block, at `f.c:27136-27152`:

```c
if (g_dat_63ed3 == 0) {                                   /* 0x000328cc-0x000328f4 */
    if (g_dat_63ed5 != 0) { fn_01545e(pl, step); PL32(0x1b) = 0; }   /* f.c:27148 */
} else {
    fn_01545e(pl, -step); PL32(0x1b) = 1;                            /* f.c:27150 */
}
```

and it is called from `fn_03272f`'s own state dispatch: `f.c:27297` ("states 1, 2") and `f.c:27354`
("state 0x28") ✓ both stores reachable, both directions, same `fn_01545e` calls.

## 3. `fn_03e20f`'s six stores are present, in `e20f_spawn`

Every one of the six shipped sites is the same tail of a handler body in `fn_03e20f` (span 0x3e20f-0x3ed40):

```
0x3e358  mov  eax, [ebp-4]                       ; the command record
0x3e35b  mov  edx, [eax + 5]                     ; the command's value
0x3e35e  mov  eax, [ebp-0x24]                    ; the resolved target
0x3e361  mov  dword [eax + 0x1b], edx            ; flag = value      (and 0x3e483, 0x3e58f, 0x3e6bc, 0x3e804,
0x3e364  mov  eax, [ebp-4]                                          ;  0x3e932 -- six handlers, one tail each)
0x3e367  mov  byte [eax], 0                      ; clear the opcode
0x3e36a  jmp  0x3ed66                            ; handler end
```

with the target resolved just above it (0x3e2f8-0x3e355 / 0x3e453-0x3e477) either from a previous local or from
`call 0x10efc` (spawn-or-get entity), which is exactly the port's helper:

```c
static uint8_t *e20f_spawn(int32_t anchor_x, const uint8_t *rec, int stamp_gate)   /* f.c:26891 */
    ...
    ((struct ua32 *)((ent + 0x1b)))->v = ((struct ua32 *)((rec + 0x05)))->v;       /* f.c:26934 */
```

called at six sites inside `fn_03e20f` -- `f.c:26958, 26973, 26980, 26989, 27004, 27011` -- i.e. **6 call sites
x 1 store = the shipped's 6 handler tails** ✓ one-to-one.

## 4. What survives of item 6, and what has to be redone

* Still true: both arms of `fn_012efa` are byte-faithful including the row anchor (docs/225 §1, 11 976 composites
  in `/tmp/rig/le6.log`: 3 712 forward, all `dest = worldx - camx - X`; 8 264 mirror, all
  `dest = worldx - camx - (W - X)`); the three clamp candidates are equivalent (docs/225 §2); the mirror is also
  a run-reversal in the blitter and the port has it.
* **Retracted**: "the flag that picks the arm is never written by the port" (§1-§3 above).  A consequence is that
  the `W - 2X` displacement table in docs/225 §3 is a *what-if*, not an observed divergence: the runtime witness
  showed the port taking the mirror arm 8 264 times with `face=1`, so the flag is set and used.
* Item 6's gate verdict (docs/226) therefore reads **DETECTED (symptom reproduced by the rig's own left-edge
  drive) / NOT LOCALIZED**, and `fix-side-glitch` stays open.  Candidates, in the order to test them:
  1. the attack-part record's per-part arrays `+0x12b` W, `+0x12f` H, `+0x133` R, `+0x137` X, `+0x13b` Y **at
     spawn** (docs/220 candidate (b), never measured): the limb is a part record, not the body, so the census in
     docs/225 §1 profiled the wrong records;
  2. which value `+0x1b` holds for the part record at draw time: the spawn copy `rec+0x05`
     (`e20f_spawn` / `f.c:26934`) versus the shipped's later writes in the `fn_013c4d` / `fn_01441e` families
     (10 + 1 stores at 0x141fe-0x1433f / 0x14f2f-0x14f35);
  3. the shipped-side pixel basis for a left-edge attack, still the documented RIG-GAP (docs/202, docs/205: the
     two dumps share no presentation basis; the shipped side has no in-process witness).

## 5. docs/65 rows added

| claim | verdict | evidence |
|---|---|---|
| The port never writes `[ent+0x1b]` in `fn_03e20f` and `fn_03272f` (8 stores missing) | **retracted: both are modelled in factored helpers, docs/229 §2-§3** | `lab_0003284e` `f.c:27148/27150` (shipped 0x328ff/0x32923) called at `f.c:27297/27354`; `e20f_spawn` `f.c:26934` called at `f.c:26958/26973/26980/26989/27004/27011` |
| A field-write census matched per function name is a coverage verdict | **no: follow the port's factored helpers (call graph), docs/229 §1** | 74 shipped sites in 28 functions; the port writes +0x1b on 80 lines, two of them one call away from the functions being checked |
| A citation-coverage test can show which shipped writes are unmodelled | **no: it over-reports 31 of 74 because many faithful port lines carry no citation** | the twelve `fn_01519f(e) > p_x / < p_x` pairs at `f.c:16439...20334` |
