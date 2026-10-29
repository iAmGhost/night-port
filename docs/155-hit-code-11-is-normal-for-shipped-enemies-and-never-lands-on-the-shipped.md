# docs/155 — hit code 11 is normal for shipped enemies and never lands on the shipped player; in the port it lands on the player

The longest both-sides run so far, `/tmp/rig/g15` (`scripts/rig/drives/ep1_kill3.txt`, 300 s, `RIG_HZ=10`,
`RIG_ENTS=12` on both halves: 1846 shipped samples, 2169 port samples, 12 entity slots each), separated by
whether the record is the player (hp field `+0x29` in 900..0x8000) or not. Reaction states are read out of the
victim's own state dword `+0x17`, because the reaction table is stride-4 in the hit code (docs/137).

| side | player states in `0x0b..0x18` | enemy states in `0x0b..0x18` | fallen band `0x5b..0x66` |
|---|---|---|---|
| shipped | `0x0c` ×71, `0x0b` ×64, **no `0x15`** | `0x0c` ×71, **`0x15` ×43** | **0 / 1846 samples** |
| port | `0x0b` ×92, `0x0c` ×82, **`0x15` ×67** | `0x0c` ×138, `0x15` ×38 | **0 / 2169 samples** |

## What this settles

1. **Reaction code 11 (state `0x15`) is a shipped behaviour, not a port invention.** The shipped side puts
   enemies into `0x15` 43 times on this drive. docs/137's "shipped has codes 1 and 2 only" was true of the
   *player* record only, and this run makes that distinction explicit.
2. **The divergence is about the target, not the existence of the code.** Shipped: 0 samples of `0x15` on the
   player in 1846 samples. Port: 67 samples of `0x15` on the player in 2169 samples. That is the discriminator
   docs/136 §7 asked for, now with counts on both sides: the port resolves reaction code 11 *against the player*
   in a situation where the shipped game resolves codes 1/2 against the player and reserves 11 for enemies.
3. **The fallen family stays unreachable on both sides under the longest observation yet**: zero samples of
   `0x5b..0x66` in 1846 shipped and 2169 port samples. Combined with docs/154's finding (the only fallen-state
   samples the rig ever saw were written by the drive itself) and docs/118's container census (one fallen-class
   record in episode 1, attacker state `0x65`, which is itself a fallen state), ledger item 6 stays open as a
   coverage gap with no differential defect to fix.

## Why the port's player gets code 11

The reaction code comes from the attacker's action record: `atype = byte[rec_e + slot + 0x38]` where
`rec_e` is selected by the **attacker's** state (docs/118). So one of two things must be true in the port, and
they are separable by measurement:

* an attacker is in a state whose record carries `11` at the slot being used, where shipped would have that
  attacker in a different state (the chain question — docs/110, and docs/152 §4b: the port spends 9.6% of
  samples in walk state `0x02` against shipped's 29%); or
* the slot index differs (`slot=0` in the port's `NIGHT_HITTRACE` reports against the same attacker
  type/state, docs/136 §4).

`NIGHT_HITTRACE` already prints `atk(type,st) vic(...) slot atype`; the missing piece is a shipped-side
reference for `byte[record + slot + 0x38]` for the same (attacker type, attacker state, slot), which needs the
`.act` container of the shipped side read through the memory API (`actdump` exists on the port side only).
That is the concrete next measurement for this item, and it is a rig capability gap rather than a code gap.

## Reproduce

```sh
OUT=/tmp/rig/g15 RIG_HZ=10 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300
# player-vs-enemy reaction census over /tmp/rig/g15/{orig,port}, using the +0x17 state dword
# and the +0x29 hp field to identify the player record (docs/125 layout)
NIGHT_HITTRACE=1 ./port/test/night --res-dir ./NIGHT --script scripts/rig/drives/ep1_kill3.txt \
    --warp-stage 1 --frames 6000 2>&1 | grep "HIT atk" | head
```

## 4. One suspect path audited and cleared

The victim-flag decision that gates whether a hit is treated as a "player-style reaction" looked like the
docs/111 defect class (a player-specific special case), because the port has one at `f.c:24781-24790`:

```c
if (cur == g_player_ent) {
    if (cur[+0x05] != 0) flag = byte[ro + 0x6c];
    else { cs = cur[+0x17]; flag = (cs >= 0x15 && cs <= 0x1e) ? byte[ro + 0x6c] : 0; }
} else flag = byte[ro + 0x6c];
```

The shipped body has the same shape (`0x14789-0x147c6`): `cmp victim, [DAT_53c7c]` -> `jne` read-the-byte;
for the player, `cmp [victim+5], 0` -> non-zero reads the byte; otherwise `cmp [victim+0x17], 0x15` /
`cmp [victim+0x17], 0x1e` and only that window reads the byte, with `mov [ebp-0x14], 0` on the else path. The
port is faithful, so this is **not** where the port's player acquires hit code 11. Notably the shipped
condition for the player is satisfiable only when the player's state is already `0x15..0x1e`, which is the same
window the divergence is about — worth remembering when the attacker-side question is answered.

Reproduce: `uv run --with capstone scripts/audit/disasm.py --entry 0x14789 --end 0x147d0`.
