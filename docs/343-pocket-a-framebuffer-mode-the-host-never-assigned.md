# 343 — 0.1.29 broke its own fit refactor: a framebuffer mode the host never assigned

Core 0.1.30.  Three symptoms reported after 0.1.29: the new Screen Fit knob cannot be moved in Core Options, the
screen is glitched, and there is no sound at all.  The second and third are one bug in my own refactor, and the
card log states it 2 280 times.

## 1. What the log said

```
core 0.1.29: boot marker
fb native 640x480 8-bit offered/attempted (base 320x240 stride=320)
fit=0 (1:1 with 20+20 black bars) for the 320x200 game picture on a 320x240 panel
...
view=0 640x480 sig=ecd967f7 chg=1 pal=0
cdda req idx=1 lba=37262 sectors=3153 (42.0 s) -> disc track 2 slot 5
fb mode -> game 0x0 rc=-1 in 20us                 ; <-- here
video CHANGED: view=1 320x200 (was 0 640x480)
view=1 320x200 ...  perf pres=15695us max=78984us ... stages last m=16 p=191 c=15010 ... copy=77920
audio cdda=2970/19639/4 ... ring=0 head=171543    ; ring empty, head stuck
```

`fb mode -> game 0x0 rc=-1` appears **2 280 times**.  The host was asking openfpgaOS for framebuffer mode
**0 × 0**, every frame, forever.

## 2. The bug: a refactor that dropped an unconditional assignment

0.1.28 set the game framebuffer mode in one straight line inside the boot probe:

```c
g_fb_game = g_fb_base;                      /* 320x240, always */
```

0.1.29 folded that into `fit_apply()`, which has three choices to make and a mode request to avoid repeating
(41-53 ms per mode change, docs/338) — and I put the assignment **inside** the "the choice changed" branch:

```c
if (want200 != g_fb_game_is_200) { ...request the mode...; g_fb_game = ...; }   /* 0.1.29 */
```

With the new default `fit=0`, `want200` and `g_fb_game_is_200` are both 0, the branch never runs, and `g_fb_game`
stays zero-initialised.  Every transition from a 640×480 slide to a 320×200 game scene then asked for mode 0x0,
which the OS refuses (`rc=-1`), so the core stayed in the 640×480 mode while the host wrote a 320-wide, 200-row
letterbox picture into that surface.

The consequence chain explains all three symptoms except the menu:

* **The glitch.**  The SDK's own `interactdemo` (src/apps/interactdemo/main.c:420-436) describes exactly what a
  size mismatch does: the scanout is a Bresenham stepper, *"a mismatch makes it skip (fb>out) or repeat (fb<out)
  one source pixel per line, and because the accumulator carries across lines **the skip WALKS down the screen**"*.
  A framebuffer that does not match the scaler slot is not a slightly wrong picture, it is a shearing one.
* **The audio.**  Presents spiked to 78 ms (`max=78984us`, `copy=77920` where 0.1.28 measured 15 ms) and the CDDA
  ring refill runs in the same pump: `ring=0 head=171543` — the ring never got filled, so the music voices played
  silence, and the DSP transfers starved behind the mode-failing present.  Silence was a symptom, not a second bug.
* **The menu knob.**  Not this bug — see §4.

The fix is the assignment back where it belongs, plus a refusal to ask for a mode with no size ever again:

```c
g_fb_game = g_fb_game_is_200 ? g_fb_native_200 : g_fb_base;   /* every call, not only when the choice changes */
if (want200 != g_fb_game_is_200) { ...mode request... }
if (!g_fb_game.width || !g_fb_game.height) NLOG("fit: the game framebuffer mode is empty ...");
```

and at the present site, where the same request is repeated 60 times a second:

```c
if (!g_fb_game.width || !g_fb_game.height) {
    static int complained;                    /* say it once; do not fill a card log with 2 280 copies */
    if (!complained) { complained = 1; NLOG("fb mode -> game refused: ..."); }
} else { ...of_video_set_mode(&g_fb_game)... }
```

## 3. A witness for the class of bug, not just this instance

`geo_witness()` logs what the OS says it is scanning out, from `of_video_get_mode()` — the SDK API, never a raw
register — at boot and once a second while a 320×200 scene is up:

