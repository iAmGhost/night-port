# 87 — The setup / attract menu (pause.spr): the attract loop never shows it, and where the shipped menu routine draws, the port is pixel-identical

Goal task `orig-setup-attract-menu`. docs/60 measured the port's side and inferred that the setup menu
"is displayed on its own after the title card". That inference is **corrected** here, and the menu itself is
now captured on the original side and scored.

## 1. The shipped routine is reachable only from a live room

Ghidra: `fn_039c03` (the five-entry setup menu, 0x00039c03, 1456 B) has exactly **one** xref in the whole
binary -- `UNCONDITIONAL_CALL` from **0x0003aac1**, inside the room play loop `fn_03a829`, under
`if (DAT_00063e89 != 0 && player[0x30] != 1)` (0x0003aa9f-0x0003aac6, the abort latch). Ghidra's decompiler
flattens that block away ("Removing unreachable block (ram,0x0003aaa4/aaa8/aac1/aaca)"), which is why the
call does not appear in the pseudocode of the room loop -- but the xref and the port's translation
(`f.c:22447`) both carry the shipped guard.

## 2. The attract loop does not show it (both sides)

`scripts/playtest/drive_orig_attract_wait.txt` -- boot, skip nothing, then hold the title with **no key at
all** for 70 s at 3 Hz (239 samples):

```
t=34.2 s  mode 640x480, plain fill (label 1s1, sim 0.0000)
t=36.3 s  wipe in: sim 0.378 -> 0.486 -> 0.649 -> 0.828 -> 0.961
t=44.9 s  settled: sim 0.9829 against title3.pcx, held to t=90.5 s   (137 samples)
t=90.6 s  fade to fill (1s1, sim 0.0000), title3 fades back to 0.117
t=96.4 s  mode 320x200 -> the 1s1/1s2/title1 slide show restarts
```

So over 100 s of no input the original never shows a menu-looking screen; it holds the title card, wipes it
out, and restarts the slide show.

Port side, same shape (`--frames 9000`, no script, `/tmp/port_attract`): the frame after the title card
scores **99.99 % within ±3, MAE 0.86** against the `title3.pcx` decode (vs 94.34 % for title1.pcx, 0.00 %
for 1s1.pcx) -- the picture *is* the title card, while the harness's resource label says `res=title1.pcx`.
That label is the last card the game **loaded**, not the picture on screen, and it is what made docs/60 read
the frame as "the menu". Rule recorded: never attribute a screen by the resource label alone.

## 3. Capturing the menu on the original side

`scripts/playtest/drive_orig_setup.txt` reaches a live episode-1 room with the docs/81 spine (forced episode,
then `key_until_mode_stable 200 enter 30 1200 4` so mode 13h *holds*), reads the latch, presses `esc`, then
walks the entries Left/Down exactly as docs/60 did on the port side. Measured guest state:

```
readmem 0x63e89 = 0   before the press      -> after `key esc 150`: readmem 0x63e89 = 1
readmem 0x63354 = 0x38202c before and after -> the menu runs NESTED inside the room loop
mode stays 320x200 for the whole menu
readmem 0x53338 = 1 (BGM), 0x51a10 = 0 (SOUND) after the toggles
readmem 0x63370 = 1 after exiting -> the menu does not advance the episode
```

Menu states are static while held (23-24 samples ≈ 4 s at 6 Hz each, `changed>3` between the first and last
sample of every window = **0.00 %**), and consecutive entry steps differ in only 1.1-1.3 % of the frame --
docs/60 measured the same 1.90 % entry-region redraw on the port side.

## 4. Port vs original on that screen (premise-checked, then region-decomposed)

Frames: `0186_menu_m0_12.png` (original) vs `0176_pause_m0.bmp` (port), both 320x200, distinct colours 124 /
121.

| basis | within ±3 | MAE |
|---|---|---|
| whole frame, **as-captured** channel order | 86.30 % | 9.06 |
| whole frame, R/B-exchanged | 1.82 % | 26.76 |
| inside the pause.spr panel mask | **100.00 %** | **0.33** |
| panel mask, original-only / port-only pixels | 0 px / 0 px | -- |
| HUD slot-0 box, rows 0..43 cols 0..0xaa (face.spr + gage.spr) | **99.77 %** | **0.19** |
| outside both panel masks (the room backdrop) | 84.82 % | 10.00 |

Channel order is premise-checked rather than assumed: the mode-13h payload comes from the harness with the
docs/70 mirror already applied, and the whole-frame score is 86.30 % as-captured against 1.82 % exchanged
(so as-captured is right; the mirror flag is `--swap13`, which conditions on `h == 200` -- see docs/86 for the
640x480 caveat).

