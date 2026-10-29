# docs/158 — what `0x62a8c` really is (the mover's wall-scan bound), and a retraction of this file's headline

> **Correction, measured in the same session (see §5).** The headline claim — that the port walks the opening of
> the episode with zero clamp records while shipped has nine — does not survive measurement. The anchor used
> below ("first live entity") is *not* a comparable event: the shipped title screen already has live entities at
> 400 ms, the port's title screen is reached at 60 s, and once the clamp-table count is compared **relative to
> the room-script burst itself**, both sides go `0 -> 9` at that burst and both hold 9 afterwards. `0x62a8c`'s
> DIFF verdict is a phase artifact, not a value divergence. The cell attribution in §1-§2 stands.

Cell `0x62a8c`, which docs/157 §2 had parked under "asset / list bookkeeping, not yet attributed", turns out to be
the **count of the trigger/wall table** `g_tab_5c8bc[0..count)` that bounds the mover's wall scan
(`f.c:9105` and `f.c:9279`, `for (i = 0; i < g_dat_62a8c; i++)`, guest `0x00015502`). Reading it sample by
sample on `/tmp/rig/g15` and anchoring on the first live entity instead of wall-clock gives a concrete
divergence:

```
ORIG: first live entity at t=400 ms   (cmdlist DAT_63e14 = 88, clamp count = 0, camera x = 0)
PORT: first live entity at t=60079 ms (cmdlist = 88, clamp count = 0, camera x = 0)

ORIG: cmdlist 88 -> 67 at 5900 ms   = anchor +5500 ms
PORT: cmdlist 88 -> 67 at 70948 ms  = anchor +10869 ms

ORIG: clamp count 0 -> 9 at anchor +5500 ms
PORT: clamp count 0 -> 9 at anchor +10869 ms
```

So on both sides the room-script drain (`88 -> 67`, 21 commands) and the clamp-table install (count `0 -> 9`)
happen in one burst, and after that both sides hold 9 for the rest of the route. What differs is **when**: the
port reaches that burst 5.4 s of gameplay later than shipped. Between anchor +5.5 s and anchor +10.9 s the port
is running its room with **zero clamp records** while shipped has nine — i.e. the mover's wall scan iterates an
empty table in the port and nine entries in shipped, for the entire opening of the episode.

## Why this cell matters more than its neighbours

`0x62a8c` is not passive bookkeeping. From `scripts/audit/mem_access.json` (10 guest accesses total):

| guest site | function | access |
|---|---|---|
| `0x00015502` | `fn_01545e` (the mover) | CMP — loop bound of the wall/clamp scan |
| `0x00015903` | `fn_01586b` | CMP — same bound in the sibling scan |
| `0x0003eb8f`, `0x3eba2`, `0x3ebb5`, `0x3ebc8` | `fn_03e20f` (room-script interpreter) | IMUL — sizes the table writes |
| `0x0003ebdb`, `0x0003ed17` | `fn_03e20f` | INC — installs clamp records (`f.c:26754`, `f.c:26777`) |
| `0x00037b1e` | `fn_037b03` (the `.stg` reader) | MOV 0 — clear |
| `0x0003ee73` | `fn_03ee0a` (list-destroy) | MOV 0 — clear |

So one function *writes* the count (`fn_03e20f`, the same interpreter docs/139 and docs/153 examined), and the
mover *reads* it on every step. A count that arrives 5.4 s late means the opening of the episode is walked
without walls.

## How it connects to the cluster

docs/149 measured the walk quantum (shipped ±60 px/s, port ±70 px/s with a port-only ±80 band) and docs/152 §4b
measured that the port dispatches a mover step only 0.155 times per step dispatch, with most displacement coming
from camera-anchor stores. The clamp table is what turns a step into a *stopped* step. A route where the clamp
count is 0 for the first five seconds, and where (docs/153, and the extra `67 -> 66 -> 65 -> 64 -> 62 -> 61`
unlinks visible above at 172–223 s) the command list keeps being re-entered later, is exactly the shape of the
population/camera divergence in docs/142: shipped camera parks at 0..27, the port reaches 158.

## What is still unexplained

* **Why the burst is later in the port.** Both transitions sit at a single 10 Hz sample on each side, so it is
  not a slower drain — it is a later start. The window between anchor +0 s and the burst is where the port and
  shipped disagree about elapsed game time, and the candidates are the slide-show / intro counts (docs/122 fixed
  `g_dat_52f08` picture counts per stage) and the room-load wait in `fn_03cad3`/`fn_03ed6f`.
