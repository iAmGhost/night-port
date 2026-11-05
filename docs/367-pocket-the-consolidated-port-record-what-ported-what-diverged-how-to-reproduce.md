# 367 — the consolidated port record: what ported, what diverged, how to reproduce every claim

Task-11's write-up half.  One page that a newcomer can start from, with the measurement each statement rests on.
Everything here is reproducible with the commands in §5; nothing is asserted from memory.

## 1. What is actually running on the Pocket

One shared game layer, two hosts.  `src/f/f.c` (the 29 k-line fixture-tested function-to-function layer) is compiled
in place by both `Makefile` (SDL host, `build/night`) and `pocket/src/night/Makefile` (RISC-V ELF, 32-bit,
`-O3 -flto=auto`, 16 MiB static arena via `NIGHT_HEAP_ARENA_MAX`).  The only change ever made to `src/f` is the
`#ifndef NIGHT_HEAP_ARENA_MAX` guard -- no behavioural fork, and `make test` (3 089 [PASS] lines) is run against the same
sources the device runs.

| layer | file | job |
|---|---|---|
| host | `pocket/src/night/main_of.c` (~2 100 lines) | boot recipe, tick pump (490 Hz from the game's own PIT divisor), present pacing, pad, mixer drain, display mode, settings, diagnostics |
| disc | `of_disc.c` + generated `src/cd/disc_table.h` | CUE/IMG → APF data slots (`slot:N` stdio, one handle per track, seek only out of order) |
| music | `of_cdda.c` | two looping MUSIC voices over two mono rings (176 400 frames = 4 s), 75 sectors/s disc clock, 300-sector prefill, 120-sector lookahead, knee gain stage |
| sfx | inside `main_of.c` | one mixer voice per board transfer, group volume, drops/steals counted |
| save/settings | `of_save.c` | 10 save slots + `night.cfg` + `nightlog.sav` (alias `save:7`), `night.ini`, the peripheral-register settings read |
| log | `of_log.c` | the card trail, `LOG=` gated (ships off) |

Core id `iamghost.night`, `VARIANT=os25` (the hardware mixer the CDDA streaming needs), 28 data slots ids 0..31.

## 2. What ported, and what proves it

| capability | proof |
|---|---|
| Boot past the logo, .fli intro, title | hardware runs reach the stage modes; `opening.fli`/`logo.pcx` appear in the Pocket host's own resource markers (docs/365 §4) |
| 640×480 slides, downscaled to the panel | 27 arena samples byte-identical to the shipped PCX, 307 200 / 307 200 bytes, on the Pocket host (docs/363) -- the same criterion docs/62 measured on the SDL host |
| 320×200 gameplay, 1:1 | panel shots (docs/331, docs/338) + the geometry witness `view=1 320x200 sig=… chg=…` in hardware logs (docs/337) |
| All nine episodes | 9/9 entered and ran clean, chains of 4/4/7/2/1/4/3/1/8 rooms, bosses at different world coordinates, 1–12 live entities (docs/363) |
| CDDA music, gapless | select sequences identical to the SDL host on three routes and identical to `g_tab_52da4` for all nine episodes (docs/365, docs/363); byte-exact audio vs the shipped disc (docs/332); `dry=0`, ring slack never under 1.59 s (docs/364) |
| Board SFX | one voice per sound, transfer census, plays/drops/steals on both hosts (docs/340); 2–23 blocks delivered per episode in the sweep |
| Saves and settings | save slots and `night.cfg` land on the card (docs/334, task-12b); three menu settings in one bridge word, persisted across eject and reboot (docs/362) |
| The game's own music-mode flag mirrored | `DAT_00051a10` turns the voices off and back on with the same track (docs/341) |
| Timing contract | `clock=…us/s` ≈ 1 000 000 on the device, `pit=490 Hz`, ticks +491…+501/s (docs/335, reconfirmed on a 2026-11-05 run) |

## 3. What diverged, or is still open -- stated as defects, not as fine print

1. **Stage palette.**  On both hosts the stage frames read `255/97/97` while the DOSBox original shows a grey-blue
   station scene (docs/333).  This is a shared-`src/f` defect the Pocket host inherits, not a Pocket bug.  Open.
2. **The interact bridge is dead on this OS build.**  Six builds, every `of_interact_get()` index returns fixed
   garbage; the ecall is dispatched but its table is never filled (docs/342).  Reading `0xF7000000` from the app
   *hangs* the core (docs/349), and app knobs at that address broke two builds (docs/336, docs/346).  Worked around: the
   menu's declared values are mirrored into peripheral-page words (`0xF700000C → 0x4000006c`,
   `0xF7000010 → 0x40000068`) that an app may read, and three settings share one word (docs/350–362).  A newer
   `os.bin` (the Doom core ships 135 400 B against the SDK's 134 424 B) might implement it properly; untested.
3. **`na<stage>NNNN.pcx` picture shows and the ending cards are unverified on the Pocket path.**  The warp route
   enters the stage through confirm pulses and does not reproduce the `fn_03a70e` show (best arena match 0.2–0.8 % =
   nothing); the `e`-card and `2end0001..2end0010` families need an ending to be reached (docs/363 §4).
4. **Level-up and continue screens have not been swept on the Pocket path** (they are on the SDL host, docs/61).
5. **`FB_MODE=0` has never been measured on hardware.**  Desktop: 21 344 µs mean present for a native 640×480 source
   against 3 608 µs for 320×200 (docs/343).  The one-line change is staged, not measured.
6. **Terminal mode on the device is unproven.**  `SCREEN=1` / the `OF_DISPLAY_OVERLAY` path produced visual confusion
   in an early build (0.1.7) and has not been re-tested since the display-mode assertion (docs/343) and the
   Pocket-LCD menu register were found (docs/350).  That is task-12a's contract line.
7. **DSP commands `0xe1` and `0xf2`** appeared once each in the transfer census and are unexplained (docs/340).
8. **Per-frame pixel equality between the two hosts is not achievable** on an animated stage: the hosts present at
   different rates (2 501 vs 5 243 frames for the same route) and enter a stage by different means, and even one host
   repeating itself differs on 6 % of stage-1 frames (docs/364 §3.1, docs/365 §2).  The state-level comparisons -- CD
   selects, view sequences, entity counts, arena-vs-PCX identity -- are the ones that carry weight.
9. **The SD read cost on the device is still unmeasured.**  The instrument is in the shipping core; the number needs
   a boot (docs/364 §5 explains why the mounted card cannot be measured from this Mac).

## 4. Numbers a future change should be judged against

Present cost (desktop shim, 2 000-frame runs): mean 446–493 µs, worst 2.5–4.5 ms; nine-stage sweep worst 2 467…16 666 µs.
Tick rate +491/s against `pit=490 Hz` everywhere.  Arena high-water 3.90–5.25 MB of the 16 MiB budget.
CDDA: fills mean 32–47 µs (the 300-sector whole-ring prefill is the 5–6 ms outlier), `ahead_min` 1.59 s, `dry=0`.
Mixer/audio levels: CD-DA peaks at 0 dBFS on 6 of 14 tracks, board SFX peak 30 720, knee gain at 20 480 with slope 4,
`SFX_GAIN=200` / `CDDA_GAIN=150` shipping (docs/361).  Card ELF 0.1.54: 822 088 bytes.

## 5. Reproduce

```sh
# shared layer and both hosts
make test                                   # repo root: fixture suite, == ALL PASS ==
make app                                    # build/night (SDL host)
make -C pocket/src/night test               # app_pc: same src/f under the SDK's SDL2 shim
make -C pocket/src/night                    # the RISC-V ELF
make -C pocket/src/night data && make -C pocket/src/night deploy   # slot files + core -> /Volumes/Pocket

# nine episodes, Pocket code path
FRAMES=4500 scripts/pocket/sweep_episodes.sh
python3 scripts/pocket/sweep_report.py /tmp/pocket_sweep            # cards, music table, verdicts

# Pocket shim vs SDL host, same route
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234
for n in 1 3 8; do
  NIGHT_CD_DIR=scratch/pocket/cd ./pocket/src/night/app_pc --warp-stage $n --frames 2500 \
      --rig-dump /tmp/xpoc/s$n --rig-witness 0 >/tmp/xpoc/s$n.out 2>&1
  ./build/night                 --warp-stage $n --frames 2500 --rig-dump /tmp/xsdl/s$n >/tmp/xsdl/s$n.out 2>&1
done
python3 scripts/pocket/diff_hosts.py /tmp/xpoc/s1 /tmp/xsdl/s1 /tmp/xpoc/s3 /tmp/xsdl/s3 /tmp/xpoc/s8 /tmp/xsdl/s8

# the asset-identity test (arena index bytes vs the shipped PCX decode)
NIGHT_VIDRAW=1 NIGHT_CD_DIR=scratch/pocket/cd ./pocket/src/night/app_pc --warp-stage 3 \
      --frames 1000 --rig-dump /tmp/a --rig-witness 0     # -> /tmp/a/arena/*.raw

# storage / interact contracts, both mechanical
python3 scripts/pocket/check_slots.py
python3 scripts/pocket/check_interact.py

# hardware: what to read back
cp /Volumes/Pocket/Saves/night/common/nightlog.sav /tmp/n.sav
grep -E '^(version|clock|memstat|perf|stages|budget|audio|view|video CHANGED)' /tmp/n.sav | tail -40
```

## 6. Picture accuracy, measured per phase against the original's own frames (docs/379-382)

The original side is 434 frames captured headless through dosbox-automation and labelled by the event in flight
(docs/379).  The Pocket side is its own rig dump: `NIGHT_VIDRAW=1 NIGHT_VIDRAW_MAX=400` writes 400 arena witnesses
(index bytes plus the game's own 768-byte palette), which `scripts/pocket/arena_raws_to_bmp.py` decodes to BMPs for
`scripts/playtest_compare_orig_port.py` -- the label/best-match basis `docs/91` used for the SDL host.

| original phase | frames | Pocket route that covers it | matched at NCC >= 0.90 |
|---|---|---|---|
| live stage play | 55 | `--warp-stage 1` | **55 / 55**, best pair 0.9998 with strict pixel agreement 1.000 |
| intermission slides | 133 | either route | 130 / 133 |
| title melt | 42 | either route | 41 / 42 |
| intro `.fli` | 54 | scripted attract route | 46 / 54 |
| pause / menu windows, whole frame | 82 | warp route with `route_esc_menu.txt` | 40 / 41 best; the other window is 1 / 41 because the stage behind the menu is at a different progression point |
| **pause / menu windows, the menu box itself** | 82 | any route that opens the menu | **41 / 41 per window at NCC 1.0000** (docs/383) |
| DOS text-mode boot (mode 3) | 24 | none: the port has no text-mode presentation | 8 / 24 |

What this section deliberately is not: one route score.  A single end-to-end number produced 0.559 and read like a
verdict while each route was simply outside several phases (docs/380 4, docs/382 1).  Its zeros have named causes -- a
a warp run skips the intro, and a route that never enters a stage cannot match its gameplay frames, and a route that never presses ESC
cannot be scored for menu fidelity at all (docs/383) -- and the routes together cover every phase the port claims
to render.

One command reproduces the table above: `scripts/pocket/accuracy_sweep.sh` (three routes, witnesses decoded to BMPs,
per-window whole-frame score plus the menu-box score).  Which window a *given* run witnesses varies with the shim's
wall-clock pad timeline (docs/333 5); the exact-match claim is the one that survives that.

## 7. Document map

`docs/330` budgets and the target mapping · `331` host layer, first port-vs-port comparison · `332` the CDDA engine
· `333` pad mapping and why wall-clock pad scripts are unstable · `334` save slots and settings · `335` the clock
source and the frozen-logo fault · `336` core JSON shape and the reserved interact slots · `337` the card log ·
`338` scaler modes and prefill reads · `339` the audio tracks were sitting in the OS save window · `340` SFX and the
transfer census · `341-347` the settings/menu thread, including two cores that died from touching `0xF7000000` ·
`348-349` a core that boots, and the address that must never be read · `350-362` the peripheral-mirror settings
channel, packing proven on hardware, and what ships by default · `363` the nine-episode sweep · `364` the frame
budget and the 150 000-visit-a-frame pump · `365` the two-host diff · `366` the install route verified against the
card.  `pocket/INSTALL.md` is the human-facing install README, and `379-382` cover the original-side capture, `--script` anchoring on the Pocket path, the per-phase picture accuracy of section 6, and the menu-region
measurement that shows the pause menu drawn identically.
