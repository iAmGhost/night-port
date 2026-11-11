# 456 — The constant stutter came back, and the card had never booted the build it blamed

A feel report, a regenerated `night.ini`, and two card logs that measure the same machine -- the blame was
wrong twice, and the only thing that saw it was the tool that asserts a boot's own banner.

**Build on the card:** `0.3.3` (deployed 01:35, ELF md5 `7a757e7bab7fbddc46b7b55711b025ff`, verified against the build tree).
**Logs:** `scratch/pocket/logs/VOID-nightlog-0.3.2-20261112-015415-0.3.2-era-card-state.log` (the run being complained about),
`scratch/pocket/logs/nightlog-0.3.3-20261112-015709-0.3.3-stutter-gone.log` (the run that stopped the complaint).

> *"I can see stutter is very constant — like every second. did the ini change do it?"*

"Like every second" is a specific accusation, because almost everything this core prints is on a one-second cadence.
And the ini question was fair: I had just deployed, and a deploy regenerates `night.ini` from the dist tree — which
means it silently reverts whatever the card's last owner had set (docs/428), and that is exactly the kind of change
that lands on feel.

## 1. What the deploy actually changed on the card

| On the card | What happened to it |
|---|---|
| `Assets/night/common/night.elf` | replaced; md5 `7a757e7b…` == the build tree's |
| `NIGHT.RES`, `track02-15.bin`, `os.bin`, `bank.ofsf` | md5-identical to `extracted/`, so `deploy.sh` skipped them — the game's data did not move |
| `Assets/night/common/night.ini` | **regenerated from the dist tree** — the only value delta the log can even see is `LOG` 1 → 0 |
| `Settings/iamghost.night/Interact/` | cleared by `deploy.sh`, so the menu values come back from `night.ini` (`MUSIC_VOL` 255, `FIT` 1) |
| 14 AppleDouble sidecars | removed |

So the ini was the only suspect with a changed value, and it deserved to be tested rather than argued with.

## 2. Before I changed anything, the card's own log said the boot was void

`scripts/pocket/logsave.sh` archives the card log and asserts the version the core stamped in its own boot marker
against the card's `core.json` (that guard went in because 0.2.24 bit the same way on 2026-11-11). On the run I was
being handed, it refused:

```
archived VOID-nightlog-0.3.2-… (126883 B, 1 boot marker(s), core 0.3.2)
VOID BOOT: the log's banner says core 0.3.2 but the card's core.json says 0.3.3.
The machine did NOT run the deployed bits.
```

**The stutter was felt on a 0.3.2 boot while the card was already carrying 0.3.3.** A warm resume relaunches the
cached app, and a card that was not cleanly ejected reads old bytes to the device — either is enough, and neither
leaves a mark on `core.json`. That is the whole reason the log exists in two files: the one the complaint was
about, and the one the complaint blamed.

The ini defence is in the same file, and it also clears the ini: that boot's own settings line reads

```
settings: music_vol=255 sfx_vol=255 cdda=1 pad=1 tick_hz=490 screen=0 file_log=1
```

so the stutter era's card **already had `LOG=1`** — the deploy's 1 → 0 was not the difference the player felt, and
the boot that ended the complaint ran `LOG=1` again. `DISC_BATCH=27` (the smooth era shipped 12) is likewise
exonerated by `mean_sector`: 2 133 µs per sector in the old boot and 1 767 µs in the new one, with
`dry=0 audio_low=0` in both. Same reads, same feed health.

## 3. The two boots, measured against each other

