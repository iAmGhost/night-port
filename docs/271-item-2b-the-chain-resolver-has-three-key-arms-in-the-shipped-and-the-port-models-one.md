# 271 — Item 2b: the chain resolver consumes the input ring at 35 shipped sites, the port at 27 — and the missing arms are two of the three copies of one key-handling triplet

Everything here is static disassembly plus two dump-side censuses on the **same** basis (`/tmp/rig/rb8`, the canonical
`ep1_kill3.txt 150` route dump ✓ both sides ✓ `--rig-hz 10` ✓).

## 1. A tool defect first: a "zero call sites" verdict that came from searching the wrong bytes

I first asked "who calls `fn_0003feb8`?" and searched NIGHT.EXE for the four bytes `b8 3f 01 00`-style *absolute*
addresses ✓ and reported **0 callers** ✓ for `fn_03feb8`, `fn_03658d`, `fn_034353`, `fn_03442e` ✓ **All four verdicts were
false** ✓ — a `call` in this binary is `E8` with a **signed rel32 measured from the next instruction** ✓, so the target's
absolute address never appears in the file ✓ Same failure family as docs/252 §2 (grep `0x3b5c0` ≠ `fn_0003b5c0` ✓) and
docs/254 §3 (accessor macros ✓) ✓

Permanent fix: **`scripts/audit/call_xrefs.py`** ✓ — walks `0x10000..0x4a000`, resolves every `E8` rel32, groups sites by
enclosing shipped function, and supports `--inside 0xLO+SIZE` to count sites inside one body ✓ It resolves **4503 direct
calls** ✓ Re-running the census gives the truth:

```
call 0x3feb8:  1 site   from fn_0x3658d              (0x365a5)
call 0x3658d:  1 site   from fn_0x3af18              (0x3af4d)   <- the stage-advance routine
call 0x347e3:  1 site   from fn_0x3a829              (0x3aae0)   <- the gameplay tick
call 0x3272f:  2 sites  from fn_0x3a829              (0x3ab02, 0x3ab0e)
call 0x34353:  4 sites  from fn_0x34635 x3, fn_0x34735 x1
call 0x3442e:  1 site   from fn_0x34735
call 0x348e2: 39 sites  from fn_0x3272f x35, fn_0x1441e x2, fn_0x12616 x1, fn_0x3a829 x1
call 0x3493e: 15 sites  from fn_0x3272f x15
```

## 2. What the ring helpers actually do (shipped bytes)

`fn_000348e2` (92 B, `0x348e2-0x3493d`) — **push a null entry and advance the cursor**:

```
0x348fc  mov  al, [0x13f50]              ; cursor = DAT_00063f50
0x34901  mov  byte [eax + 0x13f14], 0    ; DAT_00063f14[cursor] = 0        (code 0 = "no input")
0x3490f  shl  eax, 2                     ; dword stride
0x34912  mov  edx, [0x13f08]             ; DAT_00063f08 (level)
0x34918  mov  [eax + 0x13f28], edx       ; DAT_00063f28[cursor] = level
0x3491e  inc  byte [0x13f50]             ; cursor++
0x3492b  cmp  eax, 9 / 0x34930 mov [0x13f50], 0     ; wrap 9 -> 0
```

