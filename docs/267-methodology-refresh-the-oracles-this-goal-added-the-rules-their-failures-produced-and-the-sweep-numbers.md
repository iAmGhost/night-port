# 267 — Methodology refresh (docs/170 style): the oracles this goal added, the rules their failures produced, and the current sweep numbers

## 1. Oracles and instruments now in the rig (each one exists because a question could not be answered without it)

| instrument | what it answers | where |
|---|---|---|
| `route.sh` (drive + `--rig-dump --rig-hz 10`, pinned `NIGHT_RIG_STEP_CALLS`/`NIGHT_RIG_FPS`) | the only admissible **before/after** basis: dump-backed cells, frames, PCM, timeline, repeated ≥2× per side ✓ | `scripts/rig/route.sh` |
| shipped-side **flat DS window** census (`state_XXXX.ds.bin`, 0x16000 B, offset = DS − 0x50000) | any shipped DS cell's time series offline, no re-run ✓ (supersedes docs/232 §4 ✓) | docs/264 §3 |
| `state_XXXX.heap.bin` (0x56a0 B/sample) ✓ | shipped heap blocks ✓ | docs/264 §3 |
| `ent_life.py` (nearest-predecessor identity linking) | per-enemy lifetimes/hit absorption immune to slot-pointer reuse ✓ | docs/253 |
| `exp_award.py` | dump-side award census per type index (`+0x32` values ✓ the item-2b green gate ✓) | docs/190 |
| `field_writers.py` (whole-modelled-body capstone walk, gap-resumed) | every writer of a record displacement regardless of addressing form ✓ (indexed/SIB and accessor-macro stores included ✓) | docs/187/239 |
| `imm_scan.py` / `branch_xrefs.py` / `table_scan.py` | immediates, call edges, scaled-index tables ✓ | docs/181/258 |
| displacement byte-scans for DS cells (`disp32` + opcode sniff) | "which functions touch DAT_000…" ✓ **with the caveat in §2.4** ✓ | docs/264 §2, §266 |
| in-process witnesses: `NIGHT_RECBUILD`, `NIGHT_CHAINTRACE`, `NIGHT_HITTRACE`, `NIGHT_YCLAMP`, `NIGHT_ACTTRACE`, plus the new `act_level_array/value/accum`, `act_ring_codes/flags/cursor`, `act_cursor_hist` | record constructions, chain resolution per call, hit/atype census, Y clamp decisions, technique-container inventory + rejection census, the level/ring/cursor triple `fn_03272f` consumes ✓ | `f.c`, `main.c` |
| `asan_soak.sh` (now builds `night-asan` first ✓) | nine-episode fault oracle ✓ | docs/265 |
| `fuzz.py` (episodes × seeds × cap, parallel) | coverage census + crash oracle ✓ | docs/126/266 |

## 2. Standing rules this goal produced (each is a correction of a mistake I actually made)

1. **Anchor on a transition, never on a level measurement** ✓ and never compare two measurement bases: an
   `fn_03272f`-entry histogram (4857 events ✓) against a 244-sample DS census is meaningless ✓ (docs/264 §7, docs/266) ✓
2. **Ad-hoc runs are directional only.** Without `--rig-dump`, run-to-run spread is large (`level too high`
   552/394/386/210 ✓, `len != k` 4132/3244/3166/2123 ✓, ring consumed flags differing between consecutive runs ✓) — quotable
   numbers must come from `route.sh`, repeated ✓ (docs/191 §1, docs/239 §4, docs/265 §4) ✓
3. **Build freshness is part of a measurement.** A soak that reports a line number that is blank in the source is
   measuring yesterday's binary ✓; `route.sh`'s `find -newer` guard and `make`'s whole-second comparison disagree, so the
   guard now forces `make -C port app -B` and re-checks ✓ (docs/265 §2, docs/266 §1) ✓
4. **Byte-pattern scans cannot classify an instruction.** A `disp32` hit looked like a fourth writer of
   `DAT_00063f28` ✓ and was `memset(...,0,0x28)` via `call 0x41e20` ✓; grep for `0x3b5c0` missed `fn_0003b5c0` ✓; grep for
   a displacement missed every `PL16()/PL32()` accessor store ✓ (docs/264 §2, docs/252 §2, docs/254 §3) ✓
