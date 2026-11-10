# NIGHT (openfpgaOS Pocket core) — data to put in this folder

Generate everything below from your own copy of the game:

```sh
make -C pocket/src/night data     # -> scratch/pocket/, then `make deploy`
```

| file | slot | what it is |
|---|---|---|
| `NIGHT.RES` | 4 | the shipped 51 MB resource container, copied verbatim |
| `track02.bin` | 5 | CDDA track 2 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track03.bin` | 6 | CDDA track 3 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track04.bin` | 20 | CDDA track 4 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track05.bin` | 21 | CDDA track 5 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track06.bin` | 22 | CDDA track 6 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track07.bin` | 23 | CDDA track 7 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track08.bin` | 24 | CDDA track 8 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track09.bin` | 25 | CDDA track 9 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track10.bin` | 26 | CDDA track 10 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track11.bin` | 27 | CDDA track 11 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track12.bin` | 28 | CDDA track 12 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track13.bin` | 29 | CDDA track 13 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track14.bin` | 30 | CDDA track 14 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track15.bin` | 31 | CDDA track 15 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `nightlog.sav` | 17 | the host's diagnostic trail, opened through the SDK alias `save:7` -- only when `night.ini` says `LOG=1` (it ships off, so a player's card carries no log file) |
| `night.cfg` | 8 | the host's diagnostic trail (phases, clock probe, memstat) -- read it after a run |

## Settings

Three of them are in **Core Options** on the device -- Music Volume, SFX Volume, Screen Fit -- all three in one bridge word, and they survive an eject and a reboot. `L1+B` cycles the panel fit live. The rest are developer knobs in this folder's `night.ini` under `[night]`:

| key | default | what it does |
|---|---|---|
| `MUSIC_VOL` `SFX_VOL` `MASTER_VOL` | 255 | mixer stages, 0-255 |
| `SFX_GAIN` `CDDA_GAIN` | 200 / 150 | sample gain in percent, 100-212, through a knee that cannot clip and cannot attenuate |
| `FIT` | 1 | panel fit: 0 letterbox (20+20 black bars), 1 stretch (fills the panel), 2 the core scaler's 320x200 |
| `SCREEN` | 0 | 0 game, 1 terminal, 2 the diagnostic panel over the game, -1 follow the menu's Pocket LCD |
| `CDDA` `PAD` | 0 | `1` mutes the CD music / ignores the pad |
| `TICK_HZ` | 490 | cap the game's fixed step rate, 0 = whatever the game programs |
| `FB_MODE` | 1 | 1 = native 640x480 source framebuffer, 0 = the host's own 2x2 downscale |
| `DISC_PREFILL_FIRST` | 150 | sectors of CD-DA read before the voices start (2 s), in one visit; the rare 418-424 ms fill is its price (docs/396, docs/402) |
| `CDDA_CARD_STOP` | 1 | 0 = keep streaming the introduction track over the `stage<N>.pcx` hold card; 1 = silence it, as the original is (docs/413) |
| `DISC_RAMP` | 0 | 1 = spread that priming over pump visits; it removes the big fill but the ring never got ahead on hardware (dry 5->49, audio_low 5->64, ahead_min 0.00 s, docs/402) |
| `DISC_MIN_WANT` | 12 | sectors of read allowance to accumulate before filling; 1 = the old one-sector-at-the-disc-rate shape |
| `DISC_BATCH` | 12 | sectors per CD-DA slot read; 1 reproduces the old one-sector-at-a-time reads |
| `PUMP_AUDIO_TICK` | 0 | 0 = drain the board's DMA byte ring on every pump entry (the shape every build has used); 1 = drain it once per 490 Hz tick. The hardware A/B lever for the 267 000-pump-entries-a-frame finding (docs/369), off by default because that ring is the one the game is writing into |
| `LOG` | 0 | 1 writes `Saves/night/common/nightlog.sav`, the diagnostic trail this port was measured with |
| `DIAG` | 0 | which experimental bus probes a run may perform (1 periph scan, 2 analogizer, 4 interact census) |

On a desktop build the same keys are `NIGHT_MUSIC_VOL`, `NIGHT_SFX_VOL`, `NIGHT_FIT`, `NIGHT_LOG`, `NIGHT_CDDA_OFF`, `NIGHT_PAD_OFF` and so on, and the desktop build logs to `scratch/pocket/night.log` by default because the rig reads it as its witness.

In-game save keys (hold L1): `L1+Select` saves the current slot, `L1+Start` loads it, `L1+A` cycles slots 0-9. On a desktop build the same is `--save N --save-at FRAME` / `--load N`.
