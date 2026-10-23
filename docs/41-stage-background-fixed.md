# The stage finally draws: two real bugs, one in the port and one in the host renderer

`docs/40` proved the palette chain was correct and located the missing look in the background-plane path.
This session found and fixed **two independent defects**, and the stage now renders as the game does:
dark station platform, train, DANGER barrier, HUD with portrait and EXP bar, two fighters (see the
320x200 back-screen dump at frame 1200).

## Bug 1 (port): the `.wal` background map was addressed in the wrong space

`fn_03c853` (the `.wal` room-chain loader) reads a segment as
`header → bg index map → 768 B palette → u16 table`, allocates the map with `heap_malloc()` and stores the
returned **heap offset** in `node + 0x348`:

```c
uint32_t off = heap_malloc((uint32_t)n * 0xc8u);      /* 0x3c958-66 */
*(uint32_t *)(node + 0x348) = off;                    /* 0x3c970 */
fn_04192e((uint8_t *)g_heap_base + off, (uint32_t)n, 0xc8, io);
```

`fn_03ce0b` (the per-frame background/viewport blit) however treated that same field as a flat video
address and passed it through `video_map()`. `video_map()` clamps anything outside its window to the buffer
base, so every stage frame blitted **garbage from the start of the video arena** instead of the map:

```
blt_rows(video_map(sa), pitch=1600, 0x140, 0xc8, dst, 0x140)     sa = DAT_639dc + node[0x348] - node[0]
```

Evidence chain (host-side probes, `NIGHT_DBG=1` / `NIGHT_BLITSTATS` / `NIGHT_HEAPDUMP`):
* the blit accounting shows `fn_03ce0b` really does copy 64000 bytes into the back screen every frame
  (2032 calls × 64000), and `fn_038098` really flips them to the framebuffer;