5. **A cell is not an identity.** The `ent##` dump cells follow DS pointer arrays, so per-slot hit/lifetime censuses
   through them are void ✓; identity needs `ent_life.py` ✓ and slot reuse contaminates value-decrement counts (use the
   net lowest value ✓) (docs/252 §1-2, docs/246 §5) ✓
6. **A fixture can encode a wrong transcription.** Two `fn_01441e` tests asserted the invented slot rule ✓ and one
   `fn_04589e` test asserted the unaligned array start ✓ — when disassembly and a test disagree, fix the test and say so ✓
   (docs/263 §4, docs/265 §1) ✓
7. **Invent nothing in a guard.** Two landed fixes were over-strict invented conjuncts: the `fn_01586b` clamp
   (`cur < mn` ✓ docs/254) and the `fn_01441e` slot window (`w1 <= fr` ✓ docs/263) ✓ Both were found by reading the
   shipped's jump structure, not by port-side reasoning ✓
8. **`"game main unwound"` is the harness's frame-budget `longjmp`, not a fault witness** ✓; the fault oracle is
   ASan/UBSan text and `rc != 0` only ✓ (`rc=124` is a timeout ✓) (docs/223 §2, docs/266) ✓

## 3. Six-item status as the rig sees it

| item | rig detection | localization | fix | evidence |
|---|---|---|---|---|
| 1 music switch | ✓ detected | ✓ localized | **closed, no surviving defect** | docs/232→235 (the CD-age counter is aged by the shipped's timer IRQ; selection schedule matches) ✓ |
| 2a EXP text garbled | ✓ | ✓ `fn_03b718` label blit index 11 vs 10 | **fixed** | docs/221/227 (blit-footprint probe as the admissible basis ✓ pixel census insufficient ✓) |
| 2b per-type EXP award | ✓ | open (chain walk: `code0 mismatch` 32019 dominant, then `slot consumed` 5286, `len != k` 2123, `level too high` 210; container/targets/ring/window all faithful ✓) | 2 partial fixes landed (clamp ✓, slot window ✓) | docs/254/263/264: hits 30→79, enemy `+0x23` floor 23→15, player 0x33 41→65 ✓; green gate (settled kind-1, `+0x32=10`, `player[+0x52]` rising) unmet ✓ |
| 3 upgrade menu lights all items | ✓ | ✓ `fn_038f3d` row loop (cursor-indexed reads) | **fixed** | docs/206/207/228 (`NIGHT_LUMENU` witness pair ✓) |
| 4 card key-wait skipped | ✓ | ✓ `fn_00035d5d`/`fn_00035dfc` no-op stand-in | **fixed** | docs/217/231 (both-sides key-wait basis ✓) |
| 5 stage-2 crash + black floor | crash **not reproducible** across 5 route/build combos ✓ | screen half localized to the room chain (`fn_0406f5`/`fn_03f15f` candidates) | open | docs/222-224 ✓ |
| 6 left-edge limb side | ✓ detected | anchors/arms/blits/anchor arrays proven faithful ✓ | open | docs/220/229/230 ✓ |

## 4. Sweep numbers as of HEAD `0224c27`

```
make -C port test                      ALL PASS   (app binary rebuilt by the target ✓)
nine-episode ASan/UBSan soak           stage 1..9 rc=0  ubsan=0  asan=0  segv=0  4000 frames each  (/tmp/rig/soak266)
fuzz census (9 episodes, 8 seeds, 4 workers, cap 60, round 0)   coverage 248/503  crashes 0   corpus 5
   (two rounds ended rc=124 = wall timeout, 0 new functions, not faults)
state-oracle tally, canonical route, identical basis before/after the docs/263 fix:
   rb3 = rb7 = {SIZE?:35, PTR:52, SAME:69, REPR:19, DIFF-SIZE:13, DIFF:28,
                NON-GAMEPLAY:host_address_space:2, NON-GAMEPLAY:device_plumbing:27}
   activity profiles 271 cells / 275 phases all SAME; pacing orig 2.71 vs port 21.00 changes/s (ratio 0.13)
```

**No new unattributed residual rows** from either landed fix ✓ — and equally **no residual row closed** ✓: the pacing
basis (0.13 ✓) and the room-composite stop behind item 5's black floor remain the two largest open bases ✓ The sweep
must be re-run after items 5 and 6 land ✓ ("after the six fixes" is part of its contract ✓) ✓
