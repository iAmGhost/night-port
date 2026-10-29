# 138 — The room-record spawn path is faithful; the port's extra enemy family comes from the command list

docs/137 §4 left one fact standing: the port carries an entity type the shipped run never creates
(**type 0x24 = `enb-1.spr`**, 962 samples in 170 s, window 196.6–247.5 s), and that type is the attacker whose
hit resolves code 11 and flags the player dying (docs/136 §4). This turn checked the whole path that could
produce it, in the shipped disassembly, and the path is *correct* -- which moves the suspicion one level
further upstream.

## 1. New witness: `NIGHT_SPAWNTRACE=1`

`e20f_spawn` (f.c:26321, the helper behind the room-record opcode-1 handler `fn_03e20f`) now prints every
spawn with the record's own gate fields. On `ep1_kill3`, 7200 frames:

```
SPAWN t=31 stamp=0 rec=00374594 sub=1 gate=0 cgate=0 rx=324 ry=150 room11=0 cnt=1 cap=9 anchor=350
SPAWN t=31 stamp=0 rec=003745f4 sub=1 gate=0 cgate=0 rx=368 ry=193 room11=0 cnt=2 cap=9 anchor=350
SPAWN t=36 stamp=0 rec=003315e4 sub=1 gate=0 cgate=0 rx=540 ry=172 room11=0 cnt=3 cap=9 anchor=520
SPAWN t=36 stamp=0 rec=003507d4 sub=0 gate=0 cgate=0 rx=134 ry=183 room11=0 cnt=4 cap=9 anchor=154
SPAWN t=31 stamp=0 rec=00340ae8 sub=1 gate=0 cgate=0 rx=602 ry=193 room11=0 cnt=5 cap=9 anchor=582
SPAWN t=31 stamp=0 rec=00374714 ...      (same records, second room visit)
SPAWN t=31 stamp=0 rec=00384f44 ...
```

All seven went through the **population-cap arm** (`gate == 0`, `room[0x11] == 0`, `cnt < cap` with `cap = 9`
from `g_tab_528b4`), so the cap is not what limits this drive. Two of them are the enb-1 (`t=36`) spawns.

## 2. The spawn site and the gate arms are byte-faithful (verified, not assumed)

`scripts/audit/disasm.py --entry 0x3e42b` (capstone over `NIGHT.EXE`):

```
0x3e42b  mov eax, 0x147 ; call 0x41bdc        ; block 0x147 (port passes 0x14f, the widened node)
0x3e444  mov eax, 1     ; push eax            ; kind = 1
0x3e44d  push dword ptr [eax + 1]            ; type index   -> record +0x0d
0x3e453  mov  ecx, dword ptr [eax + 0xd]     ; Y            -> record +0x09
0x3e456  xor  ebx, ebx                       ; arg_ebx = 0
0x3e458  mov  edx, dword ptr [ebp - 0x10]    ; X            -> record +0x01
0x3e45e  call 0x10efc
```

and the arm the port took (`sub == 0`, `cgate != 1`, guest 0x0003e3a9-0x0003e426):

```
0x3e3b2  cmp camy,        [rec+0x0d] ; jg   fail     -> requires camy <= ry
0x3e3c6  cmp camy+0xc8,   [rec+0x0d] ; jge  pass     -> requires ry <= camy+0xc8
0x3e3da  cmp room[0x00],  [rec+0x09] ; jg   fail     -> requires room0 <= rx
0x3e3ea  cmp room[0x08],  [rec+0x09] ; jge  pass     -> requires rx <= room8
0x3e3f7  cmp [rec+0x15], 1           ; jne  0x3e408  ; cgate != 1 arm
0x3e414  cmp camx-0x32,   [rec+0x09] ; jl   0x3e426 -> fail
0x3e419  anchor = camx - 0x1e
```

The port's transcription (f.c:26365-26377) has the same four window tests with the same inclusivity and the
same `camx - 0x32 < rx` refusal. Note the two enb-1 records sit **exactly on their gate boundaries**
(`rx = 540 = camx + 0x172`, `rx = 134 = camx - 0x32`), so this is not a strict-vs-inclusive off-by-one: under
the shipped comparisons those records *should* spawn too.

The population cap table was re-checked against the image as well: the port's `g_tab_528b4` (f.c:25431, 180
entries, 10 columns x 18 rows, stride 0x28, per-stage caps of 9 with a few 5s and 1000 = no cap from row 10
on) matches `NIGHT.EXE` at `DS-0x7600` entry for entry.

(A tooling false lead, recorded because it produced a fake finding: a quick regex meant to compare that table
reported 43 mismatches. It had scraped digits out of the surrounding comment instead of parsing the
initializer. Parsed properly: 0 mismatches. A byte comparison has to parse the initializer.)

## 3. So the divergence is in the command list, not the interpreter

`fn_03e20f` has exactly **one** caller: `fn_03ed6f` (f.c:26328), the command-list pump, which walks the room's
script-command list (`+0x51` prev / `+0x55` next, cached in `DAT_00063e18` / `DAT_00063e1c`) and calls the
interpreter per record, freeing a record only when the interpreter consumed it (`rec[0] = 0`). The commands
themselves are appended by `fn_0003e0a0`.

