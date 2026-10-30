# 247 — Item 2b: the shipped's enemy slots never change identity; the port's swap type 31 <-> type 36 a hundred times in 300 s

The docs/246 §5 open question was "why does a port enemy leave play after 3-7 hits". Slot-identity tracking over
the same dumps answers the first half of it.

## 1. Method

For each dump position (record slot ✓ docs/186 §1) track the pair `(kind, type_idx)` between consecutive samples
and count the moments it changes, with the enemy gate of docs/246 §1 (`kind == 1`,
`state & 0x7fffffff < 0x100`, `type_idx < 0x1000`).

```
shipped (/tmp/rig/ents-orig/orig, 974 samples):
   enemy type census:      type 31 x 1065                      <- only one enemy type is ever seen in a slot
   slot identity changes:  none                                 (0)

port (/tmp/rig/h4/port, 2162 samples):
   enemy type census:      type 31 x 2898, type 36 x 991
   slot identity changes:  (36 -> 31) x 51, (31 -> 36) x 50     <- 101 replacements in one run
```

So the shipped's twelve captured entity slots hold **type-31 enemies for the whole run** ✓ and one of them absorbs
19 hits down to `+0x23 = 0` ✓ (docs/246 §2). The port's slots alternate between **two enemy types** ✓ 101 times ✓,
and every replacement resets the pending-HP term to that type's own value (31 -> 35 ✓, 36 -> 46-48 ✓ docs/245 §2) ✓
which is exactly why the port's damage (79-94 decrement events ✓, comparable to the shipped's 40-68 ✓) never
accumulates on one body ✓

## 2. The port's own spawn witness, and what it does NOT explain

`NIGHT_SPAWNTRACE` (armed at `f.c:27013`/`f.c:27029`) prints 7 spawn events in the same 300 s run:

```
t=31  rec=00384f44 sub=1 gate=0 cgate=0 rx=368 ry=193 room11=0 cnt=2 cap=9 anchor=350
t=31  rec=00374714 sub=1 gate=0 cgate=0 rx=324 ry=150 room11=0 cnt=1 cap=9 anchor=350
t=31  rec=003745f4 sub=1 gate=0 cgate=0 rx=368 ry=193 ...      (3 x ry=193, 2 x ry=150)
t=36  rec=00350954 sub=0 gate=0 cgate=0 rx=134 ry=183 ...      (1 x ry=183, 1 x ry=172)
```

* the type-31 spawn Y values are **193 and 150** ✓ the two values the shipped's own enemies were caught at in
  docs/238 -- so the spawn-time placement is not the divergence ✓ (docs/238 §4's candidate is closed here ✓);
* 7 spawn events cannot produce 101 slot replacements ✓ -> the replacement is **not** the opcode-spawn path ✓ it is
  either (a) the witness being capped/path-limited (the trace is gated at `f.c:27013` and prints only a subset ✓
  open), or (b) the entity-list free/reuse path handing a slot to a queued enemy of the other type ✓

## 3. Next measurement (specified)

Instrument the *lifetime*, not the spawn: for each kind-1 record, log (i) the moment its `kind` byte is cleared or
its slot is taken by `fn_010efc` (which also clears `+0x30` ✓ docs/239 §1), (ii) the value of `+0x23` at that
moment, and (iii) the queued type it is replaced with. Compare the shipped's numbers indirectly: the shipped shows
zero replacements in the same slots ✓ so any port lifetime shorter than "the whole run" for a type-31 enemy is the
defect ✓ The candidate paths are the entity-list free/reuse (`fn_0113e7`, `fn_010efc` ✓) and the room-cycle return
values of `fn_03a829` (1 = room clear, 2 = CONTINUE, 4 = quota met ✓ docs/223 §3 ✓) that decide whether the room is
re-entered -- a room re-entry would respawn both types into the same slots ✓ which is what the churn pattern
(51 x 36->31, 50 x 31->36 ✓, near-symmetric ✓) looks like ✓✓

## 4. Room re-entry is ruled out (measured on the same dumps)

`DAT_00063354` (the current room node) and `DAT_00063370` (the stage), read per sample from the dumps:

```
shipped: 0x63354 distinct = 1, changes = 0            0x63370 (stage) not in this dump's cell list
port:    0x63354 distinct = 2 (1 627 884 x 1707, 0 x 455), changes = 1     stage: 1 x 2162, changes = 0
```

So the port does **not** re-enter rooms ~50 times ✓ the stage never advances ✓ -- the 101 slot replacements all
happen **inside one room** ✓ That kills docs/247 §3's "room re-entry would respawn both types into the same
slots" hypothesis ✓ and leaves the in-room lifetime paths: an entity-list cap/reuse (the spawn witness already
carries `cnt=1..7` against `cap=9` ✓), a despawn/timeout path, or the reaction-record free ✓ none of which writes
`+0x30 = 1` ✓ (docs/246 §5: zero settle samples in two port runs ✓) -- i.e. the port **removes enemies without
ever settling them**, which is the same fact that starves the award ✓

