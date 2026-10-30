# 176 — A 68-byte sound-slot table sat at DS 0x532f4, image-initialized, and the port had zeros

docs/175 narrowed the spurious player death to the spawn/hit-selection boundary and named the timed-event walker
`fn_03f6bc` as the site that both requests 61 of 63 board sample requests (docs/166 §13) and feeds the transient
attacker population.  While checking whether that walker reads faithful data, two DS regions turned out to be
**invisible to the rig**, and one of them turned out to be a real defect.

## 1. The oracle blind spot: a DS region declared inside a struct block

`port/src/f/f.h:868` models the region as one contiguous block, because the shipped DS lays it out that way
(docs/83):

```c
struct ds_53200_block {
    uint32_t tab_53200[0x3d];   /* 0x53200: timed-event tick table (image constant) */
    uint8_t  tab_532f4[0x44];   /* 0x532f4: sound-slot indices */
};                              /* 0x138 bytes: the block ends at DAT_00053338 exactly */
```

`scripts/audit/gen_headers.py` builds `rig_table.h` from **top-level port globals whose comment claims a DS-group
address**.  A member inside a struct definition is not such a global, so:

- `port/src/f/rig_table.h` (283 cells) contains **no cell at DS 0x53200 and none at 0x532f4** (`grep -c 53200`
  → 0).  The state oracle has never compared this region.
- The static byte check `c3b_full_span_initializers` (docs/122) walks the same declaration set, so it never
  byte-compared this region either.
- The Ghidra export has no defined symbol at 0x53200 (`grep 0x53200 scripts/audit/defined_data.json` → nothing),
  so the shipped side contributes no width information here either.

Consequence: a whole 312-byte DS region that the game's timed-event layer reads was outside every oracle the rig
runs.  That is how this defect survived 175 documents of state-diffing.

## 2. The tick table is faithful; the byte table was not

Read directly against `NIGHT.EXE` (DS physical base `DS - 0x7600`):

| DS region | what the port holds | what the image holds | verdict |
| --- | --- | --- | --- |
| `0x53200`, 61 dwords | brace initializer | `NIGHT.EXE` at file offset `0x4ac00` | **0 mismatches over all 61 dwords** — the tick schedule is byte-exact |
| `0x532f4`, 68 bytes | `{ 0 }` | `NIGHT.EXE` at file offset **`0x4bcf4`** | the port held **all zeros** where the image holds a ladder |

The shipped run agrees with the image for **64 of the 68 bytes**; the 65th differs (`0x10` in the image, `0x20` in
every `state_*.ds.bin` sample across a whole route, so code writes it), and bytes 66-68 match (`02 00 00`).  Read at
`state_0000/0212/0423` the shipped bytes are identical, i.e. a constant table, not run state.

The port's own comment said *"no producer inside the EXE"* -- which the image offset disproves: the producer is the
image load itself.

## 3. What the walker was reading, before and after

`fn_03f6bc` matches its tick against `DAT_53200 + 4*cursor` and, on a match, requests the sound whose slot index is
`DAT_532f4[cursor]` (`f.c:14442`, guest `0x3f7a9-c0`).  Two binaries, same drive (`ep1_kill3`, 4 000 frames,
`NIGHT_SEED=13579`, `NIGHT_RIG_STEP_CALLS=665`, `NIGHT_RIG_FPS=60`), the only difference being the initializer:

```
before ({ 0 }):   61 slot reads,  0 nonzero   -- every read asked for g_dat_53d0c[0]
after  (68 B):    61 slot reads, 53 nonzero   -- 1 x13, 2 x8, 9 x4, 8 x3, 4, 10, 33, ...
```

So the timed-event path was asking for **the same sound slot 61 times**.  Filling the table makes it ask for the
slot the game asks for.

What did *not* change on this drive, and is worth stating rather than implying:

```
before: played=9  outside-pool=0   DMA-dump lines 75
after : played=9  outside-pool=0   DMA-dump lines 74
```

Play counts and out-of-pool rejections are identical, so this fix is a **content** correction (which sound), not a
count or level correction.  The trigger-frequency gap docs/166 §10 measured -- 324 shipped DMA programs against
73-81 port feeds -- is untouched by it.

## 4. The fixtures had encoded the disproved hypothesis

Two assertions in `port/test/test_f1.c` failed the moment the table became real:

```
[FAIL] the past-the-table dword IS DAT_532f4[0] (0: the byte table has no shipped producer)
[FAIL] seeding DAT_532f4 changes that dword: the aliasing is real, not padding
```

The first is the hypothesis itself stated as an expectation.  The second is subtler: the aliasing proof compared a
`memcpy`'d dword against a single byte and only passed **because both were zero** -- a coincidence the all-zero
table allowed.  Both are rewritten to assert the shipped bytes (`00 01 00 21` at `+0..3`, dword `0x21000100`,
seeding to `11 22` giving `0x21002211`), plus a ladder check on `+23 = 0x1f`, `+24 = 0x09`, `+59 = 0x1f`,
`+60 = 0x1d`, `+63 = 0x00`, `+64 = 0x10`.  `make -C port test` → **ALL PASS**.

Standing rule: a fixture that asserts *"this cell is zero because nothing writes it"* is a hypothesis in
disguise.  Where the shipped image can answer, assert the image's bytes.

