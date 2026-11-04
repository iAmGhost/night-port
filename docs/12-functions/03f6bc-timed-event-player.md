# 0x03f6bc — the timed event player (L12, 337 B) — PORTED

Port name `fn_03f6bc`. Test: `test_fn_03f6bc` (+ helper `f6bc_run`) in `test/test_f1.c` (8 checks).

## What it does

A script/timeline resource drives a **timed show**: each event waits for a frame tick, may start a
sound through the device command stream, then executes one resource-op block.

```
fn_03f6bc(EAX = resource name)  ->  uint32 result
  fn_0367c9(name)                                  ; seek the shared node DAT_000639b8 to the resource
  fn_04192e(hdr, 0x80, 1, DAT_000639b8)            ; 0x80-byte header
  if (WORD(hdr+0x04) zero-extended != 0xaf11) return 0
  cursor = 0; idx = 0
  DAT_00063e3c = 0; DAT_00063e40 = 0               ; reset the tick pair (only on the tag-match path)
  for (;;) {
      count = MOVsx WORD(hdr+0x06)                 ; signed!
      i = idx++; i &= 0xffff
      if ((int32)i >= count) return 0              ; timeline exhausted
      if (DAT_00063ec1 || DAT_00063ea4 || DAT_00063e89) return 1   ; user cut / abort
      while ((int32)(uint16)idx >= (int32)DAT_00063e40) {}          ; frame wait
      if ((uint16)idx == DAT_00053200[cursor]) {
          fn_04185e(DAT_00053d0c[ (uint8)DAT_000532f4[cursor] ])   ; start that sound slot
          cursor++;
      }
      if (fn_03f80d(DAT_000639b8) == 0) return 1   ; op stream finished
  }
```

Machine-code details that the port keeps exactly:

* the tag compare is `MOV AX, word [ebp-0x19c]` + `CMP EAX, 0xaf11` — **zero-extended**, and the
  buffer starts at `ebp-0x1a0`, so the tag is the WORD at header `+0x04` (same header geometry as
  `fn_03f80d`'s 16-byte block header, different tag);
* the count is read with **MOVSX** — `0xffff` is `-1`, so that header terminates instantly instead of
  running 65535 events (pinned by a test);
* the loop counter is **post-incremented inside the compare** (`MOV EAX,[i]; INC [i]; AND EAX,0xffff`)
  and the frame wait / tick-table compare use the already-incremented value;
* the frame wait is `CMP EAX, [DAT_63e40]; JGE spin` — signed, and it waits for the coarse tick to
  **exceed** the index;
* the tick table entry is a DWORD compared with a zero-extended 16-bit index (`MOV DX, word...`).

## Tables

| DS | model | size proof |
|---|---|---|
| `DAT_00053200` | `g_tab_53200[0x3d]` (61 DWORD tick values) | `0x53200 + 61*4 = 0x532f4` exactly (the next global) |
| `DAT_000532f4` | `g_tab_532f4[0x44]` (68 BYTE sound-slot indices) | `0x532f4 + 0x44 = 0x53338` (the sound-enable byte) |
| `DAT_00053d0c` | existing `g_dat_53d0c[100]` host pointers (the `1.voc`..`100.voc` slots filled by `fn_03690b`) | fed by `fn_03690b` + `fn_0417f3` |
| `DAT_00063e3c / 63e40` | existing tick pair (fine tick / coarse tick every 18) | written by the tick handler, zeroed here |
| `DAT_00063ec1 / 63ea4 / 63e89` | new bytes: two key gates + the abort byte | also the gates of `0x03a70e` |

`DAT_00053200` / `DAT_000532f4` have **no producer anywhere in the EXE** (only these two reads), so —
like `DAT_00052c3c`, `DAT_00052738`, `DAT_00052f08` — they are modelled as seedable zeroed tables that
the CD/data backend must fill.

## Untestable path (documented, not skipped silently)

The frame-wait spin can only be left when the tick handler raises `DAT_00063e40`, and this function
zeroes that counter itself before the loop — so in a unit test (single-threaded, no tick source) the
first iteration with `count >= 1` and no key held would hang. The tests therefore cover: tag refusal,
`count == 0`, the MOVsx sign, the tick-pair zeroing and all three gate bytes. The sound-start +
`fn_03f80d` op path belongs to the integration pass, once the sys timer drives the tick handler.

## Port-side limitation found while testing (open issue)

Re-seeking the shared resource node (`fn_0367c9` -> `fn_04247a(r, value, 0)`) after that node has
already been read from **does not land on the requested offset in the current port model** — the
second and later reads in a test returned data past the record instead of the record. Every case in
this test therefore re-opens the stream (`sys_file_image_set` + `fn_04270d` + `fn_0366a4`) through the
`f6bc_run` helper. Suspect: the `fn_04247a` mode-0 path consumes `value - (cur - remaining)` bytes
through the buffer *before* calling `fn_044807`, so once the buffer is drained, `remaining` no longer
tracks the handle position. This blocks random-access multi-resource loading and must be fixed for the
real CD/.IMG backend (it also gates the planned `0x03a70e` test).
