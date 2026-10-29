#!/usr/bin/env python3
"""cam_jump.py -- locate the samples where the camera JUMPS, with context, on both sides.

docs/142 measured "shipped DAT_000639dc never leaves 27, the port reaches 298" and docs/159 counted camera
steps per window.  cam_policy.py's travel census now shows what those numbers actually are:

    ORIG  camera: nonzero moves 58/1845  max |dcam| = 10   total |dcam| = 344
    PORT  camera: nonzero moves 56/2168  max |dcam| = 298  total |dcam| = 608

The port's camera does not drift further than the shipped one -- it MOVES THE SAME NUMBER OF TIMES
(56 vs 58) and the whole extra distance is one jump of 298 px in a single sample.  That is a discrete
event, not a scroll-rate difference, and the scroll-guard theory in docs/159 (26 quanta of +3) is the
wrong model for it.

This script prints every camera jump above a threshold with the context that identifies the event:
time, entity count, room command list, room extents (+0x00 / +0x08), room CD-track column (DS 0x63370),
player state and player x.  A jump accompanied by a room-extent change is a room transition; one
accompanied by a player-record change is the docs/128 dangling-record family.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

DS_LO = 0x50000
CAM = 0x639DC


def s32(v):
    return None if v is None else (v - 0x100000000 if v >= 0x80000000 else v)


def ctx_of(get, ent):
    """get(ds_addr) -> dword, ent(off) -> dword inside the pointed-to record."""
    return dict(
        cnt=get(0x53C68), cmd=get(0x63E14), track_col=get(0x63370), stage=get(0x53C40),
        lo=ent(0x00), hi=ent(0x08), r2a=ent(0x2a), r2e=ent(0x2e), r32=ent(0x32), r36=ent(0x36),
        pstate=ent(0x17), px=ent(0x01),
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--min", type=int, default=12, help="jump magnitude to report")
    a = ap.parse_args()

    def side(run, port):
        if port:
            S, cells = load_port(run)
            cm = {c["name"]: c for c in cells}

            def mk(sample):
                blob = sample[1]

                def get(ds):
                    c = cm.get("g_dat_%05x" % ds)
                    if not c or blob[c["offset"]:c["offset"] + 4].__len__() < 4:
                        return None
                    return int.from_bytes(blob[c["offset"]:c["offset"] + 4], "little")
                rc = cm.get("g_dat_63354")
                pc = cm.get("g_player_ent")

                def ent(off, cell=rc):
                    if not cell or cell.get("rec_offset") is None:
                        return None
                    o = cell["rec_offset"] + off
                    return int.from_bytes(blob[o:o + 4], "little")
                return get, ent
        else:
            S, pcl = load_orig(run)
            names = [c["name"] if isinstance(c, dict) else c for c in pcl]
            pi = names.index("g_player_ent") if "g_player_ent" in names else None
            ri = names.index("g_dat_63354") if "g_dat_63354" in names else None
            n = len(names) or 1

            def mk(sample):
                blob, hb = sample[1], sample[2]
                blk = len(hb) // n if hb else 0

                def get(ds):
                    o = ds - DS_LO
                    return int.from_bytes(blob[o:o + 4], "little") if o + 4 <= len(blob) else None

                def ent(off, idx=pi):
                    if idx is None or blk == 0:
                        return None
                    o = idx * blk + off
                    return int.from_bytes(hb[o:o + 4], "little") if o + 4 <= len(hb) else None
                # the room record on the shipped side is the block that follows the player pointer cell
                ent.room = ri
                return get, ent
        return S, mk

    def head(title, run, is_port, n):
        """First n in-room samples: the initial player-to-camera offset is what the two policy arms
        then fight over, so a steady-state lead difference (docs/161: shipped median 21 px, port 117 px)
        is usually visible at room entry rather than mid-fight."""
        S, mk = side(run, is_port)
        S = sorted(S, key=lambda x: x[0])
        shown = 0
        print(f"== {title} first {n} in-room samples")
        for sm in S:
            get, ent = mk(sm)
            if (get(0x53C68) or 0) == 0:
                continue
            cam, px = s32(get(CAM)), s32(ent(0x01))
            lead = "-" if (px is None or cam is None) else px - cam
            print(f"   t={sm[0]:>7} cam={cam:<6} px={'-' if px is None else px:<8} lead={lead:<8} "
                  f"cmd={get(0x63E14)} cnt={get(0x53C68)}")
            shown += 1
            if shown >= n:
                break

    for title, run, is_port in (("ORIG", a.orig, False), ("PORT", a.port, True)):
        head(title, run, is_port, 10)

    for title, run, is_port in (("ORIG", a.orig, False), ("PORT", a.port, True)):
        S, mk = side(run, is_port)
        S = sorted(S, key=lambda s: s[0])
        print(f"== {title}: {len(S)} samples, jumps >= {a.min} px")
        prev = None
        for s in S:
            get, ent = mk(s)
            cam = s32(get(CAM))
            if cam is None:
                continue
            if prev is not None and abs(cam - prev[1]) >= a.min:
                c = ctx_of(get, ent)
                print(f"   t={s[0]:>7} cam {prev[1]:>6} -> {cam:<6} (d={cam - prev[1]:+})  "
                      f"cnt={c['cnt']} cmd={c['cmd']} col={c['track_col']} stage={c['stage']} "
                      f"room[0]={c['lo']} room[8]={c['hi']} r2a={s32(c['r2a'])} r36={s32(c['r36'])} "
                      f"pstate={s32(c['pstate'])} px={s32(c['px']) if c['px'] is not None else '-'}")
            prev = (s[0], cam)


if __name__ == "__main__":
    main()
