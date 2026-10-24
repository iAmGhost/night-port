# Disc-absent boot: the shipped gate is ported, but the port cannot come back from it

Found while testing the `fn_03628e` view-geometry fix (docs/66) on a disc-less boot. Not one of the playtest
contract items; filed with its root cause instead of patched blind, because a fix requires a host-side decision
(hot-swapping the disc image at runtime).

## What the original does

`fn_010c3e` (0x010c3e, THE DISC-PRESENT GATE) polls the MSCDEX root-directory read until the disc looks right
(`g_dat_53c60 == 0xf` and two record dwords differing by 0xc51); until then it re-sets the VESA mode
(`fn_03f15f`), paints `cdinsert.pcx` (`fn_03628e`, dl = 1, which also pushes the palette through `board_out`) and
waits on the CD/DMA busy flags (`fn_00035d5d`). On exit, if the prompt was shown at least once, CD audio is
stopped. The port has this function verbatim (f.c ~5123) and the sys layer supplies the header/record array
through `sys_cd_set_backend()`.

## What the port does (measured this cycle)

`--res-dir /tmp/hd` where `NIGHT.RES` is present but `TG_NIGHT.CUE` is not:

```
disc: /tmp/hd/TG_NIGHT.CUE not readable -- the disc-present gate will ask for the CD
```

then nothing: no shots, no manifest, the process had to be SIGKILLed at 70 s. Reason: no CD backend is installed,
so the gate's condition can never be satisfied, and its wait `fn_00035d5d` is a **no-op standin** (the original's
body is a degenerate spin over `g_dat_63e88` flags that never move in the flat model -- documented at its
definition). The loop therefore repaints `cdinsert.pcx` with no pacing, no input pump and no presentation: the
player sees no prompt and cannot quit.

## Why it is not patched here

A correct fix needs two things the port does not have:
1. a host hook that can *install* a CD backend later (a real "insert disc" event), and
2. a paced, pumped wait in the gate's loop -- `fn_00035d5d` is game code and its degenerate-spin standin is
   deliberate (the CD-family tests pin that inert behaviour), so the pacing belongs in `fn_010c3e` (or in a new
   sys-level spin helper the gate calls), which changes code the CD tests assert on.

Both are design changes; the smallest defensible version is `sys_port_spin()`-style pumping in the gate loop plus
a host key (e.g. a mount command) that re-runs `cd_iso_open` and installs the backend.

## Related (lower value, same family)

Pointing `--res-dir` at a directory with no `NIGHT.RES` at all segfaults in `strn_len` (f.c:4289) with the
pointer value `0x4847494e` -- the ASCII "NIGH" -- i.e. a resource name's bytes used as a pointer after a failed
lookup. The original also cannot run without its hard-disk install, so this is robustness only.
