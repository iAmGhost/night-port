# 258 — Item 2b: the shipped's reaction-state write IS modelled; the missing 0x34/0x35 states must come from `atype` or the map table

Continuing docs/257's localization without stopping at the immediate scan ✓ (docs/257's candidate list was about
`cmp` instructions: `fn_01194c` 0x11be5 / 0x120e2 / 0x12216 / 0x1235c compare **`[eax + 0x11]`** against 0x33 and
0x34 ✓ and 0x11dc4 against 0x35 ✓ -- those are animation-frame-slot tests ✓ **not** state stores ✓ corrected here ✓
`fn_012616` 0x12c18 likewise compares `[eax+0x11]` ✓)

`field_writers.py --disp 0x17` (206 stores ✓ 0 indexed ✓) gives the real state-store census:

* **immediate stores**: a per-entity-type handler family writes `mov [eax+0x17], 0x32` -- 0x18d92 `fn_017f13`,
  0x19d53 `fn_018e88`, 0x1add1 `fn_019e48`, 0x1bdb1 `fn_01aec8`, 0x1ce6d `fn_01bea8`, 0x1de95 `fn_01cf64`,
  0x1ef6d `fn_01df8b`, 0x2007c `fn_01f064`, 0x210a7 `fn_020170`, 0x220c3 `fn_02119c`, 0x230a6 `fn_0221b8` (+ one
  `0x30` at 0x223a4) ✓ **no immediate store of 0x33-0x3f exists** ✓
* **increments**: exactly three -- 0x12764, 0x127a0, 0x127dc, all in `fn_012616`, each guarded by a state comparison
  (`cmp [eax+0x17], 0xae` / `0xb5`,`0xb9` / `0xbf`,`0xc8` ✓) ✓ and the port models all three ✓ (`f.c:26439-26449`
  with the `0xaa < st` / `0xb4 < st` / `0xbe < st` guards ✓) ✓ -- but they only act on states > 0xaa ✓ not on the
  0x32 family ✓
* **computed stores**: `mov [eax+0x17], edx` at 0x14cb4 (`fn_01441e` ✓) plus the same shape in the type-handler
  family ✓

**0x14cb4 is modelled in the port** ✓ `f.c:25413` (`else { cur[+0x17] = g_tab_51b60[atype & 0xff]; }` ✓ inside the
reaction block whose comment records that the shipped applies it to *every* victim ✓ docs/110 §2 ✓). `g_tab_51b60` is
the reaction-state map, statically harvested at `f.c:25058` (`static const uint32_t g_tab_51b60[0x100] /* 0x51b60 */`
✓) and described at `f.c:9147-9153` as mapping codes `0x16..0x20` and `0x3e/0x3f/0x43` into the fallen band
`0x5b..0x66` ✓

So the two -- and only two -- remaining explanations for docs/257's finding (the port's player never enters
0x33/0x34/0x35 while the shipped does ~20 times per run ✓) are:

1. **`atype` differs**: the port's `atype` (the reaction code read from the action record at `rec_e + slot + 0x4a`
   region ✓ `f.c:25400-25413` ✓) never takes the values whose table entries are 0x33/0x34/0x35 ✓; or
2. **the map differs**: the port's `g_tab_51b60` entries for those codes are not the shipped's ✓ (a harvest defect of
   the docs/176 family ✓ -- note the shipped rig dump carries **no `g_tab_51b60` cell** ✓ so this cannot be compared
   dump-side ✓ it must be compared statically: DS 0x51b60 bytes vs the `f.c:25058` initializer ✓)

Specified check, cheap and decisive: dump the shipped DS bytes `0x51b60..0x51c60` (Ghidra memory read ✓ or directly
from NIGHT.EXE with the image mapping `scripts/audit/disasm.py` already knows ✓) and diff against the 256 dwords in
`f.c:25058` ✓ If they match ✓ explanation (2) dies and the work moves to `atype`'s provenance (the action record's
reaction code ✓ `fn_012616`'s record chain ✓ docs/107's CHAIN witness already prints `rp[0x6c]`, `rp[0x00]`,
`rp[0x04]` ✓ so extending it to print `rp[slot*2 + 0x4a]` gives the code the shipped's map would consume ✓) ✓

Item 2b's status is unchanged by this doc: **fix landed and evidenced** for the `fn_01586b` clamp guard ✓
**localized, unfixed** for the missing reaction states ✓ and the award itself still has no red/green pair because no
route settles a kind-1 record ✓

## Addendum: the shipped side of that static compare (my port-side parse was invalid, so no verdict yet)

Reading NIGHT.EXE directly with the audit tooling's own mapping (`DATA_BASE = 0x7600` ✓ i.e. file offset =
DS - 0x7600 ✓ `scripts/audit/disasm.py` header ✓), 256 dwords at DS 0x51b60:

* the region is **mixed data**, not a pure state table ✓ it contains IEEE-754 values (`0x3f800000`, `0x40000000`,
  `0x3f6147ae` ✓) and negative dwords (`0xffffffb0`, `0xffffffe2` ✓) alongside the state-like band
  `0x0-0x66` ✓ so `g_tab_51b60` indexes a region whose tail is something else entirely ✓
* 249 of the 256 dwords are non-zero ✓ distinct state-like values include `0xb, 0xc, 0xd, 0xe, 0xf, 0x10-0x1f,
  0x23, 0x27, 0x28, 0x2b, 0x2f, 0x32, 0x33, 0x37, 0x3b, 0x3e, 0x40, 0x41, 0x44, 0x47, 0x4a, 0x4d, 0x4f-0x66` ✓
  -- **0x33 (and/or 0x34/0x35) appears at exactly one index: 0x2e** ✓ i.e. the shipped's map yields a
  `0x33`-band state only for reaction code `0x2e` ✓
* **my port-side parse was invalid** ✓ (a brace-in-comment stopped the extraction at 25 tokens ✓ so the "25 differing
  entries" line it printed is void ✓ and no comparison verdict exists yet ✓) -- re-parse the `f.c:25058` initializer
  properly (or read the compiled `g_tab_51b60` symbol out of `build/night` ✓) before claiming a table defect ✓

The concrete follow-up therefore has two halves: (i) re-read the port's 256 entries correctly and diff ✓; (ii) check
whether the port's `atype` ever equals `0x2e` ✓ (the CHAIN/RECDUMP witnesses already print the action record's
`+0x4a` region ✓ `f.c:26555-26578` ✓ so one route run with the reaction-code field printed answers it ✓).
