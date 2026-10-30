# 185 — Item 1 narrowed: the title track comes out of the room table, and the table is byte-exact

Continuing docs/184 §4 test 2 ("which shipped site asks for track 2 / `0x9ddf`").

## 1. The selection is a room-table lookup, and the port's lookup is identical to shipped's

`fn_010d47`'s tail computes the track from the room record (`0x010d9d-0x010db9`):

```
0x10d9d  mov eax, [DS 0x63370]          ; stage
0x10da2  dec eax
0x10da3  imul edx, eax, 0x28            ; (stage-1) * 40
0x10da6  mov eax, [DS 0x63354]          ; the room record
0x10dab  mov eax, [eax + 0x40]          ; room[+0x40]
0x10dae  shl eax, 2
0x10db1  add eax, edx
0x10db3  mov eax, [eax + 0x2da4]        ; DS 0x52da4 table
0x10db9  call 0x10b1d                   ; track index in EAX
```

The port models the same arithmetic (`f.c`, `cd_track_for_room`):

```c
int32_t idx = (int32_t)((struct ua32 *)((room + 0x40)))->v + ((int32_t)g_dat_63370 - 1) * 10;
return g_tab_52da4[idx];
```

and its 100-entry table is **byte-identical to the shipped image**: reading 100 dwords at `NIGHT.EXE` file offset
`0x52da4 - 0x7600` and comparing to the port's initializer gives **0 mismatches**.  First twelve entries, both sides:

```
0x2 0x2 0x2 0x8 0x2 0x2 0x2 0x2 0x2 0x2 0x3 0x3
```

## 2. Which is why the title track is track 2

Table index **0 is `0x2`**, and `0x2` is the index whose LBA the schedule witness prints as `0x9ddf` (261 s) — the
track shipped selects at ~30 s on the title route and the only one the shipped mix fingerprints as present (+0.837,
docs/183 §3).

So shipped's title-screen selection is simply "the room record's `+0x40` is 0 at stage 1, table[0] = 2".  The port,
on the same drive, produced 9 (and earlier 1) instead — the table and the arithmetic are both correct, so the value
that differs is the **input**: `DS 0x63354` (which room record is current) and that record's `+0x40` field, at the
moment `fn_010d47` runs.

## 3. What that means for the fix

Two statements are now separated cleanly:

1. **The port cannot make the title selection while the title screen blocks the frame loop** (docs/184 §5): the only
   path to the selection is `fn_03a829 → fn_010d47`, and while the presentation blocks inside one game step neither
   runs — the port's own cell dump shows a 65 s hole covering exactly that window.
2. **Even when it does run, the port's room pointer/`+0x40` must be the one shipped would have at that moment**,
   otherwise the lookup yields the wrong track index (the table itself is proven correct).

Both are checkable with existing tooling, and test (2) is now cheap: `DS 0x63354` and the record it points at are
readable on both sides — the port from its `cells.bin` entity/record blocks and `meta.json` pointer cells, the shipped
side from `state_*.heap.bin` (docs/124).  The next measurement is therefore "what is `[room+0x40]` on each side when
the refresh fires", not another look at the audio layer.