`fn_0003493e` — **read the previous slot's code** (`cursor == 0 → DAT_00063f1d` = slot 9 ✓, else
`DAT_00063f13 + cursor` = slot cursor−1 ✓); modelled faithfully at `f.c:8336-8342` ✓
`fn_0003feb8` — the **reset**: `memset(DAT_00063f14,0,10)`, `memset(DAT_00063f1e,1,10)` (`0x3ff3c mov edx,1 /
0x3ff41 mov eax,0x13f1e / 0x3ff46 call 0x41e20` ✓), `memset(DAT_00063f28,0,0x28)` ✓, `DAT_00063f50 = DAT_00063f08` ✓ —
and it is reached **once, from `fn_03658d` (bring-up), which itself is called once, from `fn_03af18` (stage advance)** ✓
So the shipped resets the ring per *stage*, not per frame ✓ The port models this faithfully (`f.c:1796-1800` ✓,
`fn_03658d` called from `fn_03af18` at `f.c:27837` ✓) ✓
`fn_00034353` posts an entry and **clears** its consumed flag (`0x343a1 mov byte [eax+0x13f1e], 0` ✓,
`f.c` `g_dat_63f1e[g_dat_63f50] = 0` ✓); `fn_0003272f` only ever *sets* the flag (`0x33baa/0x33bf9/0x33c51/0x33d6e/
0x33e2e/0x34028/0x340c4/0x341bf mov byte [eax+0x13f1e], 1` ✓ = 8 stores ✓, port 8 stores ✓) ✓ — polarity confirmed,
the earlier "maybe 1 means available" reading is withdrawn ✓

Call graph, both sides ✓ faithful in shape: `fn_03a829` (tick) → `fn_0347e3` → {`fn_034635` (3 posts), `fn_034735` (1 post
+ `fn_03442e`)} ✓ port `f.c:23558 → 8852/8854 → 8830/8835` ✓ ✓ `fn_03272f` is invoked from **two** shipped sites whose
argument differs ✓ — `player[+0x17] ∈ {0x66, 0x70}` → `eax = DAT_00062a9c`, else `eax = DAT_00062a98` ✓ — and the port
factors this into one call with a conditional argument (`f.c:23561-23563` ✓) ✓ **so my "the port calls it once where the
shipped calls it twice" reading is withdrawn** ✓ (factoring, the docs/229 lesson again ✓) ✓

## 3. Dump-side census: the port's ring never advances and never stays fresh

Same drive, same dumps (`rb8` ✓), port cells read out of the cell stream, shipped read out of its flat DS window
(`offset = DS − 0x50000` ✓ docs/264 §3 ✓) ✓ — widths now equal after docs/270 ✓

| measure | shipped (n=428) | port (n=1500) |
|---|---|---|
| `DAT_00063f50` cursor distribution | 75,40,43,36,36,40,45,39,34,40 over 0..9 (**uniform ⇒ it advances**) | **916 of 1500 at 0** (61 %), others 50-113 (**it stalls**) |
| consumed flags set, out of 10 slots | 6-7 in 309 samples | **10 in 903 samples** (60 %) |
| adjacent `0x1d` code pairs in the ring window | 2 pairs: 145, 3+: 167 ⇒ **73 % have ≥2** | 0 pairs: **993** (66 %), 1: 226, 2: 149, 3: 132 |

A ring whose cursor never advances and whose slots are all consumed cannot present the *window* a multi-code technique
entry needs ✓ — which is exactly the ACTTRACE rejection profile recorded in docs/264/266 (`slot consumed` 5286 ✓,
`len != k` 3166 ✓, chain MATCHED 53 ✓, target `0x33` only ✓) ✓

## 4. The localization: 35 vs 27 consumption sites, and which arms are missing

`call_xrefs.py --inside 0x3272f+7014` ✓: the shipped calls `fn_0348e2` at **35** sites inside `fn_03272f` ✓ and
`fn_03493e` at **15** ✓ The port calls `fn_0348e2` at **27** sites in the chain-resolver region (`f.c:27325-27374` and
`f.c:27459-27732` ✓ — note the first seven belong to `fn_03272f`'s body by guest address ✓ e.g. `f.c:27353` cites
`0x00032fa2-0x00032fc4` ✓ and `f.c:27361/27363` correspond to shipped `0x33089/0x330a6` ✓; my script's name attribution
said `fn_03e20f` ✗ that was my extractor's fault, not the port's ✗) ✓ and `fn_03493e` at 15 ✓ (`fn_03493e` count **matches** ✓) ✓

Each shipped site paired with its nearest preceding test:

