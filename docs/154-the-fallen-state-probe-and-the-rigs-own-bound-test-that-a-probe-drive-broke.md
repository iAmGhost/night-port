# docs/154 — the fallen-state probe, and the rig's own bound test that a probe drive broke

## 1. A rig guard that overflowed (the port-side crash was the rig's)

`scripts/rig/drives/ep1_atypes.txt` probes record offsets with a sentinel:

```
131000 ent 0 0xffffffff 0
131300 ent 1 0xffffffff 0
...  (six of them)
```

`rig_ent_poke` bounded the offset like this:

```c
if (!e || off + 4u > RIG_REC || off + 4u > rec_span(e)) return 0;
```

For `off = 0xffffffff`, `off + 4u` wraps to 3, both comparisons pass, and the `memcpy(e + off, &v, 4)`
executes. Under `night-asan-o2`:

```
ERROR: AddressSanitizer: BUS on unknown address ... The signal is caused by a WRITE memory access.
    #0 rig_ent_poke rig_sink.c:167
    #1 pt_drive main.c:684
    #2 input_pump main.c:1139
SUMMARY: AddressSanitizer: BUS rig_sink.c:167 in rig_ent_poke
```

The plain build took it as `rc=138` (signal 10), which until now would have been logged as "the port crashed
on drive ep1_atypes". Fixed by bounding the arithmetic before the span test, and reporting instead of writing:

```c
if (!e || off > RIG_REC - 4u || off + 4u < off || off + 4u > rec_span(e)) {
    if (e) { g_poke_refused++; fprintf(stderr, "rig: ent poke slot=%d off=0x%x refused (outside record)\n", slot, off); }
    return 0;
}
```

Red -> green on the same drive (`ep1_atypes`, `--warp-stage 1`, 9000 frames, `night-asan-o2`):

| | before | after |
|---|---|---|
| exit status | `rc=138` (SIGBUS) | **`rc=0`** |
| ASan errors | 1 (WRITE BUS at rig_sink.c:167) | **0** |
| refusals | — (it wrote) | **3 refused, each printed** |

`make -C port test` ALL PASS after the change. The same overflow existed in `rig_player_poke` and is fixed the
same way.

## 2. The fallen-state samples I had were injections

docs/117/118 left the fallen family (`0x5b..0x66`) as a coverage gap. Two drives exist for it
(`ep1_fallen.txt`, `ep1_grab.txt`), and a census of their dumps appeared to produce a fallen state:

```
fl_ep1_fallen  states top: [1x605, 2x378, 40x301, 41x7, 12x6, 11x4]   fallen-band: {(type 0, 0x65): 1}
fl_ep1_grab    states top: [1x494, 2x256, 40x115, 8, 51x7, 11x5]      fallen-band: {(type 0, 0x65): 1}
```

Both are rig-induced. The drive says so:

```
128060 ent 1 0x17 0x65      <-- the drive itself writes state 0x65 into entity slot 1
```

So the first "fallen state seen in the port" was the rig writing it, not the game producing it. It is recorded
here rather than filed as a finding.

## 3. The differential run without injections

In injection drives the shipped side cannot run at all — the replay script rejects the verbs
(`unknown drive step: 128000 play 0x17 101`, which is why `/tmp/rig/g13` has a `port` half and no `orig`
half). That is docs/151's injection-parity gap with a concrete cost: an injected drive is not a differential
drive. The workaround used here (and worth keeping as the rule): strip the verbs and run the plain timeline.

```sh
awk '!/^[0-9]+ (poke|ent|play) /' scripts/rig/drives/ep1_fallen.txt > scripts/rig/drives/ep1_fallen_b.txt
OUT=/tmp/rig/g14 RIG_HZ=4 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_fallen_b.txt 140
```

`/tmp/rig/g14`, reaction and fallen states by `(entity type, state)`:

| side | samples | `0x0b` | `0x0c` | fallen band `0x5b..0x66` |
|---|---|---|---|---|
| shipped | 447 | 21 | 19 | **0** |
| port | 790 | 4 | 0 | **0** |

Same route's media oracle for reference: channel-order check `declared=1.000` (a byte-exact chroma match) and
pacing `orig=5.09 / port=9.80` picture changes per second (ratio 0.52 — much closer than the combat routes'
0.16-0.18, because this drive barely fights).

## 4. Status of the fallen-state item

It stays open, and the reason is now precise rather than "we have not looked":

* no drive in the corpus produces a fallen state on **either** side, so there is no differential statement to
  make about it;
* the mechanism's precondition is documented (docs/118: the pick-up block at guest 0x00014de1+ fires when the
  **attacker** is in state 0x65/0x99, pairing attacker 0x66 with victim 0x1f through mutual `+0x4a` links), and
  the episode-1 container census found exactly one fallen-class record (attacker type 0, state 0x65, slot 0,
  atype 0x15) — a record that can only be used by an entity already in a fallen state, i.e. it has no entry
  point in this episode's record set;
* state injection cannot substitute for a drive: the entity controller rewrites `+0x17` every frame (docs/118),
  and injected drives cannot run on the shipped side at all (§3).

What would close it: a drive that reaches a *throw or pick-up* interaction — the shipped action sets that put
an entity into 0x65/0x99 — and a shipped-side replay of the same drive. Both halves are already in place
(`RIG_ENTS=12`, the state census in docs/152's scripts); only the drive is missing.

## 5. Reproduce

```sh
make -C port asanapp-o2
env ASAN_OPTIONS=detect_leaks=0 ./port/test/night-asan-o2 --res-dir ./NIGHT \
    --script scripts/rig/drives/ep1_atypes.txt --warp-stage 1 --frames 9000 2>&1 | grep -E "refused|AddressSanitizer"
OUT=/tmp/rig/g14 RIG_HZ=4 RIG_ENTS=12 sh scripts/rig/route.sh scripts/rig/drives/ep1_fallen_b.txt 140
```