## 5. Open items this leaves open

1. **Coverage**: `gen_headers.py` should harvest DS claims from struct-block members (the only such block in the
   port today, `ds_53200_block`), so `c3b` and the state oracle cover `0x53200..0x53337` instead of skipping them.
   Until then this region is checked by hand only.
2. **Byte at DS 0x53334** (`+64`): the image says `0x10`, the shipped run says `0x20` constantly, the port now
   carries `0x10`.  The writer is unidentified -- a `c3b`-style "image byte vs live byte" comparison would name the
   family, and the port currently diverges from shipped by that one byte.
3. **Sound identity proof**: to show the fix changes *which* audio the board emits, the rig needs per-transfer
   payload digests.  `NIGHT_AUDIODUMP`'s payloads are truncated in the stderr log, so a digest must be taken inside
   the witness, not scraped from the log.

## 6. Coverage closed: the region is now inside the oracle, and the boundary was wrong by 4 bytes

`gen_headers.py` now carries explicit struct-block claims, each checked against `NIGHT.EXE` at file offset
`ds - 0x7600` before it may become a dumped cell, and refuses to duplicate a cell a top-level global already owns:

```
rig_table: struct-block claim 0x53334 already held by g_dat_53334 -- not duplicated
    {0x53200, 244, 244, 0, &g_ds_53200.tab_53200, "g_ds_53200.tab_53200"},
    {0x532f4,  64,  64, 0, &g_ds_53200.tab_532f4,  "g_ds_53200.tab_532f4"},
    {0x53334,   4,   4, 0, &g_dat_53334,           "g_dat_53334"},
```

The 4-byte split matters: the first pass claimed `0x532f4 + 68`, and the oracle immediately reported

```
DIFF 0x532f4 g_ds_53200.tab_532f4 g=68 h=68 ; first differing byte @0x53334 0x20 vs 0x10 (1/68 bytes differ)
```

i.e. a *neighbouring symbol's* runtime value surfacing as a defect in the table.  `defined_data.json` defines
`DAT_00053334` as its own symbol (`0x53334..0x53337`, 4 B, `nrefs 0`), and 64 is exactly the span whose image bytes
match the shipped run.  With the span corrected the table leaves the divergence list entirely.

Canonical route (`ep1_kill3`, 150 s, `RIG_HZ=10`, `/tmp/rig/g27`, fixed build, 346 cells):

```
residual same-width gameplay divergences: 35          (unchanged from the same route before the fix)
0x53200 (244 B) and 0x532f4 (64 B) are absent from the divergence list -> SAME
```

So the tick table and the slot table are now proven faithful **by the oracle**, not by hand, and the sound-slot
transcription is confirmed not to move the residual state count -- consistent with §3: it changes which sound is
requested, not the state trajectory.

## 7. A rig-measurement trap hit while proving it: route duration changes what the oracle can compare

Short routes were run first (`ep1_kill3` 45 s, `/tmp/rig/g24`, `/tmp/rig/g25`, `/tmp/rig/g26`) and looked like an
improvement -- residual divergences 30 and 31 instead of 35.  They are not comparable, and the sample ranges show
why:

```
g20 150 s:  orig n=424 t=[47231..284206]   port n=1500 t=[16..236316]     overlap 47..236 s
g26  45 s:  orig n=156 t=[47226..178933]   port n= 450 t=[16.. 54650]     overlap 47..54.6 s  -> 244 of 274 windows SHIPPED-QUIET
g27 150 s:  orig n=853 t=[47220..284009]   port n=1500 t=[16..227883]     overlap 47..228 s   -> 57 of 274 shipped-quiet
```

The shipped timeline always begins near +47 s (boot/replay warm-up) and is stretched relative to wall clock, so a
45 s route barely reaches it: two thirds of the comparison windows have no shipped sample at all, and a smaller
residual count there measures the *window coverage*, not the port.  Rule recorded: the state oracle's numbers are
only quotable from the canonical 150 s route (`RIG_HZ=10`), and any report where `shipped-quiet windows` exceeds a
few tens of 274 is a statement about the rig.

## 8. Same pass, one more reclassification: DS 0x63e18 is an uptime stamp, not game state

Reading the cell on both sides of `/tmp/rig/g27`:

```
shipped  state_0020 -> state_0080 : 3682232 -> 3683288   (+1056)
port     sample 500  -> sample 800 : 1631896 -> 1632952   (+1056)
```

Same quantum, unrelated epochs -- the signature docs/134 §3 used for the heap pointers.  It sits at `0x63e18`,
just outside the `0x63e24..0x63e40` clock block that docs/101 entropy-masked, which is why it kept surfacing as a
gameplay divergence.  Added as the fifth non-gameplay family (`clock_stamp`) in
`scripts/audit/gen/oracle_families.json` with the increment evidence and the rule "compare increments, never
absolute values".  Re-running the oracle on the same dumps:

```
non-gameplay families: host_address_space 2, device_plumbing 27, arena_state 3, dos_extender 10, clock_stamp 1
residual same-width gameplay divergences: 34      (was 35)
```

`0x63e1c` and `0x63e20` stay in the gameplay list deliberately: no increment evidence exists for them, and
`0x63e20` is the common port-holds-zero / shipped-holds-value shape rather than a stamp.
