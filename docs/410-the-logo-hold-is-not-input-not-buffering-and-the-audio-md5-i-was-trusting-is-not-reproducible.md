# 410 — The logo hold is not input and not buffering; and the audio md5 I was trusting is not reproducible

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* section 2 (the logo hold) is superseded by docs/411.


Core 0.1.91. Two findings, one of which undoes a method I have used for several builds.

## 1. The music pause is closed on hardware

The user's report on 0.1.90 was simply "the music is good now", after a playthrough in which
the track plays through the logo, the title and the episode introduction. That is the confirmation
docs/409 asked for: `g_write = 0` after the prefill had moved the writer back onto the voice and the
voice had been reading unwritten ring for a lap. No further audio work is pending.

## 2. Two symptoms remain in the opening, and they are one symptom

Reported: pressing a key during the logo does not speed the logo up, and after the logo fades the
screen sits black for a noticeable time before the intro video starts. Both are the same stretch
measured from the outside: the gap between `logo.pcx` and the start of `opening.fli`.

What the card log of 0.1.88 and 0.1.89 says about that stretch (both runs, both reproducible):

| quantity | desktop shim | Pocket card |
|---|---|---|
| ticks from `logo.pcx` to the intro | 1 275 (2.6 s) | ~17 400 (35.5 s) |
| what the screen is doing | one static 640x480 grid, `sig=ecd967f7`, palette fading `pal=255→254` | the same signature, the same palette fade |
| `disc ops` | 0 | 0 |
| arena | 2.26 MB flat | 2.26 MB flat, +1.46 MB only when the intro actually starts |

Four explanations are now dead, each killed by a measurement rather than by reasoning:

- **It is not a key wait.** `NIGHT_KEYDBG=1` (src/f/f.c:16411, the latch witness from docs/97) prints
  every change of all five key latches. The desktop run that walks through this phase emits **no
  keydbg line at all**: the phase advances with zero input ever delivered. So there is no phantom key
  on the desktop and no "your press did not reach the game" on the device either. The reason a key
  press does not hurry it is that the wait is not waiting for a key.
- **It is not the game buffering the file.** `opening.fli` is the largest object in NIGHT.RES at
  10 055 372 B (9.59 MB of a 48.6 MB container, 444 entries). If the wait were "pull 9.59 MB into a
  buffer", the arena would have to climb by 9.59 MB before the intro started. It does not: the arena is
  flat at 2.26 MB for the entire hold and jumps 1.46 MB at the frame the intro begins. The .fli is
  streamed, not slurped.
- **It is not the CDDA feed.** `disc ops` is 0 throughout — but note what that field means: it counts
  `of_disc` audio runs only. Reads of NIGHT.RES do not go through it at all, which is why a long stretch
  of the game reading off the SD card looked, in the log, exactly like an idle game.
- **It is not the clock's rate.** The card's own `clock=` samples read 913 926 … 982 013 µs per wall
  second, i.e. the µs clock the port's every deadline rides on (src/sys/sys.c `sys_now_us`, installed at
  main_of.c:2568, wraps the OS's free-running `of_time_us` with a 32-bit carry) runs within a few percent.

What is left is deliberately narrow: either the game spins in the wait, or it reads off the SD card, and
the port currently cannot tell which. So 0.1.91 counts both, once a second, on the device:

```
resio reads=529 bytes=2302464 max=49664 B spin=10490406
```

- `reads/bytes/max` come from the seam itself (`sys_file_io_stats`, src/sys/sys.c): every read the game
  makes of NIGHT.RES — the 9.59 MB .fli, the 1.5 MB `.wal` tiles, the 300 KB `.pcx` cards — passes through
  `sys_file_read`, and nothing has ever counted it. Counted calls and bytes only, no timing, so the read
  path gains no extra clock reads.
- `spin` counts calls to the host's input pump, which `sys_key_spin` (src/f/f.c:16410) is the only caller
  of — a direct read of how many times the game turned inside a keyboard wait. The desktop measures that
  number at **~10 000 000 per second**: the game's waits are spin loops, not sleeps.

