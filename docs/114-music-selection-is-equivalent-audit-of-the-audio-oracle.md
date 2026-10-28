# 114. Music: the selection timeline is equivalent on both sides; the audio *content* oracle is what is missing

Date: 2026-10-29. Route: `scripts/rig/drives/probe_cd.txt` (intro/title), full `route.sh` run
(`/tmp/rig/ttl2`), plus the combat route `/tmp/rig/rt-sfx`.

## 1. The shipped game's CD selection logic, and the DS cells that record it

`fn_010b1d` (guest 0x00010b1d) is the selection function:

```c
if (DAT_00051a10 != 0 && idx < DAT_00053c60) {
  ...                                     /* 18-byte MSCDEX request template from DAT_00051b20 */
  _DAT_00053c44 = *(int*)(idx*5 + DAT_00053c64 + 1);              /* track start LBA   */
  len = (idx == count-1) ? *(int*)(count*5+DAT_00053c64+1) - start
                         : *(int*)(idx*5+DAT_00053c64+6) - start; /* next start - start */
  _DAT_00053c40 = _DAT_00053c44 + len;                            /* track end LBA     */
  DAT_00063e38 = 0;                                              /* selection counter */
  DAT_00053c48 = len / 75;                                       /* length in seconds */
  fn_010570(local_38, 0);                                        /* MSCDEX play, code 0x1510 */
}
```

`scripts/rig/ds_trace.py --addrs 0x53c44,0x53c40,0x53c48` samples exactly those cells on **both** sides
through the memory API, so the shipped side's music timeline needs no audio analysis at all.

## 2. Measured: selection is equivalent, on both drives

Title/intro route (`/tmp/rig/ttl2`, anchor = first stage-data load, orig t=33668 / port t=32838):

```
original schedule: -30655 ms lba=0x918e t1 | -22439 ms lba=0x2d320 t9 | +0 ms lba=0x367aa t11
port schedule    : -29168 ms lba=0x918e t1 | -24648 ms lba=0x2d320 t9 | +0 ms lba=0x367aa t11
port witness     : 3 plays, 4 stops, 0 track changes with NO stop/teardown between them
DS cells         : 0x53c44 0x918e->0x2d320->0x367aa on both sides; 0x53c48 0x2a->0x171->0xee both sides

combat route (rt-sfx): 0x53c44 0x918e->0x2d320 (P0) -> 0x367aa (P1) -> 0x9ddf (P6) -- port identical
```

So on both drives the port selects the same tracks as the original, in the same order, within 1-2 s, and the
docs/106 teardown invariant (a stop before every replacement) holds. **No selection defect is observable.**

## 3. The port's CD data path is byte-faithful (correcting my own false finding)

Searching `TG_NIGHT.IMG` sector user data (`(lba-150)*2352 + 24`) inside the port's own CD witness
(`port/cdda.raw`), with non-constant probe sectors: every announced track appears **byte for byte**, at a
stream position equal to its play-request time (track 1 at 0.0 s, 9 at 42.1 s, 11 at 44.9 s, 2 at 55.8 s,
12 at 149.5 s). An earlier version of this document claimed track 2 was never read: that was **my probe's
bug** -- the log prints LBAs in decimal (`lba=40415`) and I searched for hexadecimal `0x40415`. Withdrawn in
docs/113.

## 4. The game sets no audio volume at all

Instruction search over the whole guest program (66,938 instructions):

- `0x1511` (MSCDEX set-volume): **0 matches**
- SB mixer port `0x2204`: **0 matches**
- `0x1510` (MSCDEX play): **exactly 1**, `fn_010570` at 0x000105ab

Consequence: the original's *audible* CD level in the container (rms 791 while the disc's own track 11 is rms
2896) is DOSBox's own mixer gain, not a decision the game makes. The port's CD witness is line level (rms
3524). Comparing absolute levels between the two captures is therefore meaningless, and the port has no
volume/mixer code to fix.

## 5. Open rig gap (recorded, not solved): the shipped-side *content* oracle

Three attempts to recover "which track is audible when" from the container's `audio.wav`:

| method | port's own CD stream | container mixer capture |
|---|---|---|
| RMS envelope, 250 ms buckets (`cd_track_timeline.py`) | 1.000 / 0.999 at the announced times | 0.46-0.68, incoherent times |
| narrow-band (Goertzel, 10 log bands) correlation (`audio_spectrum.py`) | 1.588 exact / 0.851 | 0.28-0.33, incoherent |
| sample-domain NCC (`audio_ncc.py`) | 0.213 (!) | 0.10-0.11 |

The NCC row is the tell: the port's stream and the disc are *already proven byte-identical*, yet NCC scored
0.213 -- so the NCC implementation (window-shift/energy normalisation) is wrong, and the low scores on the
container capture measure nothing. Two rig items follow: (a) fix `audio_ncc.py`'s lag bookkeeping and
re-validate it against the byte-identity ground truth before trusting any shipped-side content claim; (b) if
it then scores well on the port stream but poorly on the capture, test directly whether the endpoint
`/api/v1/capture/audio` carries the cd-DA channel at all (the capture's ~99 % "active" fraction with rms 791
is consistent with music, but that has never been verified against a known-silent game moment).

Until then, the defensible music statements are §2 and §3 (selection and data path are equivalent) and §4
(no volume code to fix) -- which means the remaining "music is still broken" report is **not** reproducible by
the rig, and the honest position is that the audible layer is still not machine-covered.
