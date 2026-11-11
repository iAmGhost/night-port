# 449 — the port stops offering saves, because the game never had them

Asked whether a save was worth building out of the half-working slot feature, the answer came back the other
way: **"just don't implement it, rid of save feature. Since original game hasn't one, I want to pocket port not
having save/load."** That is the fidelity argument settling a feature question, and it is the stronger one.

## 1. What the port had, and why it was worse than nothing

Seven nonvolatile slots (ids 10-16 → `night_0..6.sav`), a CRC-guarded blob, a `RESTORE=` mask, and three pad
combos. It did not work as a save:

- **The restore half-worked.** Mask 1 (HP bars) restored; stage position and enemies did not (docs/374 was
  written about exactly this — the apply path was fine, the captured fields simply were not the state).
- **Masks 2/4 were a hazard, not a feature.** The stage selectors `DAT_63374..63384` are pointers into the
  arena; forcing them mid-stage makes the game dereference out of range, which is why they were opt-in.
- **The combos reached the shipped game.** `Select` and `Start` *are* Esc and Enter (docs/333's unavoidable
  class on 11 buttons), so every save press also poked the pause menu — which is the very code path whose
  flicker exposure docs/445 later found.
- A save of a game with no save concept is a feature the port invents, and this project's standard is that we
  do not invent: the shipped `src/f` names no save file (only `exptab.dat`), and the shipped game ships none.

## 2. What went, and what deliberately stayed

Gone: `of_save_write/read/apply`, the blob struct and its CRC, the slot resolution through `d_ino`,
`RESTORE=` and the mask, the `L1+Select` / `L1+Start` / `L1+A` combos, `--save / --save-at / --load`, and the
seven slot bindings (**26 → 19 instance slots, 28 → 21 data slots**; ids 10-19 now stand unbound, which is
allowed — README:644 reserves that window whatever we bind).

Kept, explicitly, because none of it is a save: **`interact.json` Core Options** (Music Volume, SFX Volume,
Screen Fit), `night.ini` and its peripheral mirror registers, the diagnostic log slot 17, and the `L1+Up`
(trace) / `L1+B` (panel fit) combos. `of_save.c` keeps its name out of habit-cost — it is 450 lines of
settings, and its header now says so.

## 3. Two of my own tools caught me, which is what they are for

- `mk_dist.py` **refused to ship**: *"night.ini is missing RESTORE -- an entry was lost to a comment or an
  edit. Repair the ini block above; do not paper over it with a code default."* The guard I added so a key
  could not silently vanish fired at exactly the moment I was trying to remove one deliberately.
- My first patch script used `__attribute\(\(` and matched nothing — I had dropped the second `__` of
  `__attribute__`. The second cut deleted one line out of a multi-line `/* ... */` block and broke the
  compile. Both are the same failure as a wrong regex in a report: quiet, local, and only visible in the
  output. Save-region deletions are now range-to-marker.

## 4. Leftovers and verification

A card that already has `night_N.sav` files keeps them; they are simply unbound and inert, and `deploy.sh`
deletes nothing in `Saves/`. docs/334 is marked superseded. Built `elf=0 dist=0 pc=0`, reporter fixtures
`3/3`, repo fixture suite `3091 [PASS]`; card byte-verification follows the deploy.
