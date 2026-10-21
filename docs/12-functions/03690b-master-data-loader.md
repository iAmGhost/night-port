# 0x03690b — master data loader (L13, 2555 B)

Status: **analyzed, not yet ported.** Every one of its 13 callees is ported
(`0x010010`, `0x010181`, `0x032464`, `0x036537`, `0x036626`, `0x0366a4`, `0x0367c9`, `0x03d8d4`,
`0x03d9cf`, `0x0417f3`, `0x0418ee`, `0x041af8`/`0x041bdc`, `0x042023`, `0x04270d`), so it is a
mechanical port once the ~13 destination globals are declared. It is the single biggest item on the
playability path: it is what turns `NIGHT.RES` into the game's runtime tables.

Decomp: `analysis/pseudo/149_FUN_0003690b_03690b.c`. Range `0x03690b..0x0372e2`.

## Shape

Fully **linear** — no branches except the `alloc == 0` guard around each loader call. The guard
pattern is repeated ~24 times:

```c
p = fn_041bdc(SIZE);                      /* heap_malloc wrapper (size 0 -> 1) */
tmp = (p == 0) ? 0 : LOADER(p, "name");   /* LOADER = fn_032464 | fn_010010 | fn_03d8d4 */
global = tmp;
```

Note the original keeps the result in a stack temp first, so a failed allocation stores the (NULL)
`p`, never garbage.

