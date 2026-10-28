# 112: one DS address, two port arrays -- the rig was reading the unfilled one

## What the state oracle reported

Over the whole `ep1_kill3` window (`/tmp/rig/ents-orig/orig` vs `/tmp/rig/rx1`):

```
DS 0x53d0c:  shipped live in 974/974 samples (pool[0] = 0x293038 ...)   port: 0/1308 samples
DS 0x53d08:  shipped live in 974/974 samples (e8 03 00 00 | 38 30 29 00 = 1000, 0x293038)   port: 0/1308
```

That looked like a missing sound-slot table (the docs/98 "missing punch SFX" hypothesis). It is partly a
**rig/modeling defect**: the port declares *two* objects for DS 0x53d0c:

* `g_dat_53d0c[100]` (`uint8_t *`, f.c:2433) -- the real one, filled by the master loader `fn_03690b` from
  `NIGHT.RES` (`"1.voc".."100.voc"`, f.c:2549) and used by the combat path
  (`fn_01441e`: `fn_04185e(g_dat_53d0c[g_tab_51c68[e[+0x0d]]])`, f.c:24616);
* `g_pool_53d0c[100]` (`uint32_t`, f.c:7804) -- a second model of the same address, written by nobody, used
  only by the teardown `fn_037922`.

`gen/rig_cells.json` resolved the contested cell in favour of the **unfilled** array (the tie-break is in
`port_dup_claims.json`, which already listed `0x53d0c`), so every cross-side comparison of that cell compared
shipped data against zeroes. Same class as the docs/109 `g_dat_53ea8` case, and it is why "rig defects get
fixed, not smoothed": the finding had to be retired before any SFX conclusion could be drawn.

## Fix

`g_pool_53d0c` is deleted; `fn_037922` now frees through `g_dat_53d0c` (`heap_free_host(host_to_off(...))`,
guest 0x041c31). The cell table regenerates to `g_dat_53d0c` (ptr=1) and the dump is live:

```
port after (/tmp/rig/vc1): 0x53d0c -> g_dat_53d0c nonzero 488/488 (element 0 = arena offset 0x493e8bc)
```

`make test` and `make selftest` still pass (`ALL PASS` / `ALL PASS (0 failures)`).

## Still open, and it is the aliasing family again

DS 0x53d08 is *not* a separate table. The guest reads it as `dword [EAX + 0x53d08]` (a single access, in
`fn_03a829` at 0x0003ab4b) i.e. **4 bytes below the pool base**, while the port models it as its own array
`uint32_t g_tab_53d08[101]` (f.c:22599, read at f.c:22698 as `g_tab_53d08[g_dat_63364]`) -- an interval that
*overlaps* `g_dat_53d0c` (0x53d08 + 4*101 = 0x53d08+0x194 covers the pool). In the shipped DS that dword holds
`1000` next to the pool pointer, so it is a real DS neighbour produced at runtime (the region is not
image-initialised: `0x53d08-0x7600 = 0x4c708` lies past the end of NIGHT.EXE, 0x4c217 bytes). The correct
model is one DS-adjacent object, not two independent arrays. Re-running the static audit confirms the check
already files it (`reports/static_findings.json`, c2_aliasing):

```
{"kind":"port-span-overlap","a":"g_tab_53d08","a_span":["0x53d08","0x53e9c"],"b":"g_dat_53d0c","b_span":["0x53d0c","0x53d70"],"overlap":100}
{"kind":"port-span-overlap","a":"g_tab_53d08","a_span":["0x53d08","0x53e9c"],"b":"g_dat_53e24","b_span":["0x53e24","0x53e2e"],"overlap":10}
{"kind":"port-span-overlap","a":"g_tab_53d08","a_span":["0x53d08","0x53e9c"],"b":"g_dat_53dd4","b_span":["0x53dd4","0x53dd5"],"overlap":1}   (x5 more)
```

with `c7_ds_table_bases` showing why: base `0x53d0c` has 6 access sites in 5 functions, base `0x53d08` has
exactly **one** (`fn_03a829`), and the shipped DS holds a single dword there (`1000`), not a 101-entry table.
So `g_tab_53d08[101]` is an invented object that walks over the sound pool, five `0x53dd*` globals and
`g_dat_53e24`; it has no writer in the port (only the declaration and one read, f.c:22698 via
`g_dat_63364`). Filed as the next aliasing correction -- deliberately not changed in this commit, because the
read at f.c:22698 has to be re-derived from the guest first (that site's index base is not the pool), and
guessing it would break the SFX path that docs/109 just made observable.

## Semantics correction for docs/109 (kept, but re-labelled)

`entity[+0x32]` is the **sound/event queue** head: `fn_031f10(p, val)` appends at `p[+0x32 + p[+0x31]*4]` and
bumps the count byte `+0x31`, and the reaction/death paths in `fn_01441e` push ids through it
(`+0x32 = 5`, `+0x32 = table[idx*0x88 + ...]`). `fn_015295` then adds that queued id into
`*(g_player_ent + 0x52)`. So the causal chain measured in docs/109 (dying enemy `+0x32` 0 -> 10 giving the
player 10 EXP per kill) is real, but the field is a sound-slot id rather than a designed EXP column -- which is
also why the same value feeds the SFX question. docs/109's fix and its red/green numbers stand.
