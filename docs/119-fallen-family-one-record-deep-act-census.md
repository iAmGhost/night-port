# 119 — The fallen family is one record deep in episode 1: reaction table decoded exactly, hit-record layout decoded from the live heap

Date: 2026-10-29. Rig primitive introduced here: `t heapdump <bytes>` (whole guest heap to
`<rig-dump>/heap.bin`), which replaces the in-process structure walk that crashed (docs/118 §3).

## 1. Reaction table: the fallen range is atype 0x15..0x1f, not 22..32

`g_tab_51b60` (DS `0x51b60`, 256 entries, transcribed verbatim from image bytes in docs/111) maps

| atype | victim state |
|---|---|
| 0x01..0x0a | 0x0b..0x14 (the stagger band measured on both sides) |
| 0x0b..0x14 | 0x15..0x1e (reeling / strong class) |
| **0x15..0x1f** | **0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f, 0x5e, 0x5d, 0x5b** |
| 0x3d, 0x3e, 0x42 | 0x5c, 0x5d, 0x5e |
| 0x20.. | 0x59, 0x57, 0x55, … (descending, the throw/dust family) |

docs/117 said "types 22..32"; the correct interval is **21..31 (0x15..0x1f)**. That one-off mattered: 21 is
the *largest atype actually present in episode 1's loaded data*.

## 2. Hit-record layout, decoded offline from the port's heap

`fn_01441e` resolves (guest 0x00014439-68, `f.c:24266-24274`):

```
lvl2 = *(heap + *(heap + e[+0x123] + 0x04))     ; state -> record table
rec  = *(heap + lvl2 + 4 * e[+0x17])            ; the striker's state selects the record
nf   = *(heap + rec)                            ; frame count
fs   = e[+0x11] % nf
fi   = u16[*(heap + rec + 0x6d) + 2*fs]         ; sprite frame number
```

and per **hit slot** `i` (`f.c:24341-24417, 24402, 24519`):

```
u16 pair   rec + 0x08 + 4i
dword dmg  rec + 0x20 + 4i
byte atype rec + 0x38 + i          ; dword array spans 0x20..0x37 => exactly SIX slots
word       rec + 0x4a + 2i
byte       rec + 0x56 + i
byte event rec + 0x63 + i
byte flag  rec + 0x6c
```

The adjacency proves the slot count: `0x38 - 0x20 = 0x18 = 6` dwords, so a record has hit slots `i = 0..5`
only. Reading `i >= 6` walks into other fields — which is exactly what produced the bogus "atype 55/62"
readings in docs/118 §9 and the earlier off-by-range conclusion.

Captured with `t heapdump` on drive `scripts/rig/drives/ep1_heap.txt` (`/tmp/rig/hp1/heap.bin`, 4.19 MB at
t=114 s of the kill drive), plus per-type container offsets from `t actdump` (`/tmp/rig/ad2`):

- **12** entity types have an action container loaded in episode 1: 0, 1, 11, 31, 32, 36, 37, 41, 42, 46, 201,
  250 (types 2/3/4/12/21 have an empty `g_dat_53818` slot).
- **701** action records decoded, **281** non-zero hit slots.
- atype alphabet present: **{1, 2, 3, 6, 7, 8, 11, 12, 14, 16, 19, 20, 21}** — with damage pairs that make
  sense (atype 11 with dmg 15/20/30, atype 1 with dmg 11, atype 2 with dmg 10).
- Records whose atype maps into the fallen family: **exactly one**.

```
entity type 0 (gun.act, container heap offset 0x71d88), state 0x65, hit-slot 0:
    atype 0x15 -> victim state 0x66, dmg 0, pair (0,2), frame count 9
```

## 3. What that means for the "missing fallen state" item

The only path into the fallen family in episode 1 goes through a record that is itself keyed to **state 0x65**
— i.e. an entity that is already downed keeps the one who hits it down (`0x65 -> 0x66`). No ordinary attack
in episode 1 declares a knockdown type. That matches the measurements that opened this item: the entity
oracle sees **zero** samples in `0x5b..0x66` on **both** sides for the kill drive (docs/117, docs/118 §3), and
12 Hz sampling on the shipped side shows deaths going through the dead-flag path with no fallen prelude.

So the correct statement is the opposite of the playtest note for this episode: there is **nothing to fix in
the port's fallen path for episode 1** — the reaction table, the record resolution and the hit-slot bounds are
now byte- and layout-faithful, and the shipped side does not enter the family in this scene either. Any fallen
animation seen by a human must come from a later episode's `.act` data (different containers, so a different
atype census) or from the throw/dust family (table entries 0x20+), which the drive language can reach once the
episode is in scope.

