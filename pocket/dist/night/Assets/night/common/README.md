# NIGHT (openfpgaOS Pocket core) — data to put in this folder

Generate everything below from your own copy of the game:

```sh
make -C pocket/src/night data     # -> scratch/pocket/, then `make deploy`
```

| file | slot | what it is |
|---|---|---|
| `NIGHT.RES` | 4 | the shipped 51 MB resource container, copied verbatim |
| `track02.bin` | 18 | CDDA track 2 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
| `track03.bin` | 19 | CDDA track 3 of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |
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
| `night_0..7.sav` | 10-17 | save slots, created by the game on the first save |
| `night.log` | 9 | the host's diagnostic trail (phases, clock probe, memstat) -- read it after a run |

Settings live in this folder's `night.ini` under `[night]`: `MUSIC_VOL`/`SFX_VOL` 0-255, `CDDA=1` mutes the CD music, `PAD=1` ignores the pad (edit the file on the card). On a desktop build the same knobs are `NIGHT_MUSIC_VOL`, `NIGHT_SFX_VOL`, `NIGHT_CDDA_OFF`, `NIGHT_PAD_OFF`.

In-game save keys (hold L1): `L1+Select` saves the current slot, `L1+Start` loads it, `L1+A` cycles slots 0-9. On a desktop build the same is `--save N --save-at FRAME` / `--load N`.
