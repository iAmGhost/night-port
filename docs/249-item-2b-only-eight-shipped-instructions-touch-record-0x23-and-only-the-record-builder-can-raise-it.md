# 249 — Item 2b: exactly eight shipped instructions can move `word[record + 0x23]`, and only one of them can restart it

Static census (`scripts/audit/field_writers.py --disp 0x23`, with the docs/239 §1 operand-0 fix, whole-code scan):

```
0x11041  fn_010efc   mov word [eax + 0x23], 0        ; the record builder, one branch
0x11080  fn_010efc   mov word [eax + 0x23], dx       ; the record builder, other branch  <- the only raiser
0x138f6  fn_0137c1   inc word [eax + 0x23]           ; the HUD number field -- not an entity record (fn_0137c1
                                                       draws "P. 00000"; its +0x23 is a draw cursor)
0x1494e  fn_01441e   add word [eax + 0x23], dx       ; pool overflow, dx = (int16)[B+0x56] after the subtraction,
                                                       reached only when the pool went negative -> always <= 0 here
0x14964  fn_01441e   sub word [edx + 0x23], ax       ; the debt decrement (the damage path both sides use)
0x14984  fn_01441e   mov word [eax + 0x23], 0        ; the drain, when debt reaches <= 0
0x37756  fn_0376c8   mov word [eax + 0x23], 0        ; the PLAYER spawn
0x395d7  fn_038f3d   mov word [eax + 0x23], dx       ; the upgrade menu (player)
```

Indexed/SIB writes to +0x23: **none** ✓ so the list is complete ✓ (docs/239 §1's correction is what makes "complete"
mean something ✓)

Consequence for docs/248's restart census (56 restarts in the port, 9 in the shipped ✓ where a restart is `+0x23`
jumping back up by >= 10 ✓): **nothing in the shipped can raise `+0x23` on an existing record** ✓ -- the only
raising store is `fn_010efc` 0x11080, i.e. **a record being constructed** ✓ So every restart is a construction, and
the item-2b question becomes: *what is constructing kind-1 records, and how often* ✓

Why this is not answered by the spawn witness: `NIGHT_SPAWNTRACE` is armed inside `e20f_spawn` (`f.c:27013`) and
printed **7** events in the 300 s port run -- with no budget cap on that print ✓ so `e20f_spawn` really did run 7
times ✓ The other 49 constructions must come from the remaining `fn_010efc` call sites ✓ The port's call sites are:

| `f.c` | modelled caller | visible kind argument |
|---|---|---|
| 5604, 27817 | `fn_0376c8` / the CONTINUE rebuild -- player | 0 ✓ |
| 14473, 14485 | `fn_011827` reaction spawn (dx / -dx placement ✓ 0x11884-87 / 0x118e8-eb) | cut off in the listing ✓ **unread** |
| 15393-15412 | `fn_0113e7` three-way reaction spawn (x-0x0c / x-0x2c / x+0x16 ✓ and the mirrored set) | **unread** |
| 24984 | `fn_013c4d` effect actor | 2 ✓ (`..., 0xc9, 2` ✓) |
| 27007, 27049 | `e20f_spawn` (enemies ✓ witness-covered) | 1 ✓ |

**Next step, specified:** arm a witness *inside `fn_010efc`* printing `(kind, type_idx, x, y, block)` plus a caller
tag, run it once, and attribute the port's constructions per call site and per kind ✓ If `fn_011827` / `fn_0113e7`
reaction records are being built as **kind 1** in the port, then (a) the port's dump-side "enemy" population is
partly reaction entities ✓ (which would also explain docs/246 §2's hit-per-slot numbers being larger than the
shipped's ✓), and (b) the shipped's own kind argument at those sites -- 0x11884-0x1191a and 0x11651-0x11673 ✓ --
becomes the transcription target for the fix ✓ If they are kind 2 in both ✓ then the constructions are enemies
spawned through a path the witness missed, and the witness has to move up one level ✓
