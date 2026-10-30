# 208 — Item 5: the stage-2 poke lever works on both sides, and it does not reproduce the crash

## 1. The port side reaches stage 2 by drive injection

`scripts/rig/drives/ep1_stage2.txt` = `ep1_kill3` truncated at 94 s, then the rig's DS injection

```
94500 poke 0x63370 2        (the stage cell: 1 -> 2)
96000 poke 0x6336c 2        (the exit latch docs/85 used on the shipped side)
```

followed by the rest of `ep1_kill3`'s key stream (558 events, ~300 s nominal).  Result (`/tmp/rig/s3`):

```
memstat stage=2 room=1 frames=13376 ... frames=13435   arena=3903168/50331648 rss_max=...
rc=0, unwound lines=1 (at the end of the drive), no "port fatal", no "terminate requested",
no bus error / SIGSEGV / AddressSanitizer line, and no `res stage2.*` lookup anywhere in the log.
```

So on the current build the port *runs* in stage 2 for >13 000 frames without faulting when the stage cell is
written, and it never asks for a stage-2 resource.  The historical `bus error ./test/night` at stage-2 entry is
therefore not reachable through the stage-cell poke: what the poke skips is the shipped's own stage-clear path
(`fn_03af18`'s room-clear branch: `g_dat_63370 += 1`, the `%de%d.pcx`/`stage%d.pcx` slides from `g_tab_52f90[stage]`,
then `fn_038f3d(1)` at 0x0003b54c) -- which is the code that actually loads the next stage's resources, and the same
branch docs/205 §3 showed the port not taking (it stays in the 320x200 slide-show mode and never re-enters the
640x480 arena).

## 2. The shipped side can be put in stage 2 headlessly, and its stage-2 picture is on disk

`scripts/playtest/drive_orig_levelup_hold.txt` (docs/85's spine up to `writemem 0x6336c 2`, then recorded windows
with no key presses) produced `/tmp/dbgauto/orig_lu7` (90 frames) with `readmem 0x63370 = 2`, `readmem 0x6336c = 0`.
Its last frames are 320x200 mode 0x13, i.e. real shipped stage-2 samples:

```
port stage-1 frame 005593_320x200.bin: black(<8)=0.000 mean= 74.4
port stage-2 frame 013008_320x200.bin: black(<8)=0.000 mean=163.4
shipped stage-2 frames 0087..0089_p04_*.png (320x200): black(<8)=0.002 mean=135.9
```

No black floor appears in either side's presented frame in these samples -- and the port's stage-2 presented frames
are 320x200, i.e. the slide-show mode, not the arena.  "The floor renders black" is a claim about the **video
arena** (`fn_03f15f`'s linear model, docs/79/docs/164 basis), so the measurement has to be taken from the arena
buffer, not from the presented frame.

## 3. Status of `gate3-stage2-crash`

Not detected yet.  What the rig now has: a two-line injection that puts *both* games in stage 2, a shipped stage-2
frame set for comparison, and a negative result that narrows the fault to the shipped's stage-clear branch (the
`g_dat_63370 += 1` path with its PCX slide loads and `fn_038f3d(1)`), not to the stage cell itself.  Next concrete
steps, in order: (a) drive the *natural* stage clear (a room-quota route that clears the last room of stage 1, since
`g_tab_52f90[1] = 1` says stage 1 has one end slide) and watch for the fault with the ASan build; (b) dump the arena
buffer contents at stage-2 entry on both sides and compare them as bytes; (c) check whether the port loads the
`stage2.*` family at all (`fn_036190`'s stage-card names) and what it substitutes when the lookup misses.
