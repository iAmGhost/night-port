# 206 — Item 3 detected from the unfixed build: the port takes the cursor row's purchase count for all six rows

## 1. What the shipped code does (0x00039848-0x000398a9, `disasm.py --entry 0x39848 --end 0x399a5`)

`fn_038f3d` is the level-up/upgrade screen (it loads `levelup.spr`, `number1.spr`, `number2.spr` and
`levelup.pcx`/`levelup2.pcx`, docs/61).  Its per-row upgrade-bar loop:

```
0x39848: mov eax, [ebp-0x54]                     ; i = the loop variable (0..5)
0x3985b: shl eax, 2 ; mov eax, [eax + ebp-0x11c] ; sel[i]   -- indexed by i
0x39865: mov [ebp-0x50], eax                     ; n     = sel[i]
0x3986e: mov [ebp-0x4c], eax                     ; shown = sel[i]
0x3988d: mov eax, [ebp-0x54] ; 0x39890: cmp eax, [ebp-0x60]   ; i == page (the cursor row)
0x3989a: shl edx,2 ; 0x398a0: cmp [ebp-0x50], [edx + ebp-0xec] ; n < selMax[i]
0x398ac: inc [ebp-0x50]                          ; n = shown + 1, only for i == page
0x398b3: cmp [ebp-0x50], 0 ; je 0x39943          ; nothing is blitted when n == 0
0x398c2: mov eax, [i*4 + ebp-0x104] ; add eax, [ebp-0x50]     ; idx = gbase[i] + n
```

Both `n` and `shown` are read from the **row-indexed** array `[i*4 + ebp-0x11c]`.  Only the "+1 while the
cursor is on this row" test uses `page`.

## 2. What the port does (unfixed build, `port/src/f/f.c:25818`)

```c
for (int32_t i = 0; i < 6; i = i + 1) {                       /* 0x00039848-0x000399a2 */
    int32_t n = sel[page], shown = sel[page];                 /* <-- indexed by the CURSOR row */
    if (frame % 0x32 > 0x19 && i == page && shown < selMax[i]) n = shown + 1;
    if (n != 0) { int32_t idx = gbase[i] + n - 1; fn_03bb42(... barX[i], barY[i], s4[idx], s8[idx], ...); }
```

`sel[page]` instead of `sel[i]`: the cursor row's purchase count is applied to every row.

## 3. The rig measurement that detects it (no human input)

New drive `scripts/rig/drives/ep1_lumenu.txt` (ep1_kill3 prefix truncated to 94 s, then the rig's state
injection sets the player's own selection bytes, which `fn_038f3d` snapshots into `sel[6]` at entry):

```
94500 play 0x27 1      -> "poke player +0x27 = 0x1 (ok) at t=84750ms"   (sel[0] = 1, weapon group)
95000 play 0x28 2      -> "poke player +0x28 = 0x2 (ok)"                (sel[1] = 2, ship)
run: NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60
     NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=5600  -> "showscreen warp: levelup at frame 5612"
witness (new, env NIGHT_LUMENU=1, inside the row loop):
lumenu frame=24 page=1 i=0 sel[page]=2 sel[i]=1 n=2 shown=2 max=4
lumenu frame=24 page=1 i=1 sel[page]=2 sel[i]=2 n=2 shown=2 max=4
lumenu frame=24 page=1 i=2 sel[page]=2 sel[i]=0 n=2 shown=2 max=2
lumenu frame=24 page=1 i=3 sel[page]=2 sel[i]=0 n=2 shown=2 max=1
lumenu frame=24 page=1 i=4 sel[page]=2 sel[i]=0 n=2 shown=2 max=1
lumenu frame=24 page=1 i=5 sel[page]=2 sel[i]=0 n=2 shown=2 max=1
```

Rows 2, 3, 4 and 5 have `sel[i] = 0`, so the shipped loop hits `cmp [ebp-0x50],0 / je` and blits nothing for
them; the port blits `gbase[i] + 2 - 1` on all six.  Census over the run's 196 menu frames (1 176 witnessed
rows): **338 rows carry an `n` larger than that row's own `selMax`** (e.g. `n=2` against `max=1` on rows 3, 4,
5), so the port also reads outside the row's own sprite-row range.  That is item 3's reported symptom -- one
purchase lights the whole menu -- produced by the rig on the unfixed build.

Two notes on the drive: the poke must land while a stage is live (`play 0x27 1` at 62-72 s printed
"poke player +0x27 = 0x1 (no such object)": the player entity does not exist yet), and the warp must fire
after the poke because `fn_038f3d` snapshots `sel[6]` once, at entry (0x00039012-0x00039062).

## 4. Also recorded this turn (screen-oracle basis, docs/205 §1-§3)

`scripts/rig/screen_time.py` pairs shipped and port frames by *nominal* time through the replay's rebase
table instead of `diff_screen.py`'s ±90-frame DP window, which saturates on 300 s routes (its worst window
paired shipped frames 2854-2856 with the port's black 640x480 boot frame).  Time-paired scores measure the
5x pacing divergence (mean 0.7506, 4 678 of 4 723 pairs under 0.88), so screen gates must pair on events.
The mode census is structural and quotable: shipped alternates 640x480 (the gameplay arena, VBE mode 0x101 set
by `fn_03f15f`) and 320x200 (the PCX slide show) in 8 segments; the port has 4 and never re-enters the arena
after nominal 73 s.  All nine shipped CALL sites into `fn_03f15f` are modelled, and the gates' tables match
shipped DS (`DS 0x52f90 = 00 00 00 00 01 00 00 00 01 00 00 00 02 ...`, `DS 0x52fb4 = 3`), so the divergence is
state/path, not a missing call.

Caveat for the resource oracle: the shipped run's `timeline.json["res_events"]` (435 names) are all stamped
`t_wall = 0.022 s` and contain name *templates* (`%de%d.pcx`, `stage%d.pcx`, `na%d%04d.pcx`, `st%ds.spr`) --
it is the game's resource-name table captured at bringup, not a lookup trace, so it cannot order screens in
time.  Its extension census is `.pcx 183, .voc 100, .spr 84, .act 67, .fli 1` and contains **no** `.key`,
`.stg`, `.wal`, `.chr` or `.dat` names, while the port's timed marker log asks for 10 `.key`, one each of
`.stg/.wal/.chr/.dat` (`stage1.stg`, `stage1.wal`, `night1.chr`, `exptab.dat`, `gun*.key`, `catch*.key`).
Whether those names are the shipped's own or port-side constructions is an open question in the docs/124-133
naming family.
