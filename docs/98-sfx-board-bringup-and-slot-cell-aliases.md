# 98 — Missing SFX: the port never brought the sound board up, and slot-array cells 50..54/70..79 were modelled as NULL globals

Amended/superseded: docs/113 -- DS 0x53d08 is the sound-slot table one dword below the pool base 0x53d0c
(1-based), so fn_04185e must read pool[ev-1]; the port's invented g_tab_53d08[] was never written and no hit
or punch sound ever played. Witness: played=74, outside-pool=0.


Playtest item 6: "No sound effects (punch/kick/hit) in the port; original has them."

## 1. Where the calls died (port measurement)

Every SFX goes through `fn_04185e` (0x04185e). A stage-1 combat hold (`route_fight8`, before this fix)
logged `plays=160 real=0 null=0 ... pushed=0 xfer=0` — all 160 calls returned at the gate `0x41879
(DAT_63f90 == 0)` before anything happened. `NIGHT_DBG=1` showed why the init never succeeded:

```
040d09: base=0
040d09: irq probe=0
040d09: bail irq probe
```

`fn_040d09`'s bring-up chain needs real hardware answers: the detection signature, an interrupt for
the probe command, a version id, a 64 KB buffer — on a bare host none answer, so DAT_63f90 stays 0
and *every* SFX call is swallowed. The board model (`board_boot`, gated behind `NIGHT_BOARD=1`)
also fired the wrong vector (0x0a) and never answered the id command.

## 2. What the original actually has (live measurements, docs/98 probes)

Via the container memory API (calibrated base R=0x205000; `read_orig_cd.py` anchors), at the title:

| global | value | meaning |
|---|---|---|
| DAT_53334 | 0x220 | board found in the scan (2nd window) |
| DAT_63f84 | 7 | probe fired vector 0x0f (IRQ7) |
| DAT_63f88 | 0x405 | version 4.5 = SB16 |
| DAT_63f90 | 1 | device initialised |
| DAT_51a10 | 1 | sound enabled |

Boot sampling of DAT_53e24[0]: 0 at t=3.7 s, filled at t=5.4 s — i.e. during the fn_03690b
master-data load, before the attract music (lba 37262) starts.

The "ten command-stream cursors" and the "five menu cursors" are **not separate variables**:
DAT_53d0c is one contiguous 100-dword array (0x53d0c .. 0x53e9c), and live reads confirmed
`DAT_53dd4/53dd8/53ddc/53de0/53de4 == DAT_53d0c[50..54]` and
`DAT_53e24+k == DAT_53d0c[70+k]` (all six matched at both addresses, heads `01e41400a6…`,
`01e40d0083…`). A byte fingerprint against the NIGHT.RES corpus (scripts/extract_res.py,
scratch/res) pinned the slot 70..79 buffers as the tag streams of **71.voc … 80.voc**.
Ghidra (`DumpE24Xrefs*.java`) shows the only direct accesses to 0x53e24 are the ten READ sites
0x116df-0x11814 inside fn_0113e7 — consistent with the cells being produced by the ordinary
slot loader through the array, never as a distinct global (which is why "no writer" was once noted).

## 3. The shipped board protocol (Ghidra `DumpSfxProto.java` / `DumpSfxIsr.java`, 0x40f4f..0x415c9)

* `fn_040f4f`: for base in 0x210+0x10*i, i<8: `fn_04123d` (outb base+6,1; 10×inb base+6; outb base+6,0
  — the SB16 reset pulse) then `fn_0412a3`: `fn_0412e9` polls inb(base+0xe)&0x80 **set**, then
  inb(base+0xa) == **0xAA**.
* `fn_040fb3` (IRQ probe): save+install stubs 0x41439/0x41486/0x414d3/0x41520/0x4156d on vectors
  0x0a/0x0b/0x0d/0x0f/0x72 (each: PUSHAD, DS load, `DAT_63f84 = <irq>`, ack inb(base+0xe), PIC EOI,
  STI, POPAD, IRETD), unmask PIC (outb 0x21, imr&0x53; outb 0xa1, imr&0xfb), send **0xF2** via
  fn_041340, spin until DAT_63f84 != 0.
