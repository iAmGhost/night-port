#!/usr/bin/env python3
"""What does the ORIGINAL do between the last intro slide and the stage music? Docs/413.

The port's observable is wrong there: the game reaches the stage card, holds the screen for ~3 s, and during
that hold the introduction track keeps playing -- the user hears it stop only when the stage music starts.
So: read the original's own instructions in that window and see what it can and cannot do to the CD audio.

The flat-image convention is the one scripts/analysis/findref.py already uses: virtual address - 0x2600.

    python3 scripts/analysis/cdaudio_window.py

What it prints, and why each one was asked:

  fn_00010570   the CD request dispatcher. It hard-codes the MSCDEX function code and issues INT 2F, so it
                answers "can this game pause or stop CD audio at all?" for every caller at once.
  callers       a call graph for the dispatcher, fn_00010dde (a fixed 13-byte read), fn_10b1d (pick a track)
                and fn_000405a7 (the site src/f pairs with sys_cd_audio_stop()).
  room_start    0x03b130 .. 0x03b1ba, the window between the last slide and the stage loop, annotated.
  fn_000405a7   the bytes src/f turns into sys_cd_audio_stop().
"""
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

D = 0x2600
data = open('NIGHT/NIGHT.EXE', 'rb').read()
CODE = (0x7800, 0x49000)


def flat(va, n):
    return data[va - D : va - D + n]


def dis(va, n, label, calls=None):
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    print('\n=== %s  @ %08x ===' % (label, va))
    for i in md.disasm(flat(va, n), va):
        ann = ''
        if i.mnemonic == 'call' and calls:
            ann = calls.get(int(i.op_str, 16), '')
        print('  %08x  %-22s %s%s' % (i.address, i.bytes.hex(), i.op_str, ann))


def rel32(va):
    v = int.from_bytes(flat(va, 4), 'little')
    return v - (1 << 32) if v >= (1 << 31) else v


def callers(tgt):
    """Every E8 rel32 call whose target is tgt. Sign-extend: backward calls are most of the code."""
    out = []
    for va in range(*CODE):
        if flat(va, 1)[0] == 0xE8 and va + 5 + rel32(va + 1) == tgt:
            out.append(va)
    return out


CALLS = {0x10dde: '   <-- MSCDEX INT 2F AX=1510 media read (fn_00010dde)',
         0x10570: '   <-- the read dispatcher',
         0x3628e: '   (load PCX)', 0x428f8: '   (sprintf)',
         0x40660: '   (palette fade-in)', 0x406f5: '   (palette fade-out)',
         0x405a7: '   <-- INT 10h AX=0013: set video mode (fn_000405a7)',
         0x42958: '   (delay_units)'}

dis(0x00010570, 0x7f, 'fn_00010570 -- the CD request dispatcher', CALLS)
dis(0x00010dde, 0x36, 'fn_00010dde -- the fixed 13-byte read (template at flat 0x1b50, arg1=0)', CALLS)
dis(0x000405a7, 0x36, 'fn_000405a7 -- the site src/f pairs with sys_cd_audio_stop()', CALLS)
dis(0x0003b130, 0x90, 'room_start tail: last CD read of the show -> card -> hold -> mode set -> room loop', CALLS)

print('\n=== call graph ===')
for tgt, label in ((0x00010570, 'read dispatcher fn_00010570'),
                   (0x00010dde, 'fixed read fn_00010dde'),
                   (0x00010b1d, 'track select fn_10b1d'),
                   (0x000405a7, 'video mode set fn_000405a7')):
    c = callers(tgt)
    print('  %-34s %3d callers  %s' % (label, len(c), [hex(x) for x in c]))

# Bound the MSCDEX surface rigorously: an immediate-pattern search for 0x150B/0x150C finds nothing and proves
# nothing (two-byte patterns collide with data all over the image -- the 0x1510 "hits" below are mostly that).
# What does bound it is finding every place the image can enter real mode through INT 2F at all, and showing
# that the whole MSCDEX surface runs through one dispatcher whose function code is a constant.
print('\n=== every INT 2F entry point in the image ===')
for i in range(*CODE):
    if flat(i, 2) == b"\xcd\x2f":
        print('  int 0x2f at %08x' % i)
for i in range(*CODE):
    if flat(i, 5) == b"\xb8\x2f\x00\x00\x00":
        print('  mov eax, 0x2f (the DPMI/real-mode request form) at %08x' % i)