```
0x32819 cmp eax,0x36   0x329a0 cmp eax,1     0x32a11 cmp byte[0x13ed5],0  0x32a6b cmp eax,1
0x32ab8 cmp eax,1      0x32ba9 cmp eax,1     0x32bc4 cmp [eax+0x11],0x11   0x32c72 cmp eax,1
0x32d50 cmp eax,1      0x32d85 cmp eax,0x36  0x32e39 cmp eax,1             0x32e93 cmp eax,1
0x32eb6 cmp [eax+0x11],0x67   0x32efd cmp [eax+0x11],0x62   0x32fd7 cmp [eax+0x11],0x3e
0x33089 cmp eax,0x1d   0x330a6 cmp eax,0x38  0x33138 cmp eax,2             0x331b0 cmp [eax+0x11],0x32
0x331cd cmp [eax+0x11],0x3e
0x3327f cmp eax,0x1d   0x3329c cmp eax,0x38  0x3332a cmp eax,2             0x33347 cmp [eax+0x11],0x3e
0x333f9 cmp eax,0x1d   0x33416 cmp eax,0x38  0x33481 cmp eax,2             0x33521 cmp eax,1
0x33585 cmp eax,2      0x338fa cmp eax,1     0x33976 cmp byte[0x13ed5],0   0x33e21 cmp eax,0x99
0x33f25 test eax,eax   0x33fd4 cmp dl,[eax+0x12b90]                        0x340ba cmp [0x12d58],0x71
```

The same triplet appears **three times** ✓ — (`0x1d → state 0x88`, `0x38 → state 0x89/0x8a/0x8b`, `count==2 → state
0x8c or 0x8d`, each followed by `call fn_0348e2`) at `0x33089/0x330a6/0x33138`, at `0x3327f/0x3329c/0x3332a`, and at
`0x333f9/0x33416/0x33481` ✓, each introduced by `call fn_03493e ; mov [ebp-8], al` (read the previous ring slot ✓) and a
`cmp [player+0x11], 0x3e / call fn_0348e2` prologue ✓ The port implements **one** such arm (`f.c:27361` `key=='\x1d' →
PL32(0x17)=0x88` ✓ `f.c:27363` `→ 0x89/0x8a` ✓ `f.c:27353` `PL32(0x11)==0x3e → fn_0348e2` ✓) ✓ — hence 8 unmodelled
consumption sites ✓

Supporting symptom ✓: the shipped's arms land the player in the **0x88-0x8d** state family ✓, and the port's player-state
census (`/tmp/rig/rb3`, ch2 ✓ docs/263) is `{0, 1, 2, 0x33, 0x0b, 0x0c, 0x15, 5}` ✓ — **no 0x88-0x8d value appears** ✗ ✓

## 5. Fix plan (not yet applied)

Model the second and third triplet arms faithfully from the shipped bytes — `0x331ba-0x331cd` (slot read + `+0x11==0x3e`
consume ✓), `0x3326b-0x332a1` (`0x1d → 0x88`, `0x38 → 0x8a`, consume ✓), `0x332a8-0x3332f` (`0x67`,
`[player+0x11f] == 0x1fffff`, `+0x11 == 0x1c`, `count==2 → 0x8d else 0x8c`, consume ✓), `0x33334-0x33347` and
`0x333f9-0x33481` ✓ — adding `fn_0348e2()` at each, with the shipped's own guard order and jump targets ✓ (docs/267 §2.7:
no invented conjuncts ✓) ✓ Gates for the change: cursor distribution leaving 0-dominance ✓, consumed-flag sum leaving the
all-10 mode ✓, ring windows presenting ≥2 adjacent `0x1d` codes ✓, player entering 0x88-0x8d ✓, then the item-2b green
gate (`kind==1` settle ✓, `+0x32 = 10` ✓, `player[+0x52]` rising ✓ docs/239 ✓), suite ALL PASS ✓, soak 9/9 ✓
