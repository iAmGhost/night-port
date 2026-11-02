# 302 — item 1's audible half: the CD-DA reader had no notion of elapsed time, so the heard stream lagged the request by minutes

Date: 2026-11-02.  Human evidence that started it (playtest `docs/manual-playtest/04-2026-11-02.md` item 1):
"I can confirm track change log appears when it's changing but actual played music track is not changed."
That is exactly the shape of the defect: the **selection** layer was already faithful, the **stream** was not.

## 1. What was already established (and why "no divergence" was the wrong conclusion)

docs/235 and docs/299 measured the request chain on the unfixed build against the shipped and it matches:
`stop → PLAY index1 lba 37262 → index9 lba 185120 → index11 lba 223146` (the shipped's own sequence), and
the audio *onset* count matched 9 vs 9 with every window `[SAME]`.  Those oracles measure **which track is
requested and how often playback starts**.  Neither can hear *what is playing*, so item 1 stayed open in
practice while reading as closed on paper.

## 2. The measurement that found it

The rig already dumps every CD-DA sector it feeds the device (`rig_cdda_sample` → `cdda.raw`).  A CD-DA
stream is 75 sectors/second, so the dump size *is* a rate measurement.

| run | build | sectors | audio seconds | wall seconds | **ratio** |
|---|---|---|---|---|---|
| `/tmp/rig/le8` | unfixed | 53 311 | 710.8 | 195.4 | **3.64×** |
| `/tmp/rig/le9` | first fix (wrong, §4) | 57 615 | 768.2 | 171.2 | **4.49×** |
| `/tmp/rig/le10` | budget fix | 18 264 | 243.5 | 228.3 | **1.07×** |

All three: `scripts/rig/route.sh scripts/rig/drives/ep1_leftedge4.txt 220`.

The port was reading 710 s of disc audio into the device queue during a 195 s run (125 MB of PCM in the
witness).  A queue fed 3.6× faster than it drains grows without bound, so the audio actually reaching the
device is *old* audio: the log records the new track at, say, 47 s while the stream is still playing bytes
read minutes earlier.  That is the reported symptom — the log changes, the music does not ✓

Sector *content* was checked too, so the "wrong bytes" explanation is excluded: block 0 of `cdda.raw`
appears in `TG_NIGHT.IMG` at sector 37112 (= disc LBA 37262 ✓ the requested track-1 start), block 300 at
LBA 37562 (contiguous ✓), and the mid-dump block at LBA 44253, inside the range of track index2 requested at
69.9 s (lba 40415, 19639 sectors ✓).  The reads follow the requests ✓ the **rate** does not ✗

## 3. Mechanism

`sys_cd_da_plan` (`port/src/sys/sys.c:700`) is a pure clamp — no time term:

```c
uint32_t sys_cd_da_plan(uint32_t pos, uint32_t sectors, uint32_t want, uint32_t cap)
{
    if (!sectors || !want || !cap) return 0;
    if (pos >= sectors) return 0;
    uint32_t room = sectors - pos;
    uint32_t n = want < room ? want : room;
    return n < cap ? n : cap;
}
```

The device-bound branch of `cdda_pump` passed a fixed `want = 30`, cap 16, on **every** call — and the pump
runs roughly 100 times per frame.  Any fixed per-call quantity in a pump called 100×/frame becomes a 100×
rate multiplier: 16 sectors per call, thousands of calls per second.  The *no-device* branch already paced
itself with elapsed wall time (`el = (now - g_cdda_clock) * 75 / freq`); the branch that actually produces
sound did not ✓✓

## 4. The wrong fix, kept because it was measured

First attempt replaced `30` with `el + 30`.  It **made it worse** (`/tmp/rig/le9`: 4.49× vs 3.64×), because
an allowance granted *per call* is not a lead: `el` is tiny between two calls 1/100 of a frame apart, so
`want` stayed ≈ 30 and every call still took its full 16-sector cap.

## 5. The fix

`port/src/app/main.c`: a **budget** that accumulates with wall time and is reset when a track starts
(`g_cdda_budget = 0` in `cdda_play`), with one 30-sector / 0.4 s cushion granted at the PLAY rather than per
call:

```c
g_cdda_budget += el;
if (g_cdda_stream) {
    uint64_t allowed = g_cdda_budget + 30;
    want = (g_cdda_pos >= allowed) ? 0
         : sys_cd_da_plan(g_cdda_pos, g_cdda_sectors, (uint32_t)(allowed - g_cdda_pos), 16);
} else {
    want = sys_cd_da_plan(g_cdda_pos, g_cdda_sectors, (uint32_t)(el ? el : 1), 16);
}
```

Green gate: `cdda.raw` = 18 264 sectors for a 228.3 s run = **1.07× real time** ✓ bounded to the cushion ✓

## 6. Side effect worth noticing: the pump was distorting logic pacing

Same drive, the route's own pacing check (picture changes per second, orig vs port):

| run | orig | port | ratio |
|---|---|---|---|
| `le8` (unfixed) | 13.19 | 24.50 | 0.54 |
| `le9` | 12.73 | 22.54 | 0.56 |
| `le10` (fixed) | 12.56 | 9.24 | **1.36** |

Reading 125 MB of PCM and pushing it through an SDL stream took enough of the frame pump's time that the
port's *logic* rate was inflated.  Removing it moved the ratio from 0.54 to 1.36 — i.e. across 1.0 to the
other side.  This plausibly touches the long-running pacing residual (docs/244's "the port reaches the walk
line 3.9× later", docs/269's 0.55), but pacing claims must be re-measured on the canonical 150 s
`ep1_kill3` route before anything is quoted from a 220 s left-edge drive ✓ not claimed here ✓

State-oracle tally on the same drive (same cell set, same basis, three runs) shows no regression and a
slight improvement: `SAME` 76 → 79, `DIFF` 21 → 18, `REC-SAME` 11 → 18, `REC-NULL` 15 → 10, and 271
activity-profile cells `SAME` in all three ✓ `make -C port test`: `== ALL PASS ==` (the `cd_iso` fixtures,
including `cd_iso_read_audio(37262) → IMG sector 37112`, still pass ✓).

## 7. What is proven and what is not

- Proven: the request chain is faithful; the sector bytes follow the requests; the port over-read the
  CD-DA stream by 3.6-4.5× real time; a wall-time budget brings it to 1.07×.
- Not directly measured: **what a listener hears**.  dosbox-automation 0.85.1 exposes no audio-capture
  endpoint (`scripts/rig/diff_activity.py:4`), so the rig cannot compare the two sides' *heard* audio
  sample-for-sample.  What the fix establishes is that the queue is now bounded to ~0.4 s, which is what
  makes an audible switch possible at all.  The ear remains the final check for this item, and the rig's
  role is the rate witness above.

## 8. Standing rules

- A dump size against wall-time × nominal-rate is a rate oracle: free, precise, and independent of any
  alignment or palette convention.
- An allowance granted per call is not a lead.  Leads must accumulate with time.
- When a pump runs ~100× per frame, reason about *rates per second*, never about the quantity passed per
  call.
- "The log is right" is not "the output is right": any oracle that inspects commands rather than their
  produced media leaves the audible/media half of a bug undetectable.
