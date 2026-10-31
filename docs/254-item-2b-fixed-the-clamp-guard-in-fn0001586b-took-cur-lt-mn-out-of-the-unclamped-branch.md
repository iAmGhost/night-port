# 254 — Item 2b: a one-instruction clamp-guard defect in fn_01586b, plus two more self-corrections

## 1. The defect (transcribed from shipped bytes)

`fn_01586b`'s tail, shipped 0x159d1-0x15a14:

```
0x159d1  mov  eax, [eax + 9]              ; cur = record[+9]
0x159d4  cmp  eax, [ebp - 0x10]           ; cur vs mx
0x159d7  jle  0x159e4                     ; NOT greater -> the clamping branch
0x159d9-0x159df   add [edx + 9], eax      ; cur > mx: add, NO clamp, jump to tail
0x159e4-0x159ea   add [edx + 9], eax      ; otherwise: add
0x159f0-0x159fc   if ([rec+9] < [ebp-0x14]) ...      ; jge 0x15a03
0x159f8-0x159fe       [rec+9] = [ebp-0x14]           ; clamp UP to mn
0x15a03-0x15a0c   else if ([rec+9] > [ebp-0x10]) ...
0x15a0e-0x15a14       [rec+9] = [ebp-0x10]           ; clamp DOWN to mx
```

**The outer test is a single comparison: the unclamped branch is taken only when `cur > mx`** ✓ A record that has
drifted *below* `mn` falls into the clamping branch and is pushed back up to `mn` ✓ — the shipped **pins** a record
to its lower bound ✓

The port's guard was `if (cur < mn || cur > mx)` ✓ which puts the `cur < mn` case into the *unclamped* branch and
lets the record keep drifting below its bound ✓ Fixed to `if (cur > mx)` ✓ (`port/src/f/f.c`, fn_01586b tail ✓)

That single extra conjunct is the mechanism docs/242 §3-4 measured and could not explain: "the shipped parks the
player on the room's lower bound and the port leaves it at 172" ✓

## 2. Before / after on the same 150 s canonical route (identical env, identical drive)

`PORT_ENV="SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_HITTRACE=1
NIGHT_RECBUILD=1" route.sh drives/ep1_kill3.txt 150` → `/tmp/rig/rb2` (before) and `/tmp/rig/rb3` (after):

| measurement | rb2 (before) | rb3 (after) |
|---|---|---|
| player `+0x09` samples at the lower bound 139 | 268 | **406** ✓ (+51 %) |
| player `+0x09` samples at the spawn line 172 | 416 | 398 |
| damage applications | 57 | 55 |
| hits landing on an enemy (`vic kind=1`) | 8 | **11** ✓ |
| enemy debt chain reached | 35 → 33 → 31 → **29** | 35 → 33 → 31 → 29 → 27 → **25** ✓ |
| `NIGHT_RECBUILD` kind-1 constructions | 6 | 6 |
| state-oracle rows (DIVERGENCE.md) | 35 DIFF, 24 PTR, 19 REPR, 14 DIFF-SIZE, 3 SAME | **34** DIFF, 24 PTR, 19 REPR, 14 DIFF-SIZE, 3 SAME ✓ (one DIFF resolved, nothing added ✓) |

Longer route (`... 300` ✓ `/tmp/rig/rb4`, wall-capped at 800 s ✓ so its window is partial ✓): 129 damage
applications ✓ enemy debt descends **35 → 13** ✓ 11 hits on one enemy ✓ A 460 s run (`/tmp/rig/rb5`, rc=0 ✓) landed
22 enemy hits but bottomed at debt 21 ✓ — the run-to-run variance docs/239 §4 recorded is still present ✓ so these
depth numbers are directional, not a pacing claim ✓

`make -C port test` → **ALL PASS** ✓ after the change ✓

