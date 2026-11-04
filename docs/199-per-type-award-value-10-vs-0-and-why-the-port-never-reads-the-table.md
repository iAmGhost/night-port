# 199 — Item 2b detected and localized: the shipped victim carries award 10 where the port's carries 0, and the port never reaches the write

Tool added: `scripts/rig/exp_award.py` (censuses the award field of records that reach the death site, walking
only the 0x160 entity-record blocks both dumpers store, with the docs/186 §1 admissibility gate).

## 1. The per-type award value, measured on both sides

Run: `/tmp/rig/ents-orig` + `/tmp/rig/ents-port` (drive `ep1_kill3.txt`, 10 Hz, window t=[150 000, 300 000] ms;
shipped 974 states, port 1 292 samples).

```
== shipped: admissible record-samples=13267  kinds={0:8192, 1:1460, 7:1284, 2:952, 56:428, 34:428}
   dying records (+0x30==1): 130
     kind=1 type_idx=31  award(+0x32)=10  state=0x15 hp=0  x31
     kind=2 type_idx=201 award(+0x32)=0   state=0x1  hp=0  x81
     kind=2 type_idx=250 award(+0x32)=0   state=0x3  hp=0  x8 ...
== port:    admissible record-samples=10068  kinds={0:9433, 1:562, 2:73}
   dying records (+0x30==1): 111
     kind=1 type_idx=31  award(+0x32)=0   state=0x15 hp=0  x38
     kind=2 type_idx=201 award(+0x32)=0   state=0x1  hp=0  x54 ...
```

**The same kind (1), the same type index (31), the same dying state (0x15), the same hp (0) — and the shipped victim's
award field is 10 while the port's is 0.**  `fn_015295`'s award is `player[+0x52] += victim[+0x32]`, so the shipped
game adds 10 and the port adds 0; docs/198 §1 shows the shipped player record receiving 7 then 8 and its display
field ramping 0 -> 0xa -> 0x14 while the port's stays 0 in every phase.

## 2. Where 10 comes from: the per-type table row, at the moment of the killing hit

Shipped `fn_01441e`, 0x148ea-0x14913 (the damage stage) and 0x15032-0x15070 (the settle block):

```
0x148ea movsx eax, word [cur+0x23]          ; the debt word
0x148ee test eax, eax ; jne 0x1491c         ; debt non-zero -> consume debt instead
0x148f8 sub  word [cur+0x29], ax            ; hp -= damage
0x148ff movsx edx, word [cur+0x29] ; jg 0x14917
0x1490a mov  word [cur+0x29], 0
0x14913 mov  byte [cur+0x30], 1             ; THE KILL: settle byte set in the same call
...
0x15003 cmp  byte [cur+0x30], 1             ; settle block, same call
0x15032 movsx eax, word [cur+0x29]          ; hp again
0x15060 imul eax, dword [eax+0xd], 0x88     ; eax = record's TYPE INDEX * 0x88   (3-operand IMUL: dst = src*imm)
0x15067 mov  edx, [eax + 0x3ea8]            ; edx = DS 0x53ea8 + type*0x88  == type-table row + 0x0c
0x15070 mov  [cur+0x32], edx                ; award value = the per-type table dword
```

(the hp-non-zero branch at 0x1503c-0x15054 instead pushes the literal 5 into the saved-state queue; docs/186 §6.)

The table value is measured out of the shipped DS dumps: row 31 of `DS 0x53e9c` (stride 0x88) reads
`(0x23, 0x2, 0, 0xa)` = **(35, 2, 0, 10)** in 972 of 974 states — the 4th dword is exactly the 10 the victim carries.
The port models the same table as `g_dat_53e9c[0x88 * 0x104]` with `g_dat_53ea8 = g_dat_53e9c + 0x0c`
(`f.c:8119/8121`), loads it in `fn_037b03` (`f.c:5251-5263`, from `night%d.chr`), and reads the row in the same way
(`f.c:25279-25283`).  So the *field* and the *table* are modelled; the value never arrives.

## 3. Why it never arrives, from in-process witnesses on the current build

`make app`, `NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, drive `ep1_kill3.txt`, 19 200
frames (~320 s), plus two new witnesses (`NIGHT_TYPETABLE`, `NIGHT_SET30`):

```
settlecheck (at f.c:25262, shipped 0x15003):   60 samples, every one with +0x30 = 0   (kind 1 and the player)
typetable  (the hp==0 table write, f.c:25279):  0 samples
set30 census (every +0x30 store in the port):
     75  f.c:24786  fn_013c4d 0x141c7-ca   -> always kind=2
      2  f.c:25183  fn_01441e 0x14cd2-     -> kind=0 (player)
      2  f.c:25088  fn_01441e 0x1499f      -> kind=0 (player)
      0  f.c:25069  fn_01441e 0x14913  (the kill store: debt==0 && hp<=0)   on a kind-1 record
```

So on the current build the port's damage stage — the block that shipped uses to turn a hit into a kill
(`debt == 0 -> hp -= dmg -> hp <= 0 -> +0x30 = 1`) — is reached for the player twice and **never for a kind-1
record**.  Because the settle byte is what the award block tests, the table read is never executed
(`typetable` = 0), the award is never made (`exp award path: adds=0 added=0 commits=0` in the same run), the kill
counter at `0x12a33` never increments (0 in all 2 248 samples), and no EXP number is ever drawn (docs/196).

The two sides therefore diverge one stage *before* the EXP question: the shipped game resolves damage on the victim,
the port does not.  docs/195 §1a (352 candidate pairs rejected by the vertical-overlap gate, all with the shipped
constant allowance `dl = 7`) is upstream of that, and docs/161 §5 (camera population) is upstream of that.

## 4. Open sub-question kept honest

The stored `ents-*` pair (docs/108) is the only pair in which the *port* side ever reached a counted kill (2 kills,
t=184 260 / 211 749).  Re-running the same drive and seed on the current build gives 0 counted kills in 2 248
samples (docs/198 §2), so the §1 port column describes the build that made kills and still produced award 0 — which
is the stronger form of the finding, but a *kill-count regression* between docs/108's build and today's is a separate
item to chase.
