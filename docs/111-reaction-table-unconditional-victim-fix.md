# 111: the missing stagger is a transcription defect in `fn_01441e` -- the reaction table must be applied to every victim

## Root cause (shipped disassembly, `fn_01441e`, guest 0x00014c6c-0x00014cb4)

Ghidra's decompile of the shipped function shows **one flat condition whose else arm is unconditional**:

```c
if ((((local_24 == g_pPlayerEntity) && (*(int *)(local_24 + 0x17) == 0x97)) && (uVar6 != 0)) && (uVar6 < 10)) {
    local_24[0x31] = 0;
    local_24[0x17] = 0x14;                       /* +0x18..+0x1a cleared too */
} else {
    *(undefined4 *)(local_24 + 0x17) = *(undefined4 *)(&DAT_00051b60 + uVar6 * 4);
}
```

`uVar6` is `atype` = `byte[record + slot + 0x38]`. So **every** victim that is hit has its state replaced by
`g_tab_51b60[atype]`; the player-in-`0x97` case with `1 <= atype <= 9` is the *exception* (state `0x14`).

The port had the shape inverted:

```c
if (cur == g_player_ent && *(cur + 0x17) == 0x97) {      /* port, before */
    if (atype >= 1 && atype <= 9) { ... state = 0x14; }
    else { state = g_tab_51b60[atype]; }
}
```

i.e. the table lookup only ever ran for the player while in state `0x97`, which the port (and the shipped
sample data at 6.5 Hz) essentially never shows -- so **no entity on the port could ever enter a reaction
state**, while the shipped game spends 101 entity-samples in `0x0b..0x1e` on the same drive. That is the
machine signature behind "missing hurt-stagger state", "infinite punching" (the victim's animation chain is
never interrupted, so the attacker keeps looping the punch) and "player stuck when hit mid-kick".

Second defect in the same statement: the port's table was truncated to 0x20 entries. `DAT_00051b60` is
image-initialised for the whole 0x400 bytes (file offset `0x51b60-0x7600 = 0x4a560`), non-zero through index
0xfa -- `0x0b,0x0c,...,0x15,...,0x66..0x5b` for atype 1..0x1f and then a descending family
`0x59,0x57,0x55,...,0x2f,0x2b,...` for atype 0x20..0x3b. `atype` is a **byte**, so indexes up to 0xff are
reachable; the port had nothing beyond 0x20 (the docs/94/91 "image-init vs hand-written initializer" family,
this time as a truncation rather than a wrong value).

## Fix

`port/src/f/f.c`: full 0x100-entry `g_tab_51b60` transcribed verbatim from the image bytes, and the
statement rewritten to the shipped form (flat condition, else-arm applies to every victim, `+0x18..+0x1a`
cleared in the `0x14` case, index masked to a byte).

## Red before / green after (same drive `ep1_kill3`, same rig, no manual steps)

```
before  (/tmp/rig/ents-port2):  reaction-band samples (+0x17 in 0x0b..0x1e) = 21   {0x15 x33}
                               player (type idx 0) samples in 0x0b/0x0c       = 0
shipped (/tmp/rig/ents-orig):   reaction-band samples                        = 101  {0x0b x25, 0x0c x54, 0x15 ...}
                               player samples in 0x0b/0x0c                   = 47  (24 + 23)
after   (/tmp/rig/rx1):         reaction-band samples                        = 227  {0x0b x31, 0x0c x80, 0x15 x116}
                               player samples in 0x0b/0x0c                   = 49  (31 + 18)
```

The player half now matches the shipped side almost exactly (49 vs 47). The total is still above the shipped
count (227 vs 101), which is the remaining question for this cluster: the port's `atype` distribution or the
per-hit volume is larger (the `NIGHT_HITTRACE` census on this route: 91 fn_01441e hits, 56 with the player as
victim), and the fallen family (`0x5b..0x66`, reached via atype 0x15..0x1f) still has to be measured against
the shipped side now that the table tail is reachable.

## Follow-up measurements on the fixed build (`/tmp/rig/rx1`)

* **Fallen family `0x5b..0x66`: zero on both sides** over this drive (shipped 0, port 0). Those ids are the
  pick-up / throw family in `fn_01441e` (`+0x17 == 0x65 || 0x99` -> `0x66`, and the table maps
  atype `0x15..0x1f` onto `0x66..0x5b`), so `ep1_kill3` does not exercise them on either side: the "missing
  fallen state" item needs a drive that produces a thrown/picked-up entity (or a player death), not more
  punching. Recording this so the item is not judged on a drive that cannot show it.
* **EXP did not regress**: player `+0x2b` is non-zero in 46 samples on the fixed build (the award/drain path
  of docs/109 still runs), but it ends at 0 and the kill counter `DAT_000639bc` goes 0 -> 1 @231959 and back
  to 0 @237293 in this run. The kill/quota and EXP sub-items therefore need a *longer* drive with several
  kills inside the sampled window (the `ents-port2` run gave 0 -> 2 with `+0x2b` reaching 20); the counter
  returning to 0 is itself a finding to chase (room re-entry resets it, or the quota announcement path clears
  it -- `DAT_00053c80` / `DAT_0006336c` are the quota-announcement cells).
* Reaction totals are now above the shipped side (227 vs 101 band samples) while the player half matches
  (49 vs 47), so the remaining gap sits in non-player victims: the port resolves more hits
  (`NIGHT_HITTRACE`: 91 hits, 56 on the player) or sees larger `atype` values. Next measurement: the
  `atype`/`dmg` histogram from the HIT witness compared against the shipped records' `byte[record+slot+0x38]`
  read through the memory API (`--chain-cells`), which is now the only unexplained quantity in this cluster.
