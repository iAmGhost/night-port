# The original runs the stage at ~50 screen updates/s, not 70 — and that rate does not depend on CPU speed

## The question

The player's report: **background animation in stage play is noticeably slower** on the Pocket.

Where that left us earlier in the day: `fn_038098` (the room pump) gates its entire draw branch on
`n = 0x1ea / DAT_53104`, and `DAT_53104 = 0x46`, so **7 PIT ticks per drawn frame = 70 fps, 14.28 ms**.
Every animation step — the background plane `fn_03ce0b`, the entity sweeps, the gauges — lives inside that
branch, so the tick gate does not pace the animation, it **caps** it: fewer drawn frames per second is
slow motion, not dropped frames. Two card logs agreed on ~17.3 ticks consumed per drawn frame against a
design of 7, which read as "the port runs at ~40 % of its design speed".

That number was wrong in the only way that matters: **it compared us to a rate the shipped game never
achieved either.**

## Why 70 fps was the wrong bar

70 fps is what the code asks for. Nobody ever measured what the original put on the screen. If the
original's own hardware could not fill the gate — a 64 KB window write at ~4 ms a pop on a 486 — then the
original was in slow motion against its own design too, and the port must match **that**, not the spec.
Chasing a design number we cannot verify is how a port gets optimized for a spec nobody ran.

So the original went into the measurement before any optimization did.

## Method

`dosbox-automation`'s ZMBV capture is lossless, and its recorder runs at the emulated display refresh —
**70.09 fps in VGA mode 19h** (measured, printed by the script). So a recording is a 70 Hz census of what
the screen actually showed, and diffing consecutive frames counts the picture changes.

Frame *polling* was never going to work here: polling the API from this host runs at ~8 Hz, which would
have "measured" 8 fps and called it the game. The recorder rate **is** the measurement ceiling, and both
scripts print it.

The change threshold is swept rather than chosen, because a 1-pixel scroll and a full-page composite are
different animals. In stage play the answer turned out **threshold-insensitive from 4 to 16 grey levels**
— because the game writes whole pages (≈1 000–10 450 pixels change on an updating frame), which is itself
evidence about the code path.

Scripts: `scripts/rig/orig_anim_rate.py` (drive to gameplay, record, decode, sweep thresholds) and
`scripts/rig/orig_anim_cycles.py` (sweep emulated CPU speed). Drive is deliberately minimal: one Enter at
the attract screen, then Enter whenever a 640×480 slide or card sits still ≥1.2 s (docs/447 §6 — the
stage cards are key-gated in the original too), plus a right-arrow nudge during gameplay.

## Measured

Attract demo, cycles=max, 12 s:

| threshold | changes | per second | median px changing |
|---|---|---|---|
| any single pixel | 343 | 28.5 | 10 450 |
| ≥4 grey levels | 343 | 28.5 | 10 450 |
| ≥16 (visible) | 343 | 28.5 | 10 450 |
| ≥64 (large) | 225 | 18.7 | 3 905 |

Stage play (entered with Enter, cards advanced, right-arrow nudged), 12 s, cycles=max: **46.3 updates/s**,
median spacing **1 recorder frame**, longest still run 0.14 s.

Emulated CPU speed, three separate emulator restarts with `cycles = fixed …` in the conf, 10 s each, all
in 320×200 stage play:

| cycles (DOSBox IPS) | picture updates/s | median spacing |
|---|---|---|
| 12 000 (386DX-33 class) | **52.5** | 1 refresh |
| 40 000 (486DX2-66 class) | **55.6** | 1 refresh |
| max (unlimited) | **48.5** | 1 refresh |

**A ~30× CPU range moves the rate by less than the run-to-run spread.** The original's stage animation is
therefore **clock-paced, not CPU-paced**: it is not waiting on hardware it cannot outrun, it is waiting on
its own clock, and it lands at ~50 screen updates/s regardless.

## What this says

* The bar is **~50 updates/s**, not 70. The design gate is never filled by the original either.
* The Pocket draws **~29** room frames/s → the port is at **~57 % of the original's measured speed**.
  So the report is real, and now it is quantified against the original rather than against a constant in
  the code. (My earlier "~40 % of design" was measuring the wrong thing.)
