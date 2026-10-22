# 0x03a70e — the picture/CG slide-show driver (L12, 283 B) — analysis, NOT yet ported

Derived from the decompilation (`analysis/pseudo/*_03a70e_*`) plus a full capstone listing
(`vad.py dis 0x3a70e 0x11b`). It is a **clean win** in the capstone dependency scan (every callee is
already ported), and it is on the *playability* path: it walks a sequence of full-screen pictures
(the opening/interlude slide show), driving the device command stream as it goes.

## Callee status (all ported)

| callee | port name | role here |
|---|---|---|
| 0x0428f8 | `fn_0428f8(buf, fmt, &cursor)` | builds the picture resource name (varargs pushed by the caller) |
| 0x03a594 | `fn_03a594(dest, src, count)` | copy bytes, skipping `0xff` |
| 0x036041 | `fn_036041(name, dl)` | load + decode a screen image (0xff-skip RLE) |
| 0x04185e | `fn_04185e(cursor)` | start/advance the device command stream (ported 2026-07-13) |
| 0x03a5f7 | `fn_03a5f7(base)` | progressive horizontal reveal of the back screen |

## Control flow (exactly as the machine code reads)

```
slide = 0                                        ; [ebp-0xc]
save63e24 = DAT_00063e24 ; save63e28 = DAT_00063e28
DAT_00052ff8 = 0                                 ; byte: keyboard pump OFF (gate for fn_0347e3)
loop:
    fn_0428f8(buf20, <fmt @0x16b2>, DAT_00063370, slide + 1)   ; 4 pushed/loaded args
    fn_03a594(0xaa8c0, DAT_000633b0 + 0xa8c0, 0x5140)          ; restore the saved screen band
    fn_036041(buf20, 0)                                        ; load this slide
    while (DAT_00063ec1 == 0 && DAT_00063ea4 == 0 && DAT_00063e89 == 0) {}   ; wait for a key/tick
    if (DAT_00063e89 != 0) goto end                            ; abort the whole show
    if (slide != *(int32_t *)((uint8_t *)0x52f08 + DAT_00063370 * 4))
        fn_04185e(DAT_00053dd8);                               ; advance the device stream
    while (DAT_00063ec1 != 0 || DAT_00063ea4 != 0) {}          ; wait for the keys to be released
    if (++slide < *(int32_t *)((uint8_t *)0x52f08 + DAT_00063370 * 4)) goto loop
end:
    fn_03a5f7(DAT_000633b0)
    DAT_00063e24 = save63e24 ; DAT_00063e28 = save63e28        ; tick counters restored
    DAT_00052ff8 = 1                                           ; keyboard pump back ON
```

Notes for the port:

* The two spin loops are **key-state waits** on `DAT_00063ec1` / `DAT_00063ea4` (first waits for any
  of them *or* `DAT_00063e89`, second waits for both to *clear*), so the sys/input layer must drive
  those bytes; `DAT_00063e89` is the abort/quit flag that unwinds the whole show.
* `0x52f08` is indexed with a **byte** offset `stage * 4` (the code does `shl edx,2` then
  `[edx + 0x2f08]`) — the per-stage slide count. New table global needed: `g_tab_52f08`
  (same seedable-table situation as `g_tab_52c3c` / `g_tab_52738`: no producer inside the EXE).
* The copy uses flat video addresses: `0xaa8c0` (= A0000 + 0xa8c0) and `DAT_000633b0 + 0xa8c0`;
  in the port model both go through `video_map(...)` (`g_dat_633b0` is a flat `SCREEN_BASE`-relative
  address, NOT a host pointer).
* `fn_0428f8` receives **two** pushed vararg values (`DAT_00063370` then `slide + 1`), so the port
  must build a two-word cursor array and pass a pointer to it, as `fn_037b03` does for one value.

## Open blocker before porting

The format pointer is the immediate **`0x16b2`**, which is *below* the Ghidra image base
(`VA = file offset + 0x2600`), so it cannot be read from the file with `scripts/vad.py`. Two
possibilities to settle with Ghidra (memory block / xref at `0x16b2`):

1. it points into a region Ghidra mapped below the loaded image (e.g. the DOS PSP area) and the
   string is only in the runtime image; or
2. the constant is not a printf-style format at all.

Until that is resolved the resource-name pattern (`"cg%d.pct"`-like) is unknown, and guessing it
would violate the 1:1 rule. **First action for the port: ask Ghidra for the bytes/xrefs at 0x16b2.**

## Globals still to declare

`DAT_00063ec1`, `DAT_00063ea4`, `DAT_00063e89` (bytes, input/tick state), `DAT_00053dd8`
(host pointer: the device command-stream cursor for `fn_04185e`), `g_tab_52f08` (per-stage slide
count, seedable). Already available: `g_dat_63e24`, `g_dat_63e28`, `g_dat_52ff8`, `g_dat_633b0`,
`g_dat_63370`.

## Testability once ported

Only the abort path (`DAT_00063e89 != 0`) terminates without needing the input flags to change
*mid-loop*, so the unit test should: seed `DAT_63370`, `g_tab_52f08[stage]`, `g_dat_633b0 =
SCREEN_BASE`, set `DAT_00063e89 = 1`, call once, and assert `DAT_52ff8` went 0 -> 1, the tick
counters were restored, and one image load/copy happened. The full multi-slide path belongs to the
integration/playability pass (it needs the sys input layer to flip the key bytes).
