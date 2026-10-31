# 260 — Item 2b: the whole reaction block is byte-faithful, so the missing code 0x2e must be in the action-record *data*

docs/259 left two readings: the port reads the wrong byte for the reaction code, or the record's bytes differ ✓
The disassembly decides it ✓

**The reaction block, shipped, byte-for-byte** (`fn_01441e` ✓):

```
0x1480c  dl  = byte [[ebp-4] + 0x17]            ; the attacker's state byte
0x14812  [victim + 0x4e] = dl
0x1481b  [victim + 0x4f] = byte [ebp-0x2c]      ; slot
0x1481e  edx = [ebp-0x10]                       ; action-record base (rec_e)
0x14821  edx += [ebp-0x2c]                      ; + slot   (ONE-byte stride ✓)
0x14826  al  = byte [edx + 0x38]                ; the reaction code ✓
0x14829  [ebp-0x24] = eax                       ; atype ✓
...
0x149e0 / 0x149e6  cmp [ebp-0x24], 0xb / 0x14   ; the "strong" band the port models at f.c:25344/25351/25368 ✓
...
0x14c4c-0x14c54  dx = word [ ([ebp-0x2c]*2) + [ebp-0x10] + 0x4a ] ; 0x50 value, TWO-byte stride ✓
0x14c5b          [victim + 0x50] = dx                              ; port f.c:25400 matches ✓
0x14c6c-0x14c9c  the player-in-0x97 with 1 <= atype <= 9 exception -> [victim+0x17] = 0x14 ✓ (port f.c:25405-25410 ✓)
0x14ca5-0x14cb4  else: edx = [[ebp-0x24]*4 + 0x1b60] -> [victim + 0x17] = edx
                 (0x1b60 + 0x50000 = DS 0x51b60 ✓ i.e. g_tab_51b60 indexed 4-bytes-per-entry ✓ port f.c:25413 ✓)
```

So: **stride 1 for the reaction code at `+0x38 + slot`** ✓ (the port's `f.c:25266` ✓ correct ✓), **stride 2 for the
`+0x50` value at `+0x4a + 2*slot`** ✓ (the port's `f.c:25400` ✓ correct ✓), and **the map index is atype*4 into DS
0x51b60** ✓ (correct ✓, table contents proven identical ✓ docs/259 §1 ✓) ✓ The whole block is faithful ✓
docs/259 §3's stride-mismatch suspicion is disproved ✓

## What that leaves

`atype` in the port is 1, 2 or 0x0b ✓ (55 and 127 hits across two runs ✓ docs/259 §2 ✓), and only `0x2e` maps into
the 0x33 band ✓ -- **so the bytes the port's action records carry at `+0x38 + slot` are not the shipped's** ✓ That
moves item 2b's remaining defect from code to **action-record data** ✓: the `.act` record image the port loads
(the chain `fn_012616`/`fn_01441e` walk ✓ docs/107's CHAIN witness prints `rec` and dumps `rp[0x00]`,
`rp[0x04]`, `rp[0x6c]` ✓) must be compared against the shipped's `.act` bytes on disk ✓ -- the same class of defect
as docs/176's all-zero 68-byte sound-slot table ✓ (a table that was in the file but never harvested into the port
correctly ✓) ✓

Specified check, no gameplay needed ✓: take the `RECDUMP` block the port already prints (`f.c:26565-26578` ✓ 0x80
bytes of the live action record ✓ present in `/tmp/rig/*/port.log` ✓) and diff bytes `+0x38..+0x47` against the
corresponding record in the shipped's `.act` resource file (extracted by the existing `scripts/` resource tooling ✓
the records are 0x88-sized rows in that family ✓), for the player's punch/kick record the CHAIN witness already
names (`rec=000778bc`/`farrA=0007793c` in `/tmp/rig/v5a` ✓). If the shipped's row has `0x2e` where the port's has
`0x01`/`0x02` ✓ the fix is a data transcription, and its gate is already instrumented: `HIT ... atype=46` ✓
`+0x17` transitions into 0x33/0x34/0x35 appearing ✓ enemy debt descending ✓ and finally the award pair ✓

Item 2b status: **fixed and evidenced** = the `fn_01586b` clamp guard ✓ (docs/254 ✓); **proven faithful** = the award
routine and queue posts ✓, the per-type table ✓, the damage model ✓, `fn_013c4d`'s gates/boxes/facing ✓,
`g_tab_51b60` ✓ (docs/259 ✓), and the entire reaction block including the atype load ✓ (this doc ✓); **open** = the
action-record bytes at `+0x38 + slot` ✓, which decide whether reaction code 0x2e -- and thus the shipped's
0x33/0x34/0x35 player states, its descent into the hit band, and its per-type EXP award -- ever appear in the port ✓

## Addendum: the port loads the .act files at runtime, so "the record bytes differ" needs a sharper form

`f.c:2813-2815` build the action tables by **name** at runtime -- `g_dat_53c28 = act_block("gun.act")`,
`g_dat_53c2c = act_block("gun1.act")`, `g_dat_53c30 = act_block("gun2.act")` ✓ (the shipped's own resource-name
literals appear at `f.c:2774-2775`: 0x524bc "mark.act", 0x52580 "dust.act" ✓) -- and there is **no static action
table in `f.c`** ✓ (a grep for static act/action tables returns 0 ✓) ✓ So the port reads the same `.act` bytes the
shipped does ✓ and the byte at `+0x38 + slot` cannot differ by *data* unless the **index or the record base**
differs ✓ Two remaining readings, both cheap:

1. **the slot differs**: `[ebp-0x2c]` (shipped) is the animation frame slot the `rec + 0x6d` array maps the action
   slots to ✓ and the port computes the same mapping (`idx_a = (e+0x11) % *rec`, `v34 = u16[rec+0x6d array + 2*idx_a]`
   ✓ `f.c:25386-25388` ✓); the port's own witness prints `slotA=2,3,4` on the v5a route ✓, so comparing the slot
   sequence between the two sides (the shipped side via its `+0x11` values ✓ dump-visible ✓) decides it ✓;
2. **the record base differs**: `act_block()` resolves a name to a heap block ✓ and if the port binds a different
   `.act` row to the player's punch/kick chain (docs/107's CHAIN witness prints `rec=`/`farrA=` ✓), then `+0x38`
   lands on a different row ✓ and the shipped's 0x2e would sit in the row the port never selects ✓

Either way the defect is in the **action-table binding/index**, not in a table's harvested bytes ✓
