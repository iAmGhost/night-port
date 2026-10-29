# docs/159 — the camera divergence has an onset: 94 seconds of agreement, then the port scrolls and shipped does not

Anchored on the room-script drain (`DAT_00063e14 == 67`, orig t=72005 ms, port timeline +1057 ms — docs/158 §6),
camera x `DAT_000639dc` was compared sample-by-sample across `/tmp/rig/g15` (300 s route, both sides at 10 Hz):

```
camera-x disagreements: 469 of 1846 shipped samples
first disagreement: anchor +93.9 s   (orig cam=0, port cam=78, port entity count 3)
max camera x: orig 73, port 298
```

> **Caveat published immediately (measured in the same pass).** The shipped dump is *change-triggered*
> (`timeline.json` entries carry `changed_ds`; docs/151 notes the shipped side only records a state when it
> changes), so a window with no shipped sample cannot produce a disagreement. Counting aligned steps per window:
>
> ```
> ORIG anchor+80..93 s :  0 shipped samples     PORT anchor+80..93 s : 126 port steps
> ORIG anchor+93..105 s: 73 shipped samples     PORT anchor+93..105 s: 117 port steps
> ORIG anchor+105..140 s: 226 shipped samples   PORT anchor+105..140 s: 343 port steps
> ```
>
> So "94 seconds of agreement" is **94 seconds of no shipped samples**, and the onset is really "the first window
> in which shipped sampled at all and the two sides disagreed". The usable statements are the per-window ones
> below, and closing this properly needs a shipped-side capture at a fixed cadence rather than a change-triggered
> one. For the first 94 seconds after the room script drains, the two sides
hold the same camera value at every aligned sample. Then the port jumps to 78 while shipped stays at 0, and the
port goes on to 298 against shipped's 73 maximum.

## Why the onset matters more than the maxima

docs/142 compared value *ranges* (shipped 0..27, port 0..298) and docs/158 §5 then showed a range comparison is
fragile when the windows slip. The onset statement is stronger and it rules things out:

* it is **not** room-load camera initialisation (`fn_03cad3` writes the camera at `0x3cb02`/`0x3cb14` — if that
  were wrong, disagreement would start at anchor +0 s, and it does not);
* it is **not** the clamp-table bound arriving late — `g_dat_5c8bc`'s count (`0x62a8c`) reaches 9 on both sides at
  the drain itself (docs/158 §5), 94 s before the disagreement;
* it **is** downstream of something that happens 94 s into the room. The port's entity count at the onset sample
  is 3, and the port-only extra command-list drains on this route (`67 -> 66 -> 65 -> 64 -> 62 -> 61`) sit at
  172–223 s, i.e. *after* the camera has already diverged — so the drains are not the trigger either.

What remains as the trigger candidate is the scroll driver's own guard, `f.c:13774-13901`:

```c
if (room[+0x36] == 0 || cam + 0x140 <= room[+0x36]) fn_03cb38(DAT_63354);   /* scroll right  */
if (room[+0x32] == 0 || cam >= room[+0x32])         fn_03cbd9(DAT_63354);   /* scroll left   */
```

with the `DAT_00053c68 == 1` escape arms. `fn_03cb38` adds 3 (or INC) and `fn_03cbd9` subtracts 3, and the
clamp-block bounds it consults are the ones docs/143 named. At the onset the port moves by 78 in one aligned
sample step, which is 26 scroll quanta — that looks like many scroll calls compressed into one 100 ms sample
window, i.e. the port is scrolling repeatedly where shipped does not scroll at all.

## Next measurement (queued, not yet done)

Count `fn_03cb38` / `fn_03cbd9` invocations per second on the port side across the onset, and compare against
the shipped side's camera delta over the same windows. If the port calls the scroll-right arm ~26 times per
sample window where the shipped delta is 0, then the divergence is in the guard's *condition* (`room[+0x36]` /
`room[+0x32]`, or the `DAT_00053c68 == 1` escape), which is docs/143's queued question about which block those
offsets actually belong to.

## Reproduce

```sh
OUT=/tmp/rig/g15 RIG_HZ=10 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300
python3 - <<'PY'
import sys; sys.path.insert(0,'scripts/rig')
import diff_state as dt
o,_ = dt.load_orig('/tmp/rig/g15/orig'); p,cells = dt.load_port('/tmp/rig/g15/port')
LO=dt.DS_LO; off={c['ds']:c['offset'] for c in cells}
u=lambda b,o: int.from_bytes(b[o:o+4],'little')
def first(S,a,port):
    o=off[a] if port else a-LO
    for s in sorted(S,key=lambda x:x[0]):
        if o+4<=len(s[1]) and u(s[1],o)==67: return s[0]
ao=first(o,0x63e14,False); ap=first(p,0x63e14,True); sh=ao-ap
O=[(s[0],u(s[1],0x639dc-LO)) for s in o]
P=[(s[0]+sh,u(s[1],off[0x639dc])) for s in p]
bad=[t-ao for t,c in O for tc,cc in [min(P,key=lambda x:abs(x[0]-t))] if abs(tc-t)<=250 and c!=cc]
print('disagreements',len(bad),'of',len(O),'first at anchor+',min(bad) if bad else None,'ms')
PY
```

## 5. Per-window step census (the honest version of this measurement)

Aligned sample steps and camera jumps of >= 6 px, anchored on `DAT_00063e14 == 67`:

| window | shipped samples | shipped jumps >= 6 px | port steps | port jumps >= 6 px |
|---|---|---|---|---|
| anchor +80..93 s | **0** | 0 | 126 | 10 |
| anchor +93..105 s | 73 | 2 (max +9 px) | 117 | 8 (max +7 px) |
| anchor +105..140 s | 226 | 13 (max +10 / −10 px) | 343 | 21 (max +8 px) |

Where both sides are actually sampled, the *jump sizes* agree closely (shipped +9/+10 px, port +7/+8 px) and the
*sample densities* do not (shipped 73 vs port 117 in the same 12 s). That is consistent with the shipped dump
missing quiet states rather than with a camera-arithmetic divergence, and it makes the "port reaches 298 vs
shipped 73" range gap partly a coverage gap.

Rig consequence, filed as an open item: the shipped side needs a **fixed-cadence** DS capture (not
change-triggered) before camera/population range comparisons can carry weight. `replay_original.py` already
samples at a cadence for DS cells; what is missing is that its *timeline* only lists changed states, and the
oracle's `bucket()` then carries the last sample forward — which silently equates "unchanged" with "agreement".
