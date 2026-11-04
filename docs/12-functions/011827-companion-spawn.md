# 0x011827 — the mirrored companion spawn (L11, 293 B) — PORTED

Port name `fn_011827`. Test: `test_fn_011827` in `test/test_f1.c` (12 checks).

## What it does

A stage object spawns a **paired child entity of type 0xfa (250)** at its own position offset by
±dx / +dy, on the side selected by the source's facing byte.

```
fn_011827(EAX = src entity, EDX = dx, EBX = -> new+0x05, ECX = dy, [ebp+0x10] = -> new+0x17)   ; RET 4
  if (src[+0x1b] == 0) { blk = 0x041bdc(0x147); e = blk ? fn_010efc(blk, src[+1] + dx, EBX, src[+9] + dy, 0xfa, 2) : 0; e[+0x1b] = 0; }
  else                 { blk = 0x041bdc(0x147); e = blk ? fn_010efc(blk, src[+1] - dx, EBX, src[+9] + dy, 0xfa, 2) : 0; e[+0x1b] = 1; }
  e[+0x30] = 1;                       ; byte store
  e[+0x17] = [ebp+0x10];
  if (src[+0x17] != 4) { DAT_00063364 = 0x13; DAT_00063368 = 0; }
```

* The x delta is **added** when `src+0x1b == 0` and **subtracted** otherwise (`ADD` vs `SUB` at
  `0x011887` / `0x0118eb`); the same byte is stamped into the child's `+0x1b`.
* The source's facing byte is **not** modified here, so repeated spawns stay on the same side
  (pinned by the test's third call).
* `DAT_00063364 / DAT_00063368` are the same "element event" latch pair `fn_031f5f` writes; here the
  event id is the constant `0x13` with value 0, and it is skipped when the source's `+0x17` is 4.
* Failure model kept faithful: when the block allocation returns 0 the original still writes through
  the null result (`+0x1b`, `+0x30`), so the port does the same instead of guarding.

## Model correction found while porting (applies to the whole entity layer)

The two per-type arrays and the entity-type table span **all 260 entity types (0..0x103)**, not the
101 named by the stage loader. Proof from the DS layout:

| array | start | 0x104 * element size | next DS global |
|---|---|---|---|
| `DAT_000639ec` (per-type `.spr`) | 0x639ec | 0x410 | **0x63dfc** = the stage sprite pointer |
| `DAT_00053818` (per-type `.act`) | 0x53818 | 0x410 | **0x53c28** = the gun action table pointer |

Both land *exactly* on the following global, so the span is 0x104 entries with no gap. Consequences,
all now reflected in the port:

* `g_dat_639ec` / `g_dat_53818` are `[…0x104]`, `g_dat_53e9c` is `0x88 * 0x104`.
* The teardown loops in `fn_037347` / `fn_0374ad` now use the original bounds `1..0x103` with the
  explicit `0xc9` / `0xfa` skips instead of the earlier clamp to `0x65`.
* **Why the skips exist:** `fn_010efc` dereferences `DAT_639ec[type]` with *no* null check
  (`+0x38 / +0x04 / +0x08 / +0x34 / +0x0c / +0x10` are read straight into the new block at `+0x127…
  +0x13b`), so types `0xc9` and `0xfa` must hold **shared, never-per-instance-freed** tables. Type
  `0xfa` is precisely the companion type this function spawns — which is why it is skipped: a
  companion spawn must not (and cannot) own that slot.
* Only slots `0..0x64` have file names (`kTypeSprNames` / `kTypeActNames`, 101 names each), so the
  stage loader still only fills those.

## Test notes

* Must seed `g_dat_639ec[0xfa]` with a real block: the unguarded dereference above faults otherwise
  (this is what the test's "shared sprite struct read unguarded" check pins).
* Seed the `0xfa` entry of `g_dat_53e9c` to observe the table -> block propagation (`+0x25`, `+0x29`,
  `+0x23`).
* Entity list heads (`g_dat_53c70 / 53c74 / 53c68 / 53c80`) are zeroed and restored; count and both
  link fields (`+0x13f` next / `+0x147` prev, port-widened) are asserted after the second spawn.
