# 46 — the playtest harness (scripted input + named-event screenshots)

Reproduces a human route through the game headlessly, names every screenshot after the game event that
produced it, and pairs those with frames captured from the original under DOSBox Staging.

## Port side: `--script` + `--shots`

`port/src/app/main.c`:

* `--script FILE` — a timeline of keys and explicit shots. Both keys and real SDL input land in the same
  `key_apply()` (SDL scan code → the DOS scan code the game's key state consults → the game's own key
  bytes `g_dat_63ea4/63ed0/63e89/63ec1` and the `g_dat_62c58[]` slot), so a scripted run drives exactly
  the bytes an interactive run does.
  ```
  <t_ms> key <name> down|up        # enter space esc tab up down left right ctrl shift, a-z, 0-9, 0xNN
  <t_ms> shot <name>
  ```
* `--shots DIR` — writes `NNNN_<event>.bmp` plus `manifest.tsv`
  (`seq t_ms frames event resource resource-name stage entities W H`).
* Event naming comes from a new host-side instrumentation hook in `f.c`: `g_res_trace` /
  `g_res_trace_seq`, set at the entry of `fn_0367c9` (the resource-name lookup). Every resource the game
  opens therefore becomes a named screenshot — no behaviour change, `docs/46` note in the source.
* Auto-shots: on each resource lookup; periodically (2 Hz) while a `.fli` is the current resource (so
  intro pacing is measurable); on a large sampled-screen change (`--no-scene-shots` / `--no-res-shots`
  to disable, `--shot-limit N` to cap).

Example (episode 1, `scripts/playtest/route_episode1.txt`) — measured run, manifest excerpt:

```
0    opening.fli     (resource lookup)         <- the very first resource the game opens
9000 attract_done
10039 title1.pcx     (resource lookup)
12001 menu
12518 stage1.pcx     (resource lookup)  entities=3
16000 episode_start  320x200
30000 stage_play     640x480
```

Two of the reported issues are already confirmed as *missing code paths*, not rendering artefacts:

* **no company logo** — the first resource the port opens is `opening.fli`; `logo.pcx` is never looked up
  (issue #1).
* **slideshows skipped** — no `<stage>s<N>.pcx` / `<stage>e<N>.pcx` lookup anywhere in the route
  (issue #4); `stage1.pcx` is loaded straight after the menu key.

## Original side: `scripts/playtest_orig.py`

Plays the same route against the original in DOSBox Staging 0.83:

* input via Staging's **AUTOTYPE** virtual command (`autotype -w WAIT -p PACE buttons…`, `,` = extra
  PACE) — the route's key events are snapped to a shared 0.5 s cadence so both runs see the same rhythm;
* captures into `[capture] capture_dir` with `default_image_capture_formats = raw`, named
  `NNNN_<event>.png` in shot order so they pair 1:1 with the port shots;
* `imgmount d TG_NIGHT.CUE -t iso` works (MSCDEX installed) — the earlier
  `-useimgmode none` variant failed to open the disc.

**Environment limitation found:** the screenshot action (`Cmd+F5` on macOS) is a *host* key action, and
this session cannot give the DOSBox window focus — `System Events` activation is refused by macOS focus
stealing prevention, and `CGEventPostToPid` reaches the app but not the SDL action path. Running
`scripts/playtest_orig.py` from a normal terminal (where DOSBox can be frontmost) works; the focus-free
alternative is a guest-side VGA snapper (a small `.COM` that samples 0xA0000 + the DAC to files on the
mounted C: drive — matching the port's own `vidscr*.bin` / `pal*.bin` dumps), which is the next step for
this task.

## Comparison: `scripts/playtest_compare.py`

`uv run --with pillow python scripts/playtest_compare.py /tmp/pt1 scratch/orig`
pairs shots by `NNNN_<event>`, converts both to RGB (raw framebuffer on the original side, indexed
screen + DAC on the port side), resizes to the port size, and reports per pair:

* `mad` — mean absolute RGB difference,
* `corr` — Pearson correlation of luma (structure),
* `match` — fraction of pixels within 12 levels on all channels,
plus the best-matching original frame for every port shot (`nearest …`), so a timing offset between the
two runs cannot hide an otherwise-correct frame.