The panel decomposition is the decisive part: the pause.spr panel's flat colour is (130, 81, 206) on the
original and (130, 81, 207) on the port (the ±1 DAC quantization of docs/77/86), its bounding box is
identical on both sides (rows 103..182, cols 193..312, 6252 px = 9.77 % of the frame), the mask intersection
leaves **zero** unmatched pixels in either direction, and the entry-row profile is identical (rows 103..151
and 153..182 on both sides). Inside that mask the two screens agree on 100.00 % of pixels (MAE 0.33). The
whole-frame 86.30 % is entirely the room backdrop behind the panel, where the two runs are at different
points of the room -- the docs/79 run-shape explanation, not a rendering difference.

## 5. Attribution: pause.spr, face.spr, gage.spr drawn; system.spr / system2.spr filed with the shipped gate

* **pause.spr** -- the panel above (geometry + 100.00 % within ±3 inside the mask); docs/75 already verified
  the window bytes against the shipped block.
* **face.spr + gage.spr** -- the HUD panel is drawn by `fn_01348f` (0x01348f, "THE HUD GAUGE DRAW"):
  face.spr record `[+0x0d]` blitted at `(slot*0xaa+4, row 3)` at 0x13702 and gage.spr record 0 at
  `(slot*0xaa+0x13, row 0x1d)` at 0x13738. Measured: that slot-0 box is 99.77 % within ±3 / MAE 0.19 port vs
  original, in the room frame *and* in the menu frame (the menu does not touch the HUD). New witness
  `menucell tag=hudface` (added at the 0x13702 site, own budget of 8) makes the census able to attribute the
  portrait **by content**: its 48-byte RLE probe is found in `scratch/res/face.spr` at offset **0x38**, and a
  walk-fight run emits `hudface=8, hudgage=120, stagespr=80, expnum=80` composite traces.
* **system.spr / system2.spr** (the floating value number display, rendered by `fn_03b718` from the node list
  built at `fn_0003b5c0`) stay loaded-but-not-drawn in every scripted shape, and the reason is now pinned to
  shipped code: `fn_015295` (the entity tick, 0x015295) queues a value node only past the gate
  `a0[0x30] == 1 && a0[0x25] != 0xff && a0->5 == 0 && a0->11 > 0x15` and `a0[0] == 1` (0x152d1-0x1531c) --
  i.e. a **dead type-1 entity** (0x30 is the death byte docs/81 used for the player). Scripted play never
  kills a type-1 entity (docs/80: no scripted route clears a room), so nothing is queued and the census run
  reports exactly `loaded but not drawn: face.spr (fixed above), over.spr, pause.spr, system.spr, system2.spr`.
  Next diagnostic step with its command -- warp-spawn a type-1 enemy and then set its 0x30 death byte (no host
  hook does that yet, which is the gap):

  ```sh
  cd port && NIGHT_WARP_ENTITY=ena-1 NIGHT_WARP_ENTITY_AT=600 NIGHT_GLYPHDBG=1 \
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./test/night --warp-stage 1 \
    --script ../scripts/playtest/route_walk_fight.txt --no-audio --frames 7000 --shot-limit 2 \
    2>&1 | grep -E "warp-entity:|tag=valuedisp"
  ```

  Run as-is it reports `warp-entity: ena-1 type=31 frame=2574 spawned` and **0** `tag=valuedisp` lines: the
  entity is alive, so the 0x152d1-0x1531c gate never opens. That measured 0 is the confirmation that the
  missing piece is the death byte, not the renderer.

## Reproduce

```sh
# original side: attract flow, and the menu
docker exec dbga bash -c 'for f in /proc/[0-9]*/comm; do case "$(cat $f 2>/dev/null)" in *dosbox*) kill -9 ${f#/proc/};; esac; done'
docker exec -d dbga bash -c "cd /work/dosbox-automation-0.85.1-7567ec4-linux-x86_64 && \
  xvfb-run -a -s '-screen 0 1024x768x24' ./bin/dosbox -conf /work/night.conf > /work/dbga_pa.log 2>&1"
export DOSBOX_API_TOKEN=...
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_setup1 --drive scripts/playtest/drive_orig_setup.txt --hz 6
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_attract1 --drive scripts/playtest/drive_orig_attract_wait.txt --hz 3

# port side: menu frames, and the attract tail
cd port && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./test/night --no-audio \
  --script ../scripts/playtest/route_pause_menu.txt --shots /tmp/port_pause --shot-limit 900 --frames 22000
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./test/night --no-audio --frames 9000 --shots /tmp/port_attract
```
