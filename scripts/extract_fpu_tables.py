#!/usr/bin/env python3
"""Extract the fn_0461cc (0x0461cc) FP-emulator dispatch tables as C initializers.

The engine's two indirect jumps read far pointers (offset:seg 3) out of the code segment's ROM area.
Selector 3 maps offset -> flat = 0x40000 + offset, and every entry lands *inside* fn_0461cc itself, so
the engine is self-contained: table1 arms are the 6-byte operand-form stubs at 0x4642a..0x464e3, table2
arms are the arithmetic cases at 0x46524..0x46c9d.

  table1 @ 0x45e78, index = (b | (b >> 8)) & 0x1f          (byte index, see 0x46225 area)
  table2 @ 0x45f38, index = (b & 0x38) | b  ==  b & 0x3f   (jmp dword cs:[ebx*4 + 0x35f38], 0x4624a)
"""
import struct

EXE = "NIGHT/NIGHT.EXE"
D = 0x2600            # code: file offset = flat - 0x2600
SEL3_BASE = 0x40000   # selector 3 -> flat


def table(lo: int, n: int):
    d = open(EXE, "rb").read()
    out = []
    for i in range(n):
        off, seg = struct.unpack_from("<HH", d, lo - D + i * 4)
        assert seg == 3, (hex(lo), i, seg)
        out.append(SEL3_BASE + off)
    return out


def emit(name: str, lo: int, n: int, doc: str) -> None:
    t = table(lo, n)
    print("/* %s */" % doc)
    print("static const uint32_t %s[%d] = {" % (name, n))
    for i in range(0, n, 8):
        print("    " + ", ".join("0x%05x" % x for x in t[i:i + 8]) + ",")
    print("};")


if __name__ == "__main__":
    emit("kFpuTable1", 0x45e78, 48, "index (b | (b>>8)) & 0x1f -> operand-form arm inside fn_0461cc")
    print()
    emit("kFpuTable2", 0x45f38, 64, "index b & 0x3f -> arithmetic arm inside fn_0461cc")
    emit("kFpuTable3", 0x46038, 64, "index (ah & 0x38) | al for the modrm >= 0xc0 path (0x462f7)")
    # The compound operand arm at 0x464ec calls cs:[ebx*4 + 0x35ed8]; the cs displacement is +0x10000,
    # so that is ROM 0x45ed8 = table1 + 24 entries -- the same table, upper half. No fourth table.
