# 107: fix-state worklog -- where the combat-state investigation stands

Status: in progress. Nothing fixed yet in this cluster. This page exists so the next session does not
re-derive the model; every number below was produced by the rig or by the shipped disassembly.

## What the rig measures (blind-gate evidence, docs/105 §8)

`state_hist.py` over `ep1_kill3` (route `/tmp/rig/route-k3`, t=[150,310] s, cells `g_dat_53c70` entity
list head, `g_dat_53c74` tail, `g_player_ent`):

```
class            orig     port
neutral          1045     3353
hurt/stagger      124       33        (orig ids 0x0b x54, 0x0c x66; port id 0x15 x33)
fallen 0x5b-0x66    0        0
orig ids: 0x1x623 0x2x422 0x33x72 0xcx66 0xbx54 0x34x53 0x35x49 0x29x7 0x15x4 0x36x3
port ids: 0x1x1964 0x2x1385 0x33x398 0x35x255 0x34x226 0x15x33 0x29x7 0x4x2 0x3x2
```

`NIGHT_HITTRACE=1` on `ep1_combat` (`/tmp/rig/hit-e1.log`) -- the port's own combat witness:

```
38 HIT          100 PASS2 empty-box    86 PASS2 no-live-slot    150 PASS3 miss    1 PREBOX
HIT lines: 18x slot=0 atype=2 dmg=2 | 18x slot=0 atype=1 dmg=1 | 1x slot=2 atype=2 dmg=6 | 1x slot=1 atype=1 dmg=3
HIT examples:  atk(type=0 st=33) vic(type=31 st=01) slot=0 atype=2 dmg=2 hp 0->0 debt 33
               atk(type=31 st=29) vic(type=0 PLAYER st=02) slot=0 atype=1 dmg=1 hp 1000->1000 debt 99
PASS3 victims: 144x type=31 st=01/02 fl=0   6x type=31 st=15 fl=1
```

On `ep1_kill3` the same run produced **0 HIT lines** and the run ended early
(`game main unwound (quit / fatal)`) -- the aggressive kill drive gets the player killed/quit; that drive
is not a usable combat witness yet.

## Model facts established from the shipped disassembly

* Reaction table `g_tab_51b60` (DS 0x51b60, 0x20 dwords, port f.c:24125): attack type -> state. Index 1
  -> 0x0b, 2 -> 0x0c, ..., 0x0b -> 0x15; indices 0x15..0x1f -> 0x66..0x5b. **Ghidra says the table has
  exactly ONE consumer**: `DATA` xref from `0x00014cab` inside `fn_01441e` -- and the port implements it
  (`f.c:24428`), so no reaction-table call site is missing.
* Guest `0x00014c4c-0x00014cb4` (the non-flagged reaction) matches the port line by line:
  `victim[+0x50] = word(rec_e + 2*slot + 0x4a)`; `victim[+0x11] = 0`;
  `if (victim == DAT_00053c7c && victim[+0x17] == 0x97)` then `atype in 1..9 -> victim[+0x31]=0,
  victim[+0x17]=0x14` else `victim[+0x17] = g_tab_51b60[atype]`. So the port's transcription there is
  faithful, and the table can only fire for the **player in state 0x97**.
* Consequence: the shipped game's victims sitting in `0x0b`/`0x0c` for ~1 s at a time (120 samples at
  4 Hz) are **not** explained by `g_tab_51b60` -- those ids are entered by other code (sprite/act state
  machines), and `0x0b`/`0x0c` may be ordinary animation states for `type=31` entities rather than hurt
  states. The earlier inference ("0x0b..0x1e is the hurt band, therefore the port misses the hurt
  reaction") is NOT established; it was based on the table comment, which describes the *table's* value
  range only.
* Damage path (`f.c:24316-24351`, matches the guest): HP is two words -- `+0x29` (hit points) and `+0x23`
  (carry/debt) -- plus a dword mirror at `+0x56`. If `+0x23 == 0` damage goes to `+0x29`; otherwise to
  `+0x23` (and `+0x56`). The witness shows exactly that: the player has `+0x29 = 1000, +0x23 = 99`
  (so damage lands on `+0x23`: 99 -> 98 ...), the `type=31` entity has `+0x29 = 0, +0x23 = 33` and its
  damage lands on `+0x23` (33 -> 31 -> ...). So damage *is* applied in the port; the interesting question
  is which word the shipped game actually uses per entity type.
* `PASS3` shows the reaction `flag` (`victim record byte +0x6c`) reads **0** for 144/150 examined victims
  in the port, and the flagged branch is the one that would knock/stagger them. Candidate defect: the
  victim's record pointer chain (`hb + *(hb + *(e+0x123) + 4) ...`, f.c:24168) resolving to a record whose
  `+0x6c` is 0 -- i.e. the docs/94/docs/95 offset-vs-pointer family, not the reaction table.

## Open questions for the next session (ordered)

1. What does the shipped game's `victim[+0x6c]` actually read during combat? It is not a DS cell (it is
   inside a heap record). Route: dump the victim records the port already dumps (head/tail at 0x53c70 /
   0x53c74, 0x160 B each) and read offset +0x6c on **both** sides -- the shipped side's record blocks are
   already captured (`state_NNNN.heap.bin`, PTR_BYTES 0x160). If shipped +0x6c is 1 where the port reads
   0, the record-pointer chain is the bug; if it is also 0, the flag is not the divergence.
2. Which entity field holds the *shipped* HP that reaches 0 on death (kill counter `0x639bc` increments),
   and does the port's corresponding field decrease at the same rate? Compare the +0x23/+0x29/+0x56 words
   of the tail/head records across a kill on both sides (`rec_offsets.py --cell g_dat_53c74` finds the
   monotonic-down offsets by shape, without trusting port names).
3. Which states do the shipped *victims* enter immediately after a kill (the removal samples)? Use the
   entity-count cell `0x53c68` transitions to find removal samples and read the victim record's `+0x17`
   in the samples around it.
4. `ep1_kill3` unwinds the port early: make the kill drive survive (heal/avoid death) so the HITTRACE
   census exists for a drive that actually kills.

## Answer to question 1 (measured, route-e1 t=[60,230] s, 468 shipped / 2050 port record samples)

`+0x6c` of the entity-list head (`g_dat_53c70`) and tail (`g_dat_53c74`) records reads **0 on both sides**
in every sample -- the shipped game's records have the same zero flag byte as the port's. So the
`flag`-vs-record-pointer hypothesis is dead at the head/tail records: the record chain resolves to real
records (their `+0x17` values are alive), and the flag byte is genuinely 0 in the shipped game too.
State distribution in the same view: shipped head/tail mostly `+0x17 = 1` and `2`, with `0x0c` and `4`
appearing (3 and 2 samples); the port mostly `1`/`2` but also `51 (0x33)` 24 times. The `0x33` presence
on the port and its near-absence at the shipped head/tail is still the sharpest record-level difference;
questions 2 and 3 (which HP word reaches 0 at a shipped kill, and the victim's state around a removal)
remain open.
