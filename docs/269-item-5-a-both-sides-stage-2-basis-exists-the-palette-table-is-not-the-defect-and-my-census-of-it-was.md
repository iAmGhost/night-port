# 269 — Item 5: a both-sides stage-2 basis finally exists, the palette table is *not* the defect, and my own census of it was a truncated-cell artifact

## 1. The shipped does reach stage 2 on `ep1_roomclear2` — item 5 finally has a comparable basis

`OUT=/tmp/rig/rm1 scripts/rig/route.sh scripts/rig/drives/ep1_roomclear2.txt 300` ✓ (the drive docs/224 used ✓),
read out of the shipped's flat DS window (`offset = DS − 0x50000` ✓ docs/264 §3 ✓):

```
SHIPPED  n=157   DAT_00063370 (stage index) = {1: 27, 2: 130}      <-- the transition happens shipped-side too
         DAT_00063354 (current room record) distinct = 2, min 0, max 3678252
PORT     n=2179  DAT_00063370 = {1: 1003, 2: 1176}
         DAT_00063354 distinct = 2 (0 and 1627884)
route pacing on this drive: orig 5.23 vs port 9.44 picture changes/s (ratio 0.55; the kill3 route is 0.13)
```

So both sides enter stage 2 on the same drive ✓ — **item 5's screen half can now be gated on a like-for-like basis** ✓,
which every earlier attempt lacked (docs/222-224 had port-side stage transitions only ✓; no shipped-side dump reached
stage 2 ✓ — verified: `cd-orig`, `cd-orig2`, `chain-orig`, `ents-orig`, `g33` all hold `DAT_63370 == 1` throughout ✓) ✓

## 2. The palette/glyph table `DAT_000636b8` is *not* the defect (and the shipped's stage-gate path is faithful)

Port-side loader census under `NIGHT_DBG=1` (`dbg_pal_load` ✓ `f.c:6318-6323` ✓, prints post-shift nonzero bytes ✓):

```
38x  fn_035f1b help8.pcx    nonzero=721        1x  fn_036190 stage1.pcx   nonzero=760
 1x  fn_035f1b help1.pcx    nonzero=721           fn_036190 1s1/1s2.pcx   nonzero=763
 1x  fn_036417 title1/3/4   nonzero=758           fn_036190 logo.pcx      nonzero=765
                                                  fn_036190 1e1.pcx       nonzero=751
shipped (same table, read from its DS window): nonzero-byte counts 721, 751, 758, 760, 762, 763
```

Equivalent ✓✓ — and the load/shift path is faithful byte-for-byte: shipped `0x35f6b mov edx,3 ; 0x35f70 mov eax,0x136b8 ;
0x35f75 call 0x4192e` then `0x35f89 cmp [ebp-0x14], 0x300 / 0x35f92 shr byte [eax + 0x136b8], 2` ✓ ==
`fn_04192e(g_dat_636b8, 3, 0x100, g_639b8)` + `for (i < 0x300) g_dat_636b8[i] >>= 2` ✓ `f.c:6336-6337` ✓ ✓
So the black floor is **not** a `DAT_000636b8` content or shift defect ✓ and docs/268's candidates (the `DAT_00052f90`
stage gate ✓, `fn_03f15f` ✓) remain closed ✓

## 3. Self-correction: my DAT_636b8 census above is void — the rig dumps that cell at 256 bytes, not 768

My first both-sides census reported "port 244-255 nonzero bytes vs shipped 721-763" ✓ **withdrawn** ✓:
`port/src/f/rig_table.h:189` declares the cell as `{0x636b8, 0, 256, 0, &g_dat_636b8, "g_dat_636b8"}` ✓ while
`port/src/f/f.h:974` declares the object `extern uint8_t g_dat_636b8[0x300]` ✓ (768 B ✓) ✓ — the port-side dump carries
only a third of the object ✓, so the comparison measured 256 port bytes against 768 shipped bytes ✓ A **rig defect**
(dump width ≠ declared width ✓) that both invalidated my conclusion and means the state oracle compares 256 of 768
bytes on that row ✓

Two consequences: (a) the `>>2` on values < 4 explains why a truncated window looks sparse ✓, do not read nonzero
counts across widths ✓; (b) any DS cell whose rig width is smaller than its declared size silently narrows the state
oracle ✓ — worth a systematic check of `rig_table.h` widths against the `f.h` declarations ✓ (the same generator that
docs/176 §1 fixed for *claims* ✓) ✓ Stated fix for the next step: set the `0x636b8` entry to `0x300` ✓ re-run the
canonical tallies so the residual set is re-attributed on the corrected width ✓ and confirm `gen_headers.py` does not
re-emit 256 ✓

## 4. Item 5 status after this

* fault half: satisfied (warp and natural stage-2 routes fault-free ✓ docs/268 §2 ✓ docs/265 §3 ✓) ✓
* black floor: **not yet explained** ✗ — closed now: stage gate table ✓ (`DAT_00052f90` identical ✓), `fn_03f15f`
  (returns 1 ✓), `DAT_000636b8` contents/shift ✓ (equivalent ✓) ✓ Remaining: what the composite call
  (`0x3b49e call 0x40042(0, 0x100, DAT_000636b8)`) is actually fed — i.e. the stage-2 **room record's art fields** ✓ —
  measurable on the new like-for-like basis (rm1 ✓) by comparing the room record `DAT_00063354` points at, both sides ✓
