# docs/156 — the fallen family's entry points, found in the shipped code, audited faithful, and blocked by a precondition neither side ever reaches

Ledger item 6 ("Player/Enemy fallen state is almost missing") was the last item still open as a coverage gap.
This pass found every place the shipped program can put an entity into the fallen band `0x5b..0x66`, checked
each against the port, and explains why neither side produces a sample.

## 1. Runtime census of the action data (new witness `NIGHT_ACTCENSUS=1`)

`act_census_report()` in `f.c` walks every action container the run **loaded** through the same chain the guest
uses at `0x00014743-0x00014786`:

```
block  = g_dat_53818[type]              /* DS 0x53818, per-type action-table heap offsets */
lvl2   = *(hb + block + 4)
record = *(hb + lvl2 + 4*state)
code   = hb[record + slot + 0x38]       -> victim state = g_tab_51b60[code]
```

Two things had to be fixed before this could be trusted as a coverage statement.

*The mapping.* The authoritative reaction table is the shipped image itself: `NIGHT.EXE` at file offset
`0x51b60 - 0x7600 = 0x4a560`, 256 dwords. Codes `0x15..0x1f` map to `0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60,
0x5f, 0x5e, 0x5d, 0x5b`, and codes `0x3d, 0x3e, 0x42` map to `0x5c, 0x5d, 0x5e`. **No code maps to `0x99`,
`0x70` or `0x71`.** Code `0x16` does map to `0x65`. (docs/118's `atype 0x15..0x1f -> 0x66..0x5b` is confirmed
byte-for-byte; my first parse of the port's brace initializer was wrong and produced a shifted mapping, so the
census reads the compiled array, not a re-parse.)

*The container set.* The first version walked only the non-null entries of DS `0x53818`, and the loader at
`f.c:5192` (`0x37da9`) fills those entries lazily while `f.c:5357` (`0x373d3`) clears them, so a shutdown
census of the array sees a subset. A recorder (`g_act_load[]`, filled at the load site) now remembers every
container the run loaded. The difference is large and it changed the conclusion:

| drive | episode | blocks | records | reaction-band | FALLEN, slots 0-5 | out-of-band slots 6-7 |
|---|---|---|---|---|---|---|
| `heap_all.txt` (idle) | 1..9 | **3** | 603 | 317 | **1** | 12 |
| `ep1_kill3.txt` | 1 | 21 | 3159 | 1881 | **1** | 64 |
| `ep1_kill3.txt` | 2 | 23 | 4091 | 2109 | **71** | 34 |
| `ep1_kill3.txt` | 3 | 17 | 2965 | 1571 | **15** | 30 |
| `ep1_kill3.txt` | 4 | 11 | 2037 | 891 | 1 | 18 |
| `ep1_kill3.txt` | 5 | 9 | 1552 | 605 | 2 | 13 |
| `ep1_kill3.txt` | 6 | 29 | 4314 | 2367 | 5 | 72 |
| `ep1_kill3.txt` | 7 | 35 | 5138 | 2460 | 1 | 62 |
| `ep1_kill3.txt` | 8 | 5 | 1000 | 455 | 1 | 16 |
| `ep1_kill3.txt` | 9 | 27 | 4810 | 2759 | **45** | 42 |

Slots 6-7 are reported apart because docs/118 proved the hit-slot field holds 0..5 from dword-array adjacency,
so reads at slots 6-7 run into the following record and are not evidence.

**This supersedes docs/118's "exactly one fallen-class record in episode 1".** Fallen-producing reaction codes
are common in the shipped action data: episode 2 has 71 in-band slots, episode 9 has 45, e.g.

```
fallen type=2   state=55  slot=3 code=0x19 -> state 62      (episode 2)
fallen type=2   state=16b slot=3 code=0x19 -> state 62      (episode 2)
fallen type=12  state=1c8 slot=2 code=0x15 -> state 66      (episode 2)
fallen type=201 state=1e2 slot=1 code=0x1a -> state 61      (episode 3)
fallen type=72  state=142 slot=4 code=0x1e -> state 5d      (episode 9)
fallen type=0   state=65  slot=0 code=0x15 -> state 66      (every episode)
```

## 2. Static scan of all 530 shipped function bodies: two stores of a fallen state

Scanning every body in `functions.json` for `mov dword ptr [reg + 0x17], imm` with `imm` in `0x5b..0x66`:

```
immediate fallen stores: 0x12926 fn_012616 0x66     0x14e07 fn_01441e 0x66
```

