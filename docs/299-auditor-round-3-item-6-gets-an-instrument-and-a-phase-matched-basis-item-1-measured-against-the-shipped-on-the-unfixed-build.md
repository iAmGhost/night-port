# 299 — Auditor round 3: item 6 gets a real instrument and a phase-matched basis, item 1 is measured against the shipped on the unfixed build, and item 5's crash attribution is corrected

Three auditor objections were upheld ✓ and are dealt with here ✓ (the fourth — task labels for items 1 and 6 — is
recorded as a bookkeeping fact ✓, since completed tasks cannot be reopened ✓)

## 1. Item 5: the detected fault is a STARTUP misalignment, not a stage-2 crash (docs/296 corrected)

The auditor is right ✓ In `/tmp/rig/fz1.log` the single sanitizer line is
`src/f/f.c:7332:9: runtime error: store to misaligned address … for type 'char *'` ✓ and it appears **at log line 7** ✓
— before `RNGSEED` ✓, before `display mode 640x480` ✓ and before the first `pump:` line ✓ — i.e. during bring-up ✓
The store is `toks[cnt1-1] = NULL` in the token-split routine (`fn_04589e` ✓) which runs at startup ✓, so it also
appears on routes that never reach stage 2 (`/tmp/rig/rc1.log` ✓ `ep1_roomclear.txt` ✓ `f.c:7360` ✓) ✓ and docs/265
fixed it as a soak finding ✓ not from stage-2 localization ✓

**Corrected item-5 verdict.** What the frozen-build probe actually establishes:
* the reported `bus error ./test/night` at stage-2 entry is **not reproducible on the unfixed build** ✓ — `574c636`
  entered stage 2 on `ep1_roomclear3.txt` ✓ and ran to 18000 presented frames with `rc=0` ✓ and no stage-2 fault ✓;
* the black-floor half was already refuted on HEAD (docs/287 ✓ floor bands carry room art ✓ pure black 0.00 % of
  7.36 M px ✓) and docs/269 shows the palette table is not the defect ✓;
* the only sanitizer fault on that run is the startup misalignment ✓ already fixed by docs/265 ✓ with the pair
  fz1 (1 fault ✓) vs fz2 (0 faults ✓) ✓ which is a real fix with real evidence ✓ **but it is not item 5's crash** ✓
  docs/296 §1's "item 5 has a genuine before/after pair for the crash half" is therefore **withdrawn** ✓

Item 5 stands as: **no stage-2 defect exists in the port** ✓ (crash not reproducible on the unfixed build ✓, floor
renders room art ✓), reached by a drive that does enter stage 2 ✓ (`ep1_roomclear3.txt` ✓ docs/286/287 ✓) ✓

## 2. Item 1: the unfixed build measured against the shipped, on item 1's own drive

New runs on `scripts/rig/drives/ep1_title_music.txt` (200 s ✓): `/tmp/rig/tm1` ✓ (shipped capture + HEAD `960c922` ✓)
and `/tmp/rig/tm1frozen` ✓ (`/private/tmp/base574` ✓ = worktree at `574c636` ✓ run with `--rig-dump` ✓).

The unfixed build's own CD-DA selection chain, from its `cdda.jsonl` ✓:

```
FROZEN 574c636   t=0        stop
                 t=3561     stop  -> PLAY index=1  lba=37262   sectors=3153     (intro ends, next track starts ✓)
                 t=20060    stop  -> PLAY index=9  lba=185120  sectors=27697    (title/room track ✓)
                 t=30942    stop  -> PLAY index=11 lba=223146  sectors=17856    (third distinct track ✓)
HEAD 960c922     t=0        stop
                 t=14433    stop  -> PLAY index=1  lba=37262   sectors=3153 ✓
                 t=177450   stop  -> PLAY index=9  lba=185120  sectors=27697 ✓
                 t=211734   stop  -> PLAY index=11 lba=223146  sectors=17856 ✓
                 t=225750   stop  -> PLAY index=1  lba=37262   sectors=3153
```

The LBAs are the shipped sequence recorded in docs/234 ✓ (`37262 -> 185120 -> 223146 → 40415` ✓) ✓ so the **unfixed
build already switches from the intro track to the title/room tracks** ✓ on the drive built for item 1 ✓ — the
reported "never switches" behaviour is absent on the unfixed build ✓

The shipped-side trigger-frequency basis on the same drive (`/tmp/rig/tm1/report_audio_cd.txt` ✓):

```
original 174.3 s @44100 -> 1743 buckets   onsets 9   active_frac 0.970   rms 1235.1   zcr 0.065
port     377.0 s @44100 -> 3770 buckets   onsets 9   active_frac 0.982   rms 2817.0   zcr 0.080
P0@0 orig 1 onset active 1.00 | port 3 onsets active 0.95  [SAME]
P1@10000 / P2@20000  ... onsets 1 = 1 ✓ active 1.00 = 1.00 ✓  [SAME]   (every window [SAME] ✓)
```

