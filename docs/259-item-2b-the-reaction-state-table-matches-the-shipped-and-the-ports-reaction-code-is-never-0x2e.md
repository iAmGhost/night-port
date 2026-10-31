# 259 — Item 2b: the reaction-state table matches the shipped byte-for-byte, and the port's reaction codes are never the one that maps into the missing band

Continuing docs/258's two candidates ✓ to a verdict on one of them ✓

## 1. `g_tab_51b60` is faithful (candidate 2 eliminated)

Read the shipped data directly with the audit tooling's own mapping (`DATA_BASE = 0x7600` ✓ file offset = DS − 0x7600
✓ `scripts/audit/disasm.py` header ✓), validated first on three known tables:

```
DAT_00052c3c[0]      = 172      (the stage-1 room-1 walk line, docs/242 ✓)   MATCH ✓
DAT_00052ffc[0..9]   = 1,2,2,1,1,2,1,2,1,1                                    MATCH ✓ (docs/244's table)
DS 0x51b40..0x51b7c  = 0, 0, 0x10000, 0, 0x85000d, 0,0,0,0, 0xb, 0xc, 0xd, ...  -> the table begins at 0x51b60 ✓
```

Comparing all 256 dwords at DS 0x51b60 against the port's `f.c:25058` initializer:

```
compared: 256   differing: 0 ✓
index 0x2e -> 0x33 on both sides ✓   the only entry in the 0x33/0x34/0x35 band, on both sides ✓
```

(Note: an earlier attempt of mine reported "253 differing" ✓ — that was **my own parse artifact**: I started the
extraction on the declaration line, so the `[0x100]` array size was swallowed as the first table entry ✓ shifting
everything by one ✓ retracted ✓ Lesson: parse initializers *after* the opening brace ✓)

## 2. The port's reaction codes never select that entry (candidate 1 confirmed)

`atype = byte(rec_e + slot + 0x38)` ✓ (`f.c:25266` ✓) and the `HIT` witness already prints it ✓ Tabulating the two
port runs:

```
/tmp/rig/rb3/port (150 s):  55 hits -> atype 1 x29, atype 2 x25, atype 11 (0x0b) x1
/tmp/rig/v5a/port (600 s): 127 hits -> atype 1 x77, atype 2 x50
```

**`0x2e` never appears** ✓ -- and `0x2e` is the *only* code whose mapped state lies in the band the shipped's player
occupies (docs/257: `0x1->0x34` 20/16, `0x1->0x35` 7/8 ✓ with `0x34->0x1`, `0x35->0x1` returns ✓) ✓ So the port's
player never enters 0x33/0x34/0x35 **because the action record the port feeds the map never carries reaction code
0x2e** ✓

## 3. Where to look next, precisely

The shipped contains **no `0x38` immediate inside `fn_01441e`** ✓ (`imm_scan --imm 0x38` finds no hit in that body ✓)
-- so the shipped computes the reaction-code address without a literal 0x38 ✓, which means the port's
`byte(rec_e + slot + 0x38)` is a *reconstruction* ✓ and it is worth checking against the shipped's own address
arithmetic, especially since the neighbouring read in the same block uses a **different stride**: the port reads the
`+0x50` value as `hb + rec_e + 2u * slot + 0x4a` ✓ (`f.c:25400` ✓, 2-byte stride ✓) but the reaction code as
`rec_e + slot + 0x38` ✓ (1-byte stride over the same record ✓). If the shipped's reaction-code read is
`rec_e + 2*slot + 0x38` ✓ (or `4*slot` ✓) then the port reads the wrong byte of the action record for every hit ✓
which would produce exactly the observed pattern: valid-looking small codes (1, 2, 0x0b ✓) instead of the shipped's
`0x2e` ✓✓

Specified next step: disassemble the shipped's `fn_01441e` from ~0x14c4c to 0x14cb4 ✓ (the block that ends in the map
store `mov [eax+0x17], edx` at 0x14cb4 ✓ docs/258 ✓) and transcribe how `edx` is built: which record base, which
stride times the slot, and which displacement ✓ Compare with `f.c:25266` ✓ and `f.c:25400` ✓ If the stride or
displacement differs ✓ that is item 2b's second fix ✓ and the gate is the same dump-side census this doc already
uses: `HIT ... atype=` reaching 0x2e ✓ `+0x17` transitions into 0x33/0x34/0x35 appearing ✓ enemy debt descending
toward 0 ✓ and then the award pair (`+0x32 = 10` on a settled type-31 record ✓ `player[+0x52]` rising ✓).

Item 2b status: clamp-guard fix landed and evidenced ✓ (docs/254 ✓); reaction-state table proven faithful ✓ (this
doc ✓); the open defect is narrowed to **the byte the port reads as the reaction code** ✓
