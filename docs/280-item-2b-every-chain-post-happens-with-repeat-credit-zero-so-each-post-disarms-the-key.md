# 280 — Item 2b: every chain post happens with repeat credit **zero**, so each post disarms the key — and docs/272's "the port never reaches state 4" is superseded

Canonical drive `ep1_kill3.txt` 150 s ✓, `NIGHT_ACTTRACE=1`, new counters at the post branch:

```
posts with repeat-credit (g_dat_62cd8) > 0 : 0        <-- none
posts with repeat-credit == 0 (climb ends) : 67       <-- all of fn_034735's posts
max repeat credit ever seen per key         : 0x1c:16  (Enter only -- 0x1d/0x2a/0x2c/0x36/0x38 never credited)
fn_034353 entries 106 = 67 (via fn_034735) + 39 (via fn_034635)   fn_0348e2 6   fn_03442e 201
```

## 1. The mechanism, read off the shipped body first

`fn_034735` tail (`--entry 0x34735 --end 0x347e0`):

```
0x3477a: al = [eax+0x12cd8]            ; repeat credit            (state == 4 arm)
0x34787: je 0x347a2      -> 0x3478e mov byte [eax+0x12c58],1 ; 0x3479a dec byte [eax+0x12cd8]   (keep climbing)
                       -> 0x347a7 mov byte [eax+0x12c58],0                                       (disarm)
0x347b5: al = [eax+0x12c58]            ; state                    (second arm)
0x347c0: cmp eax, 4 / 0x347c3: jge 0x347da                        (state >= 4 skips BOTH the inc and the call)
0x347ca: inc byte [eax+0x12c58]        ; 1 -> 2 -> 3 -> 4
0x347d5: call 0x3442e
```

The port is byte-faithful here ✓ (`f.c:8862-8872`: `if (state == 4) { post; credit ? (state=1, credit--) : state=0 } else if (state < 4) { state++; fn_03442e(idx); }`) ✓ — so the defect is **not** in this state machine ✓

Consequence of `credit == 0` at every post: `state = 0` ✓, so `fn_0347e3` stops dispatching that slot ✓ and the
`1→2→3→4` climb has to restart from a fresh key **make** ✓ — one post per press, never a chain ✓ Chain entries 8/7/6
(`0x1d ×2/×3/×4`, docs/279 §1) then cannot ever see their window ✓ → no `0x34/0x35/0x36` targets ✓ → no settle ✓ → no
EXP ✓✓ That is the localized root cause of item 2b's second half ✓

## 2. Superseded: docs/272 §2's "the port's key state never reaches 4"

The port reaches state 4 at least 106 times per run (each post *requires* it ✓). docs/272's census
`{1:876, 2:1}` vs the shipped's `{1:430, 2:3, 3:11, 4:21}` is a **sampling-cadence artifact**: in the port the climb
`1→2→3→4` completes within a handful of `fn_0347e3` dispatches (its pump ran 1.65 M times in this 150 s run ✓), so a
10 Hz state snapshot almost never lands on 2/3/4 ✓, while the shipped's slower cadence lands on them 34 times ✓ — same
class as docs/277's event-vs-state lesson ✓ Quote the post counts, not the state census ✓

## 3. Open sub-question for the fix (bounded, next)

Why does repeat credit reach `0x1c` (max 16 ✓) but never a chain key? The drive does hold `lctrl` for 430 ms
(`ep1_kill3.txt` lines 45-54: `157600 key lctrl down` / `158030 key lctrl up` ✓) and the drive's synthesised typematic
(defaults 250 ms delay / 100 ms rate ✓ `main.c:663-712` ✓, `rig_marker "key %#02x repeat"` ✓) should emit ~2 repeats per
such hold ✓ — so the credit is either not applied for modifier scancodes in `app/keymap.c`'s make path ✓, or it is
consumed by `fn_0343d1` (`f.c:8350-8351`: `credit ? (state=1, credit--) : state=0` ✓, shipped `0x34405/0x3441e` ✓)
before the climb reaches 4 ✓ Next witness: count makes/breaks/repeats delivered per scancode in the key pump ✓, plus
`fn_0343d1` entries per scancode ✓ Fix candidate must be chosen from that split (drive-fidelity vs port-credit), and
gates on: posts with credit > 0 ✓, ring ≥8-filled → 85 % ✓, `0x1d` window L=2/L=3 → 22.6 %/12.9 % ✓, `DAT_00062d58`
gaining `0x34/0x35/0x36` ✓, settled `kind==1` with `+0x32 = 10` ✓, `player[+0x52]` rising ✓, suite ALL PASS ✓, soak 9/9 ✓
