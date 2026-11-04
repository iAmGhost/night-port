# 0x010efc — the entity (actor) constructor

Ported as `fn_010efc` in `src/f/f.c`; verified by `test_fn_010efc` (16 checks).
Size 1025 B, L10, 248 instructions, **no loops**, only three calls.

## Why it matters

It is the constructor behind every on-screen actor. The stage/entity loaders
(`0x0376c8`, `0x0374ad`, and the AI/stage cluster above them) allocate a block and call it, and it
is the only writer of the entity list links, so `fn_0112fd` (remove), `fn_0323f8`/`fn_03237b`
(drain) and the walk loops in `0x013aad` finally have a producer. It also consumes the
**entity-type table** that `0x037b03` (stage load) fills — that table is the remaining gate for the
L15 pair `0x0374ad` / `0x0376c8`.

## ABI (capstone; Ghidra mangled the register args into `extraout_*`)

```
PUSH 0x24 ; CALL 0x418ee                 ; stack guard, EAX/EDX/EBX/ECX pass through
EAX  -> [EBP-0x10]  block                (also the return value)
EDX  -> [EBP-0x0C]  stored as dword at block+0x01
EBX  -> [EBP-0x08]  stored as dword at block+0x05
ECX  -> [EBP-0x04]  stored as dword at block+0x09
[EBP+0x10] dword  type index  (stored at block+0x0d; compared with DAT_00063370; drives all tables)
[EBP+0x14] byte   kind        (stored at block+0x00)
RET 8
```

## Behaviour

1. If `type_idx == DAT_00063370`, publish the block to `DAT_00053c80`.
2. Store the args; clear the header fields up to `+0x31`.
3. From the entity-type entry `DAT_00053e9c + type_idx*0x88`:
   * byte `+4` → `block+0x25`, word `+8` → `block+0x29`;
   * `type_idx == 0`: **dword** `+0` → `block+0x56`;
   * otherwise: **word** `+0` (same address, narrower load) → `block+0x23`, and for
     `1 <= type_idx <= 9` (signed `JL`/`JLE`) `block+0x56 = 0x30c`.
4. `fn_041e20(block+0x32, 0, 0x14)` — 20 zero bytes.
5. `block+0x123 = DAT_00053818[type_idx]` (the active action table, aliased by the master loader).
6. `spr = DAT_000639ec[type_idx]`; copy `+0x38, +0x04, +0x08, +0x34, +0x0c, +0x10` into
   `block+0x127..+0x13b`.
7. Append to the entity list (`DAT_00053c70` head, `DAT_00053c74` tail, `DAT_00053c68` count).
8. Fixed defaults — note `+0xaf = 4`, `+0xc7 = 1`, `+0xff = 0xffffffff`.
9. `block+0x73 = *(block+0x56) + (int16)block+0x23 + (int16)block+0x29`, copied to `+0x77`
   (so a negative table word shifts the spawn position), then `+0x10b/+0x107/+0x103` chain to 0.
10. `block+0x11f = prng_next() % 20` (the `SAR/IDIV` pair keeps the **remainder**).

## Port-model decisions

* **Link widening** follows the precedent already set by `fn_0112fd` / `fn_013aad`:
  `next @ +0x13f` (8 B), `prev @ +0x147` (8 B; original `0x143` shifted +4). Consequence: a block
  the original allocated as `0x147` bytes needs **0x14f** in the port — the callers must allocate
  accordingly.
* `DAT_000639ec` was modelled as a single pointer ("gun.spr"). It is really an **array of per-type
  sprite-struct pointers** indexed by entity type, so it is now `uint8_t *g_dat_639ec[0x20]` with
  the master loader storing element 0. This removes a latent model split.
* `DAT_00053e9c` is modelled as `g_dat_53e9c[0x88 * 0x20]`. It is the same DS region as the
  previously modelled `g_dat_53ea8` (base +0xc) and `g_dat_53eb8` (base +0x1c); the port keeps one
  array per observed base, so whoever fills the type table must fill those views consistently —
  **latent model split, left documented rather than silently unified** (unifying means reworking
  three already-verified ports).

## Test coverage (16 checks)

args stored verbatim; `DAT_53c80` latch on `type_idx == DAT_63370`; type-table byte/word/dword
paths (type 0 vs 2 vs 20); the `1..9` → `0x56 = 0x30c` window; the `+0x73` sum including a
negative `(int16)` table word; the six sprite fields; the 20-byte fill; the non-zero defaults;
`+0x11f` in `[0,20)`; first-entity list init and the append links.
