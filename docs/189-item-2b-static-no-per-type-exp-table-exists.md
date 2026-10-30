# 189 — Item 2b at the instruction level: there is no per-enemy-type EXP table to copy

docs/188 said the 53/54 in `rec[+0x32]` are state ids.  This document closes the static half of item 2b: what the
shipped EXP award actually adds, and which of those sites the port already models.

## 1. Both shipped award sites, read at the instruction level

`scripts/audit/field_writers.py --disp 0x52` (new, walks all 523 modelled bodies with capstone, so indexed and
immediate forms are all found) reports ten accesses to `[reg+0x52]`.  The two awards:

**Site A — `fn_01441e` at `0x15161-0x15186`:**

```
0x15161 mov eax, [ebp-4]                 ; the record just resolved
0x15164 cmp eax, [DS 0x53c7c]            ; is it the player?
0x1516a jne 0x15196
0x1516c mov eax, [ebp-4]
0x1516f cmp dword [eax + 0xbb], 0        ; player's own score delta
0x15176 je  0x15196
0x15178 mov eax, [DS 0x53c7c]
0x15180 mov edx, [ebp-4]
0x15180 mov edx, [edx + 0xbb]
0x15186 add [eax + 0x52], edx            ; accumulator += player[+0xbb]
```

`player[+0xbb]` is the hp-delta score built earlier in the same function (`delta = hp0 - hp` when the target is the
player), so site A is *self*-awarding from damage dealt.  Modelled: `f.c:25295`.

**Site B — `fn_015295` at `0x15448-0x15453`:**

```
0x15448 mov eax, [DS 0x53c7c]            ; player
0x1544d mov edx, [ebp-8]                 ; the victim
0x15450 mov edx, [edx + 0x32]            ; the victim's saved-state queue HEAD
0x15453 add [eax + 0x52], edx
```

So the EXP an enemy "awards" is **whatever state id sits in that enemy's saved-state queue head** — `+0x32` is
`fn_031f10`'s push slot and `fn_031f5f` shifts it (docs/187 §2, docs/188 §1).  There is no per-enemy-type EXP column
involved in the award.

The other eight `+0x52` touches are all modelled: init 0 (`fn_010efc` 0x1102d → `f.c:1307`), the display drain `DEC`
(`fn_0137c1` 0x138e7 → `f.c:13991`), the two clears in `fn_01441e` (0x14eaa, 0x14f77 → `f.c:25205`, `f.c:25223`), the
chain counter `INC` (`fn_03272f` 0x33b45-4d, guarded by `cmp eax, 0x70` on the table word at `[tbl+0x388]`, → `f.c`
`if (target == 0x70) { int32_t *pc = (int32_t *)(pair + 0x52); *pc = *pc + 1; }`), its clear (0x33c60 → `f.c:27358`),
and the display fold in `fn_038f3d` (0x39000/0x3900b → `f.c:25600-25601`, which is also where `player[+0x2b] +=
player[+0x52]` happens).

The six `fn_031f10` push value setups are also all matched: `fn_01441e`'s three sites each load the literal
(`0x149bd mov edx, 5`, `0x14b00 mov edx, 5`, `0x14d88 mov edx, 5` → `f.c:25086/25108/25175` `fn_031f10(cur, 5)`), and
`fn_03272f`'s three push `0x71` or a `word [tbl + 0x388 + i*2]` chain target (`f.c:27352/27398/27411`).

## 2. What that means for the playtest item

"Each enemy gains different EXP per enemy type" is, in the shipped game, a *side effect of which state id the victim's
queue head held at death* (measured shipped population 54 x325 / 53 x2 / 5 x246 / 0 x138 on `g33`, docs/186 §2), plus
damage-dealt self-award through `+0xbb`.  So the fix is not "add a table of EXP values": it is to make the port's
victims carry the same queue-head values at the moment of death — the same divergence docs/188 §4 isolated from the
other side (the port computes chain targets 0x34/0x35 but the cell never holds them; its `DS 0x62d5c` chain level is
0 everywhere).

The measurement that closes item 2b quantitatively is therefore an **event-anchored pair at the kill**: on the port
side, `NIGHT_HITTRACE` names the victim, and the victim's `+0x32` can be printed at that step; on the shipped side the
record census gives the distribution at 4 Hz.  Comparing the two numbers at the same event (kill) is the basis to fix
against.

## 3. Tool caveat found while doing this

`field_writers.py` reported `add byte ptr [ecx + 0x2b], bl` at `0x230d7` inside `fn_0221b8`'s body range
(`0x221b8..0x2319b`).  Disassembling that region shows padding runs (`add dword ptr [eax], eax`, `add byte ptr
[eax], al`) before and after it — i.e. the "instruction" is data inside a body range, exactly the class of false
positive docs/181 §6 recorded for the 15.6 KB resource-name block.  A census over body ranges needs a data filter
(`00 00` runs / entropy) before its hits can be quoted as writers; `+0x2b` hits from `fn_0221b8` are not writers.
