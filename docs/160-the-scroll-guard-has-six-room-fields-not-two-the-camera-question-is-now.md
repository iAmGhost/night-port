# docs/160 — the scroll guard has six room fields, not two; the camera question is now a data question

docs/159 §9 put the blocking measurement on the scroll guard. Reading the port's own transcription block
(`f.c:14110-14118`) the guard is a **three-level nested condition on six distinct room fields**, not the two I
have been quoting in docs/142/docs/143:

```
right:  if (room[+0x3b] == 0 && player[+1] - cam >  0xbe)      ; player near the right edge
            if (room[+0x2e] == 0 || cam + 0x140 <= room[+0x2e])
                if (room[+0x36] == 0 || cam + 0x140 <= room[+0x36]) fn_03cb38(DAT_63354);   ; +3 / INC
left:   if (room[+0x3a] == 0 && player[+1] - cam <  0x82)      ; player near the left edge
            if (room[+0x2a] == 0 || cam >= room[+0x2a])
                if (room[+0x32] == 0 || cam >= room[+0x32]) fn_03cbd9(DAT_63354);           ; -3 / DEC
```

Byte-level confirmation from `NIGHT.EXE` (guest `0x3d4ff-0x3d505`):

```
0x3d4ff: mov  eax, [ebp-4]          ; the room parameter
0x3d502: mov  al,  byte [eax + 0x3b] ; trigger field is a BYTE, not a dword
0x3d505: and  eax, 0xff
```

so `room[+0x3b]` is a byte at that offset and the port's `if (room[0x3b] == 0)` (`f.c:14128`, labelled
`0x3d4ff-50c`) matches the shipped width and offset.

## What this changes about docs/142 / docs/143

* The camera behaviour is gated by **`+0x3b`/`+0x3a`** (edge triggers, bytes), then by **`+0x2e`/`+0x2a`**, then by
  **`+0x36`/`+0x32`**. Quoting only `+0x32`/`+0x36` (as docs/142 §4 and docs/143 did) described the innermost
  condition and missed the two outer ones — which is why the earlier room-block reading "explained" the parked
  camera: with `+0x3b`/`+0x3a` non-zero, the scroll arms never run at all, whatever `+0x32`/`+0x36` hold.
* docs/159 §9's disagreement moment (population 3 and command list 67 agreeing on both sides, camera 0 shipped vs
  78 port, held across whole windows) is therefore a statement about **six fields**, and the only way to verdict
  it is to read all six from the *same* room parameter on both sides.

## The one witness that answers it

`fn_03e20f` receives the room pointer as a parameter from `fn_03ed6f` (docs/143). Add a witness at the guard
itself — not at the clamp struct — printing, per scroll decision, the parameter's six bytes/dwords plus the
camera and `player[+1]`:

```
SCROLL room=%#x b3a=%d b3b=%d d2a=%d d2e=%d d32=%d d36=%d cam=%d px=%d called=%s
```

On the port side this is a `fprintf` at `f.c:14128`/`f.c:14143` behind `NIGHT_SCROLLTRACE=1`. On the shipped side
the same six fields are readable from the DS dump only if the room pointer is reachable from a DS cell — which is
docs/143's open question, and the reason the camera item is the one the rig has not yet been able to close.
