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
