# 90 — The stage-8 "early unwind" is a one-room stage: the cleared next link is the shipped chain terminator

Goal task `stage8-early-unwind`. Symptom on file: a stage-8 scripted run printed

```
[room-chain] fn_03cc6b got a NULL next link: a0=... head=... cur=... state639e4=0 roomCount=1 stage=8 camX=0 camY=0
```

and stopped presenting frames around frame 5654, which had been read as the episode ending early.

## Verdict up front

**Not a port defect, and not an unwind.** Two separate things were conflated:

1. `frames presented: 5657` is where the *scripted run* ends, not where the game stops. The host ends the
   process 2.5 s after the route script's last event (`src/app/main.c:958`), and the same script ends a
   stage-1 run at `frames presented: 5459`. The frame number tracks the script, not stage 8.
2. The witness fires because **stage 8 is a one-room stage in the shipped data**, so its only room node
   carries the cleared `next` link that the shipped chain builder itself writes at 0x0003ca5e. Stage 5 is the
   same shape and produced the identical witness in docs/52.

## Which link, which record (port witnesses, new `NIGHT_ROOMDBG`)

`fn_03c853` builds the room chain recursively and terminates it with
`if (++DAT_0006335c == DAT_000639d8) rec_ptr_set(node, 0x34c, NULL)` (0x0003ca4e-0x0003ca5e), where
`DAT_000639d8` is the room count and is filled from the first dword of `stageN.wal` at 0x00037bda. A cleared
link on the *last* node is therefore normal shipped state; `fn_03cc6b` (the room-change state machine, called
from the player tick at 0x0003d5dd-0x0003d5f8 whenever the vertical camera condition holds) reads that link
unguarded.

New witness lines (same run, capped):

```
stage 8:  [room-chain] stage=8 node#0 counter=1/1 next=NULL (terminator 0x0003ca5e)
          [room-chain] fn_03cc6b NULL-link hit #1 node#0 head=a0 cur=a0 rooms=1 stage=8 camX=0 camY=0 state=0
          ... hit #3137, always node#0, head=a0, cur=a0, rooms=1, camX=0, camY=0, state=0
stage 1:  [room-chain] stage=1 node#0 counter=1/4 next=to be linked
          [room-chain] stage=1 node#1 counter=2/4 next=to be linked
          [room-chain] stage=1 node#2 counter=3/4 next=to be linked
          [room-chain] stage=1 node#3 counter=4/4 next=NULL (terminator 0x0003ca5e)
          (0 NULL-link hits)
```

So the record is the chain head itself (`head == cur == a0`, node index +0x40 = 0) and the link is +0x34c. The
port's guard (docs/52) returns without dereferencing, `camY` (DAT_000639e0) stays 0, and the single room keeps
playing -- 3137 evaluations of the camera condition, no state move, `rc=0`.

## The shipped counts, measured from the container

`stageN.wal`'s first dword -- the dword the loader stores to DAT_000639d8 -- decoded straight out of
`NIGHT.RES` (independent XOR-chain decode, same format as `scripts/extract_res.py`):

| episode | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| rooms (`stageN.wal`) | 4 | 4 | 7 | 2 | **1** | 4 | 3 | **1** | 8 |
| `.stg` records | 88 | 184 | 128 | 42 | 28 | 203 | 197 | **2** | 207 |

Stage 8 is the smallest stage in the game (one room, two record entries). The port's `roomCount=1 stage=8`
matches the shipped container exactly.

## The original side agrees, variable by variable

Forced episode 8 (`scripts/playtest/drive_orig_stage8_chain.txt`, dosbox-automation, 163 frames at 3 Hz), with
a new harness op `recoff <DAT> <field-off>` that reads a field of the record the game's own pointer refers to:

```
readmem  0x63370 = 8                 (episode 8)
readptr  0x6334c = 0x38202c          chain head
readptr  0x63354 = 0x38202c          current room  == head, i.e. one node
readmem  0x639d8 = 1                 room count = 1        (matches stage8.wal's first dword)
readmem  0x6335c = 1                 loaded-room counter reached the count
readmem  0x639dc = 0 / 0x639e0 = 0   camera room indices
readmem  0x639e4 = 0                 chain state
recoff   0x63354+0x34c -> 0          the next link is CLEARED in the original's own record
recoff   0x63354+0x350 -> 0x56fe0256 (the head has no back pointer: uninitialised slot)
   ... two 14 s walk-right windows (6 `key right` presses) ...
readmem  0x639dc = 0 / 0x639e0 = 0 / 0x639e4 = 0   unchanged
readptr  0x63354 = 0x38202c                         unchanged
recoff   0x63354+0x34c -> 0                         still cleared
readmem  0x6336c = 0
```

