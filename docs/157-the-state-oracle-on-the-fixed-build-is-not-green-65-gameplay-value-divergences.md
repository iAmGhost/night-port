# docs/157 — the state oracle on the fixed build is not green: 65 gameplay value divergences, now listed, plus a drift probe that does not reproduce

The closeout item "state / screen / audio diffs green on the fixed build" is **not satisfied**, and this file
states what the oracle actually says on the current tree so the claim cannot be quietly waved away.

Route: `/tmp/rig/g15` = `scripts/rig/drives/ep1_kill3.txt`, 300 s, `RIG_HZ=10`, `RIG_ENTS=12` on both halves
(1846 shipped / 2169 port samples), port binary stamped by `route.sh` itself.

## 1. Bookkeeping cells now get a named verdict instead of DIFF

`scripts/audit/gen/oracle_families.json` names the families docs/134 triaged by hand, and `diff_state.py`
reports them under a named verdict while still printing the raw values:

```
non-gameplay families: {'host_address_space': 2, 'arena_state': 3, 'dos_extender': 10}
```

* `arena_state` — `0x53350` cursor, `0x53354` block base, `0x53358` high-water. Shipped's allocator starts at
  offset 8, the port reserves `0x40` as a NULL guard (`f.c:3209`), and the counts track allocation order.
* `host_address_space` — `0x531f4` / `0x531f8`, the shipped framebuffer window (`0xa0000..0x9ffff`) against the
  port's own buffer offsets. Kept visible because blit/clip code reads them.
* `dos_extender` — `0x53378, 0x5337c, 0x53384, 0x5338c, 0x53390, 0x53394, 0x533a0, 0x533b2, 0x533b9, 0x533bb`:
  selector-form addresses, config byte, device id. The port models the slots without the extender handshake.

Effect on the tally: DIFF verdicts 76 → 65, DIFF-SIZE 16 → 13, and the 15 moved cells are each attached to a
written reason rather than dropped.

```
mode=orig-vs-port  cells=344  phases=274
tally={'SIZE?': 35, 'PTR': 52, 'SAME': 64, 'REPR': 19, 'DIFF-SIZE': 13, 'DIFF': 65,
      'NON-GAMEPLAY:arena_state': 3, 'NON-GAMEPLAY:dos_extender': 10, 'NON-GAMEPLAY:host_address_space': 2,
      'ENTROPY': 14, 'DIFF-WIDTH': 7, 'REC-NULL': 14, 'REC-DIFF': 35, ...}
residual same-width gameplay divergences: 65
```

## 2. What the 65 residual cells are

They are almost entirely the game's live-state block, and they group the way the previous docs predicted:

| group | cells | already understood as |
|---|---|---|
| camera / scroll | `0x639bc`, `0x639c8`, `0x639d8`, `0x639dc`, `0x639e8` | docs/142: shipped camera parks at 0..27, port reaches 158 after docs/147 (was 298) |
| room command list | `0x63e14`, `0x63e18`, `0x63e20` | docs/153: the unlink path is faithful, the port enters it ~6 extra times per route |
| entity population | `0x53c64`, `0x53c68`, `0x53c6c`, `0x53c88` (`g_gauge_other_off`) | docs/137: each side's count agrees with its own `+0x13f` chain, but the *levels* differ (shipped ≤4 in 99 %, port 24 % at ≥6) |
| hit / reaction bookkeeping | `0x6335c`, `0x63360`, `0x63364`, `0x6336c`, `0x6339c`, `0x633b0`, `0x633b4`, `0x62d58`, `0x62d5c` | docs/155: hit code 11 lands on the port's player (67 samples) and never on shipped's (0) |
| CD selection | `0x53c48` (`0x53c40`/`0x53c44` now DIFF-SIZE: no shipped span) | docs/131 / docs/135: selection arithmetic faithful, audibility and timing diverge |
| CONTINUE countdown | `0x53100` | docs/134 §6 → docs/147: the 3→2 drop came from the spurious player death; on this route it still differs |
| board / device block | `0x62a8c`, `0x62a90`, `0x62a94`, `0x63e44`, `0x63e48`, `0x63e5c`, `0x63e7c`, `0x63e80`, `0x63e84`, `0x63ea5`, `0x63ed0`, `0x63ed3`, `0x63ed5`, `0x63ed8`, `0x63f0c`, `0x63f50..0x63f80` (13 consecutive dwords), `0x63f90`, `0x63f94`, `0x63f98`, `0x63fac`, `0x63fb0`, `0x63fb8`, `0x63ffc` (`g_node_list_active`), `0x64000` (`g_clock_ticks`), `0x64090`, `0x64094` | not yet attributed |
| misc | `0x51c98`, `0x5333c` (`g_ss_saved`), `0x53451`, `0x53640` | `g_ss_saved` is stack-switcher state and probably belongs in `dos_extender`; `0x51c98` was docs/134 §5's "rig-basis" cell |

So the honest reading is: the rig is not green, and it is not green in a *structured* way — the largest
remaining divergence family is the population/camera cluster (docs/142 → docs/153 → docs/155), which is the
same cluster the media oracles report as the pacing flag.

## 3. The drift probe does not reproduce on this route, and its output must not be read as alignment

docs/132 reported "only 4/37 cells become >90 % equal, all at +0.8 s; 33 never align". Re-run on `/tmp/rig/g15`
with the current script:

```
cells that become >90% equal under some shift: 137 of 283
their shifts: median +0.8 s, min +0.7 s, max +0.8 s
cells never >90% equal: 146
```

Two things are wrong with that and they are rig problems, not game findings:

* `equal@lag0` is `0.838` for a large block of cells (e.g. `0x50010`, `0x51960`, `0x51ad4`, `0x531c8`) — the
  same number for cells that have nothing in common. A shared baseline that is exactly the same for
  constant-image cells and live cells means the grid comparison is dominated by something other than the cell.
* the best lag for those rows is `-40.0 s`, which is the `--max-lag` boundary. A best shift pinned to the edge
  of the search window is a search artifact, not a measurement.

Also, `drift_probe.py` samples a different cell set from `diff_state.py`: none of the 65 residual cells
(`0x53c68`, `0x639dc`, `0x63e14`, `0x64000`, …) appear in its output, so "137 of 283 align" says nothing about
whether the flagged divergences are phase artifacts. Before any "state diffs are green modulo re-phasing" claim
can be made, `drift_probe.py` must (a) sample the same cell set as the oracle, (b) refuse to report a best lag
that sits on the search boundary, and (c) explain the `0.838` baseline. Tracked as a rig defect.

## 4. What has to happen before this closeout item can be called green

1. Fix `drift_probe.py` as above, then state how many of the 65 cells are re-phaseable and how many are not.
2. Attribute the unattributed block (`0x63f50..0x63f80`, `0x62a8c..0x62a94`, `0x63e7c..0x63ea5`) to shipped
   symbols with writers named from `mem_access.json`, then verdict each.
3. Move `g_ss_saved` (`0x5333c`) into `dos_extender` only after checking that no game decision reads it.
4. Close the population/camera cluster (the one the media oracles already localize), which is also the head of
   the docs/155 hit-code-11 question.

## 5. Reproduce

```sh
OUT=/tmp/rig/g15 RIG_HZ=10 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300
python3 scripts/rig/diff_state.py --orig /tmp/rig/g15/orig --port /tmp/rig/g15/port \
        --drive scripts/rig/drives/ep1_kill3.txt --md /tmp/rig/g15/state_diff.md
python3 scripts/rig/drift_probe.py --orig /tmp/rig/g15/orig --port /tmp/rig/g15/port
```