* the logged `node[0x348] = 0x241df0`, `node[0x1a] = 1600` (the segment width), `DAT_000639dc = 0`
  (correct: `fn_03cad3` sets it to the *first* node's `worldX_left`, which is 0 for segment 0);
* a heap dump taken after the stage loaded shows the 320000-byte `stage1.wal` index map present **byte-exact
  and contiguous at heap offset 0x241df0** — exactly the value in `node+0x348`;
* yet the back buffer matched the wal slice at only 0.004 (chance level).

Fix: a documented address-space resolver `stage_map()` used by all five `blt_rows()` sources in
`fn_03ce0b`. Values that actually lie in the modelled video/screen window stay video addresses (the sprite
fixtures and sprite-RAM users hand those to the same fields); everything else resolves against the heap
arena, which is what `fn_03c853` stores there.

Result: the back buffer now matches the `stage1.wal` slice at camera x = 0 with **0.973 / 0.91**
(frame 600 / 1200) — the remainder being sprites and the HUD drawn on top, which is exactly right.

Regression coverage: `test_stage_bg_heap_source` (5 checks) builds a room node whose `+0x348` is a real
`heap_malloc` offset filled with a known pattern and pins rows 0/1/3 of the destination, including the
`node+0x1a` source-pitch advance. The pre-existing `test_fn_03ce0b` (video-address fixtures) still passes.

## Bug 2 (host): every pixel the SDL host produced had alpha in the red channel

SDL3 `SDL_PIXELFORMAT_RGBA8888` is a *packed* format with
`Rmask=0xff000000 Gmask=0x00ff0000 Bmask=0x0000ff00 Amask=0x000000ff` (verified with a 1-pixel probe:
`pxtest.c`). The host built pixels as `(0xff<<24)|(r<<16)|(g<<8)|b`, i.e. **alpha into red**, in the
on-screen path *and* in all three BMP dumps. That is why every captured frame looked washed-out pink and
why earlier palette sessions chased a non-existent palette bug.

Fix: build packed pixels as `(r<<24)|(g<<16)|(b<<8)|0xff`. Verified numerically: the sampled back-buffer
pixel for index 43 is now (109,109,101) against the `.wal` palette's (108,108,100), and the whole-frame mean
RGB of the port's back buffer (89,95,96) equals the offline `.wal` decode (89,95,94).

## Presentation geometry (host)

The room path composites a **320x200** frame (pitch 0x140, 0xfa00 bytes) and flips exactly those 64000
bytes to the framebuffer start, while the slide path fills the whole 640x480 region (`fn_03f462` with
count 0x1e0); the mode globals keep reporting 640x480 (`DAT_00063e64 = 640`, `DAT_00063e68 = 480`). The
host now follows what the game last wrote: `g_view_320x200` is set by the `fn_038098` room flip and cleared
by a full-height slide blit, and `present()` (re)makes the texture as 320x200 or 640x480 accordingly. The
headless log shows the switch: `display mode 640x480` → `display mode 320x200` when the stage starts.
Whether the original kept a 640x480 mode while drawing a 320x200 stage is still an open question (the port
only ever models VBE mode 0x101); it does not affect the ported data, which is now provably correct.

## State

Suite: **2787 -> 2792 PASS / 0 FAIL**, ASan clean. `make -C port selftest` ALL PASS.
Diagnostics kept for future sessions: `dbg_pal_load`, `fn_040660` fade-target dumps (`/tmp/palfade###.bin`),
`board_out` `dladdr` trace, `fn_038098` pacing + geometry log, `fn_03ce0b` argument log, blit accounting
(`NIGHT_BLITSTATS`), heap dump (`NIGHT_HEAPDUMP`), and the `vidscr/frame_scr` dumps.

## Follow-up measurements in the same host run (frame 1300, autokey)

* **Sound effects are still silent in the host run, and the reason is now measured:**
  `snden=0 devinit=0 plays=0` — `DAT_00053338` (the sound enable byte) and `DAT_00063f90` (the
  board-initialised flag) are both zero, and `fn_04185e` is never entered. So the audio model and its SDL
  stream are idle because the game-side gate never opens in this run; the unit path (`test_audio_path`)
  still proves the model. The next step is to find who sets `DAT_00053338` / `DAT_00063f90` (the audio
  master `0x3658d` / device bring-up chain) and why that does not happen in the host.
* **The intermittent host crash is still there but the site is stable:** `EXC_BAD_ACCESS`, fault
  `0x124e41005` (a read), frames `fn_03d606` (inlined) → `fn_03a829` → `fn_03af18`. `fn_03d606` itself
  never loops (`count = 0`), so the faulting pc belongs to code inlined into the room loop next to it. The
  statements right there dereference the room chain link `*(uint8_t **)(room + 0x34c)` twice (the
  `DAT_0006336c == 2` / `== 4` exit branches, `g_dat_63398 = **(room+0x34c + 0x11)`), which is the prime
  suspect now that the room loop really walks and takes exit branches. `heap_chain_check("fn_03a829")`
  reports nothing, so the corruption (if any) is in record fields, not allocator chains.

### Where the two audio gates are written (handoff for the silent-audio item)

* `DAT_00063f90 = 1` happens **only** at the end of the device bring-up (`fn_040d09`, `f.c:7524`,
  the original's `0x40e70`). It is 0 in the host run, so `fn_040d09` does not reach its last step there,
  even though `test_device_bringup` walks it end-to-end (base scan 0x220, IRQ probe, 0xf2/0xe1/0xd1, id
  from two data reads, ISR install, 64 KB buffer, DMA address).
* `DAT_00053338` is written **only** from the title-menu sound option inside `fn_039c03`
  (`f.c:24655`, `g_dat_53338 = 1 - f[3]`, seeded at `f.c:24611` by `if (DAT_00053338 == 0) f[3] = 1 - ...`).
* Therefore both conditions must be fixed for `fn_04185e` to ever be entered: start with `fn_040d09`'s
  late steps in the host (what differs from the unit fixture), then check whether `fn_039c03`'s config
  block runs at all in the headless autokey run.

## The intermittent host crash is solved: a 32-bit parameter truncated a host pointer

With `-fno-inline` the crash report stopped being ambiguous: the faulting frame really was
`fn_03d606 + 64` called from `fn_03a829`, and the fault was `KERN_PROTECTION_FAILURE` at an address just
past the arena. The room loop called it as

```c
fn_03d606((uint32_t)(uintptr_t)room);   /* a0 was declared uint32_t */
```

while `fn_03d606` computed `g_heap_base + a0`. The original passed a *linear address*; in the flat port the
room node is a 64-bit host pointer, so the cast threw away the high half and the arithmetic produced an
address inside the arena only by luck of ASLR — which is exactly why the fault was intermittent and why no
sanitizer build reproduced it (`heap_chain_check` correctly reported the allocator chains as clean).

Fix: `fn_03d606(uintptr_t a0)` uses the struct address directly (the `pairs` field at `+0x44` stays a heap
offset, as `fn_03c853` stores it) and the room loop passes `(uintptr_t)room`. Along the way every widened
64-bit record link (`+0x34c` next, `+0x354` back) now goes through the unaligned-safe `rec_ptr_get` /
`rec_ptr_set` helpers instead of `*(uint8_t **)`, because those slots are only 4-byte aligned.

Measured: five consecutive headless runs of 1500 frames completed with the run killed by the harness
timeout (`frames=1580` each) instead of dying around frame ~600, and the ASan suite is clean at
**2792 PASS / 0 FAIL**.