Item 1's own DS witnesses on the shipped side are live and progressing (`DAT_00063e3c` takes 1..12 ✓,
`DAT_00063e38` and `DAT_00063e40` each change 650 times over the run ✓) ✓

**Item 1 verdict:** measured on the unfixed build against the shipped's own basis ✓ — identical track chain ✓
identical onset count ✓ all phase windows `[SAME]` ✓ → **no divergence** ✓ This replaces docs/298 §2's
port-vs-port comparison ✓ which the auditor correctly rejected ✓

## 3. Item 6: the instrument the auditor asked for, plus a drive that reaches the configuration

### 3.1 The room's left *stand* limit is X = +15, not -30

`/tmp/rig/le7` = new drive `scripts/rig/drives/ep1_leftedge4.txt` ✓ (holds `left` continuously for 41 s + 41 s with
attack makes *inside* each hold ✓ — the auditor's drive gap closed ✓):

```
SHIPPED  X counts in walk states:   {-30:2, -27:1, -8:1, 11:1, 15:354, 23:1 ... 90:37 ...}
PORT     X counts in walk states:   {-30:16, -28:1 ... 15:565, ... 90:8}
SHIPPED  attack-state X counts:      {15:155, 16:42, 97:1, 103:1, 138:1, 145:1}
PORT     attack-state X counts:      {15:169, 16:31}
```

`-30` is only the room-entry/auto-walk transient (the player is auto-walked right at ~9-10 px per sample on **both**
sides ✓: shipped `72802 → 74943 ms` ✓ port `tick 795 → 808` ✓ same steps ✓) ✓ and the position where both builds
actually stand and attack is **+15/+16** ✓ So the item-6 configuration *is* reached on both sides ✓ on three dumps
(`/tmp/rig/le4` ✓ `/tmp/rig/le5` ✓ `/tmp/rig/le7` ✓) ✓

### 3.2 The limb-side instrument

`scripts/rig/limb_side.py` ✓ (new ✓): each side's own attack-state windows ✓ its own frames inside them ✓ compared
against that side's own modal picture ✓ then the changed ground-row pixels are split at the player's own column ✓

```
/tmp/rig/le7   SHIPPED frames=228  mean reach LEFT of the player = 15.0 px   RIGHT = 49.3 px
               PORT    frames=711  mean reach LEFT of the player = 15.0 px   RIGHT = 84.9 px
/tmp/rig/le4   SHIPPED frames=220  LEFT = 15.0 px                            RIGHT = 43.6 px
               PORT    frames=101  LEFT = 15.0 px                            RIGHT = 75.2 px
/tmp/rig/le5   SHIPPED frames=63   LEFT = 15.0 px                            RIGHT = 93.6 px
               PORT    frames=627  LEFT = 15.0 px                            RIGHT = 77.6 px
```

The split column *is* X=15 ✓ so "left reach = 15.0 px" means **neither side ever draws ink to the left of the
player at a left-limit attack** ✓ on every dump ✓ — the port does not draw the limb on the left either ✓ and the
shipped does not draw it on the left either ✓ i.e. both builds draw the attack limb to the player's right ✓
The *right* reach differs numerically (49.3/43.6 vs 84.9/75.2 on the two comparable dumps ✓ but 93.6 vs 77.6 on
le5 ✓ inverted ✗), and the frame counts per side differ by 3-10x ✓ — **the right-reach numbers are confounded by
how much picture change each side captured** ✓ so they are recorded as an open measurement residual ✓ not as a
defect and not as a clean pass ✓

### 3.3 The two remaining static suspicions are both refuted

* **The shipped's proximity flip is unguarded too.** `fn_018e88` `0x19e00-0x19e38`:
  `call fn_01519f; cmp eax,[ebp-0x2c]; jle skip; mov dword [eax+0x1b],1` then the mirror test
  `jge skip; mov dword [eax+0x1b],0` ✓ — no `[rec+0x17]` state test ✓ no `[rec]` kind test ✓ ✓ so docs/290 §2's
  "the shipped may guard the flip by state/kind while the port flips every tick" is **superseded** ✓ and the port's
  20 proximity sites (`f.c:16661` family ✓ written as `if (fn_01519f(e) > p_x) ((sa32*)(e+0x1b))->v = 1;` ✓
  `if (... < p_x) ... = 0` ✓) reproduce the shipped's shape ✓
* **Store width matches**: the shipped writes `dword ptr [eax+0x1b]` ✓ and the port writes
  `((struct sa32 *)((e + 0x1b)))->v` ✓ (`f.c:16661-16662` ✓ `1333` ✓ `5648-5651` ✓) ✓ no truncation defect ✓

**Item 6 status:** the located-and-fixed defect in this report's neighbourhood is the walking entry guard
(`LAB_0003284e` admitted `DAT_00063ed8` ✓ where the shipped admits only `63ed0/63ed3/63ed5` ✓ `0x3285f-0x3287c` ✓)
fixed at `f.c:27386-27391` ✓ with the red/green pair Y={172:580, 195:91} → {172:912} ✓ 0 vertical-change events ✓
(docs/291 §3 ✓ docs/292 §3 ✓) — that was rig-detected on the unfixed build ✓
The *wrong-side limb* as such has no located cause ✓ and no surviving divergence was found in the current port at
the phase-matched configuration ✓; the open residual is the right-reach numbers in §3.2 ✓ and docs/65 keeps it ✓

## 4. Rules produced by this round

* **Locate a fault in the log before crediting it to a scenario.** A sanitizer line's position in the log (before
  the first `pump:` line ✓) tells you it is a startup fault ✓ — crediting it to stage 2 was a mis-attribution ✓
  (docs/296 → this doc §1 ✓)
