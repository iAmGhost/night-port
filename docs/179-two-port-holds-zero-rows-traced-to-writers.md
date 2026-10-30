# 179 — Two "port holds zero" rows, traced to who writes them

docs/178 left four rows of the shape *shipped holds a value, the port holds zero*: `0x53451`, `0x62d5c`, `0x64090`,
`0x64094`.  `mem_access.json` names the shipped writers for the last three, and the port is missing one whole set of
them.

## 1. `DS 0x62d5c` — seven shipped writers, the port models only the decrement

Shipped writes the cell in **`fn_03272f`** (the `.act` chain matcher) at seven addresses:

```
0x33bef  0x33c47  0x33de1  0x33eef  0x3408c  0x34128  0x34280      MOV [DAT_00062d5c], EAX   (fn_03272f)
0x348ca  CMP dword [DAT_00062d5c], 0        (fn_0347e3)
0x348d3  DEC dword [DAT_00062d5c]           (fn_0347e3)
```

The port models the second pair only:

```c
if (g_dat_62d5c != 0) g_dat_62d5c--;                              /* 0x348ca-0x348d3   f.c:8735 */
```

Every other port write to the same variable (`f.c:27338`, `27352`, `27361`, `27373`, `27382`, `27395`,
`g_dat_62d5c = lvls[k-1]` next to `chain_target_log`) sits in the rig's chain-target injection path, not in
`fn_03272f`.  So on an ordinary drive nothing ever loads the counter, the decrement has nothing to decrement, and
the oracle sees `shipped [0..1]` against `port [0..0]` -- this row is a **missing-write-site** finding, not a phase
difference, and it lands in the same function the punch/reaction chain questions live in (docs/110, docs/167 §5).

## 2. `DS 0x64090` / `DS 0x64094` — written by a function this route never reaches

The port's writes are cited to guest `0x45946` / `0x4594c`, which sit inside **`fn_04589e`** -- "build a tokenized
command line in the heap" (`f.c:7272`, `fn_04589e` at `f.c:7288`):

```c
g_dat_64090 = cnt1;                       /* 0x045946 */
g_dat_64094 = g_dat_5337c;                /* 0x04594c */
```

and the shipped side reads `DAT_00064094` back inside `fn_0445c9` at `0x445fe` -- the entry thunk docs/173 had to
split and wire by hand because its callers were unreachable.  Shipped's values are constants over the whole route
(`0x64090 = 1`, `0x64094 = 1691857`, the latter an absolute flat-model address), the port's are zeros.

So these two rows are the same family as docs/173 §6's `DS 0x53451` / `fn_04933a`: a startup-side routine whose
port-side call path does not exist, leaving the cells it publishes permanently empty.  `0x64094`'s value is also an
absolute address, so even once published it needs docs/134 §3's base-relative comparison to be quotable.

## 3. What to do with them

1. `0x62d5c`: reproduce the seven `fn_03272f` stores.  The port already computes the same quantity at those chain
   arms (`lvls[k-1]`), so this is wiring the existing value into the cell at the right sites -- a small, checkable
   change, and the census is the `0x62d5c` row itself plus `0x62d58`, which the port does write.
2. `0x64090` / `0x64094` / `0x53451`: one question covers all three -- what calls `fn_04589e`, `fn_04933a`, and
   their neighbours in the shipped binary.  docs/173 showed the answer for one of them was a thunk whose wrapper had
   no caller; `call_sites.py` should be run for `fn_04589e` and `fn_04933a` before inventing a second wiring.

## 4. Neither has a CALL in the shipped binary -- which is the answer to §3

```
call_sites.py --target 0x4589e   ->  callers of 0x4589e: 0
call_sites.py --target 0x4933a   ->  callers of 0x4933a: 0
```

Zero direct callers, for both.  So `fn_04589e` (which publishes `DAT_00064090` / `DAT_00064094`) and `fn_04933a`
(which is what `DS 0x53451` needs, docs/173 §6) are reached through an **indirect** path, not a CALL: the shipped
binary has a dispatch pair `dispatch_a` / `dispatch_b` at `0x44618` / `0x44663` and a callback cell
`cb_indirect_53644` in the DS table whose dword the entry thunk now sets to `0x8000` (docs/173 §5).  A tool that
walks only `CALL` edges will always report these as unreachable, and inventing a direct call from the port would be
fabricating a call graph the shipped program does not have.

Named test, replacing §3 item 2: dump what the port stores in the dispatch table(s) behind `dispatch_a` /
`dispatch_b` and `cb_indirect_53644`, and check whether `fn_04589e` and `fn_04933a` appear as entries there.  If the
port's table entries are missing, that single table explains three "port holds zero" rows -- `0x53451`, `0x64090`,
`0x64094` -- without touching any of their bodies.

## 5. The first version of the reachability check was wrong in both directions

`scripts/audit/uncalled_port_fns.py` (new) tried to list port functions with no call site and cross-check the
shipped caller count:

```
port functions with no call site: 3
  fn_0003e14c   f.c:26624   shipped direct callers: 2
  fn_041c31     f.c:951     shipped direct callers: 18
  fn_04295d     f.c:15859   shipped direct callers: 1
```

Every one of those three is a false positive, and the two functions the check was written to catch are **absent**:

- `fn_041c31` is the shipped name of the port's **`heap_free_host`**, which is called (`f.c:3542`, `4843`, `4844`).
  `fn_04295d` is **`get_key`**.  The port renamed them, so a textual search for the shipped name finds a definition
  and no calls.
- `fn_04933a` and `fn_04589e` *are* uncalled, but they never define a call-looking occurrence beyond their own
  definition plus comments that mention `name(`, so the "occurrences == 1 means uncalled" rule discarded them.

Both failures come from the same shortcut: counting name mentions in source text.  The port already carries the
authoritative mapping -- `port/src/gen/fn_table.c` stores shipped function names next to their host pointers, and
docs/166 §12 used it with `dladdr()` to name a call site in-process.  A reachability census has to ask *the table*
which host function each shipped name is, then look at that function's identity, not its spelling.  Until it is
rewritten that way, `uncalled_port_fns.json` is a rig artifact and not a finding; the two real facts this turn
established stay the ones in §1 and §4.