In DOS a cleared DS-relative link is offset 0, so the original's read-through addresses mapped memory
(`DS:0`, `DS:0xc`) rather than faulting; observably it also moves nothing: the chain state, the camera indices
and the current-room pointer are all unchanged after the walk windows. **The port's no-op guard reproduces the
original's observable state exactly.**

## Regression test (durable, shipped-data based)

`test_stage_chain_shipped_counts` in `test/test_f1.c` (NIGHT_RES_DIR-gated, skips cleanly like the other
shipped-container tests) -- 9 checks, ALL PASS:

```
shipped stage chains: stage{1..9}.wal room counts and the 0x0003ca5e terminator
  [PASS] all nine stage{1..9}.wal entries resolve in the shipped index
  [PASS] shipped room counts stage1..9.wal = 4,4,7,2,1,4,3,1,8 (the DAT_639d8 source, extract_res cross-check)
  [PASS] stage8.wal room count = 1: the chain has exactly one room
  [PASS]   one node loaded (the shipped count)
  [PASS]   the only node carries the cleared next link the builder writes at 0x0003ca5e
  [PASS]   fn_03cc6b on that node returns without dereferencing or advancing the chain
  [PASS] stage4.wal room count = 2
  [PASS]   stage 4 chains two nodes and only the last one has the cleared link
```

## ASan (capped)

The chain build and the `fn_03cc6b` read of the cleared link now run inside the suite, so the ASan check is
deterministic and capped:

```
make test CFLAGS="-std=c11 -O1 -g -w -fsanitize=address"
NIGHT_RES_DIR=../NIGHT ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 timeout 900 ./build/test_f1
```

Result: `rc=0`, `== ALL PASS ==`, **0** `ERROR: AddressSanitizer` / SEGV lines, with the stage-8 node build and
the `fn_03cc6b` call on its cleared link exercised on shipped data.

App-level ASan runs are a separate, filed observation: two capped runs of a stage-8 route (the docs/52 route cut
at t=152000, and a dense variant with a space press every 1.5 s) both reached the episode-8 picture show and then
sat in `fn_03a70e`'s **key-release** wait (`src/f/f.c:14681`, seen with `lldb -p <pid> -o "bt all"`), with 0
ASan diagnostics. The same routes in the plain build reach the room and log the 3137 witness hits, so this is the
route-pacing/slow-build interaction (the docs/52 show stall, in a new guise), not the room chain. Next diagnostic
step if the app-level ASan soak on stage 8 is ever needed: pace the route from the show's own picture count
(`NIGHT_SHOWDBG`-style witness of `idx`/`count` in `fn_03a70e`) instead of from wall-clock slots.

## Reproduce

```sh
# port witnesses, plain build (~190 s)
for st in 8 1; do SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_ROOMDBG=1 \
  timeout 250 ./build/night --warp-stage $st --script ../scripts/playtest/route_show_keyed.txt \
  --no-audio --frames 6500 --shot-limit 2 2>&1 | grep room-chain; done

# shipped counts, no game needed
uv run python3 scripts/extract_res.py info stage8.wal     # and the first dword, or run the suite test:
NIGHT_RES_DIR=../NIGHT ./build/test_f1 | grep -A12 "shipped stage chains"

# ASan, capped and deterministic (the chain path runs inside the suite)
make test CFLAGS="-std=c11 -O1 -g -w -fsanitize=address" && \
  NIGHT_RES_DIR=../NIGHT ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 timeout 900 ./build/test_f1 | tail -2

# original side
export DOSBOX_API_TOKEN=...
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_s8chain --drive scripts/playtest/drive_orig_stage8_chain.txt --hz 3
```

## Next diagnostic step (if exact read-through semantics are ever wanted)

The port deliberately has no DOS low-memory mirror (the same decision as the latent allocation-failure store at
0x0003cab4, docs/52). To model `DS:0` read-throughs instead of guarding them, the datum needed first is what
the shipped DS:0..0x10 actually holds at that moment, and whether `DAT_000639dc == *(DS:0)` (the test at
0x0003cca4) can ever be true -- if it can, the original would repoint DAT_00063354 at DS:0. Measure with:

```
readmem 0x0 ; readmem 0x4 ; readmem 0x8 ; readmem 0xc ; readmem 0x10
```
added to `scripts/playtest/drive_orig_stage8_chain.txt` right after the `recoff 0x63354 0x34c` line. With the
measured state (camera indices 0, chain state 0 in both samples) the original's read-through changes nothing, so
no port change follows today.