* `fn_0411d9` (id): cmd **0xE1**, then two fn_0412a3 data reads; id = (first<<8)|second (4,5).
* `fn_041340`: `fn_041390` polls inb(base+0xc)&0x80 **clear** (write ready), then outb(base+0xc,cmd).
* `fn_0413e7`/`fn_041410`: cmds 0xd1/0xd3; fn_040d09 then sets DAT_63f90 = 1 (0x40e70).

## 4. Port fix

* `src/app/main.c` — `board_boot()` now models exactly the measured board and is **default on**
  (`NIGHT_BOARD=0` opts out): signature read at 0x22a, status 0x22e=0x80 / 0x22c=0x00, DAT_53334
  pre-set to 0x220 (the measured runtime value), and `board_cmd_hook` on 0x22c: 0xF2 →
  `sys_isr_fire(0x0f)`, 0xE1 → data {0x04,0x05}.
* `src/f/f.c` — the four missing probe ISR stubs ported: fn_041486/0414d3/041520/04156d
  (fn_041439 existed); all five bound in board_boot.
* `fn_03690b` (master loader) — wire the array cells: g_dat_53dd4..de4 = g_dat_53d0c[50..54],
  g_dat_53e24[k] = g_dat_53d0c[70+k] (the port models DAT as separate globals, so the loader
  assigns what the shipped layout gives for free).
* Env-gated witnesses: `NIGHT_SFXDBG` prints every fn_04185e entry with the resolved slot (found by
  matching the cursor against g_dat_53d0c / g_dat_53e24) and the 0x3f6bc slot byte.

## 5. Evidence

Port bring-up under `NIGHT_DBG=1`:

```
040d09: base=0x220 / irq probe=7 / id=1029 (0x405) / buf=… / initialised      == original table above
```

Scripted runs (`route_fight8.txt`, `NIGHT_SFXDBG=1 NIGHT_HITTRACE=1`):

```
before: plays=160 real=0 null=0   pushed=0 xfer=0            (all gated at 0x41879)
board:  plays=63  real=61 null=2  pushed=220027B xfer=61      (fn_03f6bc slot byte=0 → 1.voc)
aliases: plays=66 real=63 null=3 … slot=51 slot=53            (52.voc/54.voc via 53dd8/53ddc)
combat: port_sfxr3: 90 plays, 62 real, 28 null(=the unaliased 53ddx then), 16 HIT lines,
        slot=71 play present; sfx3/sfx5 show `sfx: play#N` lines interleaved 1:1 with HIT witnesses.
```

`test_f1`: new `test_sfx_board()` drives fn_040d09 end-to-end against the model and asserts the
**measured** values (base 0x220, IRQ 7, id 0x405, DAT_63f90=1) and that fn_04185e passes the
gates; `test_fn_03690b` now asserts the slot aliases. `== ALL PASS ==`.

Reproduce:

```sh
make -j8 build/night build/test_f1 && ./build/test_f1
SDL_VIDEODRIVER=dummy NIGHT_DBG=1 ./build/night --script ../scripts/playtest/route_fight8.txt --no-audio --frames 120 | grep 040d09
SDL_VIDEODRIVER=dummy NIGHT_SFXDBG=1 NIGHT_HITTRACE=1 ./build/night --script ../scripts/playtest/route_fight8.txt --no-audio --frames 6000
# original side:
docker exec dbga bash -c "cd /work && DBA_TOKEN_FILE=/work/.token NIGHT_EXE=/work/game/NIGHT.EXE python3 read_orig_cd.py"
# (the 53e24/53dd4 live reads and voc fingerprints are the inline python in this doc's probes;
#  memory API sometimes starts answering 500 mid-session -- `docker restart dbga` + relaunch fixes it)
```

## 6. Residual notes

* The guest seeds its RNG from the clock, so the fight route lands different hits per run (the
  original is equally non-deterministic); SFX counts vary run to run — evidence quotes several runs.
* fn_0415c9 (the DMA-complete ISR installed by fn_040d09 at DAT_63f84+8) is still unported; the
  sys audio ring drain (docs/43/57) completes blocks without it and no port-side wait depends on it.
* fn_041340's `0xd0` pre-stop and `0xd1/0xd3` are counted as board stops in the pump line (`stops=`).
* `g_dat_53e24` no longer matches "BSS, no writer" in old notes: the writer is the slot loader
  writing DAT_53d0c[70+k] cells.
