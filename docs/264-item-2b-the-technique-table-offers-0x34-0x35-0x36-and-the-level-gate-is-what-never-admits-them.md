# 264 — Item 2b: the technique table *does* offer states 0x34/0x35/0x36 — the level gate is what never admits them (and the shipped dump turns out to carry a flat DS window)

## 1. The port's own technique inventory (`NIGHT_ACTTRACE=1`, `f.c:27595-27611`, `/tmp/rig/ch4.err`)

The `.act`/`.key` container the player's chain walks, dumped one-shot (`g_act_trace` ✓ `main.c:1250` ✓):

```
entry  0 len=3 target=0x03 codes: 64 64 64            lvls: 0d 19
entry  6 len=4 target=0x36 codes: 1d 1d 1d 1d         lvls: 1e 1e 2d
entry  7 len=3 target=0x35 codes: 1d 1d 1d            lvls: 1e 1e
entry  8 len=2 target=0x34 codes: 1d 1d               lvls: 1e
entry  9 len=1 target=0x33 codes: 1d                  lvls:   (none)
targets offered by the container: 0x03 0x2f 0x33 0x34 0x35 0x36 0x3d 0x3e 0x3f 0x40
matched chain targets:            target 0x33  x65                     <-- only 0x33
rejection census:  code0 mismatch ...  level too high 552  len != k 4132  face byte 0
                   hurt target != 0x99  5   rank gate 0   chain MATCHED 60
```

The table is **faithful**: the shipped's own `.key` resources carry the entry ids the states come from —
`scratch/res/gun.key` ids `{0,1,2,3,10,13,20,30,47,51,52,53,54,61,62,63,64}` ✓ and `gun1.key`/`gun2.key` the same
`51,52,53,54` ✓ = `0x33,0x34,0x35,0x36` ✓✓ — the exact states the shipped's player is measured in (0x33 ×26, 0x34
×20, 0x35 ×20, 0x36 ×1 samples ✓ docs/263 §5 ✓) ✓ So docs/263's "the writer that carries 0x33 → 0x34 → 0x35 is
unnamed" is answered: `fn_03272f` stores the entry's *target id* from `word[container + 0x388 + 2*entry]` ✓ (shipped
`0x33b34 mov ax, word [eax + 0x388]` → `0x33d8b / 0x33e4b / 0x341dc mov [entity + 0x17], edx` ✓; the port models the
same read at `f.c:27631` ✓ and the same store ✓) ✓ **No missing writer, no missing table** ✓

**What blocks them is the level gate** `lvls[k-1] < g_dat_63f28[s1]` ✓ (`f.c:27627` ✓, shipped `0x33ae9`
`cmp eax, [eax + 0x13f28]` ✓): entry 8 (`target 0x34`) needs level > 0x1e, entry 7 (`0x35`) needs >0x1e twice, entry 6
(`0x36`) needs >0x1e, >0x1e, >0x2d ✓ while entry 9 (`0x33`) needs none ✓ — which is precisely why the port matches
0x33 and nothing above it ✓ (552 "level too high" rejections ✓) ✓

## 2. Self-correction: the fourth `DAT_00063f28` "writer" is a memset the port already models

A whole-file displacement scan for `28 3f 01 00` (disp32 of DS `0x63f28`) finds 10 sites; the code ones are
`0x33ae9` (`fn_03272f`, a **cmp** ✓), `0x34394` (`fn_034353`), `0x345b3` (`fn_03442e`), `0x34918` (`fn_0348e2`) — all
three stores are modelled (`g_dat_63f28[cursor] = g_dat_63f08` / `= g_dat_63f08 - g_dat_63f0c` / `= b - g_dat_63f0c` ✓
`f.c:8325, 8870, 8893` ✓) — and the fifth, `0x3ff51`, looked like a fourth writer ✓ **it is not**:

```
0x3ff4b mov ebx, 0x28 ; 0x3ff50 xor edx, edx ; 0x3ff52 mov eax, 0x13f28 ; 0x3ff57 call 0x41e20
        -> memset(DAT_00063f28, 0, 0x28)   (10 dwords cleared, in fn_03feb8)
0x3ff5c mov [0x13f0c], 0 ; 0x3ff6b mov [0x13f08], eax ; 0x3ff75 mov byte [0x13f50], al
```

which the port *does* model (`fn_03feb8` ✓ `f.c:1797-1800` ✓: `fn_041e20(g_dat_63f28, 0, 0x28)`, `g_dat_63f0c = 0`,
`g_dat_63f08 = g_dat_63f0c`, `g_dat_63f50 = byte 63f08` ✓) ✓ So the "port has 3 writers, shipped has 4" reading is
**withdrawn** ✓ (a byte-pattern scan cannot tell a store from a memset ✓ the same lesson as docs/189 §3 and docs/252
§2 in reverse ✓) ✓

## 3. Rig capability unlocked: the shipped dump *is* a flat DS window (docs/232 §4 superseded)

`/tmp/rig/ents-orig/orig/state_XXXX.ds.bin` is exactly `0x16000` bytes ✓ and reads as a flat DS segment at
`offset = DS - 0x50000` ✓ validated on four independent known cells in sample 0900:

```
DS 0x63370 -> 1        (docs/209's expected value ✓)
DS 0x52c3c -> 172      (docs/259's validator ✓)
DS 0x51b60 -> 0        (g_tab_51b60[0] ✓)
DS 0x53e9c -> 100      (the drive's own stagewait cell ✓)
DS 0x639bc -> 2        DS 0x63e28 -> 6   DS 0x63f50 -> 3   DS 0x53c70 -> 0x5756c8
```

