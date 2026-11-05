# 348 — 0.1.36 plays, Pocket LCD works with the probes off, and 0.1.37 brackets every probe

Core 0.1.37.  Report: *"It boots + Pocket LCD setting works."*  The card log says how much it booted.

## 1. The 0.1.36 run, in full

262 lines, every stage reached:

```
core 0.1.36: boot marker
phase: boot start
phase: display claimed                       <- new in 0.1.36, and it is why the panel is the game
night pocket: cpu=100000000 Hz sdram=65536 KB heap=35986 KB fb=320x240 stride=320 voices=32 variant=0
caps: version=4 hw=01a7e35b os=00000001
bridge: control word read disabled (DIAG=0); night.ini DIAG=8 re-enables it
bridge: lcd index 5 err=0 val=55557555 field=00000000 | interact says 55557555
settings: music_vol=128 sfx_vol=255 cdda=1 pad=1 tick_hz=490 screen=0
diag=0x0 (1 periph scan 0x40000000+, 2 analogizer probe, 4 interact census, 8 control-word read)
phase: clock source installed
clock: openfpgaOS of_time_us installed; probe: 440526 us over a 2M-iteration spin (advancing)
phase: board_boot
phase: audio_boot
phase: cdda_boot
phase: game main entered
...
memstat frames=780 ticks=9865(+492/s) pumps=1763 arena=3903168/16777216 pit=490 Hz xfer=2 plays=2 cdda=369/17856/3 clock=1005464
view=0 640x480 sig=2890181e chg=11 pal=256
perf pres=16124us max=96954us pump=1763 ticks=9865 skips=0
```

Three conclusions.

**The game runs.**  `phase: game main entered`, 780 frames, 1763 pump calls, 492 ticks/s against a 490 Hz cap,
clock advancing at 1,005,464 µs per second of wall time, arena 3.90 MB of 16 MB, CDDA streaming
(`cdda=369/17856/3` = 369 frames consumed, 17856 in the ring, 3 refill attempts), a 640×480 slide on screen.

**A probe was the 0.1.35 killer.**  Nothing in the port changed between 0.1.35 and 0.1.36 except that the four
probes became conditional.  With `DIAG=0` the same code path reaches the pump.

**Pocket LCD works without the host reading anything.**  The core answers its own menu; the host does not need a
bus read to make the LCD switch happen.  All the host ever needed it for is `SCREEN=-1` (follow the menu), which is
optional.  With the probes off, `bridge: lcd ... val=55557555` is still what `of_interact_get()` reports — the
ecall remains uninformative.

## 2. 0.1.37: every probe brackets itself, so ONE boot names the culprit

```
phase: probe periph begin            ... scan lines ...            phase: probe periph done
phase: probe analogizer begin        analogizer: ...               phase: probe analogizer done
probe: control-word read begin       bridge: control word = ...    probe: control-word read done
phase: probe interact census begin   interact[...] lines           phase: probe interact census done
```

The probe that faults is the one whose `begin` is the last line the run wrote.  Run `DIAG=15` once and the answer is
in one card cycle instead of four.  Suspect ranking, from 0.1.35's log geometry (it stopped within a few lines of
where the control-word read sits): **the raw read of `0xF7000000`** first — it is the only probe touching the page
the firmware rewrites every frame — then the periph scan, then the analogizer, then the census.

`DIAG=0` stays the shipping value.

## 3. The measurement the probes were ever for

After the boot dump, 0.1.37 logs:

```
periph: baseline set.  Every later 'periph move' line is a live change -- move Pocket LCD in Core Options and the
word that answers is the menu channel this core has been looking for
```

So: `DIAG=15`, boot, open Core Options, **move Pocket LCD a few times**, play ~30 s, eject:

```sh
grep -nE "probe .* begin|probe .* done|periph move" /Volumes/Pocket/Saves/night/common/nightlog.sav | tail -30
grep -nE "menu word=" /Volumes/Pocket/Saves/night/common/nightlog.sav | tail -6      # the control word, per second
```

| outcome | meaning |
|---|---|
| a `periph move @0x4000xxxx` line appears when Pocket LCD moves | there is a live, app-readable settings word.  Every NIGHT setting that matters can be tracked off the bus without declaring a single interact element |
| the last line is `probe: control-word read begin` | reading `0xF7000000` from an app is fatal on this bitstream → the probe is deleted permanently, and the Pocket LCD route is confirmed as core-side only |
| the last line is some other `begin` | that probe is deleted, the rest survive, and the periph question is still answerable |
| everything completes, no `periph move` | the periph page carries no menu state either: `night.ini` + L1+B is the settings surface, and it gets documented as such rather than as a workaround |

## 4. The next real bug, already measured in this log

`perf pres=16124us max=96954us` at `view=0 640x480`: a 16.1 ms average present on the native 640×480 source with a
96.9 ms worst case.  16 ms is the copy cost of a 640×480 source into a 320×240 panel (docs/343 measured ~15.7 ms)
— that is a bandwidth problem, not a blit bug — and a 97 ms spike is long enough to starve the CDDA ring.  This is
the thing to attack next, independent of the menu question.
