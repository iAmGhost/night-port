# 180 — Two "uncalled" shipped functions: contiguity is not fall-through (self-corrected)

docs/179 §4 established that `fn_04589e` and `fn_04933a` have **zero direct callers** in the shipped binary, which
left three state-oracle rows unexplained: `DS 0x64090`, `DS 0x64094` (published by `fn_04589e`) and `DS 0x53451`
(the cell docs/173 §6 could only attribute to `fn_04933a`, "writer still called zero times").  The hypothesis filed
in the first draft of this document -- that the preceding function runs into them -- is **false**, and the disassembly
says so directly.  It is recorded here because the mistake is the interesting part.

## 1. The bodies are contiguous, and each one ends with `ret`

Contiguity first looked like the answer:

From `scripts/audit/functions.json`:

```
date_norm        entry 0x4574a  body_end 0x4589d      fn_04589e  entry 0x4589e   (gap: none, 1 byte)
store_arr53730   entry 0x49325  body_end 0x49339      fn_04933a  entry 0x4933a   (gap: none)
```

`date_norm` ends at `0x4589d` and `fn_04589e` begins at `0x4589e`.  `store_arr53730` ends at `0x49339` and
`fn_04933a` begins at `0x4933a`.

## 2. The disassembly of those two boundary instructions refutes it

Reading the last instructions of each predecessor, in the byte range right before each "continuation":

```
0x45897: pop ebp
0x45898: pop edi
0x45899: pop esi
0x4589a: pop edx
0x4589b: pop ecx
0x4589c: pop ebx
0x4589d: ret            <- date_norm does NOT fall through

0x49332: shl   edx, 2
0x49335: mov   dword ptr [edx + eax], ebx
0x49338: pop ebx
0x49339: ret            <- store_arr53730 does NOT fall through
```

Both predecessors return.  Contiguity is what a linker produces; it says nothing about control flow.  Adjacent bodies
are the ordinary result of compiling separate functions, so "the previous function runs into the next one" was a
reading of the *layout*, not of the *code*.  The claim is withdrawn.

So the position after this correction is: `fn_04589e` and `fn_04933a` have no CALL edge and no fall-through entry,
yet the shipped run demonstrably produces their output values (`DS 0x64090 = 1`, `DS 0x64094 = 1691857`, both
constant over a whole route).  One of those two facts is incomplete, and the cheaper explanation is that the
extracted edge set is incomplete -- which is the honest open question now.

## 3. What is still solid: these cells are runtime-only, and the writer set is incomplete

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

## 4. The two candidate explanations, and the test that separates them

`mem_access.json` splits DS accesses into `direct` (a literal `[0x00064090]` operand, which is what all four rows in
§3 are) and `based` (`MOV EAX, [EAX+0x53d08]`-style, which touches a base register plus a displacement and is *not*
the same as touching DS 0x5337c -- the trap docs/133 §2 had to learn).  A writer that reaches these cells through a
base register would be invisible to a symbol-indexed query, and `1691857 = 0x19D381` is an absolute flat-heap
address, i.e. exactly the kind of value the heap layer publishes.

1. **The edge extraction misses a non-CALL entry.**  Ghidra knows an xref into `0x4589e` even when it is a jump or
   an address taken as data; `call_sites.py` walks CALL edges only, and `disasm.py --search 9e580400` (the
   little-endian dword for `0x4589e`) found **0 hits** in the image, which argues against a data-table entry but does
   not prove the absence of a jump edge.  Test: ask Ghidra for xrefs *to* `0x4589e` / `0x4933a` rather than deriving
   callers from disassembly text.
2. **The writer is not `fn_04589e` at all on the runtime path.**  `fn_04589e` writes those cells, but a symbol-write
   in the binary is not the same as a write on the path this game executes; the value 0x19D381 could arrive from the
   heap layer through a based access.  Test: sample `DS 0x64090` / `DS 0x64094` from the first shipped sample onward
   -- if they are nonzero before any startup routine that the port models has run, the writer is elsewhere.

What is deliberately *not* done: no tail call was appended to the port's `date_norm` / `store_arr53730`.  With `ret`
at both boundaries, appending one would have fabricated a control-flow edge the shipped binary does not have -- the
exact failure mode docs/173 warned about when `fn_0445c9` needed its thunk split.  Three rows (`DS 0x53451`,
`DS 0x64090`, `DS 0x64094`) stay open, with the two tests above as the next actions.

## 5. A static-audit class this exposes -- the weaker, correct version

"Zero callers" is not a reachability verdict, and **"adjacent bodies" is not a control-flow verdict either**.  The
useful census is three-part, and each part has to be read separately:

- a CALL-edge census (`call_sites.py`) -- answers "who calls", nothing else;
- body contiguity (`functions.json` `body_end` vs the next `entry`) -- answers only how the image was laid out; both
  boundaries in this document are contiguous *and* both predecessors return;
- the last instruction of a body (`ret` or not) -- this is the one that decides whether a boundary is a real
  boundary, and it is a one-line disassembly query that this document skipped before writing a conclusion.

The lesson is procedural: the boundary instruction should have been read before the contiguity claim was made.