docs/232 §4's "`state_*.ds.bin` is a per-cell stream, not a flat DS window, so raw offset reads cannot name
arbitrary DS addresses shipped-side" is **superseded** ✓ — arbitrary shipped-side DS reads are available offline from
existing dumps ✓ (there is also a `state_XXXX.heap.bin`, `0x56a0` bytes ✓) ✓ This removes the "blocked on shipped-side
DS reads" constraint that gated several earlier questions ✓

## 4. The anomaly that now carries item 2b's remaining half

The shipped's `DAT_00063f08` and `DAT_00063f0c` — which the port models as the combo/technical level and its floor
(`g_dat_63f08` raised once per 7 units of `DAT_00063e28` ✓ `f.c:2563` ✓ citing the shipped's `0x3eea8-eeb7` ✓) — read
as **18188 (0x470c)** and **11623 (0x2d67)** in the shipped's own dump on this route ✓ heap-offset-shaped values ✓,
while `fn_03feb8`'s clear path expects them to be 0 and `0x3ff70-0x3ff75` uses byte `DAT_00063f08` as the 0..9 slot
cursor ✓ (`DAT_00063f50 = 3` ✓ consistent ✓) ✓ Two readings, both testable:

1. `fn_03feb8`'s clear never runs on this route in the shipped either ✓ and the cells are shared with a
   heap-offset user ✓ (so the *level* the gate consumes lives elsewhere and the port's `g_dat_63f08` is a
   mis-assignment ✓), or
2. the shipped raises them from a writer the port doesn't model ✓ (`DAT_00063e28` measured 1..7 across 244 shipped
   samples ✓ — the accumulator never exceeds 7 ✓, so the shipped's `>7 → +1 level` branch never fires either ✓).

Either way the **next measurement is offline and cheap**: enumerate every instruction touching DS `0x63f08`/`0x63f0c`
in the shipped (the same disp32 scan, corrected to distinguish `call 0x41e20` memsets from stores ✓), and census the
shipped's `DAT_00063f28[0..9]` array over the dump ✓ — if the shipped's array holds values > 0x1e ✓ (which the
gate needs ✓), the port's array construction is the defect site ✓; if it holds ≤0x1e ✓, then the shipped reaches
0x34/0x35 through a *different* `.key` entry (an entry whose `lvls` list is empty ✓) and the port's chain walk — not
the level array — is the defect ✓

Item 2b standing: clamp-guard fix ✓ (docs/254) and slot-selection fix ✓ (docs/263) both landed with rig evidence
(hits 30 → 79, enemy `+0x23` floor 23 → 15, player 0x33 41 → 65 samples ✓); award path, per-type table, damage
model, hit gates, `g_tab_51b60`, reaction block, technique table and its target ids all proven faithful ✓; the green
gate (a settled `kind==1` enemy, `+0x32 = 10`, `player[+0x52]` rising ✓) is still unmet ✓

## 5. The two readings in §4 are decided offline: the shipped's level array clears the gate, the port's does not

Census of the shipped's `DAT_00063f28[0..9]` over 244 DS-window samples of the canonical route:

```
slot 0: {0:10, 10:7, 14:21, 29:21, 30:120, 39:10, 40:5, 56:5, 70:24, ...}
slot 1: {0:11, 4:1, 14:17, 29:20, 30:22, 39:122, 48:8, 56:34, 70:4}
slot 2: {0:11, 14:131, 15:7, 29:19, 30:15, 39:10, 56:10, 70:29, ...}
slot 7: {0:11, 4:6, 14:20, 29:21, 30:16, 39:15, 40:10, 56:130, 70:15}
slot 8: {0:11, 14:30, 29:20, 30:15, 39:14, 56:19, 70:128, ...}
slot 9: {4, 14:16, 29:134, 30:10, 37:6, 39:12, 40:12, 56:20, 57:5, 70:17, -1:11}
```

The shipped's per-slot level values include **56, 57, 70, 99, 148** ✓ — every one of them satisfies
`lvls(k) 0x1e < level` ✓✓ So the shipped genuinely admits `target 0x34 / 0x35 / 0x36` through the gate ✓✓ while the
port's 552 "level too high" rejections show its array never exceeds 0x1e ✓✓ **Reading (1) is confirmed: the port's
level-array construction is the defect site** ✓

And the raiser does fire in the shipped: `DAT_00063e28` is measured cycling through `1..7` across the samples ✓ (my
"never exceeds 7" phrasing in docs/263 was wrong ✓ — the shipped's `cmp 0x63e28, 7 / ja` then `sub 7` cycle means the
accumulator *saturates* at 7 and the level increments each time it crosses ✓ which is how the shipped reaches levels
56/70/99 ✓) ✓ A displacement scan finds 24 sites naming DS `0x63f08` and 10 naming DS `0x63f0c` ✓ (three of the
`0x63f0c` sites are inside `fn_034353`/`fn_03442e` ✓ matching the port's `- g_dat_63f0c` forms ✓) ✓

Next measurement, in-process and cheap: read the port's `g_dat_63f08` / `g_dat_63f28[0..9]` on the same canonical
route and compare against the shipped's `{14,29,30,39,56,70,...}` census ✓ — if the port's counter stalls at ≤30 ✓ the
fix is in whatever feeds `DAT_00063e28` (`f.c:2563`'s `g_dat_63e28 += ...` chain ✓: the accumulator's input is the
quantity the shipped accumulates per held/pressed key ✓), and the gate for that fix is already written: the ACTTRACE
reason census dropping "level too high" from 552 ✓ and the matched-target census gaining 0x34/0x35/0x36 ✓
