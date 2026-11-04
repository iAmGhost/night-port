# 0x037b03 — THE STAGE LOADER (L13, 1304 B)

Ported as `fn_037b03(int32_t resume_flag)` in `src/f/f.c`.
This is the function that turns one stage number (`DAT_00063370`, only ever 1..N) into the whole
per-stage data set: the room chain, the stage sprite struct, the stage record file and the entity
type table. It is the producer behind the L15 pair `0x0374ad` / `0x0376c8` (playability task #1)
and it fills `DAT_00053e9c`, the table `fn_010efc` (the entity constructor) consumes.

## Name construction

All four names are built by the streaming sprintf `fn_0428f8` (0x0428f8 -> `fn_044c23`), with the
stage number supplied by `DAT_00063370`:

| format | resource | loaded by |
|---|---|---|
| `stage%d.wal` | the room chain | `fn_03c6ec` + `fn_03c853` recursion, then `fn_03cad3` |
| `st%ds.spr`   | stage sprite struct (0x3c) | `fn_03d8d4` -> `DAT_00063dfc` |
| `stage%d.stg` | stage record file | `fn_03e0a0` per record |
| `night%d.chr` | the entity type table | raw stream reads into `DAT_00053e9c` |

Other string templates living next to these in the image (`0x5157c`): `number3.spr`, `gameov`,
`night%d.chr`; they belong to other loaders.

EAX (`resume_flag`) selects the mode: `0` = full load, non-zero jumps straight to the `.stg`
section (so a resume skips the room chain and the stage `.spr`).

## `.wal` — the room chain

```
g_dat_63358 = 0
name = "stage<N>.wal"
node = heap_malloc(0x354)  ->  fn_03c6ec(node)  ->  DAT_0006334c = node
fn_0367c9(name)                      ; seek the resource stream to the entry
DAT_0006335c = 0                     ; chain counter
fn_04192e(&DAT_000639d8, 1, 4, io)   ; the room count, read straight into the global
DAT_000639e8 = 0                     ; per-stage node serial
fn_03c853(DAT_0006334c, io)          ; recursive room loader
fn_03cad3(DAT_0006334c)              ; -> DAT_000639dc / e0 / e4 (stage origin)
```

Port: the allocation is **0x35c** (see `docs/12-functions/03c853-stage-room-chain.md`, the widened
back link at `+0x354`).

## `.stg` — the record loop

```
DAT_00063e14 = stream dword              ; record count
DAT_00062a90 = 0                         ; records appended to DAT_0005d0b4
for (i = 0; i < DAT_00063e14; i++) {
    rec = heap_malloc(0x59) -> fn_03e0a0(rec, io)   ; 1 kind byte + 4 sub-records (0x14 each)
    switch (rec[0]) {
      case 1:                                     ; "load this entity type now"
        t = rec[1];
        if (t < 0x65 && DAT_000639ec[t] == 0) {
            pos = fn_042928(io);                  ; remember stream position
            DAT_000639ec[t] = fn_03d8d4(heap_malloc(0x3c), kTypeSprNames[t]);
            DAT_00053818[t] = fn_010010(heap_malloc(8), kTypeActNames[t]);
            fn_04247a(io, pos, 0);                ; rewind the .stg stream
        }
        break;
      case 5:                                     ; append to the DAT_0005d0b4 array (stride 0x14)
        r = DAT_0005d0b4 + 0x14 * DAT_00062a90;
        r[0] = rec[1];  r[2] = rec[9];  r[3] = rec[0xd];  r[2] = rec[5];  r[4] = 0;
        DAT_00062a90++;  rec[0] = 0;
        break;
    }
}
```

(`r[k]` above is a dword index; the exact store order is in the C port.)

The two per-type name arrays are image constants at DS `0x51d88` (`.spr`) and `0x52198` (`.act`),
0x65 entries each. They are reproduced verbatim as `kTypeSprNames` / `kTypeActNames`; the repeated
`gun.spr` / `gun.act` entries are genuine (unused types alias the gun sprite).

Type index layout (index = entity type, 0-based):

```
  0 gun   1..9 boss1..boss7 (boss3 has 3 parts)      11..17 mboss1-1..mboss6
 32..34 ena-1..3   37..40 enb-1..4   42..44 enc-1..3   47..50 end-1..4
 52..55 ene-1..4   57..59 enf-1..3 (enf-4 in the .act table only)
 62..65 eng-1..4   72..74 enh-1..4   77..80 eni-1..4   82..85 enj-1..4   87..89 enk-1..3
 92..94 enl-1..3
```

## `night%d.chr` — the entity type table

```
fn_0367c9(name);  fn_036853(name, 2, 0)     ; re-seek to entry offset + 2 (2-byte header)
for (i = 0; i < 0x65; i++) {
    e = DAT_00053e9c + i * 0x88;            ; imul 0x88
    fn_04192e(e+0x00, 1, 2, io);            ; word
    *(dword *)(e+0x04) = fn_041c8f(io);     ; one byte, stored as a DWORD
    fn_04192e(e+0x08, 1, 2, io);  fn_04192e(e+0x0c, 1, 2, io);
    *(dword *)(e+0x10) = fn_041c8f(io);     ; one byte, stored as a DWORD
    fn_04192e(e+0x14, 1, 2, io);  fn_04192e(e+0x18, 1, 2, io);
    fn_04192e(e+0x1c, 10, 10, io);          ; the 100-byte per-type sub-table (g_dat_53eb8 view)
    fn_04192e(e+0x80, 1, 2, io);  fn_04192e(e+0x84, 1, 2, io);
}
```

Each entry consumes exactly 116 bytes on the stream (2+1+2+2+1+2+2+100+2+2). This is the same
region `fn_010efc` reads (`+0` dword-for-type-0 / word-otherwise, `+4` byte, `+8` word,
`+0x80`/`+0x84`), and the `+0x1c` sub-table is the `g_dat_53eb8` view used by the AI handlers
(`fn_015a1f`, `fn_015d16`, …).

## Port-model decisions

1. **`g_dat_53818` is an ARRAY, not the gun alias.** It was modelled as
   `uint32_t *g_dat_53818 = g_dat_53c28;`. Ghidra shows `(&DAT_00053818)[type] = actTable`, and
   `fn_010efc` reads `g_dat_53818[type]` into `block+0x123`, so it is the per-type action-table
   array (DS 0x53818 … 0x539ab) whose element 0 *is* the gun table. Now
   `uint32_t g_dat_53818[0x65]` holding **heap offsets** (the struct-field offset convention);
   `fn_03690b` stores the gun offset in element 0 and its test was updated.
2. **`g_dat_639ec` widened 0x20 -> 0x65** (DS 0x639ec … 0x63b7f = exactly 101 pointers). No other
   port global lives in that range, so no overlap was introduced.
3. The `.stg` record node pointer from `fn_03e0a0` is a **heap offset** in the port model; the
   first draft of the port cast it to a host pointer and faulted at 0x14ec.
4. The original dereferences the record node and stores the sprite pointer even when the
   allocation failed (NULL). The port guards both and documents them as latent original bugs —
   there is no DOS low-memory mirror to reproduce them against.
5. Vararg cursor: the original pushes the *value* of `DAT_00063370` as `fn_0428f8`'s vararg
   cursor; the port passes a cursor pointing at the global so the stage number renders. This is
   the one site where the real vararg ABI stays ambiguous (`DAT_63370` only ever holds small
   integers 1..N, so passing it as an address cannot be right). Pinned by the test: with
   `g_dat_63370 = 1` the loader resolves `stage1.wal` / `st1s.spr` / `stage1.stg` / `night1.chr`.

## Test (`test_fn_037b03`, 12 checks)

A synthetic `NIGHT.RES` image (6 index entries, delta-encoded like `fn_0366a4`) carries
`stage1.wal`, `st1s.spr`, `stage1.stg`, `night1.chr`, `ena-1.spr`, `ena-1.act`. The index must be
made resident first (`fn_04270d("night.res","rb")` + `fn_0366a4`), otherwise `fn_0367c9` cannot
resolve any stage name and every loader reads from a stale cursor (symptom: `fn_010010` deserialising
a garbage record count and ASan reporting overlapping memcpy ranges).

Environment mirrors `test_fn_03690b`: `g_ndpool` static pool, `g_dat_53730` device table,
`g_dat_633b0 = SCREEN_BASE` (the index staging scratch), the open/ioctl knobs, and
`sys_file_lseek_pos = 0`.

Coverage: room count lands in `DAT_639d8`; the room node (dwords + row count); the geometry block
(`rows*0xc8`) and the three 0x100-byte tables; `DAT_6335c` reaching `DAT_639d8` (chain length 1);
`DAT_63dfc` allocated; record count 2 with only the kind-5 record appended; kind-5 dword mapping
into the `DAT_5d0b4` array (stride 0x14, 5th dword zeroed); kind-1 loading `ena-1.spr` into
`DAT_639ec[32]` and `ena-1.act` into `DAT_53818[32]`; `night1.chr` entry 0 and entry 40 (proves the
0x88 stride and the 100-byte sub-table, and that the `fn_036853(name,2,0)` re-seek skips a 2-byte
header).

## Follow-ups

* Wire `fn_037b03` into the L15 pair `0x0374ad` / `0x0376c8`, then into `main()`-level stage flow
  so a real stage loads from `TG_NIGHT.IMG` (the index/name resolution is now the only missing
  piece besides the CD backend).
* Verify the per-type name arrays against the real image once the CD/.IMG backend exists (the
  arrays are read from DS pointer tables, so a single wrong entry silently loads the wrong sprite).
