# 273 — Item 2b: the key-state array bookkeeping is faithful; what the port lacks is *make supply* — the chain-key slots are armed 4 times per run, the shipped's dozens per second

Instrument added under `NIGHT_ACTTRACE=1` (`port/src/f/f.c`, counters `g_kd_*` in front of `fn_034735` ✓): it counts
`fn_0347e3` entries, entries that saw at least one armed slot, switch hits on the chain-key cases, entries of
`fn_034735` / `fn_034635` / `fn_034353`, and prints `g_dat_52ff8` plus slots `0x1c/0x1d/0x38` ✓ (two comment lines were
mangled by the first insertion — `…state mach` / `…ine (62c58/62cd8) */` ✓ — repaired in the same change ✓)

## 1. The census (`scripts/rig/drives/ep1_chainhold.txt`, 90 s, `NIGHT_ACTTRACE=1`)

```
fn_0347e3 = 1,757,000 entries          armed_ticks = 738,938   (42 % of ticks see an armed slot)
switch hits on case 0x1d/0x2a/0x2c/0x36/0x38 = 4      fn_034735 = 0   fn_034635 = 0   fn_034353 = 1
gate g_dat_52ff8 = 1            slot[0x1c] = 1   slot[0x1d] = 0   slot[0x38] = 0
```

So the pump runs 1.76 M times and *is* fed armed slots — but the armed slot is `0x1c` (Enter ✓ not a chain key ✓), and
the **chain-key cases are reached four times in the whole run** ✓ A ring that gets four posts in 90 s cannot present the
two-adjacent-`0x1d` window a multi-code technique entry needs (docs/272 §2: shipped 73 % of samples have ≥2 ✓, port 33 %
✓) ✓ That is item 2b's starvation, measured at the dispatch itself ✓

## 2. The static census says the array bookkeeping is *not* the defect

Every code site naming DS `0x62c58` (19 sites, 9 owner functions) and its port counterpart:

| shipped owner | sites | port counterpart |
|---|---|---|
| `fn_032464` | 1 (`0x32487`) | `f.c:4214` `fn_041e20(g_dat_62c58, 0, 0x80)` ✓ |
| `fn_0343d1` | 2 (`0x34405/0x3441e`) | `f.c:8345-8346` ✓ |
| `fn_034635` | 5 | `f.c:9756-9780` ✓ |
| `fn_034735` | 5 | `f.c:8856-8861` ✓ |
| `fn_0347e3` | 1 (`0x34827`) | `f.c:8878` ✓ |
| `fn_039c03` | 1 (`0x3a152`) | `f.c:26903` ✓ |
| `fn_03a829` | 1 (`0x3aa0f`) | `f.c:23569` ✓ |
| `fn_03d244` | 1 (`0x3d4a5`) | `f.c:26982` ✓ |
| `fn_03fcbd` (INT 09 handler) | 2 (`0x3fe12/0x3fe33`) | `app/keymap.c:65/69` ✓ |

The clearing sites are also positioned faithfully: shipped `0x3aa04 call 0x348e2`, `0x3aa09 mov ebx,0x80`,
`0x3aa0e xor edx,edx`, `0x3aa10 mov eax,0x12c58`, `0x3aa15 call 0x41e20` ✓ sits **immediately before the room
`for(;;)` loop**, i.e. it wipes the per-key flags and counters **once per room entry**, not per tick ✓ — exactly the
port's `f.c:23568-23570` (`fn_0348e2(); fn_041e20(g_dat_62c58,0,0x80); fn_041e20(g_dat_62cd8,0,0x80);` before
`for (;;) {`) ✓ Same for `fn_039c03` and `fn_03d244` ✓

## 3. What is actually missing: the make stream

The shipped's arming comes from its INT 09 handler (`fn_03fcbd`, modelled at `keymap.c:53-71` ✓): first make arms
`g_dat_62c58[idx] = 1` (`0x3fe33` ✓), a re-make while the slot is still armed raises the auto-repeat counter
`g_dat_62cd8[idx]++` (`0x3fe26` ✓). On DOS a held key emits a **stream** of makes (BIOS typematic + the queued makes in
the INT 16 buffer), and dosbox reproduces it — that is how the shipped gets `fn_034735`/`fn_034635` dispatched often
enough to walk a slot `1 → 2 → 3 → 4` and reach the `fn_034353` posting branch ✓ (its census: `{1:430, 2:3, 3:11, 4:21}`
✓ docs/272 §2 ✓). The scripted port path arms a slot once per drive event (~1/s ✓), the typematic repeat I added
(docs/272 §3) only re-issues makes while `*held == 1`, and `keymap.c:63` gates *all* arming behind `*held == 0` ✓ — so
repeats feed the counter, never the slot, and the slot for the chain keys was seen armed exactly 4 times ✓

**Open, and it is now a small question:** `night_key_apply` arms `g_dat_62c58[0x1d]` on the first make and nothing in the
room loop clears it except the once-per-room `memset`, yet `slot[0x1d] = 0` at every witness point ✓ So either the arming
is being erased by a path the witness does not cover, or the chain-key press never lands in a window where `fn_0347e3`
runs. Next probe: a write-witness on `g_dat_62c58[0x1d]` (and `0x2a/0x2c/0x36/0x38`) tagged at each of the nine sites in
§2 ✓ — that distinguishes "cleared by something unmodelled" from "never armed in a tick window" ✓ Do **not** quote any
dispatch-count claim from `NIGHT_CENSUS` until its 0/503 defect (docs/272 §4) is fixed ✓
