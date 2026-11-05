# 379 — the original-side capture works; the first content score against it says 0.559, and the reason is known

docs/377 §6 ended with "the launch is the flaky part".  That was wrong, and the real cause is worth more than the
capture: two defects, both in the harness, one of them mine.

## 1. Why the API reported mode 3 while the game was changing modes

```
20:05:51 [INFO] webserver: starting HTTP REST API on http://127.0.0.1:8386
20:05:51 [WARN] webserver: failed to bind to 127.0.0.1:8386, another instance is probably using the port; the API is off
```

While the log of the *game-running* instance cycled 101h/13h every ~40 s, `/api/v1/video/frame/info` answered
`720x400 bios=3` for 60 straight seconds.  It was answering a **different, older dosbox-automation instance** parked at
the DOS prompt.  `docker exec dbga63 bash -c 'pkill -f dosbox-automation'` did nothing because **`pkill` is not in the
image**, and I had been sending its failure to `/dev/null`; every "restart" left the previous emulator alive and
holding the port, so each session added another instance and the API served whichever bound first.

The clean recipe, which is now what every capture uses:

```sh
curl -X POST .../api/v1/dosbox/shutdown                                    # ask the API-holder to exit
docker exec dbga63 sh -c 'for p in /proc/[0-9]*; do
    tr "\0" " " < $p/cmdline 2>/dev/null | grep -q "dosbox-automation -conf" && kill -9 ${p#/proc/}; done'
# start exactly one instance, then CHECK the log before trusting anything:
grep -cE "failed to bind" run71.log        # must be 0
```

After that the same queries returned the truth, matching the log mode for mode:

```
info: 320x200 pitch=2560 bios=19      ;  info: 640x480 pitch=2560 bios=257
```

Note that `status.is_shell` stayed `true` throughout, so that field is not a usable "is the game running" anchor.

## 2. `docker exec` without `-i` posts an empty body

The container wrapper had to forward request bodies, and it did:

```
cmd = ["docker", "exec", a.name, "curl", ..., "--data-binary", "@-"]     # input=data
```

`docker exec` does not attach stdin unless `-i` is given, so curl read an empty stdin and posted **nothing**.  The
emulator answered `HTTP 400 {"error":"Malformed request body"}`, which looks exactly like a badly-shaped payload.  The
tell was that the capture itself worked -- 123 boot frames were written -- and only the key injections failed:

```
RuntimeError: HTTP 400 on POST /api/v1/input/sequence: { "error": "Malformed request body" }
done: 434 saved frames in /tmp/orig71        # after adding -i
```

## 3. The capture is real content

`drive_orig_episode1.txt` ran end to end through the container, anchored and labelled:

```
shot esc_win2_37  label=title1  sim=0.0395 mode=320x200 -> 0429_esc_win2_37.png
done: 434 saved frames in /tmp/orig71
PNG size histogram: {720x400: 15, 640x480: 182, 320x200: 233, 640x200: 2}
```

182 frames of the 640×480 cards/slides and 233 of the 320×200 gameplay/menu -- the two modes this port has to match,
both present.  `scripts/rig/pngs_to_frames.py` converts such a capture into the rig's `frames.bin` signature layout
(capture_original's 40×30 grid formula and record layout), which is what lets `diff_screen.py` read it.

## 4. The first Pocket-vs-original content score: 0.5590, and what that number means

```
$ ./pocket/src/night/app_pc --frames 12000 --script scripts/playtest/route_pause_menu.txt --rig-dump /tmp/port71
script: 12 key spans applied on the direct key path, 23 non-key or unknown lines skipped
$ python3 scripts/rig/diff_screen.py --orig /tmp/orig71sig/frames.bin --port /tmp/port71/frames.bin
orig frames=434 distinct_pictures=434 rate=8.0/s
port frames=12001 distinct_pictures=2049 rate=54.5/s
aligned pairs=434 route score=0.5590 (1 = identical)
divergent runs (score < 0.88): 4
  orig t=2125..2875 ms   (0.8s)  worst 0.628
  orig t=4625..15250 ms  (10.6s) worst 0.411
  orig t=16000..31625 ms (15.6s) worst 0.249
  orig t=32000..54125 ms (22.1s) worst 0.605
pacing check: picture changes per second orig=8.02 port=9.30 ratio=0.86
```

What is trustworthy here: the original side is content (434 distinct pictures, no stuck frames), the pacing is close
(ratio 0.86, inside docs/82's tolerance), and the alignment ran across the whole route.

What is **not** a fidelity verdict: 0.559.  The Pocket run's keys were wall-clock, because `app_pc` honours only `key`
lines -- the `wait res logo.pcx` anchors that keep the SDL host's route in phase with the game (docs/55) are skipped
and reported as skipped.  So the two sides visit the same scenes at different moments, and a monotone alignment over
mismatched phases produces exactly the reported shape: four long divergent windows rather than a uniformly poor
score.  The score measures "were they showing the same picture at the same time", and one side was not keyed to the
game at all.

## 5. The step that would make this a fidelity number

Give `app_pc` the anchor vocabulary: `wait res <name>` parked on the game's own resource-lookup counter (the shim
already reports resolver and load-edge activity, and `--script` already parses the file).  With both sides anchored,
the same route on both sides becomes comparable moment by moment, and `diff_screen.py` -- which is now proven to read
both sides -- can answer the question the objective actually asks about the picture.

Card state is unchanged by this work (0.1.67, harness-only commits), fixture suite `== ALL PASS ==`, `src/f`
untouched.
