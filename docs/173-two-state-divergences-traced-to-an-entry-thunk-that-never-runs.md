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
