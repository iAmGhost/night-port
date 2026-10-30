# 180 — Two "uncalled" shipped functions are fall-through continuations the port truncates

docs/179 §4 established that `fn_04589e` and `fn_04933a` have **zero direct callers** in the shipped binary, which
left three state-oracle rows unexplained: `DS 0x64090`, `DS 0x64094` (published by `fn_04589e`) and `DS 0x53451`
(the cell docs/173 §6 could only attribute to `fn_04933a`, "writer still called zero times").  The reason they have
no callers is not a dispatch table.  It is that **they are not entered at all in the shipped control flow -- the
preceding function runs into them.**

## 1. The bodies are contiguous

From `scripts/audit/functions.json`:

```
date_norm        entry 0x4574a  body_end 0x4589d      fn_04589e  entry 0x4589e   (gap: none, 1 byte)
store_arr53730   entry 0x49325  body_end 0x49339      fn_04933a  entry 0x4933a   (gap: none)
```

`date_norm` ends at `0x4589d` and `fn_04589e` begins at `0x4589e`.  `store_arr53730` ends at `0x49339` and
`fn_04933a` begins at `0x4933a`.  There is no CALL edge to find because there is no boundary to cross: the shipped
code simply continues.  Ghidra cut them into separate functions (which is why `call_sites.py` -- a CALL-edge tool --
reports 0 callers), and a port that models functions as separate C functions emits only the first half.

## 2. Confirmation that these cells are runtime-only

`DS 0x64090` / `DS 0x64094` are **not image-initialized**: `0x64090 - 0x7600 = 0x5CA90`, which is past the end of
`NIGHT.EXE` (311 831 B = `0x4C157`), so they live in BSS.  Their only shipped accesses are

```
WRITE  0x45946  fn_04589e   MOV dword [DAT_00064090], EBP
WRITE  0x4594c  fn_04589e   MOV [DAT_00064094], EAX
READ   0x445fe  fn_0445c9   MOV EDX, [DAT_00064094]      <- the entry thunk docs/173 had to wire by hand
READ   0x44604  fn_0445c9   MOV EAX, [DAT_00064090]
```

so the shipped run reaches `fn_04589e` before the thunk reads what it published -- consistent with a startup path
that the port's own thunk split (docs/173 §5, `fn_0445c9_startup()`) still does not complete.

## 3. The fix, and how it must be verified

Both port counterparts exist and stop at exactly the shipped boundary:

- the port's `date_norm` equivalent runs `0x45757-0x457ac` (`f.c:6912-6930`) and returns; the shipped body keeps
  going to `0x4589d` and then falls into `fn_04589e`;
- `store_arr53730` is called from the port (`f.c:9968`, `10105`, `10298`) but nothing continues into `fn_04933a`.

Because the shipped predecessor has its own early returns (`0x4576c-79` sanity check, `0x45799-9b` negative-year
check) and the port reproduces them as C `return -1`s, appending a tail call is a faithful reproduction rather than
an invention -- but only under the shipped fall-through condition, so two checks are required before claiming it:

1. `make -C port test` plus the existing date/handle fixtures (`04297a(clock2)`, the `0x44a3e-40` / `0x44c02`
   handle cases) must stay green, since the return value at the boundary is what callers consume.
2. The oracle must show `DS 0x53451`, `DS 0x64090` and `DS 0x64094` leave the "port holds zero" set on the canonical
   150 s route -- three rows from docs/178's 28 -- without introducing new ones.

## 4. A static-audit class this exposes

"Zero callers" is not a reachability verdict.  Two mechanisms defeat a CALL-edge census, and both are now visible in
this codebase:

- **fall-through continuation** (this document): a function's shipped body ends at another function's entry point;
- **indirect dispatch** (`dispatch_a` / `dispatch_b`, `cb_indirect_53644`).

A check worth adding: for every port function named after a shipped guest address, compare the shipped predecessor's
`body_end` with that entry, and flag contiguity -- that single comparison is what turned three unexplained
zero-cells into a named defect here.