A boot that shows `bytes` in the megabytes during the hold is an SD-card wait; a boot that shows
`bytes=0` and `spin` in the tens of millions is a loop whose exit condition resolves slowly on the
device. Either way the next boot names the cause instead of this document guessing at it.

**Superseded by docs/411, in both halves.** The hardware boot answered neither branch (`bytes=0` *and*
`spin` ≈ the host poll rate, not tens of millions), and the 17 400-tick figure this table compares
against belonged to 0.1.89 alone — 0.1.88 measured 4 460 ticks for the same phase. The comparison
below is against the pathological build, not the steady state.

## 3. The audio md5 is not a determinism test, and I have been treating it as one

While checking whether the new counters perturbed anything, the same binary with the same seed and the
same command was run three times:

```
run 1: cdda.raw=824942e0…  4 600 512 B
run 2: cdda.raw=0bfdf30b…  4 628 736 B
run 3: cdda.raw=824942e0…  4 600 512 B
```

Not "the tail differs": `cmp` puts the first disagreement at **byte 2 215 585**, about 1019 sectors and
24 s of audio into a 4.6 MB recording. `cdda.jsonl` request counts likewise differ (121, 122, 121).

`cdda.raw` is a recording of what the ring saw, in write order, and the ring's fill schedule is
wall-clock driven (the lookahead ceiling, `DISC_MIN_WANT`, the per-tick allowance). So the byte stream
depends on how fast the host ran. It is a **sampling log, not a content hash.**

Consequences, stated plainly:

- Every claim of the form "the rig's `cdda.raw` md5 is unchanged, therefore behaviour is unchanged" in
  docs/405 through docs/409 is not the evidence I took it to be. It could pass by luck and fail by luck.
  docs/407 §3 in particular *bisected* on this md5 to conclude that threading `CD_LOOKAHEAD_SECTORS`
  through a shared setter had moved the shim's audio. The code change is real and the host-pinning fix
  is right, but the observation it was bisected on was not reproducible and does not support the
  causal claim. That conclusion should be re-derived before it is relied on again.
- `make test` (the fixture suite, 3 089 lines) is unaffected: it is differential against the python
  reference models, not against a recording. It stayed `ALL PASS` through all of this.
- `pcm_cd.wav` and the per-sector audio content are still what "byte-exact against the shipped disc"
  (docs/332) means. The robust version of the gate is per-request: **every `(lba, n)` the game asks for
  must return the shipped disc's bytes at that lba** — a statement about content that does not care when
  the read happened. Run-length and schedule agreement are not gates.
- A diagnostic that costs anything in a hot path must stay off the basis host regardless, since the
  schedule is the thing being varied. `resio` and the spin counter are `#ifndef OF_PC`; the seam counter
  returns early under `OF_PC`. This is the same rule that pinned `CD_LOOKAHEAD_SECTORS` per host, and this
  is the second time the rule earned its keep.

## 4. Reproduce

```sh
# the audio witness is not reproducible — three runs, expect 1–2 distinct md5s
for i in 1 2 3; do rm -rf /tmp/det$i; mkdir -p /tmp/det$i
  NIGHT_SEED=1234 NIGHT_CD_DIR=$PWD/scratch/pocket/cd pocket/src/night/app_pc \
    --warp-stage 1 --frames 1200 --rig-dump /tmp/det$i >/tmp/det$i/run.log 2>&1
  md5 -q /tmp/det$i/cdda.raw; wc -c < /tmp/det$i/cdda.raw; done
cmp -n 4600512 /tmp/det1/cdda.raw /tmp/det2/cdda.raw     # first mismatch: byte 2215585

# the key-latch witness, which is what ruled out the input theory (no output = no key ever delivered)
NIGHT_KEYDBG=1 NIGHT_CD_DIR=$PWD/scratch/pocket/cd pocket/src/night/app_pc --frames 420 --rig-dump /tmp/kdbg 2>&1 | grep keydbg

# the resource table, which is where opening.fli's 9.59 MB came from
python3 scripts/extract_res.py list | sort -k2 -n | tail
```
