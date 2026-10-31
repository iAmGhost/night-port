# 293 — Item 6 gets its both-sides screen basis: ground-line drawing activity is distributed identically (L/R 0.70 = 0.70, column shares within 0.7 pp), and the two rig defects that had been blocking that basis are fixed

## 1. The basis that finally works (`/tmp/rig/rm5`, drive `ep1_roomclear3.txt`, 300 s)

Why earlier attempts failed, in order ✓: `le1` compared shipped rooms against port slide cards (docs/288 §1 ✓);
`le4`'s cluster census was contaminated by other motion at a 0.57 cadence ratio (docs/288 §4 ✓); `lk1`'s port-side
witness frames all landed in the card phase (docs/292 §5 ✓). The usable recipe:

1. **Anchor each side on its own room entry**, not on time ✓
   * shipped: the first sample with an admissible player record (`kind 0 ✓, +0x29 ∈ 1..0x2000 ✓, x ∈ (-2000,4000) ✓`)
     → `t0 = 48069 ms ✓`, take its `320x200` payload frames in `t0 .. t0+60 s` → **99 frames** ✓
   * port: the `res stage1.wal` marker ✓ → `t0 = 66950 ms ✓`, take witness bins whose name maps to a
     `frames.bin` record in `t0 .. t0+60 s` → **98 frames** ✓ (near-equal frame counts ✓, so neither side is
     sampled thinner ✓)
2. **Compare each side against its own modal picture** (per-pixel modal luminance over that window ✓, delta > 24 ✓)
   → no cross-side colour convention is touched (docs/287 §2 ✓).
3. **Compare distribution shares, not absolute ink** ✓ (absolute ink differs 837166 vs 546582 ✓ — that gap is a
   dump-side convention and is not quotable ✓).

Result, ground-line rows 150-200, columns normalised to a 320-wide room:

```
SHIPPED  frames=99  delta=837166   cols<80: 214353   80-199: 318408   >=200: 304405   L/R = 0.70
                                        share   25.6 %        38.0 %       36.4 %
PORT     frames=98  delta=546582   cols<80: 140604   80-199: 205112   >=200: 200866   L/R = 0.70 ✓✓
                                        share   25.7 %        37.5 %       36.8 %   (<= 0.7 pp apart ✓)
row band on the same window: shipped meanRow 156.3 (rows<170 : >=170 = 2.33)  port 160.6 (1.63)
```

A limb drawn on the wrong side at the left edge has to move ground-line activity from one column band to the
other ✓ it does not: the shares agree to within 0.7 percentage points and the left/right ratio is identical
(0.70 = 0.70 ✓✓) on both sides on the same drive with a controlled window ✓ **Item 6 does not reproduce on HEAD** ✓
The row-band ratio still differs (2.33 vs 1.63 ✓, meanRow 156.3 vs 160.6 ✓) — recorded as an open, *vertical*
distribution residual of the docs/242/255 ground-line family ✓ it is not item 6's side-of-limb symptom ✓ and it is
cadence-sensitive in a way the column shares are not (the window is anchored, but the shipped replays at ~4 Hz and
the port dumps at 10 Hz, and the row census is dominated by which frames happen to animate ✓) ✓

## 2. Item 6 verdict

| basis | result |
|-------|--------|
| record state at the left bound, both sides | identical: `X = -30 ✓, Y = 172 ✓, +0x1b = 0 ✓, state 0 ✓` (docs/288 §3, re-confirmed `/tmp/rig/le3`: shipped 35 samples, port 277 ✓) |
| every input the limb draw consumes | byte-faithful: sprite arrays (docs/212), anchor arrays + both arms (docs/220), mirror stores (docs/229), part arrays from the spawn routine (docs/230), box arithmetic + `0x13db7 cmp [eax+0x1b],0` polarity (docs/255) |
| the shipped's facing writers for the player | key-driven `0x328ff/0x32923` ✓ (modelled at `f.c:27394/27396` ✓), argument-driven `0x38232` ✓ (`f.c:15683` ✓), orientation-driven `0x378ad/0x378cf`, `0x3d38a` ✓ (`f.c:5648/5651`, `f.c:27006` ✓) — none proximity-driven for the player ✓; the proximity tails belong to the per-type enemy handlers and the port matches them (docs/291 §1 ✓) |
| both candidate mechanisms | closed: invented flips (retracted, docs/291 §1 ✓), key latch (refuted, docs/292 §1 ✓) |
| both-sides screen basis at the ground line | **identical distribution ✓✓ (§1 ✓)** |
| a defect that *was* wrong at the left edge and is fixed | the walking-body entry guard (`LAB_0003284e`, `0x3285f-0x3287c ✓`): pre-fix the port dragged the player to the `195` clamp on keys the shipped ignores (`Y = {172:580, 195:91} ✗`), post-fix `Y = {172:912}` with 0 Y-change events ✓ and its left-edge screen row band moved `1.90 → 1.54` (docs/292 §5.1 ✓) |

**Conclusion:** item 6's symptom is not produced by the current port. The report is attributed to the pre-fix
left-edge walking/clamp behaviour fixed by docs/254 and docs/291 §3 (the player stood on the wrong ground line at
the bound, which is exactly the kind of thing that makes a limb appear "on the wrong side" ✓) — and the claim is
backed by a same-drive, event-anchored, both-sides screen measurement rather than by absence of evidence ✓

## 3. The two rig defects that had been blocking this basis, now fixed

1. **`scripts/rig/capture_original.py` died at a mode transition** (docs/287 §4 ✓). `frame_geom()` now derives
   geometry from the payload actually delivered when the header describes a different mode: if
   `n_px < pitch*h` and `n_px % pitch == 0`, then `h = n_px / pitch` and `w = pitch / bpp` ✓ — a 512000-byte
   payload at pitch 2560 is **200 rows × 640 columns at 4 bytes** ✓ (measured directly: adjacent 4-byte groups are
   identical in only 69 % of cases ✓ → nothing is duplicated ✓, so docs/218's "8 bytes per 320 column" reading is
   wrong ✓ and any shipped-side row mapping built on it was 2× off ✓). `/tmp/rig/rm5` captured 905 shipped frame
   samples with no geometry error ✓
2. **Witness density was hardcoded** (`rig_screen_init(g_rig_dir, 10)` ✓ `main.c:1585`): 10 changed frames per
   witness left `/tmp/rig/lk1` with all 67 `320x200` witnesses inside the card phase ✗✗ It is now
   `NIGHT_RIG_SCREEN_EVERY` (default 10 ✓ clamped 1..100 ✓); `rm5` with `=2` produced **1558** `320x200`
   witnesses ✓
3. **Naming correction (docs/287 §4 refined):** a witness file's numeric prefix is the harness **frame number**
   (`"%06llu_%dx%d.bin"` with `frame` ✓ `rig_sink.c:470` ✓), while the *selection* is every
   `g_witness_every`-th changed frame ✓ so bins map to `frames.bin` records through the record's `frame` field ✓
   (1558/1558 mapped ✓) — not through an ordinal counter ✓

## 4. Standing rules

* Compare **shares** across sides, never absolute ink counts or hues ✓
* Anchor a window on each side's own event (its own room-entry witness), then compare windows of comparable
  frame *counts* ✓
* When a dump-side header and its payload disagree, trust the payload arithmetic ✓ and test a layout claim
  structurally (adjacent-group identity ✓) before using it ✓
* A screen instrument must be able to produce witnesses in the phase under study — density and phase coverage are
  part of the instrument, not an afterthought ✓
