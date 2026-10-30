# 204 — Retraction: `fn_0383a4` / `fn_038c23` draw the CONTINUE countdown, not kill EXP text; and every frame reading in docs/200-202 was a channel-split artifact

## 1. `DS 0x53100` is the CONTINUE countdown

Disassembled `fn_038445` (guest 0x38445..0x38900, 329 instructions, `uv run --with capstone scripts/audit/disasm.py --entry 0x38445 --end 0x38900`):

```
0x38470: mov esi, 0x3108 / 0x3120 / 0x3138   rep movsd x6   -- copies three 6-dword tables (DS 0x53108, 0x53120, 0x53138)
0x384fd: cmp dword ptr [ebp - 0x20], 3                      -- a k=0..2 loop over those three rows
0x385c7, 0x38601: call 0x3bb42                               -- two sprite (label) blits
0x38606: push dword ptr [0x3100]   ; DS 0x53100             -- the value to render
0x3861e: call 0x383a4                                        -- the 7-column integer draw
0x38637: call 0x10b1d                                        -- music select (same callee as docs/184)
```

`call_sites.py --target 0x38445` → **1 caller: `fn_03a829` at 0x3abca**, i.e. the same dispatcher that is the only
shipped caller of `fn_010d47` (docs/184 §2).  The port's own model already names the value:

```
f.c:25360   uint32_t g_dat_53100 = 0x3;              /* DAT_00053100: CONTINUE countdown */
f.c:23399   if (g_dat_53100 == 0) { ... return 0; }  /* 0x0003aeb7-0x0003aec8 no credits */
f.c:23400   if (fn_038445() == 0) { g_dat_53100 = 3; return 0; }   /* 0x0003aecf-0x0003aee1 game over */
f.c:25558   g_dat_53100 = g_dat_53100 - 1;           /* 0x000389f3 countdown tick  */
f.c:25757   g_dat_53100 = g_dat_53100 + 1;           /* one CONTINUE credit        */
f.c:25468   fn_0383a4(..., 0x118, 0xae, g_dat_53100); /* 0x00038606-22 */
f.c:25812   fn_038c23(..., 0x68, 0xac, g_dat_53100); /* 0x0003981f-3f */
```

So the panels at `x=0x68,y=0xac` and `x=0x118,y=0xae` are the **game-over CONTINUE screen**.  That is why docs/196 §1
found `fn_0383a4`/`fn_038c23` called zero times, and why docs/182 row 2a's guessed coordinates hold static map tiles
(docs/200 §2): **docs/182 row 2a's draw-site guess, and docs/195 §1b / docs/196's narrowing of item 2a onto these two
routines, are superseded.**  Item 2a's kill-text draw site is unidentified again, and the "same-region static tiles"
reading is now explained rather than mysterious: those coordinates belong to a screen no rig route reaches.

## 2. Every shipped/port frame reading in docs/200-202 was misparsed

```
/tmp/rig/ents-orig/orig/state_0373.frame.bin: file 512015 B, header 40 01 00 00 c8 00 00 00 00 0a 00 00 20 00 00
    payload 512000 B = 2 x (320 x 200 x 4)   -- two RGBA frames, halves not equal
/tmp/rig/ents-port/frames/005600_320x200.bin: 256000 B = 320 x 200 x 4  -- RGBA
```

Reading those payloads as **indexed** bytes makes each pixel four bytes (R,G,B,A) with alpha = 0, which is exactly the
"(1,1,1,0) in 16 000 of 16 000 four-pixel blocks" pattern I reported as a 3-on/1-off period-4 dither in docs/202, and
exactly the "2-px pairs", "3x1 blocks", "3x2 rows spaced every 4 columns" shapes in docs/200 §1-2 and docs/201 §1-2.
**Retracted:** docs/202's dither claim, and the shipped-side component shapes in docs/200/201.  Some shipped states
are also a different mode entirely (timeline entry 0: mode 0x101 = VESA 640x480x8; state 373: mode 0x13 = VGA 320x200
double-scanned), so a fixed 320x200 slice is wrong for the whole run.

## 3. What survives, on the corrected basis (RGBA, per-pixel compare)

Shipped kill window, `half0` of consecutive states, per-pixel RGBA compare (`/tmp/rig/ents-orig/orig`):

```
pair 366->367:  996 changed px, 53 components      pair 374->375: 1660 changed px, 51 comps
pair 367->368:  654, 54                            pair 375->376: 3240, 15
pair 368->369: 2070, 17                            pair 376->377: 3406, 18
pair 369->370: 2178, 17                            pair 377->378: 2100, 52
pair 370->371: 5444, 16                            pair 378->379: 3348, 30
pair 371->372: 2350,  7                            pair 379->380: 1710, 29
pair 372->373: 2528, 15                            pair 380->381:  762, 59
pair 373->374: 1714, 50                            pair 381->382: 1554, 21
```

No component in that window is text-shaped (3-9 rows tall and >=18 px wide).  Shipped samples ~3.25 states/s here, so
a popup shorter than ~300 ms is invisible to this capture; a wider scan of all 974 states with a permissive
text-band filter returns 216 957 candidates (scrolling sprites dominate), which is a measurement of the filter, not of
text.  **`gate3-exp-text` stays open, and the honest next step is code-side, not pixel-side**: name the shipped routine
that renders a per-kill number by finding the callee that takes a number derived from a victim record, then compare
its invocation gate with the port's.