```
geo boot: applied 320x240 stride=320 | asked 320x240 | panel 320x240
geo view: applied 320x240 stride=320 | asked 320x240 | panel 320x240
```

`applied` vs `asked` is the whole failure mode of §2 in one line: 0.1.29 would have read
`applied 640x480 ... asked 0x0`, which indicts the host immediately instead of leaving it to a screenshot.  The
desktop fit witnesses still separate the two modes after the refactor:

```
fit0 0002..0004: rows 20..219  duprowpairs=38-39    ; 20+20 black bars, 200 rows of game
fit1 0002..0004: rows 0..239   duprowpairs=40-41    ; panel filled, 40 doubled rows (200 + 40 = 240)
```

## 4. The menu verdict: the interact read answers garbage

0.1.29 was the first build that kept the ecall's error half (`of_interact_probe`), and its card log answers the
question docs/342 asked:

```
interact[0]: err=-419071091   val=00000000     interact[1]: err=0 val=4f40446d
interact[2]: err=0            val=29138574     interact[3]: err=0 val=52a1cde1
interact[4]: err=-1819641228  val=00000000     interact[5]: err=0 val=45d1f560
interact[6]: err=-277204179   val=00000000     interact[7]: err=0 val=73e157d5
interact[8]: err=0            val=5986c511
bridge decoded: music=0 fit=-1 lcd=00000000
```

Neither half is meaningful.  Those "errors" are not `of_error.h` codes (which are small negatives, -2 being
`OF_ERR_NOT_SUPPORTED`), and not one value is a value any of the declared option lists could produce.  Nothing
moved when the knobs were moved.  Conclusion: **this openfpgaOS build does not answer an app's interact read**,
so every app-facing knob is decorative, including the Screen Fit knob the report was about.

0.1.30 therefore declares **no app knobs** — Core Options keeps only the SDK's reserved entries and the
firmware's own Pocket LCD, which the bitstream acts on without the app:

```
7 vars: Enable Analogizer, SNAC Adapter, SNAC Controller Assignment, Analogizer Video Out,
        Video H Offset, Video V Offset, Pocket LCD
```

It is one flag to put them back (`APP_INTERACT_KNOBS` in `scripts/pocket/mk_dist.py`); `of_save.c` keeps both
decoders behind `HAVE_MUSIC` / `HAVE_FIT` so a knob reappears with the flag alone, and
`check_interact.py` now checks agreement in both directions — no knob in the JSON without an index constant, and
no `OF_IDX_*` in the generated header pointing at a variable that is not declared:

```
[PASS] no app knobs declared, and no dangling OF_IDX_* in the generated header
       (openfpgaOS does not answer the interact read -- docs/342)
```

What a player can actually change today, all of it independent of the bridge:

| setting | route |
|---------|-------|
| panel fit | **L1 + B** on the pad, live (letterbox → stretch → scaler), or `night.ini` `FIT=0\|1\|2` |
| music volume | `night.ini` `MUSIC_VOL=0..255` (deployed 128) |
| sfx volume | `night.ini` `SFX_VOL=0..255` |
| display | `night.ini` `SCREEN=0` game (deployed), `SCREEN=2` game + log overlay |
| CD-DA / pad | `night.ini` `CDDA_OFF=`, `PAD_OFF=` |

`SCREEN` is pinned to 0 rather than following the Pocket LCD read for the same reason: a garbage word with bits
16-17 set could have moved the app to the console on its own.

## 5. The screenshot, measured (`Memories/Screenshots/20261105_200401.png`)

The capture is **640×480**, which is itself the first fact: the core was still in the native 640×480 framebuffer
mode at the moment the picture was taken.  Decoding the PNG row by row (left half vs right half of the same row,
mean absolute difference over 140 sampled pixels):

| row band | mean abs diff, left vs right half |
|----------|--------------------------------|
| 0-119    | **26**  |
| 120-239  | 82 |
| 240-359  | 80 |
| 360-479  | 87 |

with `row 40 left` against `row 41 right` differing by only **7**, and **20 all-black rows** in the capture — the
letterbox bars, present in the surface and compressed by the mismatch.

