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
| 0x00010010–0x00014fff | Data/asset layer: `0x10efc` builds player state; `0x1194c`, `0x1441e` large loaders; resource fetch helpers `0x1519f` (x433 calls), `0x151d5` (x275) — both read a global base from `0x418ee` and subtract `DAT_000639dc` (entity x/y getters) |
| 0x0001545e | Called x89, 1 KB — collision/clamp helper: box test against `DAT_0005c8c0` table (stride 0x14), reads entity field `+0xd/+9/+1/+0x17` |
| 0x00017f13–0x0002fe1e | **Per-enemy-type AI/update handlers (20 fns, 3.8–7.3 KB).** All are the case arms of one `switch(*(int *)(&DAT_00053eac + type*0x88))` inside `0x31a8e` (case 0→`0x17f13`, 1→`0x18e88`, … 19→`0x2fe1e`). `0x31a8e` first runs the shared entity state machine (field `+0x17` = state, `+0x6b` = timer, `+0x103/107/10b` = last-known pos, `+0x11` = anim/action, `+0xd` = type id) then dispatches. Type table: `DAT_00053eac`, 0x88 bytes/entry. Player entity base: `DAT_00053c7c` |
| 0x00031a8e | Entity update dispatcher (state machine + per-type switch) |
| 0x0003203d–0x0003628e | Frame pipeline: `0x3203d` entloop, `0x3209c` pipe, `0x322f5` keypoll, `0x347e3` keypoll2, `0x38098` render, `0x3d4e4` stgev (per earlier batch dumps) |
| 0x0003690b–0x0003cfff | Stage/combat core: `0x3690b` (2.5 KB), `0x37b03` stage load, `0x38f3d` (3.2 KB), `0x3af18` **main**, `0x3a224` menu |
| 0x0003cb38–0x0003cc6b | trig a/b/c — inline-trig ASM helpers |
| 0x00040042 | LPT1 (port 0x3c8/0x3c9) status-poll write loop — printer/DA output? caller TBD |
| 0x00040475–0x0004052c | Memory helpers: byte/word/dword copy, fill, strided fill (row pitch via `DAT_00040010`) — video buffer primitives |
| 0x0004185e–0x00041fff | Runtime helpers: `0x418ee` LOCK/UNLOCK stub (returns global base ptr), `0x41f76` `abs()`, `0x41e9f` (x223), `0x4192e` (x71), `0x41bdc` (x57, resource alloc); `0x418e6` saves SS; `0x41c28` **heap insert** into the free list rooted at `DAT_00053350` (node stride 8, size 0x14, high-water `DAT_00053358`) |
| 0x000420bc | Program entry (DOS4GW tail, raw segment regs) |
| 0x000461cc–0x00048f9e | **CD/MSCDEX drive library (isolated 39-fn cluster).** Hub `0x461cc` scans device strings for drive letters (`d/e/f/g`, `&`, `.`, `>`, `6`) and runs an 8-slot ring state machine (`DAT_00053668`, slot stride 10, masks at `0x45dcc/0x45e1a/0x45e22`, callback `DAT_000536c2`). Invoked only via function pointers — consistent with the MSCDEX INT 0x2E usage in `docs/06-cdda.md`. Porting plan: analyze for observable behavior, then mark `[-]` replaced by `sys/cd.c` |
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

Status is tracked in `docs/12-functions/STATUS.md` (per-function table,
protocol, work queue, decisions log) and can be queried any time with
`uv run scripts/status_report.py [next_count]` (totals, per-level counts,
next pending in work order, ordering violations).

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

1. ~~Identity of the 20 uniform 4–7 KB functions~~ **RESOLVED**: per-enemy-type AI handlers (switch in `0x31a8e`, §2).
2. ~~Isolated 39-fn cluster~~ **RESOLVED (hypothesis high)**: CD/MSCDEX drive library, function-pointer invocation (§2). Confirm entry into the cluster from the CD-init call sites in Phase 2.
3. Global-variable map per subsystem (Ghidra audit) — do per subsystem just before porting it.
4. ~~Indirect dispatch tables~~ **PARTIAL**: entity-type switch found (`0x31a8e`); still need: any other function-pointer tables (e.g. `DAT_000536c2` CD callback, `PTR_LAB_*` jump tables in the CD cluster).
5. Confirm entry chain 0x420bc → 0x3af18 and the DOS4GW→game handoff.
6. What is LPT1 output `0x40042` for (printer? DIP input board?) — trace its caller.
