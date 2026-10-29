# 129 — The audio oracle was comparing the wrong stream: the port's music lives in `cdda.raw`, and its timeline sits ~7 s off

## 1. Rig accounting defect #1 — the PCM witness was throttled by the host device

`audio_pump()` fills the emulated board's DMA ring and hands samples to both SDL and the rig sink:

```c
while ((got = sys_audio_fill(buf, 2048)) != 0) {
    if (SDL_GetAudioStreamAvailable(g_astream) > 96000) break;   /* bound the latency */
    if (!SDL_PutAudioStreamData(g_astream, buf, ...)) { ...; break; }
    if (g_rig_dir) rig_pcm_write(buf, (size_t)got * 2);
}
```

`route.sh` runs the port with `SDL_AUDIODRIVER=dummy` (docs/116: the host CoreAudio ladder costs ~5 min per
refused device). A dummy device never drains the stream, so once ~96 KB sat in it the loop `break`ed **before**
`rig_pcm_write`, and the fill loop stopped advancing: `pcm.wav` covered 15.9 s of a 300 s route. In rig mode the
sink is now written unconditionally and the SDL latency bound only guards the host path (device-independent,
which is also what a differential rig wants: the shipped side's DMA is driven by the emulated timer, not by a
host device). Measured on the same 3200-frame drive: **699 KB → 958 KB** of PCM.

## 2. Rig accounting defect #2 — the comparison never included the port's music

`diff_audio.py` scored the port from `pcm.wav`, which is **only the Sound Blaster board DMA**. CD-DA music is
written to a different witness: `cdda.raw` (76 938 624 B = 32 712 sectors = **436 s** of audio on the `ep1_kill3`
route, docs/114 proved those bytes are faithful to the disc). Hence the absurd verdict:

```
windows 275     orig active 216 (259 onsets)     port active 1 (1 onset)
```

…which is a statement about the *oracle*, not about the game: 216 active shipped windows are almost entirely
music.

New oracle half: `scripts/rig/port_mix.py` rebuilds the port's music timeline from `cdda.raw` + `cdda.jsonl`
(a CD-DA sector is 2352 B = 588 stereo frames = 1/75 s, so a play mark at `t0` with `S` sectors is a signal
running from `t0` for `S/75 s`) and time-shift-searches it against the shipped mixer capture:

```
cdda.raw: 76938624 B = 32712 sectors = 436.2 s; 4 play marks
  play t=3.55s  index=1  lba=37262  sectors=3153  -> placed 3153 sectors (42.0 s)
  play t=47.35s index=9  lba=185120 sectors=27697 -> placed 27697 sectors (369.3 s)
  play t=50.19s index=11 lba=223146 sectors=17856 -> placed 1862 sectors (24.8 s)
  play t=70.92s index=2  lba=40415  sectors=19639 -> placed 0 sectors (0.0 s)
shipped mix: 268.7 s, 538 buckets, active 530
port  music: 165.2 s, 331 buckets, active 314
best alignment: lag +14 buckets (+7.0 s) corr=0.933
```

So with the bytes proven identical, the music layer's remaining defect is **temporal**: the port's music timeline
aligns with the shipped mix at corr 0.93 only when shifted ~7 s. That is the first quantitative statement on the
music cluster that is not a schedule-table comparison (docs/106/114 compared play/stop marks and LBAs and found
them equivalent).

Caveats recorded so the number is not over-read: the placement model assumes `cdda.raw` is one sequential cursor
across plays, and two of the four plays were clamped (1862 of 17856 sectors, and 0 for the last) — the cursor
model must be replaced by the per-marker byte counts before a per-play offset can be quoted. Next step is a
*local* alignment per play mark rather than one global lag.

## 3. The SFX path is alive end to end — so "missing punch sound" is a trigger question now

`NIGHT_PLAYVOC=7` hands one shipped sound slot to the game's own player (`fn_04185e`) and dumps the slot bytes:

```
playvoc: slot 7 cursor=0x102c58618 first bytes=01 73 14 00
pcm.wav: 0.7 s, peak |sample| = 13568, 13355 nonzero samples
```

`01 73 14 00` is a Creative VOC block header (type 0x01, length 0x1473 = 5235), and the resulting PCM has real
amplitude. Combined with docs/113's witness (`element-event sounds: played=74 outside-pool=0`), the container →
DSP → DMA → PCM chain is proven working with real assets. What is *not* proven is that the in-game triggers fire:
as `audio_pump`'s own comment says, those triggers are action-record opcodes (`SND -> fn_0113e7`, plus
`fn_03f6bc`) that a blind key script does not reach. That is now the specific question for the punch-SFX item,
and it is the same `.act` chain question as the punching-loop item (docs/110).
