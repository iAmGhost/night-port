#!/usr/bin/env python3
"""Read the per-episode music table from NIGHT.EXE and print the base track
per episode, matching the port's stage.c episode_music_track().

Ghidra (FUN_0003a829 / FUN_00010d47): the stage frame selects
    track = *(RAM 0x52da4 + epData[0x40]*4 + (episode-1)*0x28)
The table is at RAM 0x52da4 = file offset 0x4b7a4 in NIGHT.EXE: 9 rows
(episodes) x 10 wave-slots of u32. The dominant value of each row is the base
track for the intro + stage; one wave-slot in most rows overrides to track 8.
"""
import struct
from pathlib import Path

EXE = Path(__file__).resolve().parent.parent / "NIGHT" / "NIGHT.EXE"
TABLE_OFF = 0x4b7a4          # file offset of RAM 0x52da4
ROWS, SLOTS = 9, 10


def main() -> None:
    d = EXE.read_bytes()
    print(f"music table @ file 0x{TABLE_OFF:x} ({ROWS} eps x {SLOTS} waves, u32 LE)")
    print("ep |  w0 w1 w2 w3 w4 w5 w6 w7 w8 w9  | base  cd  override")
    bases = []
    for ep in range(ROWS):
        off = TABLE_OFF + ep * (SLOTS * 4)
        vals = [struct.unpack_from("<I", d, off + w * 4)[0] for w in range(SLOTS)]
        # dominant value = base track
        base = max(set(vals), key=vals.count)
        bases.append(base)
        ovr = [f"w{w}={vals[w]}" for w in range(SLOTS) if vals[w] != base]
        print(f"ep{ep+1} | " + " ".join(f"{v:2d}" for v in vals) +
              f" |  {base:2d}   {base+1:2d}   " + (", ".join(ovr) or "-"))
    print("\nbase tracks ep1..9:", bases)
    print("port episode_music_track:", "{0," + ",".join(map(str, bases)) + "}")


if __name__ == "__main__":
    main()