That gives the next audit target with a name: **who appends the opcode-1 commands that declare type 0x24**, and
whether the shipped run appends them at all on this drive. Concretely:

1. disassemble `fn_0003e0a0`'s append sites and the room-script reader that feeds them, and compare the
   port's record-field decoding (the `+0x01` type, `+0x05` sub, `+0x11` gate, `+0x15` cgate fields) against the
   shipped stores -- docs/94's offset-vs-value family is exactly this shape;
2. make the command list itself an oracle: it is a heap list reachable from `DAT_00063e18`, so the rig can
   walk it on the port side (a `cmdlist` witness like the entity dumper) and, on the shipped side, read
   `DAT_00063e18` / `DAT_00063e1c` from the DS dumps plus the record blocks from `state_*.heap.bin`. If the
   shipped list never contains a type-0x24 command while the port's does, the defect is in the append path;
   if it does and the shipped gate never consumes it, the defect is in a gate the port passes too early.

The population numbers from docs/137 §5 are consistent with an append-side defect: the port's live-entity count
distribution (max 9, 24% of samples at 5 or more) sits above the shipped one (max 6, mode 2).

## 4. The command list is on both sides, but its record counter is not

The commands are not invented by the port: `stage%d.stg` is read as a dword record count into `DAT_00063e14`
followed by that many 0x59-byte records, each appended by `fn_03e0a0` (f.c:5070-5074, guest 0x37c8d-0x37cdc).
So the enb-1 (`t=36`) opcode-1 commands exist in the shipped asset, and the shipped run does carry a command
list. Reading the three list globals straight out of the shipped DS dumps and the port's cell records:

```
                      DAT_63e14 (record count)              DAT_63e18 (list head) nonzero
shipped (491 samples) 67 x456, 88 x33, 0 x2                 489 / 491
port     (2047 samples) 67 x752, 0 x473, 61 x427, 66 x231,  1574 / 2047
                       88 x109, 62 x27
```

The shipped side keeps the count at the value the `.stg` declared (67, or 88 in the window where a different
stage's script is loaded) and essentially always has a list head. The port's count wanders down to 61/62 and
is zero in 23% of samples. The zeros are plausibly the phases the shipped sampling never covered (its trace
starts at 47 s, the port's at 0 s), but **61/62 vs a constant 67 is not a phase effect**: the port's
`fn_03e14c` decrements `g_dat_63e14` on every unlink (`g_dat_63e14--;  /* DEC [0x63e14] (always) */`,
f.c:1915) and nothing brings it back up until the next stage load, while the shipped run's value does not
drift. Since a record is only *consumed* (unlinked and freed) when the interpreter clears its opcode byte, a
counter that drifts down on the port while the list keeps being retried is the signature of unlinking records
the shipped game would have left pending -- which is exactly the shape needed to explain both the extra
enb-1 spawns and the larger live-entity population in docs/137 §5.

Open item, stated as a check rather than a conclusion: locate every guest write to `DAT_00063e14` (the
`DEC` inside `fn_0003e14c`, the load in the `.stg` reader, and any increment) and compare each against the
port's placement; then make `DAT_63e14 / 63e18 / 63e1c / 63e20` a first-class oracle row (they are dumped on
both sides) so a list-length divergence is reported automatically instead of discovered through an entity
census.

## 5. Every guest access to `DAT_00063e14` is placed faithfully -- and the counter gap has a cadence explanation

`scripts/audit/mem_access.json` lists exactly four accesses to `DAT_00063e14` in the shipped binary, and the
port has all four in the same functions:

| guest | access | port |
|---|---|---|
| `fn_037b03` 0x37c88 | `MOV EAX, 0x63e14` (address passed to the `.stg` reader) | f.c:5070 |
| `fn_037b03` 0x37cae | `CMP EAX, [0x63e14]` (record-loop bound) | f.c:5072 |
| `fn_03e14c` 0x3e169 | `CMP [0x63e14], 1` | f.c:1897 |
| `fn_03e14c` 0x3e1f8 | `DEC [0x63e14]` | f.c:1915 |

There is no shipped increment the port is missing, so §4's drift is the port executing `fn_03e14c`'s unlink
more often -- **or** it is the sampling cadence. `fn_037b03` re-reads the room script on (re)load and resets
the count to 67, so a low-frequency sampler mostly sees 67 while a 10 Hz sampler catches the intermediate
61/62/66 values between a unlink and the next reload. The shipped trace ran at 4 Hz, the port's at 10 Hz.

Discriminating run started: `/tmp/rig/g11`, same drive, `RIG_HZ=10` and `RIG_ENTS=12` on the **shipped** side
too, so `DAT_63e14` and the live-entity census can be compared sample-for-sample instead of normalised. If the
shipped trace at 10 Hz also shows 61/62/66 excursions, §4's counter gap is a rig-cadence artifact and the
enb-1 question stays where docs/137 §6 left it; if the shipped trace stays pinned at 67, the port is
unlinking commands the shipped game leaves pending, and `fn_03e14c`'s call sites become the fix target.