That is the arithmetic of writing 320-byte rows into a 640-wide surface at a 320-byte stride: display row *k*
receives source row *2k* on its left half and source row *2k+1* on its right, so

* the 240-row panel picture (bars included) collapses into the **top 120 display rows**, two source rows wide —
  which is why the two "copies" at the top of the screenshot are not copies but consecutive rows, and why they
  differ by 26 rather than 0;
* the remaining 360 display rows are **stale bytes from the previous 640×480 slide**, read out under the game's
  palette, which is the multicoloured mess in the bottom half.

So the glitch had a shape, and the shape was the stride.

0.1.31 makes the class of bug unreachable rather than merely unlikely.  The host now caches what the OS says it
applied (`applied_refresh()` after every mode request, plus `geo` logging it), and the blit is one loop over the
rows the surface *has*, each written at the stride the surface *has*, with the fit choosing which source row a
panel row shows:

```c
uint8_t *d = fb + (size_t)y * stride;                 /* the stride the OS reported, not PANEL_W assumed */
if (g_fb_game_is_200) sy = y;                                     /* 1:1 into a 320x200 surface */
else if (fit_wants_stretch()) { grp = y / 6; r = y % 6;          /* five source rows fill six panel rows, */
                                sy = 5 * grp + (r < 5 ? r : 4); } /* the fifth one duplicated             */
else sy = (y >= BAR_ROWS && y < BAR_ROWS + GAME_H) ? y - BAR_ROWS : -1;
```

Byte-for-byte identical to the three hand-written copies it replaces when the surface is the 320×240 base it
expects — all twelve panel witnesses, fit0 and fit1, hash-match the pre-refactor run — and no longer capable of
putting a 320-byte row at a 640-byte offset.  The 640×480 → 320×240 downscale keeps its fast word loop only when
the surface really is the base one, and says so once a second when it is not.

## 6. Reproduce

```sh
cd pocket/src/night && make test && make            # app_pc + the RISC-V ELF
cd ../../
python3 scripts/pocket/mk_dist.py | tail -1 && python3 scripts/pocket/check_interact.py
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234 NIGHT_CD_DIR=scratch/pocket/cd
for f in 0 1; do NIGHT_FIT=$f NIGHT_PANEL_SHOT=/tmp/g$f ./pocket/src/night/app_pc --frames 1500 >/tmp/g$f.log 2>&1; done
# rows 20..219 (fit0, bars) vs rows 0..239 with 40 doubled row pairs (fit1)
make test                                            # fixture suite: ALL PASS
# on hardware, after this build:  grep -E "^geo |fb mode|audio cdda=" Saves/night/common/nightlog.sav
#   want:  geo view: applied 320x240 stride=320 | asked 320x240 | panel 320x240
#   and no "fb mode -> game 0x0" at all
```

## 7. Open

* Whether the picture is right on hardware after this fix — the `geo` line plus one screenshot.  If `applied` and
  `asked` agree and the picture is still wrong, the suspect moves to the scaler slot rather than the host's mode
  choice, and `FIT=2` (the core's declared 320×200 slot) becomes the interesting measurement.
* Whether `of_video_set_display_mode()` and the Pocket LCD can be reconciled without the interact read: the
  bitstream switches the display on its own, so a Terminal choice in the menu while the host pins
  `OF_DISPLAY_FRAMEBUFFER` is a state the port should describe honestly.
* The interact read is the blocker for every menu setting.  If openfpgaOS answers it in a later OS image, the
  knobs come back with one flag; if it never does, the honest product statement is "settings live in night.ini".
* 0.1.31 is now deployed, so the hardware question is whether `geo view` reads `applied 320x240 stride=320 | asked 320x240` with no `fb mode -> game 0x0` anywhere, sound restored, and L1+B moving the fit.
* My own process note, since this is the second core-breaking edit in three versions: both breakages came from
  changing *when* something was assigned rather than *what* — 0.1.27 moved a value into a register, 0.1.29 moved
  an assignment behind a condition.  A build that only compiles and only runs on the desktop shim has no way to
  catch either; the `geo` line, the bounded refusal, and the boot marker exist so the next such mistake names
  itself in the log instead of being inferred from a photo.
