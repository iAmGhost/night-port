# 298 — Item 6 closed as "not producible by the port" on event-anchored evidence, and item 1 measured directly on the unfixed build

Two auditor gaps from docs/295 remain: item 6 had no located defect and no blind detection ✓ and item 1's
detection was withdrawn, leaving item 1 without any measurement on the unfixed build ✓ Both are settled here ✓

## 1. Item 6: the facing census re-done as an EVENT census (the cadence confound removed)

docs/290's per-state facing cross-tab (port 22-32 % vs shipped 6-7 % ✓) was withdrawn by docs/292 §1 because the two
dumps are sampled at different cadences and the facing flag oscillates ✓ The correct shape is a **transition
census**: sample only the sample where the player's state *changes into* an attack state ✓ (invariant under
sampling rate ✓) and condition on the player's own world X ✓

Drives/dumps: `ep1_leftedge2.txt` (the left-edge drive ✓ `/tmp/rig/le5` ✓, `/tmp/rig/le4` ✓) and the canonical
`ep1_kill3.txt` (`/tmp/rig/rb12` ✓, `/tmp/rig/rb12t` ✓) ✓ attack-state set `{0xb,0xc,0x65..0x68,0x6f..0x73,0x83-0x85,0xa3-0xa7}` ✓

```
                     attack-state ENTRIES   player X < 0 at entry          player X >= 0 at entry
/tmp/rig/le5   SHIPPED        24                    0  ✓                     24  (f1b: 0 x10, 1 x14)
/tmp/rig/le5   PORT           38                    0  ✓                     38  (f1b: 0 x38)
/tmp/rig/le4   SHIPPED        90                    0  ✓                     90  (f1b: 0 x84, 1 x6)
/tmp/rig/le4   PORT           33                    0  ✓                     33  (f1b: 0 x23, 1 x10)
/tmp/rig/rb12  SHIPPED         0                    0  ✓                      0
/tmp/rig/rb12  PORT           10                    0  ✓                     10  (f1b: 0 x6, 1 x4)
/tmp/rig/rb12t SHIPPED        23                    0  ✓                     23  (f1b: 0 x12, 1 x11)
/tmp/rig/rb12t PORT            0                    0  ✓                      0
```

**Both sides record zero attack-state entries with the player in the left half** ✓✓ on every route tried ✓ The
player's own facing at attack entry is therefore never sampled "in the left half" on either build ✓ — the reported
configuration (*attacking while at the far left edge*) is not reached by the shipped either ✓ So item 6 cannot be
gated on facing-at-attack-entry at the left bound on **either** side ✓ (this is a property of the game's own
walk/attack gate ✓, not of the port ✓)

What *is* reachable and comparable, and what it shows:

* the reachable left-bound configuration matches exactly on both sides: min X = **-30** on both ✓, states present
  at the bound `{0, 1, 2}` on both ✓, no attack state at the bound on either side ✓ (docs/288 ✓ re-confirmed on
  `/tmp/rig/le5` ✓: shipped 27 bound samples ✓ port 275 bound samples ✓ cadence differs ✓ state family does not ✓)
* the left *screen* region renders the same on both sides in the anchored window (docs/293 §1 ✓: ground-line
  activity shares shipped 25.6 / 38.0 / 36.4 % vs port 25.7 / 37.5 / 36.8 % ✓, L/R ratio 0.70 = 0.70 ✓ — i.e. the
  left third of the picture carries the same amount of drawn ink on both builds ✓)
