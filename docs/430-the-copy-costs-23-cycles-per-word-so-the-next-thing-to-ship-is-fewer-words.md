# 430 — the surface costs ~23 cycles per word, so the next thing to ship is fewer words (`ROW_SKIP`)

docs/429 said 70% of a run is inside `present()`. This turn chased what that means and ended with a feature
that ships switched off, a self-check, and one of my validation plans withdrawn as impossible.

## 1. The cost is per word, not per byte, and there is no hardware help

At 640x480 the copy is 307 200 bytes in ~17.5 ms (docs/418, docs/422). `copy_row` is already word-wise
(`main_of.c:611`), so 76 800 stores cost 17.5 ms = **~23 cycles per store**. That is not SDRAM; it is what
writing into an aperture behaves like. Before inventing an optimization I checked whether the OS has one:

- `of_blit_opaque` (`of_video.h:509`) -- the SDK's own "opaque blit" -- is `__builtin_memcpy` per row. The same
  shape we already use. `of_blit` is a per-pixel loop with a transparency test.
- `of_services.h` has no 2D/blit/DMA entry in the os25 table.
- `of_cache_flush_video()` is just `video_flush_cache`, and our flush stage costs ~660 us.

So nothing hands the copy to hardware. The only lever left is **writing fewer bytes**.

## 2. `ROW_SKIP`: the docs/400 condition, met by keying on the buffer

docs/400 killed partial updates for a precise reason -- three display buffers, a flip that does not wait, so
"unchanged since last frame" describes the wrong buffer, and on hardware that was ghosted rows. Keying the
test **by buffer** answers that specific objection: buffer b's table describes what buffer b holds, so
skipping a row leaves behind bytes that are already correct. No three-frame proof is needed, and the tables
are 24 KB of `uint64` per row per buffer instead of 921 KB of shadows.

Two details I refuse to hand-wave:

- **The buffer index is not guessed.** It comes from comparing `of_video_surface()` against
  `of_video_buffer_addr(0..3)` (`of_video.h:181`) -- addresses the OS handed us. Unknown buffer means write
  every row, because a wrong guess here *is* the docs/400 bug.
- **The hash is of the destination bytes, not the source row.** The game branch can write a source row, a
  doubled source row (the 200-onto-240 stretch), or a bar of zeros; two of those share a source row and must
  not share a verdict. Padding we write is folded into the hash without touching memory; padding this branch
  never writes is left out, and switching branches invalidates the tables.

Invalidation: `applied_refresh()` (mode re-programming), any present through a branch we do not model, and
`stage_commit` (which writes the surface itself).

## 3. My desktop validation plan is impossible, so I withdrew it

The plan was: same binary, `ROW_SKIP=0` vs `1`, compare captured frames. Two things killed it.

1. **Two identical runs differ.** `off1` vs `off2`: **18 of 45** captured frame files differ. The capture is a
   wall-clock sampling of the run (docs/410/418 said this about `cdda.raw` and `frames.bin`); it was never a
   determinism basis. My A/B would have been noise reading as failure either way.
2. **The shim never reaches the code.** With branch counters added, a desktop run reads
   `br n/g/d/o=0/0/273/0` -- every frame takes the `downscale_2x2_frame` branch, which is dead on the shipping
   device config (`FB_MODE=1`). A row test inside that branch is worthless too: its destination pixels are
   *computed*, so hashing them costs as much as writing them.

So the honest statement is: **the desktop rig cannot validate this path.** Which is precisely the situation
docs/403 was in with BRAM ("no desktop number can validate it"), and the answer there was a hardware A/B.

## 4. The substitute: the skip checks its own claim against the buffer, on device

`ROW_SKIP=2` runs the whole verdict machinery and writes **every row anyway**. On a skip verdict it
byte-compares the row it would have skipped against the bytes the buffer actually holds -- a read, never a
write -- for the first 4096 verdicts, and a single mismatch prints:

```
ROWSKIP MISMATCH: buf=1 row=217 (hash said same, buffer says otherwise)
```

So the first hardware boot of this answers three questions without risking the picture: how many rows would
be skipped, whether any skip is wrong, and which blit branch each frame took. `ROW_SKIP=1` is only for after
`mm=0` and a skip rate worth having. The perf line now carries all of it permanently:

The perf line carries all of it, permanently. Measured on the desktop shim, where the row machinery never
runs -- which is the expected result there, and the honest reading of it:

```
perf pres=857us max=3954us pump=2443 ticks=2456 skips=0(hash 0 smp 0) | rows c=0 s=0 dry=0 f=0 inv=0 mm=0 chk=0 | br n/g/d/o=0/0/273/0
```

`br n/g/d/o=0/0/273/0` says every one of those 273 frames took the downscale branch, so `rows` and `chk` are
legitimately zero. That output proves the plumbing prints; it proves nothing about the skip. The device
numbers are the ones that will mean something, and they come from the next boot.

(Write this paragraph rather than trusting memory: my first draft of this section quoted `dry=3820 mm=0`,
which was a number I had never printed. The real one is above.)

## 5. Shipped state

`0.2.7`: `ROW_SKIP` default **0** in the release ini (behaviour unchanged for a player); the test card runs
`ROW_SKIP=2` via `devini.sh`. Branch counters ship on both hosts. `make test` passes both halves, root
fixtures `== ALL PASS ==` (3091), card verified byte-for-byte against the build tree.

Open, unchanged from docs/429: even a perfect row skip leaves the *first* write of a changed row at ~23
cycles per word, and gameplay already skips almost nothing. If the row skip pays, the next question is whether
a smaller destination (320x240 with the OS upscaling, or a 320x200 surface) beats 640x480 at the aperture --
docs/401 condemned the software downscale, not the small surface.
