# 97 — Slide-show auto-skip = fn_00035dfc, the key-wait gate, was stubbed as a no-op

Playtest item 3: "Stage slideshow skips automatically: different from original behavior."

## 1. What the original does (measured in the container)

`scripts/playtest/probe_orig_slide_pacing.py` (fresh boot; calibrates the data base with the
playtest_orig_drive.py anchors; live reads of DAT_53c44 + video mode over the REST API):

- one Enter at the title starts the show (playing window switches to 223146/238 s = track index
  11, VESA 480);
- with NO further keys the card region stays put for the whole probe: LBA=223146, height=480 at
  t=135 s (`/work/api/ss4.txt`) and again in `/work/api/ss5.txt`: 240 s of idle samples all
  `h=480 LBA=223146`, and one further Enter did not end the stage-card/room chain by itself —
  cards are consumed one key at a time (`hold1.txt` drive run earlier today reached the room
  (LBA 40415, mode 200) after three Enters, matching g_tab_52f2c[1] = 3 start slides).

So: each "%ds%d.pcx" card is held until a game key is pressed (ESC aborts the slide-show into the
stage card); the stage card itself is then a *time*-bounded hold and needs no key.

## 2. The shipped gate (Ghidra)

`scripts/ghidra_scripts/DumpKeyWait.java` disassembles 0x00035dfc (0x35e14-0x35eb6):

```
A) while any of DAT_63ec1/63ea4/63ea5/63ec0 is held: spin (0x35e3c -> 0x35e14)
B) then spin; ESC latch DAT_63e89 -> spin until it clears, return 1 (0x35e68..0x35e7a)
C) a press of any of the four -> spin until all are released (0x35e85..0x35ead), return 0 (0x35eaf)
```

The four bytes are the MEASURED press latches from docs/92: 0x63ea4 enter, 0x63ea5 punch (CTRL/Z/V),
0x63ec0 kick (ALT/X/B), 0x63ec1 space. (No 0x63eb2/0x63ebe here; those two shift latches appear in
the *shipped* release-drain set of fn_038cde at 0x38edc-f21, where the port already uses them, and
they are themselves measured bytes now.) The slide loops call it at 0x3b0ec/0x3b272/0x3b4e1.
The stage card hold that follows was also confirmed shipped-time-based: 0x3b166-0x3b19e runs 0xc00
iterations of the fn_042958(1) thunk = delay_units (0x4564f: waits (n·DAT_64000+500)/1000 DOS-clock
reads — self-calibrating pacing, docs/62's g_delay_unit_us), with only the ESC latch (DAT_63e89)
able to cut it short.

The port had `int fn_00035dfc(void) { return 0; }` ("degenerate spin") — every card fell through
immediately. That is the auto-skip.

## 3. Fix + evidence

fn_00035dfc ported verbatim (A/B/C with sys_key_spin, comment carries the addresses); f.h updated.
test_f1 `fn_035d5d + fn_035dfc` rewritten to pin the semantics with the scripted key timeline:
held key drained first, press of a game key -> 0, ESC -> 1, latches left clear — ALL PASS (3067).

Route `scripts/playtest/route_slide_keyed.txt` (content-anchored; note fn_03a224's own release
drain eats the first title press, as shipped) through the rebuilt port
(`/tmp/dbgauto/port_sk14.log`, `NIGHT_SLIDEDBG=1` witness prints, env-gated like NIGHT_KEYDBG):

```
cdda: track index 11 ...            one press started the show at t=45.5s
slide: card 1/3 wait                1s1 rendered 47.6s
slide_hold_b  t=54.6 res=1s1        HELD 7 s with no key            (one press) 
slide: card 2/3 wait                1s2 from 56.1s
slide_hold_c  t=63.1 res=1s2        HELD 7 s with no key            (one press)
slide: card 3/3 wait                1s3 from 64.6s                  (one press at 90.0s)
room_final2   t=94.6 res=stage1.pcx + cdda track index 2 (room track) -- the stage card
                                    advanced on its own time-bounded hold, no key.
```

Reproduce:

```sh
cd port && make -j8 test/night && \
SDL_VIDEODRIVER=dummy NIGHT_SLIDEDBG=1 ./test/night \
  --script ../scripts/playtest/route_slide_keyed.txt --shots /tmp/dbgauto/port_sk14 \
  --no-audio --frames 13500
docker cp scripts/playtest/probe_orig_slide_pacing.py dbga:/work/ && docker exec dbga \
  bash -c "cd /work && NIGHT_EXE=/work/game/NIGHT.EXE DOSBOX_BASE=http://127.0.0.1:8386 \
           DBA_TOKEN_FILE=/work/.token python3 probe_orig_slide_pacing.py"
```

## 4. Residual notes

- The port's scripted key scheduler can land a down+up pair inside one spin when the sim clock
  catches up (150 ms presses during a busy wait); route presses are 2.15 s wide so the gate sees
  them; a harness-side press-width guarantee is a polish item, not guest behavior.
- DAT_64000 in the container measured 404522 (calibrate_clock counts INT 21h/2Ch reads per second
  on the running machine) — the delay_units model (`g_clock_ticks`, `g_delay_unit_us=500`) keeps
  pacing per docs/62; the stage-card hold matched (room ~2 s after the last slide press).
- The attract demo legitimately drives the slide gates with its own injected key stream; this route
  presses real keys, and the gates hold between presses.
