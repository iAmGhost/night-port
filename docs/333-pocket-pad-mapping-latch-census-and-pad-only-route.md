# 333 — The pad mapping, measured: which DAT_63e88 slot each Pocket button arms, and what a pad-driven route reaches

Task-3 wired the pad pump; this doc is task-6's verification: the mapping must not invent its own input
path, it must land in the same held-bytes the shipped IRQ 9 handler wrote (docs/91, docs/92).

## 1. The map, and the slots it lands in

`pocket/src/night/main_of.c`. **The layout below is the one the game runs now**; §1b is what it used to be
and what did not change when it moved.

```c
static const pad_key_t g_pad_keys[] = {
    { OF_BTN_UP,     0x48 },   { OF_BTN_DOWN,   0x50 },
    { OF_BTN_LEFT,   0x4b },   { OF_BTN_RIGHT,  0x4d },
    { OF_BTN_X,      0x1d },   /* CTRL  slot = punch */
    { OF_BTN_A,      0x38 },   /* ALT   slot = kick  */
    { OF_BTN_B,      0x2a },   /* SHIFT slot = jump  */
    { OF_BTN_Y,      0x39 },   /* Space            = duck */
    { OF_BTN_R1,     0x39 },   /* Space again: duck has two homes */
    { OF_BTN_START,  0x1c },   /* Enter  (confirm)   */
    { OF_BTN_SELECT, 0x01 },   /* Esc    (pause menu, docs/93) */
    { OF_BTN_L1,     0x3b },   /* F1 */
};
```

Every entry is handed to `night_key_apply(sc, make)` — the host-side model of the shipped handler at
0x0003fd40 (src/app/keymap.c), which is the *only* writer of the DAT_00063e88 held-bytes in either host.
The attacks deliberately use the modifier slots themselves (0x1d/0x38/0x2a) rather than the Z/X/C aliases,
because the original's own alias table (0x0003fd9d-0x0003fdd9) maps Z,V → 0x1d, X,B → 0x38, F,G,C → 0x2a:
the slots are the semantics, the letters are one way of reaching them.  No on-screen keyboard is needed and
none is drawn.

`of_btn()` takes a **button mask** (`OF_BTN_A = 1 << 4`), not an enum — the first version of this map passed
an index and compiled anyway.

## 1b. The layout moved; the measurement did not

The button-to-slot assignment above was changed from `A punch / B kick / X jump / Y duck` to
**`X punch / A kick / B jump / Y and RB duck`** (Xbox naming: the two attacks on the two buttons a thumb
alternates between most comfortably, duck — held longer than anything else in the game — on the button least
likely to be wanted elsewhere, plus RB, the only other button in reach of that thumb). Nothing about §3's
census or §4's pad-only route changes, because both were measurements of *which path arms which slot*, and
the paths and the eight slots are untouched: a button is still one `night_key_apply` call.

