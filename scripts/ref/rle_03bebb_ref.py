# Reference for 0x03bebb (forward/reverse constant-fill RLE) and 0x03bb42 (copy).
# Registers: EDX=dest_off (row offset, <0x140), EBX=start_row, ECX=col,
#            [EBP+0x14]=rle, [EBP+0x18]=mode, [EBP+0x1c]=fill_byte.
# EAX (base) = canary -> 0 in flat model; dest = EAX + start_row*0x140 + EDX.
# Row width 0x140 (320). Screen rows 0..0xc7 (200).

def run_03bebb(screen, base, dest_off, start_row, col, rle, mode, fill):
    # screen: bytearray indexed by (dest_off + r*0x140 + x) within base's 0x14000 buffer
    if dest_off > 0x13f: return
    if not (dest_off + col) >= 0: return
    if start_row > 0xc8: return
    if not (start_row + 0) >= 0: return   # EBX + 0 (EBX is the row)
    # clamp rows
    rows = start_row
    if start_row < 0:
        # skip |start_row| rows
        p = 0
        for _ in range(-start_row):
            n = rle[p]; p += 2
            for _i in range(n):
                c = rle[p]; p += c + 2
            p -= 1
        rows = 0
    else:
        if start_row + 0 > 0xc8:
            pass  # in_stack = 0xc8 - start_row (row count)
    # forward (mode==0): dest = base + start_row*0x140 + dest_off
    if mode == 0:
        d = base + start_row*0x140 + dest_off
        left = 0
        if base < 0: left = - (0)      # -iVar (high dword, ~0 in flat)
        elif dest_off + col > 0x140: left = 0x140 - dest_off
        right = col
        for r in range(rows):
            p = 0  # (in real code rle pointer advances; here per-row is separate)
            pass
    print("skeleton only - full port in C")
