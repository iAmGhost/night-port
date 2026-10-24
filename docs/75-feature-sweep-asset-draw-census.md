# 75. Feature sweep: which shipped sprite families are actually seen being drawn

Closes the playtest contract's `feature-sweep` item. Companion to docs/64 (which counted asset *lookups*) and
docs/73 (which proved entity composites draw from the asset their own entity type names).

## The census: `scripts/playtest_asset_census.py`

For a fixed set of headless runs the port prints every sprite composite it composites -- the cell's raw row-RLE
stream plus the bytes the blit wrote (`NIGHT_ENTSPR=1` for entities, `NIGHT_HUDDBG=1` for the gauge,
`NIGHT_MENUBG=1` for screens/menus). The script then asks, for each composite: **which extracted shipped `.spr`
file contains exactly those bytes?** (first 48 bytes of the stream, searched across all 75 extracted `.spr` files).
Attribution is by content, so it does not simply re-read the port's own name tables.

Runs: `--warp-stage N` with the keyed route for N=1..9 (docs/59), the pause-menu route, and the screen warps
`NIGHT_SHOWSCREEN=help|levelup|gameover|ending`.

### Observed, per run (composites attributed / distinct assets)

```
stage1    200 / 3   ena-1, gage, gun
stage2    240 / 4   ene-1, ene-2, gage, gun
stage3    240 / 4   enh-1, enh-2, gage, gun
stage4    240 / 4   ene-4, enf-3, gage, gun
stage5    240 / 4   enk-1, enk-2, gage, gun
stage6    240 / 4   ena-1, ena-2, gage, gun
stage7    240 / 5   ena-1, ena-2, enb-2, gage, gun
stage8    200 / 3   boss6, gage, gun          <- a boss family observed in play
stage9    160 / 2   gage, gun                 (its one entity was off-screen at the sampled moments)
pause    2381 / 1   pause.spr
gameover   --      over.spr, number1.spr      (screen warp run)
help / levelup / ending: 0 composites -- census-harness limitation, see "Filed gaps"
```

### By family (union of the runs)

| family | status |
|---|---|
| ena, enb, ene, enf, enh, enk | **drawn** (ena-1/2, enb-2, ene-1/2/4, enf-3, enh-1/2, enk-1/2) |
| boss* | **drawn**: boss6.spr in stage 8 |
| gage (HUD gauge), gun (player) | **drawn** in every stage run |
| pause | **drawn** (pause.spr, 2381 composites in the pause-menu run) |
| over | **drawn** (over.spr, game-over warp) |
| enc, eng, eni, enj | not observed in this window -- filed below |
| st1s..st9s | not observed -- filed below |
| mboss1..mboss6 | not observed and **not referenced by any lookup in the whole feature sweep** (docs/64) |
| boss1..boss5 | not observed -- filed below |
| dust, mark, exp, face | not observed -- filed below |
| levelup, system | not observed in the census window -- filed below (both *are* byte-verified elsewhere, see below) |

Total: 17 distinct shipped `.spr` assets seen being drawn by content attribution, out of 75 extracted.

## Screens: screenshots and the original

* **Pause menu** -- `docs/manual-playtest/05-shots/0168_pause_m2.bmp` / `0169_pause_m2_off.bmp`. Compared against the
  original's own captured pause window (`/tmp/dbgauto/orig_full/0*_esc_win*`, decoded BGR per docs/70):
  best frame `0200_esc_win_02.png` at **86.18 % of the 64,000 pixels within +-3**, NCC 0.860, MAE 9.04. The residual
  is the live gameplay backdrop behind the window (entity positions), not the window: the window itself is
  byte-verified against `pause.spr` (2381 composites above; 547,781 bytes in docs/63). This also re-scores the
  pause-menu number that docs/70 flagged as having been measured against a channel-mirrored reference.
* **Game over** -- `docs/manual-playtest/05-shots/gameover_continue.bmp`; the census adds `over.spr` observed in the
  game-over warp, and docs/61/63 already pin the screen byte-for-byte against the shipped data
  (`continue.pcx` 0 mismatches over 64,000; `gameove1..3` card 95.1 % plus 1557 bytes of `over.spr` composites;
  the `warp_NN` animation series 29 frames).