The map is now also one shared table rather than three copies. `src/app/keymap.c`'s `night_pad_sc[]` (declared
in `keymap.h` with a port-neutral `NIGHT_PAD_*` enum — no `OF_BTN` and no `SDL_GamepadButton` in sight) is the
layout, `test/test_f1.c`'s `test_gamepad_layout()` asserts each position arms the slot named above — including
that `Y` and `RB` are the *same* code, which is the property the pad paths depend on when they diff a SET of
codes rather than a list of buttons (docs/457: lifting one thumb off a two-home crouch must not stop the
crouch), and `pocket` (`g_pad_keys`), the browser port (`wasm/runtime/shell.js`'s `GP_MAP`) and the SDL host
(`src/app/main.c`'s `g_pad_btns`, which maps a physical button to a position and takes the code from the
shared table) all run it. It lives in a fixture because no machine any of this is built on has a controller
plugged into it — the table is the whole of what can be tested without hands.

One consequence the shared table makes visible on every host at once: a pump that sends an edge *per button*
breaks the duck the moment either of its two homes lets go.  pocket had that shape
(`night_key_apply(g_pad_keys[i].make, held)` per bit) and now runs the same union diff the browser port and the
SDL host do (`pad_set_apply`) — and it had been here before under a different name, for R1 and L1 both typing
F1: the removed `test_pad_share.c`, the 0.3.2/0.3.3 history.  Measured on the desktop host through
`NIGHT_LATCHDBG`: `500:600:y` alone writes the duck byte twice (make, break); `400:900:r1` plus `500:600:y`
still writes it twice — made when RB arrives, broken when RB leaves, so Y's release breaks nothing.
`NIGHT_PAD_DIRECT` deliberately keeps its raw per-span edges: it is the control path, and if a script ever
holds two buttons of one code, the difference between the two paths *is* the rule.

## 2. The scripted-pad harness (no pad required)

`NIGHT_PAD_SCRIPT=<from>:<to>:<name>[,...]` holds a button for a span of `pad_pump()` polls (one poll = the
8 ms pad period), names being `up/down/left/right/x punch/a kick/b jump/y or r1 duck/start/select/l1` or a
raw scan code.
`NIGHT_PAD_DIRECT=1` applies the *same* spans straight into `night_key_apply()` on the same poll grid,
bypassing the pad buttons.  The two runs may only differ in which path arms the slot — which is exactly the
claim to be tested.  (Parser note: nesting `strtok(",")` with `strtok(":")` corrupts the parse because
strtok has one global cursor — the first version silently produced "1 span"; the split is done by hand now.)

`NIGHT_LATCHDBG=1` (src/app/keymap.c) logs every latch write: `sc=… make=… idx=… before=…`.

## 3. Census: identical in the pad path and the key path

Script (13 spans covering every mapped button), 2000 frames, `NIGHT_SEED=1234`:

```
== pad     slots written:  2x01  6x1c  2x1d  2x2a  2x38  2x39  2x3b  2x48  2x4b  2x4d  2x50
== direct  slots written:  2x01  6x1c  2x1d  2x2a  2x38  2x39  2x3b  2x48  2x4b  2x4d  2x50
writes 26 / 26, identical sequence: True
first writes: sc=1c make=1 idx=1c before=0 | sc=1c make=0 idx=1c before=1 | ...
```

`idx` is the DAT_00063e88 index, i.e. the held-byte the shipped readers test — so the pad path arms the same
slots, in the same make/break order, with the same `before=` state as the key path.  The 6 × 0x1c writes are
three Start presses (make+break each); the 2-each entries are one press each of Select, punch, jump, kick,
Space, F1, up, left, right, down.  Nothing outside docs/91 §"the game's own key bytes" is touched: no fake
scan codes, no synthetic key queue.

## 4. Game-visible half: a pad-only route into stage 1

10 000-frame run, input only through the pad spans (Start presses at polls 5600…7400, then right/a/b/x/left/
up/a).  The music the game requests is the state proof, because the selection is driven by the game's own
schedule (docs/96):

```
cdda: index 1  lba=37262  sectors=3153  (42.0 s)   opening      (track#1)
cdda: index 9  lba=185120 sectors=27697 (369.3 s)  title        (track#2)
cdda: index 11 lba=223146 sectors=17856 (238.1 s)  stage-1 start slides (track#3)
cdda: index 2  lba=40415  sectors=19639 (261.9 s)  episode 1    (track#4)
```

docs/96 §1 lists exactly `index 1 / 37262 / 3153` for the opening, `index 9 / 185120 / 369 s` for the title
and `index 11 / 223146` for the stage-1 start slides, and docs/96 §3's episode table starts
`ep1..9 → 2,3,9,4,...` — so the run reached episode 1's stage through Start/A/B/X/d-pad alone.  Its witness
frame at 009980 (320x200) shows stage 1: the EXP/HP HUD with `8000`, the player and two enemies on the
station platform, the train behind the fence.

State-activity control, same 2600 frames, same seed, same build:

| run | distinct 40×30 signatures |
|---|---|
| no input | 728 |
| the 13-span pad script | 1502 |

The pad is not writing latch bytes the game ignores.

## 5. Two findings worth keeping

**Wall-clock pad scripts are unstable across runs.**  Re-running the same spans sometimes parks the game in
an opening↔title loop (`index 1 → 9 → 1 → 9 → 1`) because a Start press lands a few ticks before or after the
key-wait it is meant to satisfy.  That is the same failure docs/55 recorded for the SDL host, whose answer
was the `wait res` / `wait ds` anchors in `scripts/rig/drives/*`.  The Pocket host has no anchor support yet;
the episode sweep (task-8) needs one, and `cdda: index N` is the natural first anchor because both hosts
log the same game-visible event.

**Stage-phase palette: the Pocket host reproduces the SDL host exactly, and both differ from the original.**
The stage frames of the pad-driven Pocket run and of the SDL host run on `scripts/rig/drives/ep1_combat.txt`
have identical mean RGB:

```
pocket stage witnesses (frames 8371..9590): 255 / 97 / 97   (1514 distinct frames, all of them)
sdl    stage witnesses (frames 3204..5505): 255 / 97 / 97
NIGHT_PAL=cache  -> 255 / 0 / 0        NIGHT_PAL=cacheraw -> 255 / 0 / 0
```

while `analysis/refs/stage1-screenshot.png` (the original under DOSBox) is the grey-blue station scene.  So
this is a pre-existing defect of the shared port's stage palette model, not something the Pocket host
introduced — the Pocket host inherits it, and the pocket-vs-SDL comparison proves the inheritance is exact.
It is recorded here as an open item for the final pass (task-11); `0x3c8/0x3c9` DAC mirroring, the
DAT_633b8 staging table and the DAT_63e50 VBE cache are the three candidate sources and none of them is
currently producing the shipped stage palette.

## 6. Reproduce

```sh
cd pocket/src/night && make test                       # app_pc
cd ../../                                            # repo root
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234 NIGHT_CD_DIR=scratch/pocket/cd
S="100:104:start,300:304:start,600:604:start,1000:1040:right,1100:1104:a,1200:1204:b,1300:1304:x,1400:1404:y,1500:1504:select,1600:1604:l1,1700:1740:up,1800:1840:left,1900:1940:down"
NIGHT_PAD_SCRIPT="$S" NIGHT_LATCHDBG=1 ./app_pc --frames 2000 --rig-dump /tmp/pad_pad
NIGHT_PAD_SCRIPT="$S" NIGHT_LATCHDBG=1 NIGHT_PAD_DIRECT=1 ./app_pc --frames 2000 --rig-dump /tmp/pad_direct
diff <(grep '^latchwrite' /tmp/pad_pad.log) <(grep '^latchwrite' /tmp/pad_direct.log)   # no diff
```

Panel witnesses are capped by `NIGHT_PANEL_LIMIT` (default 12) and spaced by `NIGHT_PANEL_EVERY` (default 40
frames); raise both to look at a late phase.
