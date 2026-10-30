# 173 — Two state divergences trace to the entry thunk `fn_0445c9`, which never runs

Continuing docs/172's list on `/tmp/rig/g20`.

## 1. `DS 0x51c98` is a phase difference, not a mechanism difference

Both sides oscillate between the same two values; only the schedule differs:

```
port    transitions: 0x1 @16ms, 0x2 @74050, 0x1 @77150, 0x2 @78950, 0x1 @80750, 0x2 @81950, 0x1 @84050 ...
shipped transitions: 0x1 @47231ms, 0x2 @89231, 0x1 @99231, 0x2 @105231, 0x1 @118231, 0x2 @123231, 0x1 @137231 ...
port census   0x2 x812, 0x1 x688      shipped census  0x2 x345, 0x1 x79
```

`g_dat_51c98` is the collision mode flag (`1 = reinit -> 2 = active`, reinit step at `f.c:9625-9626`).  The port
spends 54 % of samples in `active`, shipped 81 %.  Same alphabet, same alternation, different duty cycle -- the
divergence belongs to whatever paces that reinit, not to the flag's wiring.

## 2. `DS 0x53451` and `DS 0x53640`: the port keeps the image values, shipped has run an init path

```
DS 0x53451   port  0x00 x1500 (constant)         shipped  0x02000004 x424 (constant)
DS 0x53640   port  0x10 x1500 (constant)         shipped  0x8000  x424 (constant)
image at 0x53451: 00 00 00 02      image at 0x53640: 10 00 00 00
```

The port's initializers are faithful to the image (`g_dat_53640 = 0x10` at `f.c:10667` matches `10 00 00 00`), and
shipped's values are what the game holds *after* a start-up path has run.  A call-count witness (`NIGHT_CALLTRACE`)
settles which path the port is missing -- **zero calls on a 12 000-frame drive**:

```
calltrace set_53640 ...   0 occurrences      /* f.c:10668  void set_53640(void) { g_dat_53640 = 0x8000; }  (0x49187) */
calltrace fn_04933a ...   0 occurrences      /* f.c:5687, "rebuild the active node list from the file-structure
                                             * table", whose body sets g_dat_53451 = (x & 0xf8) | 4 at 0x4933d-56 */
```

Both are in `gap_frontier.json`'s **never-entered** set (199 functions), and `fn_04933a` is additionally in
`no_known_caller` (55).  The caller of the setter is `fn_0445c9` -- `set_53640()` is invoked at `f.c:27763`
(guest call `0x0445fe`), and `fn_0445c9` is described in the port as *the entry thunk around the game main*
(the last step of the DOS entry sequence, called at `f.c:27726` from a function that is itself never entered).

So these two cells are **coverage gaps in the entry/init path, not wrong statements**: the port models the DOS
entry sequence implicitly instead of running `fn_0445c9`, and the observable residue is exactly the two values
that thunk's callees would have set.

## 3. Why it is worth fixing anyway

`DS 0x53640` is not inert.  `f.c:3441` uses it as a lower clamp:

```c
if (t < g_dat_53640) t = g_dat_53640 & ~1u;      /* 0x43202-43208 */
```

With the port's `0x10` this clamps to 16; shipped's value `0x8000` clamps to 32768.  A clamp that is four orders of
magnitude different is a behavioural difference, and it originates from an init path the port never executes.

## 4. Named test for the next step

Run `fn_0445c9`'s body at start-up the way the shipped entry sequence does (its callees `set_53640` and the
stack-reservation pair `DAT_0005339c` / `DAT_000533a0`), then re-measure these two cells cross-side.  Red:
`0x53640 = 0x10`, `0x53451 = 0x00`, calltrace 0.  Green: `0x53640 = 0x8000`, `0x53451` low byte `0x04`, and the two
rows leaving the DIFF set.  If the values then match but the clamp's *effect* diverges, `f.c:3441`'s call sites
become the next measurable item.

## 5. The fix, and the red -> green

The thunk was unreachable for a structural reason: `fn_0445c9` is called only from `startup_body()` (the port's
model of the DOS4GW startup body, `f.c:27626`), whose wrappers `fn_0420bc` / `fn_042046` (`f.c:27735`, `f.c:27741`)
have **no caller anywhere** -- `main.c:1569` enters `fn_03af18` (the game main) directly.

Fix: split the thunk's pre-main half out and run it before the game main, exactly where the shipped order has it
(`set_53640` at guest `0x0445fe`, game main at `0x04460e`):

```c
void fn_0445c9_startup(void) { ...stack reservation...; set_53640(); }   /* 0x0445d8-0x0445fe */
void fn_0445c9(void) { fn_0445c9_startup(); fn_03af18(); fn_0423b7(0,0,0,0); }
```

and in `main.c`, `fn_0445c9_startup();` immediately before `fn_03af18();`.

```
before:  calltrace set_53640 = 0 calls ;  DS 0x53640 = 0x10 in 1000/1000 samples
after :  calltrace set_53640 #1        ;  DS 0x53640 = 0x8000 in 1000/1000 samples
```

Shipped holds `0x8000` in 424/424 samples, so the cell now agrees, and `0x53640` left the oracle's DIFF rows on a
fresh 150 s route (`/tmp/rig/g22`).  Value-oracle tally on the same drive, calibrated cadence:

| | g20 (before) | g22 (after) |
|---|---|---|
| SAME | 64 | **67** |
| DIFF | 38 | **35** |
| DIFF-SIZE | 13 | 12 |
| REC-SAME | 15 | **19** |
| REC-NULL | 11 | **7** |
| activity profile | 269/269 SAME | 269/269 SAME |

`make -C port test` ALL PASS.  `DS 0x533a0` now carries a host stack address (the reservation the thunk records);
the oracle already classifies it `NON-GAMEPLAY:dos_extender`, so it adds no noise, but it is a host value and is
not comparable -- recorded rather than hidden.

## 6. What is still open on this axis

`DS 0x53451` stays DIFF (`0x04` shipped vs `0x00` port): its writer is `fn_04933a` ("rebuild the active node list
from the file-structure table"), which is still called zero times and is in `gap_frontier`'s `never` and
`no_known_caller` sets.  Unlike the entry thunk, its caller is not yet identified in the shipped call graph, so it
stays a named coverage gap rather than a fixed item.

*(Self-note: the first attempt at the `f.h` declaration for `fn_0445c9_startup` put the new line inside the
continuation of `fn_0445c9`'s multi-line comment -- the docs/169 shape again, caught by the compiler rather than
by reading the diff.  Declarations go after a comment is closed.)*
