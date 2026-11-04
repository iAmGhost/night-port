# 291 — Two retractions about item 6, one real transcription defect found, and the measurement that shows item 6's real remaining lead is the vertical band

## 1. RETRACTED: docs/290 §2's "the port flips facing in five handlers the shipped never touches"

`scripts/audit/field_writers.py --disp 0x1b` reported 58 shipped writes ✓ and I then scanned five suspect
bodies with `disasm.py --entry 0x<fn> --end <entry+body_bytes>` and found zero `+0x1b` references ✓✗ The scan was
**short of the body**: `functions.json` gives `fn_017f13` as `body_start 0x17f13 … body_end 0x18e87` (3956 B) but
its `body_bytes` field is 3863 ✓, so `entry + body_bytes = 0x18e2a` stopped 93 bytes early ✓ and the flip tail was
exactly in those 93 bytes:

```
0x18e51  fn_017f13 tail:  mov eax,[ebp-4]; call 0x1519f; cmp eax,[ebp-0x2c]; jle 0x18e68
0x18e5e                    mov [eax+0x1b], 1                      ← the proximity flip, present in the shipped ✓✓
0x18e68                    … cmp eax,[ebp-0x2c]; jge 0x18e7f
0x18e75                    mov [eax+0x1b], 0
0x18e7f  mov esp,ebp / pop ebp,edi,esi,edx,ecx,ebx / ret
0x19e11  fn_018e88 tail (0x19e11-0x19e38): same shape, [eax+0x1b] = 1 / = 0 ✓
```

So the port's 20 proximity-flip sites (`fn_017f13, fn_018e88, fn_019e48, fn_01aec8, fn_01bea8, fn_01cf64,
fn_01df8b, fn_01f064, fn_020170, fn_02119c, fn_0221b8, fn_02319c, fn_024bf9, fn_0265ac, fn_027bec, fn_029709,
fn_02adf5, fn_02c838, fn_02e2cc, fn_02fe1e ✓`) match the shipped's per-type handler tails ✓ — **no invented
stores there** ✓ docs/290 §2's five-site claim is withdrawn ✓

**Rule recorded:** when walking a shipped body, use `body_start`/`body_end` from `functions.json` ✓ — never
`entry + body_bytes` ✓ (docs/179-181's "check the boundary instruction" lesson, new failure mode ✓: a truncated
window silently drops a *tail* block, which is exactly where these handlers keep their facing stores ✓).

## 2. RETRACTED: docs/290 §1's facing-share table as a red signal

The port's own dump shows the player's `+0x1b` is a **fast oscillation**, not a stable value
(`/tmp/rig/le4/port/player_recs.jsonl ✓ 23 flip events over 1086 samples ✓`):

```
tick 913 x=15 st=11 1->0 | tick 922 st=1 0->1 | tick 926 st=11 1->0 | tick 935 st=2 0->1
tick 939 st=11 1->0 | tick 948 st=2 0->1 | tick 952 st=11 1->0 | … period ≈ 7-8 ticks ✓
flips per state: {2:10, 11:9, 12:3, 1:1}   direction: 0->1 x12, 1->0 x11 ✓ (balanced ✓)
```

The port dumps at 10 Hz and the shipped replay at ~4 Hz ✓, so the *phase* of a ~0.8 s oscillation is sampled
differently on each side ✓ and the per-state mirror shares (shipped 7 % vs port 22-32 % ✓) are a sampling
artifact, not a state difference ✓ docs/290 §1 is void as evidence ✓ (docs/277 §1 / docs/279 §4 again ✓) ✓
What *is* admissible and unchanged: the **bound-state comparison** of docs/288 §3 — both sides park at `X=-30`
with `+0x1b = 0` and state 0 ✓✓ (re-confirmed on `/tmp/rig/le3`: shipped 35 bound samples `Y=172, f1b=0 ✓`,
port 277 `Y=172, f1b=0 ✓`) ✓

## 3. A real defect, found statically and fixed: the walking body's entry guard

`LAB_0003284e` (shipped `0x3284e`, the body shared by states 1, 2 and 0x28 ✓) is admitted by a je-chain that
tests only the three keys that have a make flag *and* a movement arm here:

```
0x3285f cmp byte [0x63ed0],0 ; je 0x3286a      ; Up (scan 0x48 -> cell 0x63ed0, keymap.c:47 ✓)
0x32868 jmp 0x32873                              ; Up held -> body ✓
0x3286a cmp byte [0x63ed3],0 ; je 0x32875       ; Left (scan 0x4b -> 0x63ed3 ✓)
0x32871 je / fall -> 0x32873 -> 0x32882          ; Left held -> body ✓
0x32875 cmp byte [0x63ed5],0 ; je 0x3294d       ; Right (0x4d -> 0x63ed5 ✓): none held -> EXIT 0x3294d ✓
```

`DAT_00063ed8` (Down ✓) is tested only *inside* the vertical sub-branch at `0x328cc` ✓ — it never admits the
body ✓ The port's guard read `if (g_dat_63ed8 != 0 || g_dat_63ed0 != 0 || g_dat_63ed3 != 0 || g_dat_63ed5 != 0)`
✗✗ so a Down press alone entered the body, ran `fn_01586b(pl, +step)` and forced `state = 2` ✓ Fixed to
`if (g_dat_63ed0 != 0 || g_dat_63ed3 != 0 || g_dat_63ed5 != 0)` with the chain quoted in the comment
(`src/f/f.c` `lab_0003284e` ✓) ✓ `make test` → **ALL PASS** ✓ (no fixture asserted the old guard ✓)

**Its own before/after measurement is not a clean pair, and I am not claiming one.** New drive
`scripts/rig/drives/ep1_downonly.txt` (kill3 bring-up + walk to the left bound, then 118-161 s of **Down-only**
makes ✓ no Left/Right/Up):

```
                    samples in the Down window        Y histogram in that window
