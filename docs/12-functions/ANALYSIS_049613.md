# 0x049613 — recurring-event date matcher (fn_04574a `mode<0` path)

Status: **ANALYZED** (disasm + ROM tables + structure + ABI). Python ref + C mirror + diff
tests: **DEFERRED to next context** (236 insn; started fresh, setup cost consumed budget).
Full disasm saved to `scratch/dis_049613_raw.txt` (236 instructions).

## ABI (confident)
```c
int32_t fn_049613(struct dec_datetime *d);
```
- In: `EAX = d`. Internally `ECX = d` (working pointer to the input struct).
- Out: `EAX = EDI` (result). **Also writes `[d+0x20] = EDI`** (i.e. `d->mode = EDI`) @0x498b9.
- Sole caller: `fn_04574a` (date_norm): `if (d->mode < 0) fn_049613(d);` — return ignored,
  but the `d->mode` side-effect is real. Currently a no-op stub in the C port.

## Global dependencies (confident — addresses from disasm)
- `0x537ca`: holds a **pointer**; `flag = *(uint8_t *)*(void **)0x537ca`. If `flag == 0`
  the function returns 0 immediately (@0x4961e-0x49629). (Gate on some "events enabled" state.)
- `0x53734` = **G1**, `0x53758` = **G2**: two global `struct dec_datetime` reference dates
  (same field layout: day@0x0c, month@0x10, year@0x14, dow@0x18, doy@0x1c, mode@0x20).
- ROM day-count tables (int16, little-endian) — **extracted & verified** (standard Gregorian):
  - nonleap cumulative @`0x5195e` (14 entries): `[0,0,31,59,90,120,151,181,212,243,273,304,334,365]`
  - leap cumulative    @`0x5197a` (13 entries): `[0,31,60,91,121,152,182,213,244,274,305,335,366]`
  - direct 16-bit read @`0x51962` = 31 (a doy threshold, @0x4977c and @0x4987a).

### Day-in-month formula (verified)
The code loads two int16 entries 2 bytes apart and subtracts:
- nonleap: `A=[0x5195e + m*2]`, `B=[0x51960 + m*2]` → `days = B - A = nl[m+1]-nl[m]`
- leap:    `A=[0x51978 + m*2]`, `B=[0x5197a + m*2]` → `days = B - A = lp[m]-lp[m-1]`
  (0x51978 is the last nonleap entry `nl[13]=365`; the leap table begins at 0x5197a.)
This yields the standard day counts (Jan=31, Feb=28/29, …) **with `m = d->month` behaving
1-based** (m=1 → Jan → 31). NOTE: fn_04574a treats `d.month` 0-based for its own table
(`month % 12`). Whether the normalized month handed to fn_049613 is 0- or 1-based must be
confirmed when the ref is written — **the C-vs-Python diff test does not depend on this**
(both replicate the same raw memory access).

## Callees (all ported)
- `fn_0495eb` (0x495eb): date compare `int32_t fn_0495eb(struct dec_datetime *x, struct dec_datetime *y, int32_t ref_year)`.
- `fx_less` (0x498c6): `uint32_t fx_less(const struct fxnum *a, const struct fxnum *b)`.
- `is_leap` (0x494cb): leap-year flag.

## Control-flow structure (medium confidence — pin exact branches in the ref)
1. **flag gate** (0x4961e-29): `flag = *(u8*)*(void**)0x537ca; if (!flag) return 0;`
2. **date_cmp(G1,G2)** @0x4963c → result saved at `[ESP+0x10]`. Depending on the sign, pick
   `EBX/ESI` = {G1,G2} vs {G2,G1} (@0x49641-5a). EBX becomes the "primary" reference, ESI the secondary.
3. **is_leap(d->year+0x76c, d->month)** @0x4966a → leap; **day-in-month** → `[ESP+0xc]`.
4. `[ESP+0x14] = 0` (a "match" flag).
5. **Pass 1 on EBX** (0x496a8-0x497b1):
   - if `[EBX+0x20] != 0` (G1 has a mode/doy set): **doy-based** match path (0x49763-0x49791),
     using `is_leap` + the 31 threshold @0x51962.
   - else: **month gate** — compare `d->month` vs `[EBX+0x10]`; if greater → match=1; if less →
     match=0; if equal → **IDIV-7 week-of-month arithmetic** (below) using `[EBX+0x0c]` (a week
     field, 1..5) and `[EBX+0x18]` (a reference dow). Result stored to `[ESP+0x14]`.
   - if `[ESP+0x14] != 0`: `EDI = (fx_less(d, EBX) == 0)`.
6. **branch** (0x497b4-0x497c6): if `EDI != 0` → Pass 2. else `ESI=[ESP+0x10]` (cmp result);
   if `ESI == 0` return 0; else `EDI = ESI`, return.
7. **Pass 2 on ESI** (0x497cb-0x4989a): the **same** doy/week arithmetic as Pass 1, but against
   the secondary global ESI (near-duplicate block, uses `[ESI+...]` fields).
8. **tail** (0x498a9-0x498b5): `EBX=[ESP+0x10]`; if `EBX != 0`: `EDI = EBX - EDI`.
9. **return** (0x498b7-0x498c5): `EAX = EDI; [d+0x20] = EDI; RET`.

### The IDIV-7 week arithmetic (the intricate core — MUST mirror exactly in the ref)
For a global S (EBX or ESI) with `S->month == d->month` (0x496c7-0x49761 / 0x497ec-0x49861):
```
q1 = (d->dow + 7 - S->dow) / 7        (signed, SAR sign-extend then IDIV 7 -> toward-zero)
a  = d->day - q1
q2 = (d->dow + 6 - S->dow) / 7
b  = d->day - 1 - q2
week = S->day                          (the "which week of month" field, 1..5)
dim  = day-in-month(d->month, leap)    (= [ESP+0xc])
if (week == 5):   match = (dim - 7 > a) && (dim - 7 >= b)      // 0x49717-0x49761
else:             lo = (week - 1) * 7 + 1
                  match = (lo <= a) && (lo > b)                 // 0x4973c-0x49761 (confirm dirs)
```
The exact inequality directions (JGE/JG/JLE/JL) are easy to get subtly wrong; the Python
reference must transcribe each compare verbatim from the disasm, and the diff test will
lock the transcription.

## DEFERRED — next-context checklist
1. Python ref `scratch/fp_049613_ref.py` mirroring the disasm exactly (both passes verbatim).
   **NOT standalone — transitive callee ports required in Python:** the `fn_0495eb` call @0x4963c
   (args EAX=G1, EDX=G2, EBX=`d->year`; returns 0/1) transitively needs Python ports of
   `fn_049506 -> fn_04574a -> fn_0498f6`. The cycle is broken because `fn_049506` builds its
   temp struct with `mode=0`, so `fn_04574a(temp)` takes the `TEST EBX,EBX;JGE` path and SKIPS
   the `fn_049613` call (no infinite recursion). `is_leap` + `fx_less` are self-contained (easy).
   All five are already ported + verified in C (f.c: fn_0495eb~3855, fn_049506~3826, fn_04574a~3785).
2. C mirror `fn_049613` in `port/src/f/f.c`; **replace the no-op stub**; declare in `f.h`.
   Model globals `0x537ca` (ptr-to-flag), `0x53734`/`0x53758` (G1/G2) as `extern` vars the
   test controls.
3. Differential test (C vs Python) over varied (d, G1, G2, flag) inputs.
4. **Re-run** `test_fn_04574a_date_cluster` + `test_fn_04297a` to confirm the previously
   stubbed absolute values now match (they currently assume the no-op stub).