Still not met for item 2b: no `+0x30 == 1` on a kind-1 record and no award in the port dumps (`exp_award.py`:
port `+0x32` values per type = `[0]` for types 0, 31, 36, 201, 250 ✓ shipped `type_idx=31 → +0x32 = 10` ✓), because
an enemy still needs 18 landed hits (debt 35, 2 per hit ✓ docs/245 §2 ✓) and the best run delivered 11 ✓

## 3. Retraction: "fn_03272f's two player settle conversions are modelled nowhere" (docs/239 §4 item 3)

They are modelled, twice, inside `fn_03272f` ✓ `port/src/f/f.c` (the state-0x1d and state-0x16 arms):

```c
if (PL16(0x23) == 0 && PL8(0x30) == 1) PL8(0x30) = 2;
```

against shipped 0x32c48-0x32c6e and 0x32d26-0x32d4c ✓ both verified byte-for-byte this turn ✓ (each shipped site:
`mov word [player+0x50], 0`; `movsx eax, word [player+0x23]`; `test/jne`; `mov al, [player+0x30]`; `cmp 1`;
`je -> mov byte [player+0x30], 2`; `call 0x348e2` ✓) ✓ The earlier search missed them because the port writes the
field through the `PL8()/PL16()` accessor macros ✓ **Rule for the static audit: a displacement grep over the port
misses every store written through an accessor macro** ✓ (same family as docs/252 §2's `0x3b5c0` vs `0x0003b5c0`
miss ✓ and docs/229's factored-helper miss ✓) ✓

## 4. Dump-side player settle-flag census (reproducible, both sides, the basis for §3's consequence)

Records selected by content (kind byte 0, `+0x29 == 1000` — the player ✓ the `g_player_ent` cell's first dword is a
pointer in the port dump ✓ so cell-name-based reads of the player are unreliable ✓):

```
shipped /tmp/rig/ents-orig/orig : +0x30 = {0: 698, 2: 312}        (never 1 ✓)
shipped /tmp/rig/g37/orig        : +0x30 = {0: 2499}
port    /tmp/rig/rb1/port        : +0x30 = {0: 876, 1: 23}         (1, not 2 ✓)
port    /tmp/rig/rb2/port        : +0x30 = {0: 755, 2: 5}
shipped player +0x17: 1 ×351, 2 ×231, 0 ×120, 51 ×79, 52 ×58, 11 ×58, 53 ×57
port    player +0x17: 1 ×281, 0 ×241, 21 ×174, 2 ×126, 51 ×42, 5 ×18, 12 ×9   (rb1)
```

The shipped's player does hold `+0x30 = 2` (312 samples ✓ the converted form ✓) and never sits at 1 ✓; the port's
player was caught at 1 (23 samples ✓ rb1 ✓) — the un-converted form ✓ That is consistent with the port simply
reaching a different arm of `fn_03272f` (its conversions are guarded by `PL32(0x11)` thresholds and by the attack
keys `'*'`/`'6'` ✓), not with the stores being absent ✓ docs/239 §4 item 3 is withdrawn ✓

## 5. Where item 2b stands

Faithful and exonerated: the award routine and its queue posts ✓ the per-type table ✓ the damage model ✓ the walk
step table ✓ the reaction/effect record kinds ✓ the fn_03272f conversions ✓ the fn_01586b clamp *body* ✓ (only its
guard was wrong ✓ now fixed ✓).

Remaining for item 2b's green gate: **land 18 hits on one enemy in a rig route** ✓ so a kind-1 record settles
(`+0x30 = 1` ✓), posts to the `fn_0003b5c0` list ✓ and `player[+0x52]` rises by the per-type value (10 for type 31 ✓
shipped census ✓) ✓ The clamp fix moved the player onto the bound the enemies' lines sit near ✓ (139 for 406 samples
✓ up from 268 ✓) and raised enemy hits 8 → 11 ✓ with debt descending to 25 (150 s ✓) and 13 (300 s ✓) ✓ Next step:
a route/hold-policy sweep that keeps the player on the bound *while attacking* — the misses are X-box misses
(docs/245: 114 GATE5 rejections, 3-14 px short ✓) on top of the now-improved Y band ✓