SHIPPED le4b        141                               172: 23, 195: 117  ✗ (the shipped is already at 195)
PORT  le4b post-fix 418 (all at the bound)            172: 418
PORT  le4b PRE-fix  0 (its sample stream ends early)  whole run: 172: 580, 195: 91
```

The shipped's player sits at **Y = 195** through most of this room and at 172 only at the left bound ✓, so a
"Down does not admit the body" difference is not observable as a Y change on this drive ✓ The fix stands on the
disassembly ✓ (byte-faithful guard ✓) and is recorded as a transcription fix, **not** as item 6's fix ✓

## 4. The lead that survives: the vertical band (port 172 vs shipped 195 in the same room and phase)

`/tmp/rig/le4b` whole-run player Y: **shipped {172: 204, 182: 1, 195: 408} ✓ vs post-fix port {172: 912} ✗✗**
while the same drive on `/tmp/rig/le3` (walk + attack) gave shipped `{172: 210, 195: 433}` and port
`{172: 548, 195: 472}` ✓ — so the port *can* be at 195 (when a horizontal key is held and the vertical arm runs
through the body ✓) but **stays at 172 when it is parked at the left bound, where the shipped is at 195** ✓✓
That is docs/242 / docs/255 ("both sides' enemies stand on Y 141; the shipped parks the player at the lower
bound, the port leaves it at 172") still being open ✓ — and it is the only item-6-adjacent divergence the rig
currently measures at the left edge ✓ Since the limb's destination is computed from the record's own X/Y plus
the anchor arrays (proven faithful, docs/220/255) and the arm selector from `+0x1b` (identical at the bound ✓),
a player standing on the wrong ground line is the remaining mechanism that can put the limb where item 6
describes ✓

Next step for item 6 (unchanged in kind, sharpened): find the shipped writer that puts the player at 195 in this
room and is absent in the port — candidate sites are the vertical movers' callers in the action chain
(`fn_034295` gated by `+0x1b` ✓, `fn_0342ff` ungated ✓, `fn_01586b` callers ✓) and the room-entry placement at
`fn_0376c8`/`fn_03d244` (`0x37897-0x378cf`, `0x3d384-8a` ✓) which the port models for X only in one arm ✓
(`0x37897: edx = DAT_639dc - 0x1e` ✓ / `0x378bc: + 0x15e` ✓ — the shipped writes *only* `[player+1]` there ✓,
never a Y ✓, so the 195 must come from a mover ✓ not from spawn ✓).