* **Level-up / upgrade** -- `docs/manual-playtest/06-shots/` screenshots from docs/61's warp, with the card and all
  24 number glyphs byte-exact (912 compared bytes, docs/61) and 1.06-1.38 % difference confined to the number
  column (docs/63). No original-side frame is reachable without clearing a stage (see gaps).

## Filed gaps (with root cause, per the contract)

1. **Original-side game-over frame not captured.** The scripted attempt to die in episode 1
   (`scripts/playtest/drive_orig_episode1`-style drive, 150 s of gameplay with no input, `/tmp/dbgauto/orig_die2`)
   produced 321 frames of which only 19 were 320x200, and **none** matched `continue.pcx` / `gameove1..3.pcx`
   (NCC scan): the original went back to its 640x480 title/attract phase instead of showing the death screen, i.e.
   an idle player is not killed inside the window. Next step is now cheap and concrete: the guest-memory API is
   calibrated (docs/74), so zeroing the player's health variable and re-driving gives the frame deterministically.
   Note on the level-up screen: its number glyphs come from `number1.spr` / `number2.spr` (docs/61), and
   `number1.spr` **is** observed drawn -- in the game-over warp run above. So the level-up digit asset family is
   covered even though `levelup.spr` itself is not.

2. **`levelup.spr` / `system.spr` / help and ending screens report no composites in the census.** Root cause is the
   census run shape, not the game: with the keyed route the host screen warp fires after the route's own keyed waits
   have ended, so `NIGHT_MENUBG` produced no cells for those three runs. The same screens *are* byte-verified through
   dedicated runs -- pause menu cells (docs/60, docs/63), the level-up digits via the `lvlnum` trace (docs/61), the
   ending slide show with 116 frames byte-identical over 307,200 arena bytes (docs/66). Follow-up: point those three
   census entries at the routes/frames that reach them. Attempts made and ruled out: `route_levelup_warp.txt`, and
   `route_attract_wait.txt` with `NIGHT_SHOWSCREEN=levelup` at the frame docs/61 measured (2777) -- both produce zero
   traced composites, so something about the census' process shape (short shot limit, no `NIGHT_BOARD`) keeps those
   screens from compositing; the digit-glyph attribution was also widened to accept streams shorter than 8 bytes and
   that changed nothing, confirming no composites were emitted at all.
3. **`mboss1..mboss6` never drawn and never looked up.** docs/64's sweep of 293 resource lookups never names them;
   they are shipped but unreferenced content. Not fixable, recorded.
4. **`enc`, `eng`, `eni`, `enj` and `st1s..st9s`, `boss1..boss5`, `dust`, `mark`, `exp`, `face`, `end-1..end-4` not
   observed** in the sampled windows. Cause: the census samples a fixed few seconds per stage, and entity dumps are
   only taken for composites that lie wholly inside the 320x200 room (docs/73) -- stage 9 shows exactly one entity and
   it was never fully on screen; the effect sprites (`dust`, `mark`, `exp`) fire on hits, which scripted play does not
   produce; `face`/`system` belong to the setup/status menus. Follow-up: per-stage dwell with the player walking
   right, plus a scripted fight to fire the effect sprites.

## Harness caveats found

* `NIGHT_SHOWSCREEN=gameover NIGHT_SHOWSCREEN_AT=1500` **segfaults** (exit 139): frame 1500 on the
  attract-wait route is still inside the company logo / intro film, where no game state exists for
  `fn_038445` to tear down. With `AT=4000` or `AT=7000` the same run exits 0 and draws the screen. The host warp
  should be guarded on the game having actually started (filed, not fixed).
* The census must select the route per screen: see gap 2.

## Reproduce

    python3 -u scripts/playtest_asset_census.py            # all runs, prints the tables above
    python3 -u scripts/playtest_asset_census.py stage1 pause gameover     # subset