No store writes `0x5b..0x65` directly, and no reaction-table code maps to `0x65` or `0x99` — the two states
docs/118 identified as the pick-up precondition. The same scan counted 36 register-sourced stores into
`+0x17` across 27 functions (state changes fed from tables/registers rather than immediates); the ones that
matter for this band are the two immediates above.

Both stores are transcribed in the port, and both keep the shipped polarity:

* `fn_01441e` (`f.c:25028-25035`, guest `0x00014de1-0x00014e3b`): attacker in `0x65` or `0x99` and `flag == 0`
  -> attacker becomes `0x66`, victim becomes `0x1f`, mutual `+0x4a` links — faithful to docs/118.
* `fn_012616` (`f.c:25993-26001`): the interesting one, and the polarity is easy to get backwards.
  Shipped `0x12905-0x12926`:

  ```
  0x12908: cmp  [e+0x17], 0x70 ; jne skip
  0x12911: mov  eax, [e+0x4a]          ; the pair link
  0x12914: mov  al,  [eax+0x30]        ; partner's dying flag
  0x1291c: cmp  eax, 1
  0x1291f: jne  0x12923                ; partner NOT dying -> fall through to the store
           jmp  0x1295e                ; partner IS dying    -> skip it
  0x12926: mov  [e+0x17], 0x66         ; FALLEN
  ```

  Port: `if (e[0x17] == 0x70) { pair = hb + e[+0x4a]; if (pair[0x30] != 1) { e[+0x17] = 0x66; ... } }` — same
  polarity (`!= 1`, not `== 1`), and the follow-on stores at `0x12930`/`0x12956` (`+0x50 = 200`, pair `+0x17 =
  0x1f`, pair `+0x11 = 0`) are present.

## 3. What is actually missing is the attacker state, not the fallen code

The data side is fine: reaction codes that put a victim into `0x5b..0x66` exist in the loaded containers of
episodes 1, 2, 3, 5, 6 and 9, attached to attacker states like `0x55`, `0x7e`, `0x142`, `0x16b`, `0x1c7`,
`0x1e2`. So the fallen family is reachable **if an attacker is ever in one of those states while it lands a
hit**. Measured runtime state population (docs/155's run `/tmp/rig/g15`, 1846 shipped / 2169 port samples):

```
ORIG states >= 0x30: {0x33:72, 0x34:61, 0x35:59, 0x36:3}
PORT states >= 0x30: {0x33:117, 0x34:66, 0x35:70}
ORIG 0x65:0 0x66:0 0x70:0 0x71:0        PORT 0x65:0 0x66:0 0x70:0 0x71:0
```

Neither side ever holds any state above `0x36` except the reaction states `0x0b/0x0c/0x15`. No entity on
either side is ever in `0x55`, `0x7e`, `0x142`, `0x16b`, `0x1c7` or `0x1e2`, so none of those fallen-producing
records is ever selected. The two direct stores of `0x66` are equally gated on states neither side reaches
(`0x65`/`0x99` in `fn_01441e`, `0x70` in `fn_012616`), and no reaction-table code maps to `0x70` or `0x71`.

So item 6 is not "the fallen code is missing from the port": both sides agree on zero because both sides fail
to reach the same higher attacker states. The open question is which mechanism raises an entity from the
`0x01/0x02/0x33..0x35` band into `0x55`-and-above, and it is the same question docs/110 (the `.act` chain
matcher), docs/152 (the port steps through the mover 0.155 times per dispatch where shipped effectively steps
once) and docs/155 (who receives hit code 11) all point at. The 36 register-sourced stores into `+0x17`
(27 functions, `fn_03272f` carrying 4 of them) are where those state values come from, and they are the next
thing to trace.

## 4. Reproduce

```sh
make -C port app
for s in 1 2 3 4 5 6 7 8 9; do
  NIGHT_ACTCENSUS=1 ./port/test/night --res-dir ./NIGHT --script scripts/rig/drives/heap_all.txt \
      --warp-stage $s --frames 2600 2>&1 | grep ACTCENSUS
done
uv run --with capstone scripts/audit/disasm.py --entry 0x128f8 --end 0x12930
# the 530-body immediate-store scan and the +0x17 state census: scripts/audit/disasm.py primitives
# reused over scripts/audit/functions.json with capstone (see this file's §2)
OUT=/tmp/rig/g15 RIG_HZ=10 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300
```