* The frame budget to reach 50/s is **20 ms**. The card log says ~34.9 ms: display 4.2, audio 5.9, disc 4.8,
  game compute ~20. Reaching the original is therefore not a single fix: the host buckets have to roughly
  halve *and* the game's own compute has to come down.
* It is not the display knobs. A scrolling background never hashes equal, so `PRESENT_SKIP` and friends
  never engage; and the tick source is healthy (491/s steady in every second of both boots).

## Instruments added with this measurement

* `pace` on the 1 Hz log line: room `calls/s`, `drawn/s`, ticks/s, `n`, and the animation rate against
  **two** references — design (70) and the original's measured 50 — plus gate drift.
  `presents/s` is neither `calls/s` nor `drawn/s`; every animation statement before this was an inference
  from presents.
* `g_room_calls` / `g_room_drawn` in the shared layer (counters only, no behaviour).
* `mix=` — the mixer drain as its own bucket. It runs on **every pump entry** (~10 per drawn frame,
  ~287/s in a stage) while the byte-ring drain is gated to the tick; it had always been hiding in `rest`.
* `ROOM_PROFILE` (`night.ini`, default 0) — times the draw branch's sections (sort / bg / sweep / panel /
  flip) on the device. A host-side profile cannot substitute: on an Apple Silicon host the whole draw
  branch costs ~0.03 ms/frame and 40 % of samples land in `fn_0347e3` (the keyboard/frame pump) plus 37 %
  in `mach_absolute_time` — that is a machine **waiting**, not working.

## How this measurement nearly lied, five times

1. **`protected_mode` in the cycles PUT was silently ignored.** The first "sweep" printed four rows —
   45.2 / 61.5 / 57.9 / 54.2 updates/s — from **four identical configurations**. The response echoed
   `protected_mode: null` and only that gave it away. The API wants `{"cycles": N}`, refuses in legacy
   mode ("set cycles via config"), and the correct value only takes effect through the conf plus a
   restart. Both scripts now read the value back and skip the row if it does not match.
2. **`ffprobe -of default=nw=1:nk=1` does not emit fields in the order requested.** My parse assigned
   `frames=320, duration=200` (those were width and height) and printed a confidently wrong 1.6 updates/s
   three times running.
3. **"320×200" does not mean gameplay.** The attract demo is 320×200, and so is the stage; the intro and
   the stage cards are 640×480. Two recordings captured the attract and a card before I separated them by
   requiring a 640×480 *after* the Enter, and requiring the 200-mode to be **sustained** (a card mid-run
   resets the warmup).
4. **`sample` attached to bash, not the app.** `pgrep -n -f` fixed it; the first profile was a bash
   recursion tree.
5. **Cumulative counters read as per-second**, again — this session, twice, in two different tools. The
   check that caught it was that two methods disagreed (44.6 presents/s from cumulative snapshots vs 29/s
   from per-second deltas); the cumulative figure was wrong because `view=` only prints on change, so the
   window swallowed frames from before the geometry flipped.

## Reproduce

```sh
# container: dosbox-automation + the CD image, webserver on 127.0.0.1:8386 INSIDE the container,
# so a relay is needed on the published port (8387 here). No pkill/ps in this image -- scan /proc.
TOKEN=$(docker inspect dbga --format '{{range .Config.Env}}{{println .}}{{end}}' | grep DOSBOX_API_TOKEN | cut -d= -f2)
docker exec -d dbga bash -c 'socat TCP-LISTEN:8387,fork,reuseaddr,bind=0.0.0.0 TCP:127.0.0.1:8386 >/dev/null 2>&1'

python3 scripts/rig/orig_anim_rate.py --token "$TOKEN" --base-url http://localhost:8387 \
    --secs 12 --walk --out scratch/pocket/orig_anim          # drives into a stage and records
uv run --with numpy python3 scripts/rig/orig_anim_rate.py …  # or diff scratch/pocket/orig_anim/frames.raw

# cycles sensitivity needs an emulator restart per point ([cpu] cycles = fixed N), see /tmp/anim_sweep.sh
# shape; capture.avi -> ffmpeg -f rawvideo -pix_fmt rgb24 -> numpy diff.
```

On the port side, `pace` and `mix=` appear in `nightlog.sav` from 0.2.25; `ROOM_PROFILE=1` adds the
`room:` line naming the draw branch's sections.
