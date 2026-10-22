# The disc-present gate (0x010c3e) and the CD backend seam

`fn_00010c3e` is the guard that refuses to start the game without the disc:

```
for (;;) {
    fn_010927();                       /* MSCDEX root-directory read */
    if (DAT_00053c60 == 0xf &&
        dword(recs+0xb) - dword(recs+6) == 0xc51) break;
    shown = 1;
    fn_03f15f();                       /* VESA mode 0x101 again */
    fn_03628e("cdinsert.pcx", 1);      /* the prompt + palette push through board_out */
    fn_00035d5d();                     /* CD/DMA busy flags (degenerate spin here) */
}
if (shown) fn_000405a7();              /* MSCDEX "stop audio" (real-mode request 0x13) */
```

`DAT_00053c64` is a 5-byte-stride record array (the allocation is `(count+1)*5`), and the gate
reads the dwords at raw +6 and +0xb, i.e. the dword stored at +1 of records 1 and 2. The expected
difference is 0xc51 = 3153 bytes.

## The seam

`fn_010927` reaches MSCDEX through DPMI real-mode call structures (`fn_10570` / `fn_107b9`), inert
in a flat process. `src/sys/sys.h` therefore exposes:

```c
void sys_cd_set_backend(void (*header)(uint8_t *hdr7),
                        void (*records)(uint8_t *recs, uint32_t n));
```

* `header` fills the 7-byte reply at the `DAT_00053c5c` far buffer; `fn_010927` derives
  `DAT_00053c60 = hdr[2] - hdr[1] + 1` from it, exactly as before.
* `records` fills the `DAT_00053c64` array with the per-file records.

With no backend installed nothing changes, which keeps the inert behaviour the CD-family tests pin.

## DPMI DOS-segment pool (fixed)

`sys_dos_seg_alloc` was a bump image with no reclaim, while the original releases the segments it
allocates (INT 31h AX=0101h). One `fn_03f15f` (VESA mode set) pass costs `0x21 + 0x11` paragraphs
= 0x320 bytes out of an 0x2000 pool, so the tenth mode set would have failed -- and the disc gate
performs a mode set on every retry. It is now a first-fit block table whose `sys_dos_seg_free`
reclaims.

## Disc facts for the real backend

`TG_NIGHT.IMG` is `MODE1/2352` raw: user data at `sector*2352 + 16`, PVD at sector 16, root LBA 22,
one directory `/NIGHT` (LBA 23, 2048 bytes) holding 18 files, of which the game uses
`NIGHT.DAT` (51,563,830 B) and `NIGHT.TGS` (23,100,441 B); the rest is the InstallShield installer.

**Open:** no file is 3153 bytes and the root has a single entry, so the meaning of the 0xf entry
count and the 0xc51 gap is not yet confirmed against the image; the real-mode MSCDEX stub that
filled those records is not present in NIGHT.EXE (only its DPMI call template is), so the modern
backend will satisfy the gate as a disc-presence check rather than a byte-exact replay.
