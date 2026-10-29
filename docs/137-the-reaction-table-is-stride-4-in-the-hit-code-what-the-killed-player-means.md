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

On **each** side the count cell agrees with that side's own chain: the two censuses are identical per side
(port: 2047/2047; orig: 486/491, the five off-by-one samples being spawn/free events landing between the DS
read and the heap read inside one sample). So the port does **not** corrupt the entity list -- docs/128's
crash was a fault-landing difference (already guarded through `sys_linear0`), not a bookkeeping divergence.

The **levels** however do not agree, and that is a divergence in its own right:

```
orig  live-entity count distribution: 2:238 3:191 1:32 4:23 0:3 5:3 6:1      (max 6, mode 2)
port  live-entity count distribution: 3:866 0:473 6:330 4:114 1:109 7:62 5:61 9:28   (max 9, 24% at 5+)
```

In the 190-250 s window specifically, the shipped side averages 4.8 live entities per sample and the port
7.6 -- the same window in which the port carries the extra type-0x24 population (§4). The port is running a
**larger entity population**, which is also what makes more hits land.

A first attempt to compare spawn *rates* (slot-level appearance events) is reported here as unusable rather
than as a result: the two dumpers sample at 520 ms and 139 ms, so short-lived entities register as an
appearance+disappearance pair on the port and are invisible on the shipped side, and the raw ratio (port
type-31 54.2 vs orig 6.6 per 100 s) is dominated by that cadence gap. Rate comparison needs equal cadence on
both sides (`--hz`), which is the right way to run it.

## 6. The spawn path itself is faithful -- checked, not assumed

The room-record opcode-1 handler `fn_03e20f` and its spawn helper were checked line by line against
NIGHT.EXE (capstone, `scripts/audit/disasm.py --entry 0x3e42b`):

```
0x3e42b  mov  eax, 0x147 ; call 0x41bdc          ; block 0x147 (the port passes 0x14f, the widened node)
0x3e444  mov  eax, 1     ; push eax            ; kind = 1
0x3e44d  push dword ptr [eax + 1]              ; type index  -> record +0x0d
0x3e453  mov  ecx, dword ptr [eax + 0xd]       ; Y           -> record +0x09
0x3e456  xor  ebx, ebx                         ; arg_ebx = 0
0x3e458  mov  edx, dword ptr [ebp - 0x10]      ; X           -> record +0x01
0x3e45e  call 0x10efc
```

which is exactly the port's `e20f_spawn` (f.c:26321-26334). The population cap the handler compares against,
`g_tab_528b4[(stage-1)*10 + room[+0x40]]`, is also faithful: the port's table (f.c:25431, 180 entries, 10
columns x 18 rows, stride 0x28) matches `NIGHT.EXE` at `DS-0x7600` entry for entry -- caps of 9 with a few
5s, and 1000 (effectively no cap) from row 10 on.

(A false lead worth recording, because it was produced by the audit tooling rather than by the game: a
quick regex over the initializer text reported 43 mismatches for this table. The regex had matched numbers
in the surrounding comment, not array entries. Re-checked against the declared table: 0 mismatches. Lesson:
a byte comparison must parse the initializer, not scrape digits near it.)

So the extra population is not produced by wrong spawn arithmetic or a wrong cap: it comes from **more
opcode-1 records being executed**, or from gates/coordinates that let records fire where the shipped run
keeps them dormant (`rec[0] = 0` after a spawn is a one-shot arming, so a gate that passes earlier spawns
more). That is where the type-0x24 population must come from.

## 7. Next measurement

Whoever creates the type-0x24 entity is the head of the chain that ends in the player death. Plan: add a
loader witness (type index + object record offset + stage/room index) at the `.stg`/`.chr` object instantiation
path, run the same drive, and compare the per-room type list against the shipped side's entity census in the
same windows. If the port's type list per room contains 0x24 where the shipped census shows only 0x1f/0xc9,
the defect is in the object-type decoding (docs/94 family: guest base-relative fields read as values), not in
combat.
