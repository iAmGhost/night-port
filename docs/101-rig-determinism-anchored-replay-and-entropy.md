# 101 — Runtime rig layer 1: pinned cycles, anchored replay, measured entropy

`rig-runtime` → `rig-determinism`. This is the layer everything else in the rig stands on: if the
original's state sequence is not reproducible, no diff against the port can mean anything.

## 1. What the emulator actually offers (measured, dosbox-automation 0.85.1-7567ec4)

`scripts/rig/bringup.sh` starts the container (image `dbga:ready`, the 0.85.1 tarball on the host
mount) with the config it writes to `/tmp/rig/rig.conf`:

- **cycles must be pinned in the config**, not through the API. With `cycles = max` the API reports
  `{"mode":"legacy","auto_adjust":true}` and `PUT /api/v1/cpu/cycles` answers
  `Legacy cycles mode active; set cycles via config`. With
  `cycles = fixed 120000` + `cycles_protected = fixed 120000` it reports
  `{"auto_adjust":false,"current_max":120000,"pmode":true}` — `cycles_protected` matters because
  NIGHT is an LE binary and runs in protected mode.
- **the web API is loopback-only by default**: `webserver: starting HTTP REST API on
  http://127.0.0.1:8386` means a published docker port reaches nothing (host `curl` gets an empty
  reply). `webserver_bind_address = 0.0.0.0` + `webserver_allow_remote = true` fixes it.
- `keyboard_layout = us` is pinned so key names cannot resolve differently between runs.
- `GET /api/v1/cpu/state` gives the real segment registers (`ds = 0x168`), but
  `GET /memory/<segment>/<offset>` treats that as a real-mode paragraph and returns low memory (all
  zeros) — useless for an LE selector. Physical addressing is the usable route.
- Cost per sample: DS group `0x50000..0x66000` in 6×16 KB reads ≈ 0.4 s; one raw frame (512 KB) ≈
  14 ms. Practical poll rate ~4 Hz.

## 2. DS calibration is found, not assumed (`calibrate_ds.py`)

`/api/v1/memory/search` scans physical RAM for distinctive image-initialised DS values (the `0.5`
float table at DS 0x5274c, the kill quota `1000` at DS 0x52a1c). Every candidate base is then tested
by comparing 64 bytes of the shipped `NIGHT.EXE` against emulator memory, and the run records
`phys_ds_base` (this boot: `0x1B5000`, i.e. physical = base + DS address). It must be re-run per
container.

## 3. Replay = one input stream released at a game-defined moment (`replay_original.py`)

Input goes in as a **single** `POST /api/v1/input/sequence` (`{"t": ms, "type":"key", "key":
"KBD_enter", "pressed": true|false}`), scheduled on the emulator's own PIC timeline, so the input is
identical by construction across runs. Before that POST the tool waits for an **anchor** declared in
the drive file:

```
anchor static 3.0 90        # start when the framebuffer has stopped changing for 3 s
0 key KBD_enter down
60 key KBD_enter up
```

Anchoring is mandatory: releasing the timeline at "12 s after the container started" shifts the whole
game by however long calibration took, and the previous version of this rig failed for exactly that
reason (198 states vs 452).

## 4. State = masked digest over the cells the guest touches

`scripts/audit/gen_headers.py` also writes `gen/ds_mask.bin`: one byte per DS byte, set for the cells
with instruction-level guest accesses (2907 bytes of 90112). Without that mask the digest changed on
every sample because frame-local `DS:[EBP+disp]` traffic lives in the same region.

## 5. Result: the original *is* reproducible, and its entropy sources are now named

Three fresh boots, pinned cycles, same anchored input (`scripts/rig/det.sh 3 35` →
`scripts/rig/entropy_report.py`):

```
compared 125 states across 3 runs
entropy bytes excluded: 23 (of 2907 masked)
  0x5335c g_rng_state    differs in 125/125 states     0x63e30 g_dat_63e30  125/125
  0x5335d (same cell)    differs in 125/125 states     0x63e34 g_dat_63e34  125/125
  0x63e24 g_dat_63e24    differs in 125/125 states     0x63e38 g_dat_63e38  125/125
  0x63e40 g_dat_63e40    differs in 125/125 states     0x63f08 g_dat_63f08  124/125
run2: deterministic-digest states matching reference: 125/125
run3: deterministic-digest states matching reference: 125/125
DETERMINISM (entropy cells excluded): PASS
```

The DS image at every state index is byte-identical across boots except **23 bytes**: the game's RNG
word (`g_rng_state`, 0x5335c — the image initialises it to 1, but the game reseeds it from a
hardware timer before gameplay) and a clock block at `0x63e24..0x63e40` plus `0x63f08`, `0x5335e/f`.
That list is machine-readable in `scripts/audit/gen/entropy_cells.json`, and the reduced digest mask
is `gen/ds_mask_det.bin`. Every later oracle compares with that mask and prints the entropy cells
separately, instead of treating them as divergences.

Residual reported per run: `alignment residual (s): min/max/spread` between matching state indices
(host scheduling jitter only — no state consequence), and `frame hashes identical at the same state
index: 112/125`. The frame residual is a sampling artefact: polls land at arbitrary points of the
emulator's 70 Hz refresh while sprites animate. The per-frame framebuffer oracle therefore does not
poll — it uses the emulator's lossless ZMBV capture (`POST /api/v1/capture/video/start`), where
frames carry emulator frame numbers.

## 6. Reproduce

```sh
RUN_GAME=1 bash scripts/rig/bringup.sh              # pinned cycles, API on 0.0.0.0:8386
python3 scripts/rig/calibrate_ds.py                 # -> scripts/rig/gen/ds_base.json
python3 scripts/rig/replay_original.py --drive scripts/rig/drives/title_basic.txt \
        --out /tmp/rig/det/run1 --seconds 35         # anchored replay + state snapshots
bash scripts/rig/det.sh 3 35                        # 3 fresh boots, then:
python3 scripts/rig/entropy_report.py /tmp/rig/det/run{1,2,3}
```