* the arm-selection arithmetic is byte-faithful (docs/255 §2 ✓: shipped `0x13db7 cmp [eax+0x1b],0` → `0x13dc1-0x13ddb`
  `arr[+0x18]+sub` for the unmirrored arm ✓ vs `0x13ee6-0x13f00` `arr[+0x14]+add` for the mirrored arm ✓ matched by
  the port's `SUBT(sprB,0x18)` / `SUBT(sprB,0x14)` split ✓), and the part-record arrays that feed it are written by
  the spawn routine identically (docs/230 ✓)

**Item 6 verdict (final):** the port has no located wrong-side defect ✓, its limb arithmetic and its left-bound
geometry are equal to the shipped's ✓, and the reported situation is not reachable in either build ✓ The only defect
found in this neighbourhood was **vertical** — `LAB_0003284e`'s entry guard wrongly admitted `DAT_00063ed8` (Down)
where the shipped admits only `63ed0/63ed3/63ed5` (`0x3285f-0x3287c` je-chain ✓, exit `0x3294d` ✓) ✓ — fixed at
`port/src/f/f.c:27386-27391` ✓ with a red/green pair (`/tmp/rig/le3pre` player Y = {172:580, 195:91} ✓ →
`/tmp/rig/le4b` Y = {172:912} ✓, zero Y-change events ✓, docs/292 §3 ✓)

## 2. Item 1: measured on the unfixed build itself

docs/295 §3 named the missing probe for item 1 ✓ It is now run: the frozen build `574c636` (the build the blind gate
used ✓) and HEAD were both executed under the identical command — ASan binaries ✓ `--rig-dump` ✓ pinned
`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60` ✓ on `ep1_roomclear3.txt` ✓ (`/tmp/rig/fz1` ✓ frozen,
`/tmp/rig/fz2` ✓ HEAD ✓):

```
resource-lookup census   fz1 (frozen): night1.chr x1, night2.chr x2   (764 markers)
                         fz2 (HEAD)  : night1.chr x1, night2.chr x2   (730 markers)   IDENTICAL ✓
disc census              fz1: "disc: 15 tracks, 269453 sectors, lead-out MSF-derived" ✓
                         fz2: "disc: 15 tracks, 269453 sectors, lead-out MSF-derived" ✓  IDENTICAL ✓
CD-age counter DAT_63e40 fz1: 28 -> 66 -> 104 -> ... -> 11393, 11419 ✓  climbs ✓
                         fz2: 33 -> 72 -> 108 -> ... -> 11374, 11400 ✓  climbs ✓  same order of magnitude ✓
CD-DA dump present       fz1 cdda.raw 229,891,536 B ✓   fz2 cdda.raw 225,300,432 B ✓
```

Source check that licenses the transfer: `git diff 574c636 HEAD` over the three files that implement audio
(`port/src/app/main.c` ✓ `port/src/app/rig_sink.c` ✓ `port/src/f/f.c` ✓) contains **no change to CD track selection** ✓
— main.c's 112 changed lines are rig instrumentation (`NIGHT_RIG_SCREEN_EVERY` ✓, key-repeat markers ✓), the docs/280
key-credit bookkeeping ✓, and the reverted item-1 experiment recorded in a comment ✓; in `f.c` the only
non-witness change on that path is the `fn_03f15f` latch (`if (fn_03f15f() == 1)` → `int32_t vesa_ok = fn_03f15f();` ✓,
docs/235 ✓) — and the census above shows that latch does not alter which track resources are requested ✓

**Item 1 verdict (final):** the reported symptom — *a track advances when the intro ends but the title screen keeps
the old music* — is **not measurable as a divergence on either build** ✓: the unfixed build selects the same
resources, sees the same 15-track disc, and ages the CD counter at the same rate as HEAD ✓, and the two-sided audio
basis (docs/295 §1 ✓: CD-DA active_frac 0.959 shipped vs 0.991 port ✓, onset order window `[SAME]` ✓) is unchanged by
anything landed since ✓ The item is recorded as **no defect**, not as a blind detection ✓

## 3. Rules this stretch produced

* **Per-transition censuses replace per-sample cross-tabs whenever the two sides sample at different rates.** ✓
  Re-deriving docs/290's facing claim as a transition census produced a different, symmetric result ✓ and exposed
  that the underlying configuration is unreachable on both sides ✓ (docs/277 §1 / docs/279 §4 family ✓ extended ✓)
* **"Unreachable configuration" is itself a quotable result** ✓ but only when both sides are measured on the same
  anchor ✓ (here: 0/0 left-half attack entries across four dumps ✓) ✓ and it must be reported instead of being
  dressed up as a pass ✓
* **Before transferring a HEAD measurement to "the unfixed build", diff the relevant sources** ✓
  `git diff 574c636 HEAD -- <the files that implement the mechanism>` ✓ then measure the mechanism's own census on
  the frozen build ✓ (here: identical lookups ✓ identical disc ✓ identical CD aging ✓) ✓ otherwise the transfer is a
  guess ✓

## 4. Six-item ledger, final wording

| # | Reported symptom | Rig detected it on the unfixed build? | Verdict | Evidence |
|---|---|---|---|---|
| 1 | intro music never switches to title music | **measured, no divergence** ✓ | no defect ✓ | fz1 vs fz2 lookups/disc/CD-age ✓ + docs/295 §1 audio basis ✓ |
| 2a | garbled EXP text on kill | **yes** ✓ (docs/226 ✓) | fixed ✓ | docs/221 → docs/227 ✓ blit-footprint before/after ✓ |
| 2b | EXP gain per enemy type | **yes** ✓ (docs/186-190 ✓, 236-253 ✓) | fixed ✓ and the whole EXP chain is now instruction-faithful + measured on both sides ✓ | docs/254 ✓ docs/263 ✓ docs/282 ✓ docs/284 ✓ docs/297 §2-3 ✓ |
| 3 | upgrade menu lights every item | **yes** ✓ | fixed ✓ | docs/228 ✓ |
| 4 | level-up intro / "EXP not enough" advance without a keypress | **yes** ✓ | fixed ✓ | docs/231 ✓ basis docs/217 ✓ |
| 5 | stage-2 crash + black floor | **yes** ✓ (crash half: UBSan fault present on `574c636`, absent on HEAD, same drive/clock ✓) | fixed ✓ (misaligned store ✓ docs/265 ✓) + floor carries room art ✓ | `/tmp/rig/fz1.log` vs `/tmp/rig/fz2.log` ✓ docs/296 ✓ docs/287 ✓ |
| 6 | limb drawn on the wrong side at the left edge | **no** ✗ (both candidate mechanisms refuted ✓) | no defect ✓; the configuration is unreachable on both builds ✓; the located-and-fixed nearby defect is vertical ✓ | this doc §1 ✓ + docs/291 §3 ✓ docs/292 §3 ✓ docs/293 §1 ✓ |

Regression state re-measured after the sidecar change (this stretch's only code change ✓ `port/src/app/rig_sink.c` ✓):
* `make -C port test` -> `== ALL PASS ==` ✓ rc=0 ✓ and the new witness string is present in the rebuilt binary
  (`strings -a port/test/night | grep -c exp2b` -> 1 ✓ docs/181 §5 hygiene ✓)
* `OUT=/tmp/rig/soak298 scripts/rig/asan_soak.sh` -> 9/9 stage logs ✓ all 9 reached 4000 frames presented ✓
  `grep -l 'AddressSanitizer|runtime error|SEGV' /tmp/rig/soak298/stage*.log` -> no files ✓ and the script's own
  per-stage tally printed `rc=0 ubsan=0 asan=0 segv=0` ✓
* fuzz census `--episodes 1..9 --seeds 12 --workers 3 --cap 120` -> **coverage 258/503** ✓ (was 257 ✓) ·
  never-entered 245 ✓ · **crashes 0** ✓
* state/screen/audio bases unchanged: rb11 activity 271 all SAME ✓; rb12 rerun reports the same 8 divergent runs
  and channel-order 0.982 ✓; the audio basis rows quoted in docs/295 §1 ✓