The 0.3.2 boot cannot be an A/B basis (its bits are not the card's), but its numbers still describe what the
player was living with, so here they are side by side with the clean boot — same scene order, same instruments,
`gap worst` sampled once per second:

| | the complained-of boot (core 0.3.2, `STALL_MS=400`) | the clean boot (core 0.3.3, `STALL_MS=120`) |
|---|---|---|
| one-second windows, `gap worst` | min 0 / **median 51 ms** / p90 488 / max 2984 ms, n=56 | min 0 / **median 52 ms** / p90 497 / max 3012 ms, n=47 |
| `STALL` lines | 7: `opening.fli`, `title1.pcx` ×2, `1s1.pcx`, `1s2.pcx`, `stage1.pcx` ×2 | 12: `logo.pcx`, `opening.fli` ×2, `title1.pcx` ×6, `1s1.pcx` |
| frames / wall | 2 188 / 61.4 s = **35.6 fps** | 1 830 / 52.3 s = **35.0 fps** |
| `split` of the wall | `pres 18 786 disc 8 396 aud 10 093` **`rest 23 953`** (39%) | `pres 21 633 disc 5 430 aud 6 756` **`rest 18 378`** (35%) |
| disc feed | `mean_op=28 ms max_op=121 ms` `deferred=9848/10126` `ahead_min=0.70 s` `dry=0` | `mean_op=25 ms max_op=125 ms` `deferred=5078/5259` `ahead_min=0.76 s` `dry=0` |
| the mixer, asked directly | `silent=0ms laps=0 vfull=0 rate=1.000x` | `silent=0ms laps=0 vfull=0 rate=1.000x` |

Read the middle row of both: **neither boot contains a hitch of one second while the game is running.** Every gap
above 400 ms names a picture being swapped in (`logo.pcx`, `opening.fli`, `title1.pcx`, `1s1.pcx`) — the
stage-entry tail docs/433 was already chasing, whose p99 was 1 380 ms and max 2 998 ms. The 12-line count versus
7 is my `STALL_MS=120`, which asks about the 120-400 ms hitches the shipped threshold never mentions. So:

* The instrument that a "constant stutter every second" would show — `gap worst` climbing to hundreds of ms in
  every window, or `silent`/`laps`/`vfull` leaving zero — stayed flat in both boots.
* The code difference between the blamed build and the innocent one is `9b02d04`, the revert of the R1-also-types-F1
  binding: −25 lines in `main_of.c` (a scan of a 12-entry table inside `pad_pump`), −30 in `of_pad_share.h`,
  −70 in its test. Nothing in that set is a second of anything.

That is the honest state of the claim: **the stutter is gone, and nothing I changed is why.** The boot that ended
it is the first boot of the bits the manifest named. It is a weaker statement than "fixed", and I would rather
write the weak one.

## 4. Two things the instruments still cannot see, and which this report cannot settle

1. **`rest` is 35-39 % of the machine and it has no name.** docs/429 ended on exactly this open step ("attribute
   `rest`: game compute / resource reads / audio pump / log writes"), and a degraded exFAT — the state a card
   left half-flushed is in — would spend its cost precisely there, invisible to `disc=` (which counts the
   port's own track reads) and invisible to `pres=`. 20 log lines a second against a filesystem that is
   re-reading its directory is not a wild guess, it is the only unnamed bucket big enough to hide it.
2. **35 fps with 15 ms and 50 ms frames arriving alternately *looks* like stutter**, and no line in the log
   separates that from a stall. `gap worst` reports only the worst gap per window; docs/433's `choppiness`
   percentiles were the right shape and are not in this build's output either. The original runs the stage at
   ~50 screen updates/s (docs/450), so a 35 fps port is a portrait of judder even when nothing is wrong.

Next instrument, if this comes back: a per-window histogram of inter-*frame* gaps (buckets at 20/40/80/160 ms)
printed on the `gap worst` line. Cheap, no new cadence, and it answers "constant" and "every second" with one
number each. Not built today, because the thing it would have to be A/B'd against does not exist yet.

## 5. What got changed today, and what deliberately did not

* `scripts/pocket/logswitch.sh` takes `--stall N` and prints the whole shape
  (`shape: STALL_MS=120 PRESENT_CAP_MS=0 DELAY_US=0 DISC_BATCH=27 TICK_HZ=490 DIAG=0 LOG=1`) before its verdict.
  A stutter hunt is now one command — `logswitch.sh on --stall 120` — instead of a hand-edit that the next deploy
  quietly erases (docs/428), and a quoted boot always carries its thresholds (docs/452's rule).
* `pocket/INSTALL.md` §2-3: the full-power-off and clean-eject rule with the reason; `logsave.sh` as the way to
  report a run (it is the tool that catches a void boot); the version grep that still looked for `0.1.x`; the
  `DISC_BATCH` table that still said 12.
* **No ini value was promoted as "the unstutter default", because no value was shown to unstutter anything.**
  The card is back in the shipped shape — `LOG=0 DIAG=0 STALL_MS=400` — and `night.ini` in the dist tree is
  unchanged.

## 6. Rules this buys

1. **A feel report carries the banner.** Run `logsave.sh` after the boot you are complaining about. If it says
   `VOID BOOT`, there is no report yet: full power off (not sleep), clean eject and re-insert, boot, archive.
   Ask for the banner before spending a bisect.
2. **"The ini changed" is a checkable claim, not a hypothesis.** The card's log prints its settings line; if both
   boots say `file_log=1`, the ini is out of the dock. Here it left the dock in one grep.
3. **A baseline is a file, not a memory.** For 0.3.3 on `LOG=1 STALL_MS=120 DIAG=0`: 35.0 fps, `gap worst`
   median 52 ms / p90 497 ms, all stalls on picture changes, mixer `silent=0 laps=0 vfull=0`. The next "did it
   get worse" is answered by diffing against that, and only by diffing against that.
4. **`rest` stays on the list.** It is the largest thing the log does not name and the only place left for a cost
   this big to hide.

## 7. The fade is the stutter, and the change that came out of it

The report sharpened while this doc was being written: not "everywhere", but **"it fades in, and the screen pauses
almost every second during the fade-in"** -- on the *title* card, the 640x480 one, and with the music never
hitching. That is the plateau this doc's §3 measured and mislabelled as unremarkable:

```
fb mode -> native 640x480 rc=0 in 49757us        <- the mode switch alone costs ~50 ms
stages last m=16 p=211 c=23914 f=215 x=242        <- copy stage 23.9 ms, flip 0.2 ms
frames 485 -> 607   11.4, 12.2, 13.3, 13.7, 13.3, 13.7, ... fps for eleven seconds
   of each ~75 ms frame:  present 25 ms  disc 11 ms  audio 14 ms  rest 26 ms
```

`mk_dist.py` already carries the copy's own number in its `FB_MODE` comment: the native path moves
307 200 B at 19.5 MB/s, i.e. **15.8 ms**. The card spends **23.9 ms**. The 8 ms in between is the change test:
`PRESENT_SKIP=1` hashes every byte of the surface before deciding not to copy it, and docs/422 measured that
pass at ~8.3 ms. So during a fade the port pays 8.8 ms to be told what the previous four frames had already
said -- the game's `fn_040660` is rewriting all 307 200 bytes every step (`sig` moves every window in the log).
15.8 + 8.8 = 24.6 ≈ the measured 23.9. That is the whole budget explained.

`0.3.4` therefore adds **`PRESENT_TESTLESS`** (default 24): once four consecutive frames have changed, present the
next N without asking. Skipping the test is the *safe* direction -- an untested frame is always written, never a
stale one -- and the hold win docs/416/422 bought (logo hold 41 s -> 6 s) survives, because a hold restarts the
test within four frames and re-proves the three display buffers in three more. `PRESENT_TESTLESS=0` is the exact
pre-0.3.4 shape, one ini line; the boot line now prints it (`blit shape: … present_testless=24`) and `perf`
counts it (`skips=…(hash … testless …)`). Expected on the same instrument: `stages … c=` 23.9 ms -> ~16 ms,
and the plateau 13 fps -> ~17-18. A third off the worst second, not a cure: the other ~26 ms of that frame is
`rest`, which is still unnamed (§4.1).

## 8. Why the original's own fade rate was never measured, and what that leaves open

The right question before touching the code was "does the *original* fade this coarsely too?" -- and that
question is still unanswered, because the container route (docs/46) could not get the game to accept the disc:

| mount | what the original drew |
|---|---|
| `imgmount D TG_NIGHT.CUE -t iso`, C: = `extracted/` | DOS4GW **error (2002): transfer stack overflow on interrupt 1Ch** (the `NIGHT.BAT` -> `HCODE /E` TSR hooks the timer IRQ) |
| same, `NIGHT.EXE` direct, `cycles=auto` | black 640x480 for 20 s at 70 fps sampled: **zero picture changes** |
| `imgmount D TG_NIGHT.CUE -type cue` | `cdinsert.pcx` -- the girl-and-disc card, "insert the game CD and press any key" |
| per-track BIN/CUE (see below) | still `cdinsert.pcx` |

Two real things came out of the attempt anyway. `scripts/disc_bin_cue.py` now writes
`extracted/cd/track01.bin` (the data track's raw 2 352-byte sectors, which the extractor had never needed to
keep) plus `extracted/cd/disc.cue` naming one BIN per track with the source's PREGAP preserved; every audio BIN's
size is checked against the CUE's boundaries, so the layout the emulator is being handed is verifiable rather
than assumed. It did not satisfy the game's disc check, so the check is looking for something a *ripped image*
does not carry -- and with `NIGHT/` deleted from the repo, the layout that the earlier captures (which did reach
the game: `scratch/pocket/orig_logo2`, 106 frames) mounted no longer exists on this machine.

So the honest position on the fade: the port's logo/title *durations* match the original (docs/76's basis: ramp
6 002 vs 6 005 ms, logo 2 603 vs 2 592 ms), the fade is driven by the guest's own step loop
(`fn_040660(target, 0x19)` = 25 steps; docs/447 measured a 25-step fade at 612-892 ms = 28-41 changes/s), and the
port was presenting that at 13 fps -- so the port was **below** the rate the guest is asking for, which is the
one case where speeding the copy is unambiguously the right move. Whether the original's fade *looks* like 13 fps
on real hardware is a question that needs a disc the emulator will accept, and that stays open.

## 9. What 0.3.4 actually did, measured -- and §7's mechanism was wrong

The player's report was "it got better", so the boot went through `logsave.sh` and the numbers were read apart
by display branch (`br n/g/d/o` counts presents per branch, which is the only way to separate the two paths in
a log whose `frames=` counts flips). Same instrument, same card, four boots:

| boot | native branch (640x480): fps / present per frame | game branch: fps / present per frame | `testless` fired |
|---|---|---|---|
| 0.3.1 | 31.5 / 25.2 ms | 46.0 / 4.0 ms | no counter |
| 0.3.3 (stutter-gone) | 38.2 / 25.2 ms | 45.6 / 4.0 ms | -- |
| 0.3.3 (fade-pause) | 38.2 / 25.2 ms | 46.0 / 4.0 ms | -- |
| **0.3.4** | **39.0 / 24.7 ms** | **45.7 / 3.2 ms** | **722, all on the game branch** |

So `PRESENT_TESTLESS` did what it says on one path and nothing on the other. It trimmed the game branch's
present from 4.0 ms to 3.2 ms per frame (−20%), and it **never armed once** on the native 640x480 path -- the
slide path, the one §7 was written about. Four consecutive changed frames is the arming rule, and on the slide
path that rule was never satisfied. The consequence has to be said plainly: **§7's arithmetic (15.8 ms copy +
8.8 ms hash = 24.6 ≈ 23.9 measured) is retracted as an explanation.** Whatever the extra 8 ms on that copy are,
they are not a hash pass that the code decided to skip, because the counter that would have followed those
changed frames stayed at zero.

And the fade itself is not a present-throughput problem. The plateau, read by branch:

```
0.3.3   11.4 12.2 13.3 13.7 13.3 13.7 13.3 13.7 fps   br n = 11-14 presents/s   pres 28-34% of wall   aud+disc 29-55%
0.3.4   11.8      12.8            13.8 fps            br n = 11-13 presents/s   pres 21-26% of wall   aud+disc 43-110%
```

~14 presents a second, each costing 25 ms, is 0.35 s of a 1.0 s window. The window is not full. `PRESENT_SKIP`
did its part -- the present share fell from ~33% to ~22-26% -- and the frame rate did not move, because the
frames are not waiting on the copy. They are sitting in **audio and disc**, which occupy 30% to more than 100%
of the same windows. Over 100% is not magic: the CD-DA feed's SD read happens inside the audio pump's span, so
the two buckets are not disjoint.

That last fact breaks the very instrument docs/429 was pointing at. `rest` is computed as
`wall - (pres + pump + disc + aud)` **clamped at zero**, as a cumulative counter -- so in any window where the
buckets overlap, `rest` freezes at 0 and every later window's delta is a lie. The 35-39% `rest` this doc has
been calling "unnamed" is partly an arithmetic casualty of music playing. `0.3.5` prints `acc=` (the sum of the
buckets) on the `split wall` line, so a reader can tell a quiet machine from a clamped one: `rest` stuck at 0
with `acc >= wall` is the clamp.

Open, in priority order:

1. **Why the arming rule never fires on the native branch.** `ROW_SKIP=0` makes `use_rows` false, so the hash
   block is reached on both paths; `applied_refresh()` clears `g_hash_passes` and the proof. If the slide era
   re-programs the display path (or the hashed `src` is not the surface the fade is darkening), arming would
   be cleared as fast as it is set. Distinguishing those needs one more counter (`changed=`), not a guess.
2. **The audio feed's share of a slide window.** With music on, `aud` alone reaches 60-100% of wall during the
   plateau; with the fade steps only 14/s, that is where the wall is. `logswitch.sh --music off` now sets
   `CDDA_OFF=1` as a flag (not a hand-edit, so a deploy cannot eat it): one boot with the music off separates
   feed-bound from copy-bound, and it is the cheapest thing left to do.
3. **`rest` with disjoint buckets.** Reporting the disc wait as nested inside the pump (or not reporting it
   twice) would let `rest` mean what its name says -- and only then can docs/429's step "attribute rest" be
   attempted honestly.

## 10. The smooth 0.3.5 boot, which says the fade's frame rate was never the variable

The player's verdict on 0.3.5: no stutter. `logsave.sh` archived it with its banner asserted against
`core.json` (0.3.5 == 0.3.5, so the boot is the deployed bits), and `scripts/pocket/branchfps.py` -- the
reader written to stop re-deriving this by hand -- says:

```
br n (640x480 slide path):  8 win  fps med 33.1 [6.8-43.3]   pres/frame 25.2 ms
br g (game view):          19 win  fps med 45.4 [41.9-48.0]  pres/frame  3.3 ms    PRESENT_TESTLESS fired 705
windows below 25 fps: 7, of them 11.7 fps (br n=11/s) and 6.8 fps (br n=26 in 3.8 s)
```

Two things follow, and the second is the one that matters for how this problem gets closed.

**The 0.3.4 trim held on the game view** -- 4.0 ms to 3.3 ms a present, 45.6 to 45.4 fps, with the counter
firing 705 times. That path is the one the change touched, and it stayed touched.

**But the slide path still runs sub-12 fps stretches in a boot the player calls smooth.** The 6.8 fps window
puts 26 presents across 3.8 s: **146 ms between slides**, present 17% of the window, aud+disc 31%, `rest` 51%.
A gap far longer than anything in the stutter boot, and it is not felt.

So mean rate does not separate the two reports, and here is what does. The stutter boot's fade was *periodic*:

```
0.3.3 stutter   11.4 12.2 13.3 13.7 13.3 13.7 13.3 13.7 13.7 fps      eight windows, alternating on 0.4 fps
0.3.5 smooth     6.8      11.7      17.0      21.6      22.3 fps      irregular, and reported as smooth
```

Nine consecutive windows landing on 13.3/13.7 -- two ticks, three ticks, two ticks, three ticks -- is a
judder with a period, and "the screen pauses almost every second" is a fair description of a fixed 74 ms
cadence that occasionally slips a tick. The smooth boot's windows never line up like that.

That is the argument, at last, for the one instrument this doc has kept proposing and not building: a
per-window **histogram of inter-frame gaps** (buckets 20/40/80/160 ms) on the `gap worst` line. It measures
regularity directly, it costs nothing to collect (the gaps are already timed), and it is the only reading
that would have separated these two boots without a player telling me which was which. Until it exists, the
rule the numbers force is the negative one: **do not tune against feel across two boots whose per-branch
medians agree** -- 38/38/39/33 fps on the slide path across four boots says the copy speed is not the knob,
and 0.3.4 is the proof of that, measured against myself.

## 11. Same bits, one key: the log turned out to be the anesthetic

The reports, in the order they arrived, with the card's own version as the stamp:

| boot | shape | verdict |
|---|---|---|
| 0.3.5 | `LOG=0 STALL_MS=400` | "current one doesn't have stutter" |
| 0.3.5 (same bits, ini flipped by me) | `LOG=0` after a deploy | "now stutter is back" |
| **0.3.6** | `LOG=1 STALL_MS=120` | **"stutter's gone"** |

Nothing else changed. `diskutil verifyVolume /Volumes/Pocket` came back *appears to be OK*, exit code 0, so
there is no degraded-filesystem story to reach for either; and the code delta between 0.3.5 and 0.3.6 is four
compares per present plus a longer log line -- work added, so it cannot be the thing that fixed anything.

**The one variable that has tracked every verdict in this whole thread is whether the app was writing its
log.** docs/452's line *"play feel measured on this card is not player feel"* was written as a disclaimer and
turns out to be the clue: `nightlog.sav` is written into an SD slot every 32 lines, and the CD-DA feed reads
that same card. If *adding* SD traffic makes playback smoother, then what stutters is SD arbitration inside
the audio feed -- which is exactly where the branch counters had already put the wall time (aud+disc 30-110% of
the fade windows). The fix that follows from that lives in the feed (read size, lookahead, pump placement),
and not one further knob on the blit path has any claim on it.

The histogram (0.3.6) works, and its first verdict was falsified by the very boot it was measured on:

```
gap hist, whole run:  <20ms 597 (55.3%)  20-40 397 (36.8%)  40-80 70 (6.5%)  80-160 8 (0.7%)  160+ 7 (0.6%)
bimodal windows (>=5 under 40 ms AND >=3 over 40 ms): 10 of 28, 1 of them WITHOUT a hold
```

Ten windows looked like "the stutter shape" in a run the player called smooth, because the long tail is the
stage-entry card: a hold legitimately puts 160+ ms between frames. `branchfps.py` now prints both numbers and
says out loud when every bimodal window is a hold. The lesson is the same one as §9's: a threshold that fires
on the best boot on the record is the wrong threshold, and it needs a stuttering boot with a histogram in it
before it means anything -- which is the boot `LOG=0` would give us.

So the decisive boot costs one key and holds the bits still: **0.3.6 with `LOG=0`**
(`./scripts/pocket/logswitch.sh off --stall 400`). Stutters → the log's SD cadence is the variable, proven
with the ELF identical. Smooth → the observer theory dies and the hunt goes elsewhere. What is still owed
either way, if LOG is confirmed: a byte counter for the log's own writes on the `split wall` line
(`log_kb=`), so the observer has a size and the interaction can be modelled instead of felt. Not shipped yet,
because every ELF change confounds the A/B in progress -- 0.3.4 and 0.3.6 each taught that the hard way.