ABI: no register arguments (it does `canary_id` with the frame size from the disasm, then works
entirely through `DAT_000639b8`). Returns `1` in EAX with EDX preserved (`CONCAT44(extraout_EDX,1)`
is Ghidra's reg-ABI artifact).

## Step by step

1. `canary_id(?, frame)` — `push <frame>; call 0x418ee` at the entry.
2. `g_639b8 = fn_04270d("night.res", "rb")` — `fn_04270d` = `fn_0426f2(extra=0)` (ported). If it
   returns 0 → `fn_036537("NIGHT.RES File Not Found!!!\n")` (the error/exiting helper) and the
   function continues in the original (no early return — keep it faithful).
3. `fn_0366a4(g_639b8)` — reads the obfuscated index into `g_dat_633a0` + `g_tab_5d884` (ported).
   Everything after this resolves names through that table.
4. Ten `0x4b4`-byte blocks loaded with **`fn_032464`** (the numeric text-table loader):
   `"gun.key" → DAT_62aa0`, `"gun1.key" → DAT_62aa4`, `"gun2.key" → DAT_62aa8`,
   `"gun3.key" → DAT_62aac`, `"gun4.key" → DAT_62ab0`, `"catch.key" → DAT_62ab4`,
   `"catch1.key" → DAT_62ab8`, `"catch2.key" → DAT_62abc`, `"catch3.key" → DAT_62ac0`,
   `"catch4.key" → DAT_62ac4`. (These are the key-binding tables whose latches live in
   `g_dat_62ac8`/`g_dat_62b90` — see the `0x032464` port note.)
5. Three `8`-byte blocks loaded with **`fn_010010`** (record-table loader): `"gun.act" → DAT_53c28`,
   `"gun1.act" → DAT_53c2c`, `"gun2.act" → DAT_53c30`.
6. Aliases: `DAT_62a98 = DAT_62aa0` (active gun key table), `DAT_62a9c = DAT_62ab4` (active catch
   key table) — the "currently selected weapon" indirection.
7. `0x3c`-byte block with **`fn_03d8d4`**: `"pause.spr" → DAT_63e0c`.
8. The 100-entry sound loop:
   ```c
   for (i = 0; i < 100; i++) {
       const char *nm = kVocNames[i];          /* PTR table @VA 0x525a8 -> "1.voc","2.voc",... */
       int32_t size = fn_036626(nm);           /* index-table lookup, returns the value @+0x0d */
       DAT_53d0c[i] = heap_malloc(size * 2);   /* 0x041af8 */
       fn_0417f3(nm, DAT_53d0c[i]);            /* load the resource into the buffer */
   }
   ```
   The 100 names are near-pointers at VA `0x525a8` (entries every 6 bytes of string data from
   `0x51198`: `1.voc`, `2.voc`, `3.voc`, …). Dump them from the image when porting; they are the
   voice/sfx slot names.
9. More `0x3c` blocks through `fn_03d8d4`: `"system.spr" → DAT_639c0`, `"system2.spr" → DAT_639c4`
   (the `struct blit_ctl *` global already declared), `"over.spr" → DAT_63e08`,
   `"face.spr" → DAT_53c8c`, `"gage.spr" → DAT_63e00`, `"exp.spr" → DAT_63e04`,
   `*(uint32_t *)0x51d88 == "gun.spr" → DAT_639ec`.
10. `DAT_53818 = DAT_53c28` (active gun action table).
11. `*(0x520ac) == "mark.spr" → DAT_63d10`; `*(0x524bc) == "mark.act" → DAT_53b3c` (`fn_010010`);
    `*(0x52170) == "dust.spr" → DAT_63dd4`; `*(0x52580) == "dust.act" → DAT_53c00` (`fn_010010`).
12. Experience table:
    ```c
    fn_0367c9("exptab.dat");                                  /* name @VA 0x5156b */
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 5; j++)
            fn_042023(g_639b8, "%d ", &DAT_53c94[i * 0x14 + j * 4]);   /* fmt @VA 0x51576 */
        fn_042023(g_639b8, "\n", &slot_unused);                        /* fmt @VA 0x5157a */
    }
    ```
    i.e. a 6 x 5 dword table at `DAT_53c94` (row stride `0x14`) parsed from a text file.
13. `DAT_63358 = 0; return 1;`

## Port work left

1. **Declare the missing globals** in `f.h` (all `uint32_t` heap/pointer fields):
   `g_dat_62aa0 … g_dat_62ac4` (10 key tables), `g_dat_62a98`, `g_dat_62a9c`, `g_dat_53c28`,
   `g_dat_53c2c`, `g_dat_53c30`, `g_dat_53818`, `g_dat_63e08`, `g_dat_63e0c`,
   `g_dat_53d0c[100]` (sound buffers), `g_dat_53c94[30]` (exp table, byte offsets +0x00..+0x74).
   Already present: `g_dat_53c8c`, `g_dat_53b3c`, `g_dat_53c00`, `g_dat_639c0`, `g_dat_639c4`,
   `g_dat_639ec`, `g_dat_63d10`, `g_dat_63dd4`, `g_dat_63e00`, `g_dat_63e04`, `g_dat_63358`,
   `g_639b8`, `g_dat_633a0`, `g_tab_5d884`.
2. **Pointer/name tables** (image constants, read them with Ghidra `read_memory`, *not* `vad.py` —
   its `VA = file + 0x2600` mapping only holds inside the code segment, the data segment maps
   elsewhere): `kVocNames[100]` @`0x525a8`; single-name pointer cells `0x51d88`, `0x520ac`, `0x524bc`,
   `0x52170`, `0x52580`.
3. **Keep the heap-size oddity**: the `.key` tables get `0x4b4`-byte blocks, `.act` blocks `8`,
   `.spr` blocks `0x3c` — these are the *struct* sizes of `fn_032464`'s block, `fn_010010`'s block
   and `fn_03d8d4`'s block respectively, not data sizes.
4. **Integration requirement (mid-subsystems task):** `fn_04270d("night.res","rb")` and every
   `fn_0367c9`/`fn_04192e` read eventually reach `sys_file_read` / `sys_file_lseek`, which are stubs
   ("no CD backend"). To run for real, the sys layer needs a resource backend that
   (a) opens `NIGHT.RES` from `NIGHT/TG_NIGHT.IMG` (or the loose `NIGHT/NIGHT.RES`),
   (b) tracks a per-handle position so `fn_04247a`'s mode-0 seek sees `cur - remaining == buffered
   start`, and (c) refills the reader node in `fn_041d18`/`fn_041d47`. Until then, tests feed
   synthetic streams (see the `0x0366a4` / `0x03d9cf` tests and the `sys_file_lseek_pos` knob).

## Test plan (unit, no CD needed)

- Build a synthetic in-heap stream: run `fn_0366a4` first on a small fake index whose entries are
  `gun.key`, `gun.act`, `pause.spr`, `exptab.dat`, `1.voc` … (so the lookups hit), then call
  `fn_03690b` with `fn_04270d`/`fn_0367c9` neutralised (empty `g_tab_5d884` for the open, entries for
  the loads) and assert: the ten key-table globals non-zero and distinct, `DAT_62a98 == DAT_62aa0`,
  `DAT_53818 == DAT_53c28`, `g_dat_53d0c[i]` allocated for the names present, the exp table parsed
  from the text (values + row stride), `DAT_63358 == 0`, return value 1.
- Split the port into three slices so the suite stays green: (A) open + index + 10 key tables +
  3 act tables + aliases, (B) sound loop + `.spr`/`.act` tables, (C) `exptab.dat` + tail.

## Attempt log (2026-07-13h): port written, reverted because the open chain SEGVs

A complete port (globals + `kVocNames[100]` + the three guard helpers + the linear body) was landed
into `f.c`/`f.h` and compiled cleanly, but its unit test crashed and the change was reverted to keep
the tree green (`1905 PASS`). The insertion script that regenerates the port verbatim is kept at
`scratch/ins3690b.py` (scratch is git-ignored; re-run it to re-land the port).

Failure chain (ASan, stable across attempts):

```
fn_03690b -> fn_04270d("night.res","rb") -> fn_0426f2 -> fn_044ab5 -> fn_04262a
          -> fn_04488b -> fn_0492d0 (read of g_dat_53730 == NULL)
          -> store_arr53730 (write of g_arr_53730 == NULL)
```

Seeding the whole `test_fn_0426f2_04270d` recipe (0x53800 pool arena as `g_ndpool`, `g_dat_53730`
**and** `g_arr_53730` both pointed at an 8-dword table, `g_dat_536dc = 8`, `g_dat_53619 = 0`,
`g_dat_536d8 = 0`, `sys_ioctl_dx/cf = 0`, `sys_open_ret = 0`) did not stop it: the fault moved but
stayed inside `fn_04488b`. Two port-model warts were confirmed on the way and are worth fixing
independently:

1. `DAT_00053730` exists twice in the port as unrelated C variables (`g_dat_53730` used by
   `fn_0492d0`, `g_arr_53730` used by `store_arr53730`), so any caller must keep both in sync -- the
   existing tests already do this by hand ("Both reference DAT_00053730 via two C aliases").
   Unify them (one variable + an alias reference) before wiring real CD state.
2. `fn_04488b` has no non-CD / in-memory file path: it assumes a DOS handle with the device-table
   entry and `ioctl_device` succeeding, so `fn_04262a` cannot open anything in the unit harness.

**Consequence for the plan:** `0x03690b` is no longer a standalone function port; it is gated on the
CD/resource backend (mid-subsystems task). Order of work: (1) unify the `DAT_53730` aliases,
(2) give `sys_file_open/read/lseek` + `fn_04488b` a real handle model (a memory or file-backed
resource reader over `NIGHT/NIGHT.RES` / `TG_NIGHT.IMG`), (3) re-run `scratch/ins3690b.py`, add the
structural test (its body is in this file's history and worked up to the open), then mark the row.
