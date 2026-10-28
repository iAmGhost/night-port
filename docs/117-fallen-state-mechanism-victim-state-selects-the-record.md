# 117. How a knockout (fallen) state is actually entered: the victim's state selects the action record

Date: 2026-10-29. Prompted by the user's clarification: *"fallen = knockout into ground by hitting multiple
times in a short period; in that state the entity is vulnerable to lower attacks (e.g. the player's first
kick), i.e. its hitbox changes."*

## 1. The reaction table, decoded tail

`g_tab_51b60` (DS 0x51b60, 0x100 entries, image-initialised -- docs/111 transcribed it in full):

```
index : 10  11  12  13 ... 21 | 22  23  24  25  26  27  28  29  30  31  32 | 33  34  35
state : 13  14  15  16 ... 1e | 66  65  64  63  62  61  60  5f  5e  5d  5b | 59  57  55
```

So the fallen family is reached by reaction **types 22..32**, and type 12 is what puts a victim into state
**0x15** (the reeling state docs/115 saw as `21` decimal in the census).

## 2. The mechanism that produces type 22+

`fn_01441e` picks the hit record **through the victim's current state** (port `f.c:24266`, guest
0x00014439-59):

```c
rec_e = *(hb + *(hb + *(e + 0x123)) + 4 * *(e + 0x17));   /* chain head, then indexed by state */
atype = hb[rec_e + slot + 0x38];
state = g_tab_51b60[atype & 0xff];                        /* docs/111: unconditional else-arm */
```

That is the machine-readable form of "hitting multiple times in a short period": a first hit puts the victim
into a reaction state; a **second hit while the victim is still in that state** resolves through *that
state's* action record, whose declared type is larger -- atype 22..32 -- and lands the victim in 0x66/0x65/
…/0x5b, the fallen family. The 0x65 -> 0x66 transition at guest 0x00014de4-07 and the `0x1f` at 0x00014e3b
are the fallen *continuation*, not the entry.

The hitbox claim follows from the same lookup: the box arrays are indexed by the sprite frame number that the
record supplies (`hb + *(u16*)(hb + rec_e + 0x6d) + 2*slot`, port `f.c:24272-24274`, docs/94 §frame-index), so a
fallen entity's boxes are whatever the fallen state's record declares -- vulnerability to low attacks is data,
and the port already resolves it the shipped way.

## 3. Measurement on episode 1 drives (why the rig has not shown fallen states)

`NIGHT_HITTRACE=1` census, port side, `--res-dir ./NIGHT`, `SDL_AUDIODRIVER=dummy`:

| drive | hits | atype alphabet | victims in state 0x15 at hit time | fallen samples |
|---|---|---|---|---|
| `ep1_kill3` (12 000 frames) | 41 | {1, 2} | 0 | 0 |
| `ep1_combo` (new: 220 ms attack bursts, 46 bursts) | 24 | {1, 2, 11} | 0 | 0 |

Episode 1's ep1 attacks as driven here only ever declare types 1/2/11; the second hit does not land while the
victim is in **0x15**, which is the precondition for a 22+ type. Both sides show zero fallen samples
(docs/115), so there is **no divergence claim** -- the item is a *coverage* gap, and the witness that closes it
is the HITTRACE pair (victim state, atype) rather than a state census.

## 4. Next lever (queued in fuzz-coverage)

Fuzz objective for this item, stated so it is machine-checkable: maximise the census of
`(victim state at hit, atype)` pairs, targeting `victim state == 0x15` at hit time. When that cell becomes
non-empty, the fallen-family state census becomes comparable against the shipped side and the item gets a real
red/green signature. Practical candidates: tighter bursts than 220 ms, chaining a *different* attack into a
reeling victim, and later episodes (heavier enemy rows), which the nine-episode fuzz sweep will cover.
