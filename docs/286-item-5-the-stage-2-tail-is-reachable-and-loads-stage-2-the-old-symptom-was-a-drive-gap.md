# 286 — Item 5: the stage-2 tail *is* reachable and *does* load stage 2; the "no stage-2 assets" symptom was a gap in my drive, and docs/269's "both-sides stage-2 basis" was a level-up-menu basis, not gameplay

## 1. Where the tail actually stops (port witness, `/tmp/rig/s2c`, HEAD `29cb1f2`)

Witnesses at `room_start`, at the room loop's `fn_03a829` return, and at each line of the stage-advance tail
(`NIGHT_ROOMSTART=1` ✓):

```
roomstart: stage=1 vesa=1 slides=3 roomrec=0x1042f62ac
slide: card 1/3 wait / card 2/3 wait / card 3/3 wait
roomloop: r=1 stage=1            (fn_03a829 returned 1 = room clear, 0x3abca-0x3abdd ✓)
endcard: slide 1 wait
tail: stage=2                    (g_dat_63370 += 1 at f.c:28065 == 0x3b520 ✓)
                                 <-- nothing after this: fn_038f3d(1) never returns ✗✗
```

No `tail: levelup call done`, no second `roomstart` ✓ — the port is inside `fn_038f3d(1)` for the rest of the run
(`memstat stage=2 room=1 frames=11903..11961` ✓ frames keep presenting because `fn_038f3d` presents them itself ✓).

## 2. Why: the level-up screen's loop exit is Shift/Esc, and my drive only pressed Enter

Shipped `fn_038f3d` loop head (`0x3920a-0x39225`):

```
0x3920a: cmp byte [0x13e89], 0     ; DS 0x63e89 = Esc held-byte
0x39213: cmp byte [0x13eb2], 0     ; DS 0x63eb2 = LShift
0x3921c: cmp byte [0x13ebe], 0     ; DS 0x63ebe = RShift
0x39225: jmp 0x39a56               ; exit the screen
```

== port `f.c:26028` `while (g_dat_63e89 == 0 && g_dat_63eb2 == 0 && g_dat_63ebe == 0)` ✓ **faithful** ✓, and the
key-byte identity is `port/src/app/keymap.c:18-20,43-46` (`0x63e89`=+0x01 Esc, `0x63eb2`=+0x2a LShift,
`0x63ebe`=+0x36 RShift ✓).  `scripts/rig/drives/ep1_roomclear2.txt` sends only `poke 0x6336c 3` + `key enter`
pairs to its last event at t=215350 ms ✗ Enter is `0x63ea4` — it drives the menu's purchase rows, it cannot exit
the screen ✓ **The port's wait is correct behaviour; the drive could not leave the screen** ✓

## 3. With Shift presses the whole tail runs (`/tmp/rig/s2d`, new drive `scripts/rig/drives/ep1_roomclear3.txt`)

```
tail: stage=2
tail: levelup call done           (fn_038f3d(1) returned ✓)
tail: stage reset done            (fn_0374ad() returned ✓ -- its 0x37615 full reload included)
roomstart: stage=2 vesa=1 slides=2 roomrec=0x1027422ac     <-- room_start RE-ENTERED with stage 2 ✓✓
slide: card 1/2 wait              (g_tab_52f2c[2] = 2 ✓ -- the stage-2 start slides ✓)
memstat stage=2 room=1 frames=15951..15992     rc=0, 0 ASan / 0 UBSan / 0 SEGV ✓
resource lookups: stage2.wal ✓ stage2.stg ✓ night2.chr ✓ 2s1.pcx ✓ (plus stage1.* from stage 1 ✓)
```

So the next-stage loader chain is intact in the port: `fn_0374ad` → `room_start` → `fn_03f15f() == 1` ✓ →
`g_tab_52f2c[2] = 2` slides → `stage2.pcx`/`stage2.wal`/`stage2.stg`/`night2.chr` requested ✓✓

## 4. Self-corrections this forces

* **docs/285 §1's framing is superseded.** "On HEAD the natural room clear reaches stage 2 but the port never asks
  for a stage-2 file" is *true of that run* but its cause was §2's key-gate, not a port defect ✓ Item 5's "no
  stage-2 art" symptom does **not** reproduce on a drive that can leave the level-up screen ✓
* **docs/269's "both-sides stage-2 basis" is withdrawn as a gameplay basis.** Shipped `rm1`: `stage {1:159, 2:779}`,
  stage-and-`DAT_63354 != 0` `{1:156, 2:779}`; port `rm1`: `stage {1:1003, 2:1176}`, in-room `{1:373, 2:1176}` ✓ —
  but `g_dat_63370` is incremented **before** `fn_038f3d` runs (`0x3b520` precedes `0x3b54c` ✓), so every
  "stage 2" sample on *both* sides can be, and on that drive is, a sample taken **inside the level-up screen** ✓
  `DAT_00063354` staying non-zero is not evidence of a live room: it is a stale pointer from the previous room ✓
  Rule recorded: **`DAT_00063354 != 0` is not a "live room" predicate; a stage-2 gameplay basis needs a run whose key
  stream can leave `fn_038f3d`** ✓
* Cross-side pointer archaeology on the same run was also uninformative: shipped `DAT_53c7c`/`DAT_53c84` = `0x5756c8`
  before *and* after the increment, `DAT_53c70` `{0,0x382fb8,0x383104,0x5756c8}` → `{0x383104}` ✓, and the port's
  equivalent cells hold host pointers ✓ — no reload signature to read there, because no reload had happened ✓

## 5. What item 5 still needs

The playtest report describes a human pressing keys ✓ — so the human's path *does* leave `fn_038f3d` ✓, and the
report's two observations (crash at stage 2, floor black before it) must be tested on the **`ep1_roomclear3` basis**:

1. both sides on `scripts/rig/drives/ep1_roomclear3.txt` (`route.sh`, ≥ 300 s) → stage-2 room samples that are real,
   then the screen oracle's floor-region census on the shipped's stage-2 room vs the port's;
2. the same drive under the ASan build for the fault half (the fault half was already green on warp + natural
   routes, docs/223, docs/268 — it must be re-confirmed on a drive that actually composites stage-2 rooms);
3. item 6's left-edge limb drive (open, unchanged).
