# 137 — The reaction table is indexed `4 * hit_code`: the shipped player is never hit with code 11

All measurements from `/tmp/rig/g10` (`ep1_kill3`, 170 s, entity slots dumped on **both** sides thanks to
docs/136's `route.sh` default, port binary stamped by the route's own build guard).

## 1. The decode that makes the shipped side readable without a debugger

`DAT_00051b60` (DS 0x51b60, 0x100 bytes, byte-identical to `NIGHT.EXE` at file offset `DS-0x7600`) is a
**stride-4** table indexed by the hit/reaction code:

| code | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `tbl[4*code]` | 0x0b | 0x0c | 0x0d | 0x0e | 0x0f | 0x10 | 0x11 | 0x12 | 0x13 | 0x14 | **0x15** | 0x16 | 0x17 | 0x18 |

(`tbl[0x2c] = 0x15` is the only entry equal to 0x15.) So the **victim's state field is a record of the hit
code that produced it**. That turns the shipped entity dumps into a hit-code census with no instrumentation of
the guest -- which is what the rig has been missing for the "hurt-stagger / punching / stuck-when-hit" items.

## 2. What each side's PLAYER record actually holds

Identified by shape (`dword[+0x0d] == 0`, the constructor's type index), not by slot number:

| | shipped (`orig`) | port |
|---|---|---|
| live player records sampled | 488 | 1544 |
| `word[+0x29]` (hit points) census | **1000 in 488/488** | 1000 in 1503, **997 in 41** |
| `byte[+0x30]` (dying flag) census | **0 in 488/488** | 0 in 1503, **1 in 41** |
| state census | 0x01×192, 0x02×131, 0x33×38, 0x35×34, 0x00×32, 0x34×26, **0x0c×18, 0x0b×16**, 0x36×1 | 0x01×571, 0x02×283, **0x15×168**, 0x33×116, 0x00×109, **0x0b×94, 0x0c×69**, 0x35×62, 0x34×59, 0x05×13 |
| hit codes implied by §1 | **1 and 2 only** | 1, 2 **and 11** |

The shipped run *does* react (states 0x0b/0x0c = codes 1/2, 34 samples) but never takes a single point of
damage and never enters 0x15. The port enters **0x15 in 168 samples** -- hit code **11**, the first member of
the 0x0b..0x14 band that docs/136 §3 showed sets `+0x30 = 1` -- and 41 of those samples carry the flag at
997/1000 hp.

So docs/135's "spurious player death" is confirmed and now attributed one level further: **the port resolves
hit code 11 against the player where the shipped run only ever resolves codes 1 and 2.** The death dispatch,
the CONTINUE countdown decrement (`g_dat_53100` 3→2, docs/134 §6) and the extra CD track idx12 (docs/135 §2)
are all downstream of that resolution.

## 3. The guard that decides whether code 11 is lethal is a countdown, not a debt

The flag arm is guarded by `cmp word[victim+0x23], 0` (guest 0x00014cba-0x00014cc0). On the player, `+0x23`
and `+0x56` are a **fill pair with the invariant `+0x23 + +0x56 == 100`**, seeded from
`g_dat_53e9c` dword 0 (port f.c:5395, guest 0x377a1-3e) and decremented once per frame (f.c:13625, guest
0x0138ea-fd):

```
orig  t=72577 -> 74863 : +0x23   1 -> 100 while +0x56 100 -> 0   (99 units in 2286 ms = 43.3 units/s)
port  t=71031 -> 73047 : +0x23   1 ->  87 while +0x56 100 -> 13  (86 units in 2016 ms = 42.7 units/s)
```

Two consequences:

- **the game clock rates agree** (43.3 vs 42.7 units/s, 1.4% apart) on a field that advances purely per
  frame. The pacing flag (picture changes per second 4.54 orig vs 28.95 port) is therefore *not* a logic
  clock divergence.
- docs/118's reading of `+0x23`/`+0x56` as a damage-debt accumulator is superseded for the player: they are a
  100-step countdown, so `+0x23 == 0` -- the condition that makes codes 0x0b..0x14 instantly lethal -- is
  satisfied only at one end of that countdown. The flagged port samples are exactly that: `+0x23 = 0`,
  `+0x56 = 0`, state 0x15, hp 997.

## 4. The port has an entity type the shipped run never creates

Type census over every live entity record in the run (`dword[+0x0d]`):

| type | shipped samples (share) | port samples (share) |
|---|---|---|
| 0 (0x00, player) | 488 (40.9%) | 1544 (25.0%) |
| 31 (0x1f) | 670 (56.2%) | 3384 (54.9%) |
| **36 (0x24)** | **0** | **962 (15.6%), window 196.6 s .. 247.5 s** |
| 201 (0xc9) | 32 (2.7%) | 181 (2.9%) |
| 250 (0xfa) | 3 (0.25%) | 99 (1.6%) |

**Type 0x24 exists only in the port.** It is also the attacker recorded in docs/136 §4
(`atk(type=36 st=0x1d) -> vic(player) atype=11 dmg=8`), i.e. the entity whose hit resolves to the lethal
code. The port's known spawn sites all pass 0xc9 or 0xfa (f.c:14844-14863, 13976-13992, 24369-24373 --
matching the shipped-side census of those types), and `fn_0113e7`'s own type-band dispatch (f.c:14826-14833)
lists `[0x24,0x27]` as one of the families the *shipped* code knows, so type 0x24 is a legitimate game type
that the shipped run simply does not instantiate on this drive: it must arrive through the stage/object
loader path rather than a spawn site. That is the next question, and it is measurable: census the loader's
per-object type byte against the shipped `.stg`/`.chr` object records.

## 5. Count-vs-chain oracle: green on both sides (closes docs/128 open item 1)

Comparing `dword DS 0x53c68` (the list count) against the length of the `+0x13f` chain walked by the dumper:

```
orig  count census: 2:238 3:191 1:32 4:23 0:3 5:3 6:1
orig  chain census: 2:239 3:190 1:32 4:23 0:3 5:3 6:1     (5 of 491 samples disagree by exactly 1)
port  count census: 3:866 0:473 6:330 4:114 1:109 7:62 5:61 9:28
port  chain census: identical in all 2047 samples
```

The distributions match on both sides; the shipped side's five off-by-one samples are spawn/free events
landing between the DS read and the heap read inside one sample, not a corrupt list. So the port does **not**
corrupt the entity list -- docs/128's crash was a fault-landing difference (already guarded through
`sys_linear0`), not a bookkeeping divergence.

## 6. Next measurement

Whoever creates the type-0x24 entity is the head of the chain that ends in the player death. Plan: add a
loader witness (type index + object record offset + stage/room index) at the `.stg`/`.chr` object instantiation
path, run the same drive, and compare the per-room type list against the shipped side's entity census in the
same windows. If the port's type list per room contains 0x24 where the shipped census shows only 0x1f/0xc9,
the defect is in the object-type decoding (docs/94 family: guest base-relative fields read as values), not in
combat.
