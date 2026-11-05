# 361 — what the volume chain actually was, how much headroom the audio has, and why 255 is a ceiling not a knob

The question was *"what's the default volume? will setting it to 255 be sufficient?"*  Both halves needed measuring,
and the measurements are in `scripts/analysis/gain_scan.py`.

## 1. The chain, as it actually stood until 0.1.51

| stage | what the port set | effect |
|---|---|---|
| mixer **master** volume | **nothing, ever** | an OS default nobody measured.  openfpgaOS-Doom sets it explicitly (`i_sdlsound.c:155 of_mixer_set_master_volume(255)`), which is itself the tell that a core may not assume it |
| CDDA voice volume | `MUSIC_VOL`, and **night.ini shipped `MUSIC_VOL=128`** | 0.502 |
| CDDA **group** volume | the same `MUSIC_VOL` again | 0.502 again — the mixer multiplies group by voice, so the music sat at **0.25 of the recording, −12 dB**, at the default nobody asked for |
| SFX per-voice | `g_sfx_volume = 200` — openfpgaOS-Doom's headroom number | 0.784, −2.1 dB on every sound effect |
| SFX group volume | `SFX_VOL` (255) | unity |

Doom has a reason for its 200: it mixes up to 32 simultaneous DMX voices and attenuating each one keeps the *sum* off
the ceiling.  NIGHT's sounds live in 8 blocks and are typically one or two at a time, so the port was paying Doom's
insurance premium with none of Doom's risk.

0.1.51: `of_mixer_set_master_volume()` with `MASTER_VOL=255`; group MUSIC pinned to 255 (the menu knob lives on the
voice volume, because *group volume alone did not move the music on the device* — docs/340's finding, which is why both
were set in the first place); `g_sfx_volume = 255`; `MUSIC_VOL=255` in the shipped `night.ini`.  Music is 4× louder at
the default, every sound effect 1.27×, and every stage names itself in one log line:

```
settings: music_vol=255 applied from the menu | mixer master=255 music_voice=255 sfx_group=255 sfx_voice=255 | gain sfx=200% cdda=150%
```

## 2. Is 255 sufficient? It is the ceiling, not headroom

255 means "the samples are the level".  Whether there is anything above that is a property of the audio, measured:

```
CD-DA, the shipped disc byte-exact:
  track03 peak 32768 (0.00 dBFS)  rms 4128 (-17.99 dBFS)   samples >0.9 FS: 44
  track04 peak 32768 (0.00 dBFS)  rms 4468 (-17.31 dBFS)   samples >0.9 FS: 198
  track07 peak 32759 (-0.00 dBFS) rms 3154 (-20.33 dBFS)   samples >0.9 FS: 589
  ... 6 of the 14 tracks peak at 32767-32768: the disc is already AT full scale
  worst peak over the disc: 32768 = 100.0% of full scale
  gain available before ANY sample clips: 1.00x
  125%: naive gain clips 0 samples        150%: 52 766 of 273 056 616 (0.019%)
  200%: 623 657 (0.228%)                  250%: 4 884 955 (1.79%)

the port's board-only SFX stream (the samples NIGHT's own driver sent to the Sound Blaster):
  worst measured SFX peak: 30720 = 93.8% of full scale, most captures 24320 (-2.59 dBFS)
  rms around -20 dBFS: the game's effects are recorded quiet
```

So: **255 fixes the port's own attenuation (up to +12 dB on music), and that is probably most of what felt quiet — but
it cannot make the *loudest* material louder, because the disc has nothing left to give.**  Anything above unity is a
compressor, and a naive multiplier hard-clips: 623,657 samples at 2×.

## 3. The gain stage: `of_gain16()`, a knee instead of a clip

`pocket/src/night/of_cdda.c`, shared by the music and the effects:

```c
#define OF_GAIN_KNEE  20480          /* 0.625 of full scale */
#define OF_GAIN_SLOPE 4              /* above the knee, the excess is carried at one quarter size */
...
if (v > OF_GAIN_KNEE) {
    int32_t c = OF_GAIN_KNEE + (v - OF_GAIN_KNEE) / OF_GAIN_SLOPE;
    v = (c < a) ? a : c;             /* a gain setting may not attenuate what the disc recorded */
}
```

Below the knee a sample is multiplied outright; above it only the excess survives, at a quarter of its size — and the
result is never lower than the sample that went in.  That last clause is a fix, not a detail: the first version mapped
full scale to 27 647 at 150%, so a knob labelled "louder" quietly made the loudest hits **softer**.  A compressor with
the makeup gain in the wrong place.

The curve as shipped (`python3 scripts/analysis/gain_scan.py` prints it):

| input | 100% | 125% | 150% | 200% | 212% |
|---|---|---|---|---|---|
| 1 000 | 1 000 | 1 250 | 1 500 | **2 000** | 2 120 |
| 10 000 | 10 000 | 12 500 | 15 000 | **20 000** | 20 660 |
| 20 000 | 20 000 | 21 610 | 22 860 | 25 360 | 25 960 |
| 30 720 — the loudest board sample ever measured | 30 720 | 30 720 | 30 720 | 30 720 | 31 641 |
| 32 767 — full scale | 32 767 | 32 767 | 32 767 | 32 767 | 32 767 |

Quiet and mid material is lifted, the peaks are untouched, nothing clamps.  Because the compressed value cannot reach
full scale before 212%, `clamp_gain()` allows 100–212 and a naive `x * 2` is not needed anywhere.

Defaults on the card: `SFX_GAIN=200` (the game's own effects are the quiet half of this port — 0.106% of the samples in
the board captures are even above the knee), `CDDA_GAIN=150`.  `CDDA_GAIN=100` puts the disc back at exactly its own
level, which is the honest setting if the lift is not wanted.

**The PC build leaves both at 100%.**  The differential rig compares the port's CDDA against the shipped disc byte for
byte (docs/332) and its board stream against the captured original (docs/164); a gain stage in that path would turn
every one of those comparisons into a lie about faithfulness.  `NIGHT_SFX_GAIN` / `NIGHT_CDDA_GAIN` override explicitly
when a measurement wants the gain on the PC side.

## 4. What is not in this port, and what to check on the hardware

* The original's own Sound Blaster **mixer commands are not emulated** (`src/sys/sys.c` handles DSP 0x40 rate,
  0x14/0x17 8-bit, 0x75/0x77 16-bit, 0xd0/0xd1 stop — no 0x04/0x0e volume writes).  So the game's own attempt to set
  its board volume is ignored, in both directions: the port neither inherits an attenuation nor a boost the DOS game
  asked for, and DOSBox's `MIXER` preamp in the captured originals is a different scale factor again.
* The Pocket's own headphone volume is a separate stage nobody in this chain controls; if the port's log says every
  stage is 255 and it is still too quiet, that is where to look next.
* Sample widths are faithful: 8-bit transfers map `(b − 128) × 256` (full signed range, and `<<8` on a negative value
  would be UB — `src/sys/sys.c:277`), 16-bit transfers pass through.  No headroom is lost at the decode stage.

Core 0.1.52 on the card, `DIAG=11`, `MUSIC_VOL=255 SFX_VOL=255 MASTER_VOL=255 SFX_GAIN=200 CDDA_GAIN=150`.  Build
clean, `check_interact.py` ALL PASS, fixture suite ALL PASS, `src/f` untouched.
