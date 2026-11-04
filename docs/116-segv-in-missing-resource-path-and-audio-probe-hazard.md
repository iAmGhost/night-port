# 116. A SEGV the rig walked into: the missing-resource path, and the host audio-probe hazard

Date: 2026-10-29. Found without looking for it: while re-running the atype probe I invoked the port from the
repo root instead of through `route.sh`, and it died.

## 1. Red: SIGSEGV whenever the resource directory is not found

```
$ ./build/night --frames 60                      # from the repo root, no --res-dir
Segmentation fault: 11        rc=139
fault: KERN_INVALID_ADDRESS at 0x000000004847494e     # 4e 49 47 48 = "NING"
  strn_len  <- fn_0451bc <- fn_044c23 <- fn_0446c3 <- fn_042394 <- fn_036537 <- fn_03690b <- fn_03af18 <- main
```

The fault address is the first four bytes of the game's own message string `NIGHT.RES File Not Found!!!`.

Two coupled defects, both in the port (the shipped code path is fine):

1. `fn_0451bc` (the `%s` conversion of the shipped printf) read a **32-bit varargs slot** and cast it
   straight to a host pointer: `src = (uint8_t *)(uintptr_t)ptr`. In the shipped flat model a varargs slot
   holds a 32-bit *address*; on a 64-bit host the cast truncates and dereferences garbage. Every other guest
   address the port resolves goes through `flat_ofs()` (`g_heap_base + (int32_t)off`).
2. The call site in `fn_03690b` passed the message **string itself** as the varargs block, so the conversion
   read the message's own bytes as an address -- hence "NING".

The shipped `fn_036537` prints the banner and ends in `fn_0423b7(0)` -- i.e. print, then terminate. The port
reproduces that sequence, so the correct behaviour is a printed banner and a clean exit, not a crash.

Fix: `%s` now translates the slot through `flat_ofs`, and the call site copies the message into a flat-heap
block and passes that offset in a one-slot varargs block, which is what the guest's stack actually held.

## 2. Green

```
$ SDL_AUDIODRIVER=dummy ./build/night --frames 60          # still no --res-dir
terminate requested (DAT_5336c/70/74 callback, arg 0)
rc=0
```

`rc: 139 -> 0`, no DiagnosticReport, and the terminate chain (`g_dat_5336c` -> `on_terminate`) is reached as
in the shipped path. Normal startup is unaffected (`make test` still runs its suite).

## 3. Host hazard that will silently eat rig time

On this machine the port's audio device ladder now spends ~5 minutes **per refused device** before falling
back (`audio: alt 1 ... refused (CoreAudio error ...)` five minutes apart, then alt 2, alt 3 ...). A rig run
that lands in that ladder spends longer probing audio than the whole drive takes. `route.sh` therefore
defaults `PORT_ENV` to `SDL_AUDIODRIVER=dummy`: the rig's audio witnesses (the board PCM ring and the CD-DA
sector stream) are teed before the device, so a dummy driver does not reduce what the oracles can see, and
the analysis stays reproducible. Override with `PORT_ENV=...` when a run genuinely needs the host device.

Measured after the change: `route.sh probe_cd 60` completed with zero `audio: alt` lines and the usual
oracle outputs (state cells 326, screen pacing orig 3.08 / port 3.33 changes/s, ratio 0.93).
