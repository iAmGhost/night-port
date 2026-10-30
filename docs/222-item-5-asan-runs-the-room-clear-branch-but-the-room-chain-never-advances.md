# 222 — Item 5, route attempts on the ASan build: the room-clear branch runs but the room chain never hands over a next room, so stage 2 is unreachable naturally

Gate: `gate3-stage2-crash`, unfixed build, `port/test/night-asan` (built by `make -C port test/night-asan`).
Canonical pinned env: `NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, 13200 frames, 2200 cell
samples at 10 Hz.

## 1. Two routes built, both aimed at fn_03af18's "1 -> room clear" tail

The target tail is `f.c:27776-27785` (guest 0x0003b1ce-0x0003b54c): `fn_03ace9()` composite, the `%de%d.pcx`
end slides from `g_tab_52f90[stage]` (`room_end_slideshow`, `f.c:27601`), `g_dat_63370 += 1`, `fn_038f3d(1)` at
guest 0x0003b54c, `fn_0374ad()`, then the stage-2 start slide show (`g_tab_52f2c[stage]` = 2 → `2s1.pcx`,
`2s2.pcx`) and `stage2.pcx`.

**Route A — `scripts/rig/drives/ep1_roomwalk.txt`** (poke DS 0x639bc = 4000 every 1500 ms, hoping to satisfy the
kill-quota test).  Negative, and the reason is structural: the quota test is not a periodic check.  It lives in
the victim's settle path (`f.c:26357-26360`, guest 0x00012a55: `quota = g_tab_52a1c[...]; if (quota <=
g_dat_639bc) g_dat_6336c = 4;`), so it only runs when an entity dies.  Raising the counter changes nothing:
no `endcard` line, `memstat stage=2` count 0, rc=0.

**Route B — `scripts/rig/drives/ep1_roomwalk2.txt`** (poke DS 0x6336c = 4, the value the shipped quota test
writes; `fn_03a829`'s tail at `f.c:23465`, guest 0x0003ac80-0x0003accd, turns it into "take the next room record
from the chain link" + `fn_03d244(room)`).  74 pokes, each logged accepted:

```
script: poke ds 0x6336c = 4 (ok) at t=88150ms            (x74)
```

## 2. The fault census: no fault

`/tmp/rig/rw2` (route B under ASan + UBSan, `ASAN_OPTIONS=halt_on_error=1:detect_leaks=0`,
`UBSAN_OPTIONS=print_stacktrace=1`):

* `rc = 0`; **zero** `ERROR: AddressSanitizer` reports; zero SIGSEGV; zero bus error.
* one UBSan report, pre-existing and stage-independent: `src/f/f.c:7360:9: runtime error: store to misaligned
  address 0x00010305abb9 for type 'char *', which requires 8 byte alignment` (it appears at the same site on
  routes that never touch the room-clear path, so it is not a stage-2 artifact).
* the port's own escape line at the end: `game main unwound (quit / fatal)` — the host `longjmp(g_game_escape)`,
  not a fault (docs/208 already recorded that the historical `bus error ./test/night` is not reproducible by
  writing the stage cell).

Cell census over the 2200 samples:

| DS cell | meaning | census |
|---|---|---|
| 0x6336c | stage/cycle event request | 0 in 2200/2200 (the branch consumes it in the same frame it is poked) |
| 0x63370 | stage | 1 in 2200/2200 -- **never 2** |
| 0x63354 | room record pointer | 3 distinct: 0 (502 samples), 1627884 (388), 1628768 (1310) -- the room record does change |
| **0x63398** | **the next-room id taken from the chain link** | **0 in 2200/2200** |
| 0x63e24 | room tick | 2101 distinct (the room loop is live) |

Resource lookups (`markers.jsonl`, 291 markers / 165 distinct) show `1s1.pcx`, `1s2.pcx`, `1s3.pcx`,
`stage1.pcx` requested, and **no** `1e1.pcx` (stage 1's end slide), no `2s1/2s2.pcx`, no `2e1.pcx`, no
`stage2.pcx` -- `fn_03af18`'s room-clear tail was never entered.

## 3. Why the walk stops: the chain link's next-room byte is never non-zero

The shipped's read is at guest 0x0003ac8b-0x0003ac99:

```
0x3ac8b  mov eax, [eax + 0x34c]        ; the room record's chain-link struct pointer
0x3ac91  mov al,  byte [eax + 0x11]    ; the NEXT ROOM ID lives there
0x3ac99  mov [0x13398], eax            ; DS 0x63398
```

and the port models it exactly (`f.c:23466`: `g_dat_63398 = *(const uint8_t *)(rec_ptr_get(room, 0x34c) + 0x11);`,
with `rec_ptr_get` at `f.c:168` reading the widened host pointer the port stores at +0x34c).  So the *code* is
faithful; the *data* is not: nothing in the port ever writes that `+0x11` byte in the room-chain context.  A
grep for writers of a record's `+0x11` in `f.c` returns only the doubly-linked-list slots (`node+0x11`,
`g_dat_639d0+0x11`, `prev+0x11` at `f.c:1138-1174`), the entity/heap slots (`block+0x11`, `a+0x11`,
`g_player_ent+0x11`, `e+0x11` at `f.c:1292-9286`) -- none of them the struct the room record points at through
+0x34c.  With that byte pinned at 0, `fn_03a829` re-enters the same room and never returns 1, so
`fn_03af18`'s stage-advance tail -- the exact code the playtest report says crashes -- is unreachable from
*inside* the game on every route tried so far (this one, docs/208's stage-cell poke, and the natural route that
no drive has produced).

## 4. Shipped contrast, and its limit

The same drive on the shipped side (`/tmp/rig/ow1`, 719 distinct states at 4 Hz over 220 s, pinned
`cpu_cycles = 120000`) accepted every poke and read it back: `poke 0x6336c <- 4 : read back 4` at every poke
stamp from t=87055 to t=153088 ms.  That is **not** yet a behavioural contrast: the read-back happens in the
same guest frame as the write, while the port's 0 is a *sampled* value taken later, and this run's
`timeline.json` carries only `ds_sha`/`frame_sha` hashes per state, so the shipped's DS 0x63398 series has to be
recovered from the state dumps (or by adding 0x63398 to `replay_original.py`'s scalar cell list) before any
claim about the shipped's room chain can be quoted.

## 5. Next measurement, in the order that settles the item

1. Write-census for `byte [X+0x11]` where `X = [room_record + 0x34c]` over the shipped bodies
   (`scripts/audit/field_writers.py`, docs/187) to find who builds the room chain -- expected: the stage data
   loader `fn_0374ad` / the room-record builder, reading the 100-entry room table at DS 0x52da4 (docs/185).
2. Same census on the port side, so the missing writer is named rather than inferred.
3. A stage-2 route that does not depend on the chain: `writemem 0x6336c 2` is the shipped's *exit-rect* request
   value, and `docs/208` shows the port survives it; the crash candidate is the *room-clear* tail only, so the
   chain fix (1-2) is what makes a natural stage-2 route possible at all.
4. Screen oracle for "the floor is unrendered": measure the room floor band in the video arena
   (`fn_03f15f`'s linear model, `f.c` composite, docs/208 §4) rather than the presented frame, because the two
   dumps' presentation bases are not comparable (docs/202, docs/205).
