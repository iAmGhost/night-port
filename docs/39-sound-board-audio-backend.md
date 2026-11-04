# The sound-effects path is now audible plumbing: Sound Blaster protocol model + SDL audio stream

The audio master and its VOC loaders were already ported (`0x03658d`, `0x0417f3`, the 100 `N.voc` slots in
`0x03690b`), but nothing in the port turned the game's board traffic into samples. That gap is closed on the
sys side and wired into the SDL host.

## What the game actually does (from the ported functions)

* `fn_04185e` (play) is gated by `DAT_00063f90` (board initialised) **and** `DAT_00053338` (the SOUND MODE
  enable byte), and sends the stop command `0xd0` before a second play.
* `fn_040839` walks the VOC tag stream as records `[kind][24-bit len][payload]`; kind 1 = `fn_04097e`
  (arm + first chunk), kind 2 = `fn_040b83` (one DMA chunk), other kinds skip their payload.
* `fn_04097e` reads the payload header as `byte0` = a **DSP time constant** handed to `fn_040948`
  (`outb(cmd, 0x40)` then `outb(cmd, t)` — the Sound Blaster "set digitized-output sampling rate" pair) and
  `byte1` = a block kind selecting the transfer command through a 4-entry table
  (`0 -> 0x14`, `1 -> 0x75`, `2 -> 0x77`, `3 -> 0x17`).
* `fn_040b83` copies the chunk into the 64 KB device buffer `DAT_00063fb4`, then programs the DMA itself:
  `outb(0x0a,5)` mask, `outb(0x0c,0)` clear flip-flop, `outb(0x0b,0x49)` preset, the 24-bit address as two
  bytes on `0x02`, the page on `0x83`, the byte count (**n-1**) as two bytes on `0x03`, `outb(0x0a,1)`
  unmask; finally the start command plus the count low/high on the command port.

## The model added to `src/sys/sys.c`

* **8237 side**: address/count/page registers per the writes above, with the byte flip-flop; the unmask
  write arms the channel. The count is stored as programmed, so a transfer is `count + 1` bytes.
* **DSP side**: `0x40 t` → `rate = 1e6 / (256 - t)`; `0x14`/`0x17` → 8-bit transfer, `0x75`/`0x77` →
  16-bit; `0xd0`/`0xd1` → stop.
* Each start command drains the armed transfer into a 128 KiB PCM ring, exposed as
  `sys_audio_fill(int16_t *, frames)` (8-bit unsigned → signed 16-bit, or 16-bit little-endian), with
  `sys_audio_rate()`, `sys_audio_width()`, `sys_audio_transfers()`, `sys_audio_stops()`,
  `sys_audio_pending()`, `sys_audio_pushed()`, `sys_dma_addr/count/armed`.
* **PORT DEVIATION (documented in the source)**: in this flat port `DAT_00063fac` holds a truncated host
  pointer rather than a physical address, so the host registers the buffer a DMA transfer reads with
  `sys_audio_set_source(buf)`. The original reads physical memory at the DMA address.

## Host wiring (`src/app/main.c`)

`audio_boot()` opens an SDL3 output device (48 kHz stereo S16) and an `SDL_AudioStream`; `audio_pump()` runs
on a 20 ms cadence inside the tick pump, re-points the stream format whenever the board rate changes (DSP
`0x40`), and pushes the ring's samples. Headless runs are unaffected (audio failures only log). The pump
diagnostics line now carries `audio=<rate> pushed= pend= xfer= stops=`.

## Verification

`test_audio_path` drives the **game's own** functions — `fn_04185e` → `fn_040839` → `fn_04097e` →
`fn_040b83` — with a synthetic VOC play block (rate constant `0x80`, block kind 0, 300 payload bytes) against
the sys model, and pins: exactly one DMA transfer per play block; `1e6/(256-0x80) = 7812 Hz`; block kind 0
→ the 8-bit command; the whole payload reaching the ring; the DMA registers programmed as address low/mid
and count `n-1`; the ring replaying the bytes as signed 16-bit PCM; the second play emitting `0xd0` first;
block kind 1 → the 16-bit command.

`sys_dos_seg_free_ptr()` was added so tests that allocate the 64 KB device buffer release it again — the
real-memory pool was otherwise exhausted and the following device bring-up test started failing.

Suite: **2778 -> 2787 PASS / 0 FAIL**, ASan clean, `make selftest` ALL PASS.

## Two other fixes in this change

* **Unaligned 64-bit record slots**: the widened sibling/chain links at record `+0x34c` / `+0x354` were read
  with `*(uint64_t *)`. Records sit at 3-byte-aligned offsets, so those loads are not 8-byte aligned and can
  fault (macOS arm64 raises `SIGBUS` on aligned-only access patterns). They now go through `memcpy`.
* `fn_03d606` gained an opt-in diagnostic (`g_dbg_03d606`, host flag `NIGHT_DBG=1`) printing `a0`, `count`
  and the pair table; on the real data every call has `count = 0`, i.e. the room pair rotation is inert.

## Open host crash (still intermittent, documented for the next session)

The `-O2` host intermittently dies around frame ~600-700 (`SIGBUS`, `KERN_PROTECTION_FAILURE`) with the
crash report attributing the fault to inlined code reached from `fn_03a829` via `fn_03d606`:

```
EXC_BAD_ACCESS (SIGBUS) KERN_PROTECTION_FAILURE at 0x0000000108997a30
  night 0x35fc0 fn_03d606      <- inlined into the frame below
  night 0x35fc0 fn_03a829
  night 0x41170 fn_03af18
```

Measured facts, sharpened: the arena is a **64 MiB** static array while the host sets the cap to
`0x3000000` (48 MiB), and the faulting address `0x108997a30` is base + `0x5d12_8ac` — i.e. **inside the static
array but past the cap**, in the untouched tail whose pages were never faulted in. So the value used was a
heap offset in the 48-64 MiB band, which is what a 32-bit field holding part of a 64-bit host pointer looks
like: the port widens several record slots (`+0x34c`, `+0x350` -> `+0x354`, `+0x348` doubling as a video
address) and those fields sit only 4-8 bytes from allocator bookkeeping (`B_SIZE`/`B_PREV`/`B_NEXT`).
`fn_03d606` itself never loops (`count = 0` in all 108 calls), the ASan build ran to frame 3212 clean, and an
`-O1 -fno-inline` build did not crash in 40 s — consistent with a corrupted allocator chain rather than a
plain out-of-bounds access.
Probe added: `heap_chain_check("fn_03a829")` walks the region chain and each region's block chain (bounded)
and reports the first link or block size that cannot be a heap offset, gated by `g_heap_chain_dbg`
(host flag `NIGHT_DBG=1`). The first run with the probe reached frame 2015 without detecting corruption and
without crashing, so the probe now sits where the fault happens and will name the broken link when it does.