* Whether the *content* of the nine installed clamp records matches between the sides — the count agreeing does
  not mean the entries agree. `g_tab_5c8bc` is a ROM clamp table (docs/158 header note: "512 entries, count =
  `g_dat_62a8c`"), so the installed records live in the heap and need a record-level dump to compare.

## Reproduce

```sh
OUT=/tmp/rig/g15 RIG_HZ=10 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300
# per-sample walk of 0x53c68 (entity count), 0x62a8c (clamp-table count), 0x63e14 (room command list),
# 0x639dc (camera x) on both sides, anchored on the first live entity:
python3 - <<'PY'
import sys, json
sys.path.insert(0,'scripts/rig'); from diff_state import load_port
LO=0x50000; u=lambda b,o: int.from_bytes(b[o:o+4],'little')
tl=json.load(open('/tmp/rig/g15/orig/timeline.json'))
O=[(st['t_ms'], u(open(f'/tmp/rig/g15/orig/state_{k:04d}.ds.bin','rb').read(), a-LO))
   for k,st in enumerate(tl['timeline']) for a in (0x53c68,0x62a8c,0x63e14,0x639dc)]
PY
uv run python3 -c "import json;m=json.load(open('scripts/audit/mem_access.json'));print([r for r in m if any(x.get('to')=='0x62a8c' for x in r.get('refs',[]))])"
```

## 5. Retraction: the numbers in §3 came from a broken probe, and the cell is a phase artifact

> **The probe bug.** §3's times (`orig 400 ms`, `burst 5900 ms`, `port 60079 ms`) were produced by a throwaway
> script that read `timeline.json` entries as `st.get("t_ms", k*100)`. Shipped timeline entries do not carry
> `t_ms` at all — they carry `t_wall` (plus `ds_sha`, `frame_sha`, `mode`, `w`, `h`, `changed_ds`) — and
> `diff_state.load_orig` converts the emulator clock into drive-nominal time through `timeline.json["rebases"]`.
> So the script invented a fake clock (`k*100`), and every conclusion drawn from it is void.

Re-measured through the oracle itself, with a **transition** anchor (`0x63e14 == 67`, the room-script drain, a
event both sides define identically):

```
anchor 0x63e14:67: orig t=72005 ms, port t=70948 ms -> port timeline shifted by +1057 ms
residual same-width gameplay divergences: 39 -> 38
g_dat_62a8c: DIFF -> not flagged
```

So the two sides were only ~1 s out of step, not 65 s; the clamp-count transition sat close enough to a phase
window boundary for a 1 s slip to flip its verdict. **`0x62a8c` is a phase artifact and leaves the residual
list.** There is no evidence that the port walks its opening with an empty clamp table, and §1's headline is
withdrawn.

## 6. What came out of it: `--anchor` in the state oracle

`diff_state.py` now takes `--anchor <ds>:<value>`: it finds the first sample on each side whose DS cell holds
that value and rebases the port timeline onto the original's before cutting any phase window. That is the
docs/101 rule (align on an event, not a frame index) enforced inside the oracle rather than left to the operator
— and a level measurement ("first nonzero entity count") is explicitly rejected as an anchor, because the
shipped title screen already owns entities while the port is still booting.

Residual gameplay divergences on `/tmp/rig/g15` after the anchor (38 cells):
`0x51c98`, `0x53100`, `0x53451`, `0x53640`, `0x53c48`, `0x53c64`, `0x53c68`, `0x53c6c`, `0x53c88`,
`0x62a90`, `0x62a94`, `0x62d58`, `0x62d5c`, `0x6335c`, `0x63360`, `0x63364`, `0x6336c`, `0x6339c`,
`0x633b0`, `0x633b4`, `0x639bc`, `0x639c8`, `0x639d8`, `0x639dc`, `0x639e8`, `0x63e14`, `0x63e18`,
`0x63e20`, `0x63e5c`, `0x63ea5`, `0x63ed0`, `0x63ed3`, `0x63ed5`, `0x63ed8`, `0x63ffc`, `0x64000`,
`0x64090`, `0x64094`.

The camera block (`0x639bc..0x639e8`), the population cells (`0x53c64/68/6c`, `0x53c88`) and the room command
list (`0x63e14/18/20`) survive proper anchoring, which is what docs/142 → docs/153 → docs/155 point at: it is
behavioural, not phasing.

## 7. Reproduce

```sh
OUT=/tmp/rig/g15 RIG_HZ=10 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300
python3 scripts/rig/diff_state.py --orig /tmp/rig/g15/orig --port /tmp/rig/g15/port \
        --drive scripts/rig/drives/ep1_kill3.txt --anchor 0x63e14:67 --md /tmp/rig/g15/state_diff_anchored.md
# the probe bug is reproducible: shipped timeline entries carry "t_wall", not "t_ms"
python3 -c "import json;t=json.load(open('/tmp/rig/g15/orig/timeline.json'));print(list(t['timeline'][0].keys()))"
```