## 4. Attempt to force the single fallen record (negative, recorded)

`scripts/rig/drives/ep1_fallen.txt` writes, 40 times over ~10 s, `ent 1 +0x123 = 0x71d88` (point the entity's
action container at `gun.act`), `ent 1 +0x17 = 0x65`, `ent 1 +0x11 = 0` — 120 injections, all confirmed by the
marker log. Result: hits still land with atype {1, 2}; **no atype 0x15**, no victim in 0x66.

Why the force fails, and what would work: `fn_01441e` builds the victim-overlap box from the **sprite** arrays
(`spr_e+0x1c / +0x20`, indexed by the frame number `fi` from `rec+0x6d`), not from the record's `+0x08` pairs;
the state is also re-written by the entity's own AI between injections. Forcing the record therefore needs an
entity that is genuinely in a downed state with a matching sprite frame — which in episode 1 means knocking a
type-0 gun entity down first (the chicken-and-egg the shipped data encodes), not a scripted poke. Recorded as
a limit of injection: it can hold a field, but it cannot manufacture a state the entity's own logic rejects.

## 5. Rig verdict

`fix-state` sub-item *fallen reachability*: **no divergence to fix in episode 1**; closed on a positive
equivalence argument (identical loaded-container census, verbatim reaction table, layout proven from field
adjacency) plus identical zero-sample behaviour on both sides. The follow-up that would extend it is a
per-episode act census (`heapdump` at a settled point of each of the nine episodes) — the same primitive now
works for any episode, and it also feeds the fuzzing coverage signal.

## 2b. The pick-up / ground path at guest 0x00014de1+ (read directly, this turn)

`fn_01441e` ends with a block the act census cannot see, because it does not depend on a record's atype at
all (`f.c:24561-24577`, annotated 0x00014de1-0x00014e77):

    if (e[+0x17] == 0x65 || e[+0x17] == 0x99) {          /* the ATTACKER is in a grab / lower-attack state */
        if (flag == 0) {                                 /* victim not "flagged" */
            e[+0x17]  = 0x66;                            /* attacker enters the ground family */
            e[+0x50]  = u16[rec_e + 2*slot + 0x4a];
            e[+0x11]  = 0;
            fn_01545e(cur, -1);
            cur[+0x17] = 0x1f;                           /* victim is lifted / held */
            cur[+0x4a] = e - heap;   e[+0x4a] = cur - heap;  /* mutual link: the pair is now joined */

Together with the table (0x15 -> 0x66, **0x16 -> 0x65**, 0x17 -> 0x64 ... 0x1f -> 0x5b) this is the real
chain, and it matches the user's description: a hit whose record declares atype 0x16 puts the **victim into
0x65**, and the next contact while it sits in 0x65 (or 0x99) routes through the pick-up block, moving the
attacker to 0x66 and the victim to 0x1f. The "its hitbox changes, it is vulnerable to lower attacks"
observation is a consequence of that state change, not a separate mechanism. The "several hits in a short
period" part is visible one block earlier in the same function: damage only reaches `+0x29` (hit points)
while the debt word `+0x23` is zero, otherwise it piles into `+0x23` / `+0x56`, and
`+0x23 == 0 && atype in 0x0b..0x14` is what sets the lethal flag `+0x30 = 1` -- repeated hits inside one
reaction window are what let a strong-class record finish the sequence.

## 6. Injection cannot force the chain; the item rests on static equivalence

`scripts/rig/drives/ep1_grab.txt` holds the player in the attacker states 0x65 / 0x99 / 0x66 / 0x5b / 0x64
(six attempts each, `play 0x17 <state>` immediately before each attack): all hits still land with atype
{1, 2}. `ep1_fallen.txt` does the same on an entity slot (120 injections, marker-confirmed). In both, the
entity's own controller rewrites `+0x17` every frame, so a forced state never survives to the hit test. That
is a stated limit of the injection primitive: it can hold a field between frames, it cannot hold a state that
the entity's controller overwrites each frame.

What can be asserted is static equivalence: the pick-up block is transcribed with the guest's own address
annotations (0x00014de1-0x00014e77), the reaction table is verbatim, and the atype offset is proven from field
adjacency rather than guessed. A follow-up that would close it dynamically is a controller-level drive --
earning state 0x65 through the game's own move chain (which needs the chain/slot advance decoded in docs/110)
-- and the per-episode act census this turn's `heapdump` primitive now makes cheap.
