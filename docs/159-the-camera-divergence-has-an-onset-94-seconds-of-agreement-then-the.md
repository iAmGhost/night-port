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

Rig consequence, filed as an open item: the oracle should *report* shipped-quiet windows instead of papering
over them. `replay_original.py` stores a state only when `changed_ds` says something moved, and `diff_state`'s
`bucket()` carries the last sample forward (which is correct for value comparison but hides the fact that a
13-second window had zero shipped activity while the port was busy). A `SHIPPED-QUIET` per-window verdict, with
the window length and the port's step count in it, is the missing label — it turns "no disagreement" into a
statement about the game rather than about the capture.

## 6. Done: the oracle now labels quiet windows instead of hiding them

`diff_state.py` counts shipped samples captured *inside* each window (before the carry-forward) and reports:

```
anchor 0x63e14:67: orig t=72005 ms, port t=70948 ms -> port timeline shifted by +1057 ms
shipped-quiet windows: 27 of 274  (89.8 s of the route; 27 of them with the other side taking samples)
residual same-width gameplay divergences: 38
```

Every quiet window is also written per-cell as `SHIPPED-QUIET` with its length and the port's sample count for
that window, so a reader can see which comparisons were between two live sides and which were between a static
original and a busy port. The residual stays 38 — none of those cells was disagreement-only-in-quiet-windows —
which is the point: the count is now a statement about comparable windows.

On this route 89.8 s of 387 s had no shipped sample at all. The claim "the port was animating while the shipped
program was frozen" needed the same treatment as everything else in this file: measure which cells the port
actually changed inside those windows, excluding entropy cells on both sides (docs/101), and using the port's own
cell table so the entity record slots (`ent00..ent11`) are visible. Result — **5 of 27 are entropy motion only
(agreement), 22 of 27 carry real gameplay motion on the port side**:

```
shipped-quiet windows: 27 of 274  (89.8 s of the route); 22 with non-entropy port activity, 5 entropy-only
   P15@88000  : 64.0 s quiet, 22 port samples, 13 cells: ent00, ent01, ent02, ent03, 0x51c98, 0x53c68, 0x53c74...
   P17@155000 :  2.6 s quiet, 25 port samples, 13 cells: ent00..ent02, 0x62a94, 0x63360, 0x6339c...
   P29@162570 :  0.7 s quiet,  7 port samples, 18 cells: ent00..ent03, 0x53c68, 0x53c6c, 0x53c74...
   (plus 19 more windows of 0.7-2.6 s, all moving entity records, 0x53c68 entity count, and the hit/gauge
    bookkeeping 0x63360 / 0x6339c)
```

So the finding survives, but with the right shape: a **64-second window** (`P15`, anchor-relative ~+16 s) in which
no masked byte of the shipped data segment changed while the port moved its player and enemy records, its entity
count (`0x53c68`) and its hit/gauge bookkeeping (`0x63360`, `0x6339c`) across 22 samples — and 19 further
short windows of the same character. The rig's first rephrasing was wrong (it counted 27/27 as "port active");
with entropy cells excluded and record slots visible, 5 of them are pure clock/RNG motion and are agreement.

The rig defect that produced the wrong split is worth naming: the quiet-window activity scan iterated the shared
rig cell table, which does not contain the port's `ent##` record slots. Those slots are where almost all real
port activity happens, so a scan over the shared table reports "nothing moved" for 27 of 27 windows. Fixed to
iterate the port's own meta cell list.

## 8. Flagged as unresolved: the oracle and an ad-hoc probe disagree about `P15`

The oracle's own quiet-window scan says `P15@88000` is 64.0 s with 22 port samples and 13 cells moving
(`ent00..ent03`, `0x53c68`, `0x53c74`, `0x51c98`, …). A throwaway probe written to ask "what is the port actually
doing in that window" printed the opposite for the cells it looked at:

```
PORT samples in P15: 21
  t=88062  g_dat_6336c=0  g_dat_53100=0  g_dat_53c68=0  g_dat_639dc=0  g_dat_63e14=0  g_dat_51c98=0 ...
  t=151960 g_dat_6336c=0  g_dat_53100=0  g_dat_53c68=0  g_dat_639dc=0  g_dat_63e14=0  g_dat_51c98=0 ...
```

Entity count zero and command-list zero in a window where the oracle says 13 cells moved: the two readings cannot
both be right, and by docs/143's rule the defect is in the reading layer, not the game. Two concrete suspects,
both mine:

1. **Time base.** The shipped `timeline.json` entries carry `t_wall` in **seconds** (`0.031`, `0.267`, `0.475`),
   and `load_orig` converts that clock to drive-nominal milliseconds through `timeline.json["rebases"]`. The probe
   compared raw `t_wall` values against window bounds expressed in nominal ms, so it selected the wrong shipped
   states — the same class of bug as docs/158 §5's `t_ms` invention, one file later.
2. **Cell mapping.** The probe looked names up in the *port* meta cell table by `offset`; entity-record slots and
   DS cells have different offset conventions there (`offset` vs `rec_offset`), so a zero can be a misread rather
   than a value.

Until this is reconciled inside the oracle (one code path, one clock, one cell table), the honest statement is:
**27 of 274 windows are shipped-quiet, 22 of them show non-entropy motion on the port side per the oracle's own
scan, and the specific characterisation of the largest one (`P15`, 64.0 s) is contested by a probe and therefore
not a finding yet.** §6's per-window cell lists stay as the oracle's output; the interpretation "the port animates
entities where shipped's data segment is inert" is downgraded to a hypothesis with a known measurement conflict,
and the first step to settle it is to have `diff_state.py` print, for each quiet window, both sides' entity count
and command-list value from the same code path that produced the verdict.

## 9. Resolved inside the oracle: the port's camera is 78 while shipped's is parked at 0

`diff_state.py` now prints, for every quiet window, both sides' entity count (`0x53c68`), room command list
(`0x63e14`) and camera x (`0x639dc`), read through the same offsets the verdict loop uses and the port's own blob
offsets (§8's fix). The conflict dissolves:

```
   P29@162570: 0.7 s quiet, 7 other-side samples, 18 cells moving: ent00..ent03, 0x53c68, 0x53c6c, ...
        shipped carried sample: g_dat_53c68=3  g_dat_63e14=67  g_dat_639dc=0
        port first/last in window: g_dat_53c68=3  g_dat_63e14=67  g_dat_639dc=78
   P31@163400: (same reading)   shipped cam=0   port cam=78
   P24@160160: (same reading)   shipped cam=0   port cam=78
```

So: **entity count and command list agree (3 and 67 on both sides); the camera does not — shipped is parked at 0
for a whole 0.7 s window (and 64 s in `P15`) while the port holds 78.** The §8 probe's `g_dat_639dc=0` for the
port was the misread, exactly as docs/143's rule predicts: a dump-agrees-with-dump / disagrees-with-live conflict
locates the defect in the reading layer.

That restores the camera divergence as a finding, and sharpens it: the disagreement is a *parked vs scrolled*
state, held constant across whole windows, with the population and the command list in agreement at the same
moment. The scroll guard at `f.c:13774-13901` (`room[+0x36]` / `room[+0x32]` with the `DAT_00053c68 == 1` escape
arms) is the code that decides parked vs scrolled, and docs/143's queued question — which block `room[+0x32]`
and `room[+0x36]` actually belong to — is now the blocking measurement, because at the disagreement moment the
port's camera sits at 78 = a value the scroll-right arm produces (`+3` per call, 26 calls) while shipped stays 0.
