# 12 — Function Map (function-to-function port baseline)

Phase-0 inventory of **all 418 functions** in `NIGHT.EXE` (Ghidra project,
Ghidra 12). Every function was decompiled and dumped to
`analysis/pseudo/NNN_<name>_<addr>.c` (2.4 MB). This document is the map we
port against: one original function ⇒ one analyzed entry ⇒ one ported C
function ⇒ one verification.

## 0. Artifacts

| Path | Contents |
|---|---|
| `analysis/pseudo/*.c` | Decompilation of every function, 418 files |
| `analysis/functions.tsv` | `idx, start, end, size, name, thunk, external` |
| `analysis/callgraph.tsv` | `caller, callee, to_addr` — 3,478 call references |
| `analysis/rank.tsv` | Bottom-up work order: `level, addr, size, name` (level = call-graph depth, L0 = leaves) |
| `ghidra_scripts/DumpAll.java` | Regenerates all of the above (Ghidra 12 API) |

Caveat: `callgraph.tsv` contains only *direct* call references. Computed
calls (`call [reg]`, switch tables, function pointers) are missing, so
levels are a lower bound and some "roots" are actually called indirectly.

## 1. Scale

- 418 functions, 215,518 bytes of code, 3,478 call edges
- 10 weakly connected components: one giant game component (370 fns,
  0x10010–0x4a7a2), one isolated 39-fn cluster (0x461cc–0x48f9e, ~7 KB,
  identity TBD — candidate: standalone driver/library), and 8 tiny ones
- 81 leaf functions (called, but call nothing) — first porting batch
- Top-level entry chain (from `docs/05-logic.md` §1):
  `0x000420bc` (DOS4GW entry) → … → **`0x0003af18`** (game main)
  → `0x0003ae8e` init / `0x0003658d` audio / `0x0003690b` data /
  `0x0003a224` menu / `0x00037b03` stage load …

## 2. Preliminary zone observations (from pseudo inspection)

| Zone | Notes |
|---|---|
| 0x00010010–0x00014fff | Data/asset layer: `0x10efc` builds player state; `0x1194c`, `0x1441e` large loaders; resource fetch helpers `0x1519f` (x433 calls), `0x151d5` (x275) — both read a global base from `0x418ee` and subtract `DAT_000639dc` |
| 0x0001545e | Called x89, 1 KB — likely a central dispatch/handler |
| 0x00018e88–0x0002fe1e | **20 large functions, 3.9–7.3 KB each, suspiciously uniform** — leading hypothesis: per-asset/per-episode decode or draw routines (or one huge switch split by the analyzer). Identify in Phase 1 before porting anything that calls them |
| 0x00031a8e–0x0003628e | Frame pipeline: `0x3203d` entloop, `0x3209c` pipe, `0x322f5` keypoll, `0x347e3` keypoll2, `0x38098` render, `0x3d4e4` stgev (per earlier batch dumps) |
| 0x0003690b–0x0003cfff | Stage/combat core: `0x3690b` (2.5 KB), `0x37b03` stage load, `0x38f3d` (3.2 KB), `0x3af18` **main**, `0x3a224` menu |
| 0x0003cb38–0x0003cc6b | trig a/b/c — inline-trig ASM helpers |
| 0x0004185e–0x00041fff | Runtime helpers: `0x418ee` LOCK/UNLOCK stub, `0x41f76` `abs()`, `0x41e9f` (x223), `0x4192e` (x71), `0x41bdc` (x57, resource alloc) |
| 0x000420bc | Program entry |
| 0x000461cc–0x00048f9e | Isolated 39-fn cluster, `0x461cc` (3 KB) is its hub — not referenced by the main component in the direct-call graph; find its indirect callers (entry-point table? interrupt vector? function pointer) |
| 0x000490b0–0x0004a7a2 | Tail: `0x49613` (691 B), `0x4a24d`, `0x4a7a2` — small, probably CRT/DOS4GW bookkeeping |

## 3. Porting discipline (function-to-function)

For **each** original function (in `rank.tsv` order, L0 → L19):

1. **Analyze** — read `analysis/pseudo/<f>.c` + xrefs (globals, assets) →
   entry in `docs/12-functions/<subsystem>.md`:
   *signature, semantics (what it computes/does, invariants), constants,
   edge cases, globals touched, assets touched, verification method.*
2. **Port 1:1** — `port/src/f/<name>.c`, function name = Ghidra name,
   annotated `/* orig: 0xNNNNNNN */`. Mirror control flow and constants
   exactly; no redesign, no "improvements". Where decompiler output is
   ambiguous (cse artifacts, float10, register spills), consult disassembly.
3. **Verify** —
   * pure functions: unit selftest with fixed inputs, byte-exact expected
     output (expected values cross-checked against DOSBox runtime state or
     an independent Python re-implementation);
   * stateful/rendering: headless run + screenshot/state-log diff vs
     DOSBox-staging capture.
4. **Name** — once semantics are confirmed, rename the function in Ghidra
   (`mcp rename_function`), so Ghidra, docs, and port code share one name.

Status is tracked in `docs/12-functions/STATUS.md`
(`[x]` ported+verified / `[~]` analyzed / `[ ]` pending), seeded from
`analysis/rank.tsv`.

## 4. Target layout

```
port/
  Makefile
  src/
    main.c            # new entry, --selftest / --frames / --shot / --log
    f/                # one file per original function (grouped by subsystem)
      <name>.c        #   /* orig: 0xNNNNNNN */
    res/              # byte-exact asset pipeline (rebuilt from old port)
      container.c pcx.c spr.c fli.c voc.c wav.c png_out.c
    sys/              # modern replacements for DOS layer ONLY
      video.c audio.c input.c cd.c timer.c  # each documents the INT/DMA it replaces
```

`sys/` is the **only** place modern code is allowed: each file must state
which DOS mechanism (INT 0x10/0x21, SB16 DMA, MSCDEX INT 0x2E) it
replaces, and expose a 1:1 signature so callers read like the original.

## 5. Open questions (Phase 1)

1. Identity of the 20 uniform 4–7 KB functions (0x18e88–0x2fe1e).
2. What the isolated 39-fn cluster (0x461cc hub) is and how it's invoked.
3. Full global-variable map: audit globals per subsystem
   (Ghidra `audit_globals_in_function`) before porting its functions.
4. Exact switch tables / function-pointer tables the direct-call graph
   cannot see (candidate: `0x1545e` dispatch).
5. Confirm entry chain 0x420bc → 0x3af18 and where the DOS4GW→game handoff
   happens (0x420bc uses raw segment registers — DOS4GW startup tail).