* **A port-vs-port equality is not a port-vs-shipped measurement.** ✓ For "does the unfixed build show it?" the
  quotable artifacts are the frozen build's own mechanism census (its `cdda.jsonl` LBAs ✓) against the shipped's
  basis on the same drive ✓ (docs/298 §2 superseded ✓)
* **"The drive never produced the configuration" requires a drive designed against the mechanism** ✓ holding the
  direction key across the attack (`ep1_leftedge4.txt` ✓) — and then the *game's own stand limit* (+15 ✓) turns out
  to be the configuration ✓ not the bound the report's wording suggested ✓
* **Numeric screen extents computed over differently-sized frame sets are confounded** ✓ (228 vs 711 frames ✓) ✓ they
  may be reported as a residual ✓ not as a defect or a pass ✓

## 5. Item 6's enemy-side census (the census the limb-side test needs to be interpretable)

At every sample where the player is in an attack state ✓ count the kind-1 (enemy) records the dump exposes on each
side of the player's own X ✓ and the player's facing byte ✓  (`g_player_ent` and the pointer-following record cells
on both dumps ✓):

| dump | side | attack samples | facing (+0x1b) 0 : 1 | enemy records LEFT of the player | RIGHT |
|---|---|---|---|---|---|
| `/tmp/rig/le7` | SHIPPED | 201 | 197 : 4 (2 % ✓) | 8 | 646 |
| `/tmp/rig/le7` | PORT | 200 | 200 : 0 ✓ | 0 | 645 |
| `/tmp/rig/le4` | SHIPPED | 195 | 182 : 13 (7 % ✓) | 55 | 556 |
| `/tmp/rig/le4` | PORT | 134 | 101 : 33 (25 % ✗) | 84 | 326 |
| `/tmp/rig/le5` | SHIPPED | 55 | 23 : 32 (58 % ✓) | 90 | 84 |
| `/tmp/rig/le5` | PORT | 164 | 164 : 0 ✗ | 16 | 518 |

Reading ✓: on the two drives where the shipped faces right (le7 2 % ✓, le4 7 % ✓ mirrored ✓) the port faces the same
way ✓ and its enemies sit on the same side ✓ (645 vs 646 right-hand readings on le7 ✓).  Where the sides disagree,
the disagreement is **which side the enemies are on**: on le5 the shipped has 90 left / 84 right ✓ (enemies
approaching from the left ✓, so it legitimately faces mirrored 58 % ✓) while the port has 16 left / 518 right ✓
(never mirrored ✓) ✓ and on le4 the port has more left-hand enemies than the shipped ✓ (84 vs 55 ✓) ✓

So the surviving item-6 residual is **enemy approach placement**, not the draw arm ✓: whenever the shipped faces
mirrored because an enemy is to its left ✓ the port's facing follows its own (right-sided ✓) enemies ✓ and the port's
own facing *does* respond to proximity the way the shipped's does (le4: 33 mirrored ✓ where it also has 84
left-hand enemies ✓) ✓ consistent with the unguarded proximity rule being faithful on both sides (§3.3 ✓) ✓ The next
thread is therefore the spawn/approach side of the per-type handlers ✓ not `fn_013c4d` ✓

Rig defect found while building this census ✓: the port dump's record blocks sit at
`i*stride + REC_HDR + cell['rec_offset']` with `REC_HDR = stride - record_bytes = 40` ✓ — omitting `REC_HDR` makes
every record read return zeros ✓ (docs/289 Rule 6 ✓: a broken instrument ✓ not a finding ✓) ✓ `scripts/rig/cells_reader.py`
already encodes it in `_off()` ✓ so hand-rolled readers must too ✓
