# 113: no punch/hit sound ever played -- DS 0x53d08 is the voice pool, one dword low

## The shipped path (guest 0x0003ab31-0x0003ab60, inside `fn_03a829`)

```
CMP  dword [0x63364], 0            ; the element-event latch (set in fn_01441e from byte[record+slot+0x63])
JZ   skip
CMP  dword [0x63368], 0            ; repeat/hold counter
JNZ  decrement
MOV  EAX, [0x63364]
SHL  EAX, 0x2
MOV  EAX, dword ptr [EAX + 0x53d08]
CALL 0x4185e                       ; play the sound object
MOV  dword [0x63364], 0
```

So the event index is **1-based into a dword table whose base is `DAT_00053d08`**. The master loader
`fn_03690b` writes that table at base `0x53d0c` (guest `0x00036ef9` `MOV [EDX + 0x53d0c], EAX`) for the 100
voice slots `"1.voc".."100.voc"` -- the same region, 4 bytes higher. Hence `entry(ev) == pool[ev-1]`, and the
measured shipped event ids (`DAT_00063364` fires with **1** and **19**) land on `pool[0]` and `pool[18]`,
exactly the slot numbering the loader uses.

## What the port did

The port modelled `0x53d08` as its own object, `uint32_t g_tab_53d08[101]`, which **nothing ever wrote** (one
declaration, one read). The call site therefore always passed null and no hit, punch or death sound could
ever play -- the playtest item "missing punch SFX". The static audit had already filed the modelling error
independently: `c2_aliasing` reports `port-span-overlap g_tab_53d08 [0x53d08,0x53e9c]` overlapping
`g_dat_53d0c` (100 entries), `g_dat_53e24` (10) and five `g_dat_53dd*` globals, and `c7_ds_table_bases`
shows base `0x53d08` has exactly **one** guest access site while `0x53d0c` has six across five functions.
This is the same one-address-two-arrays defect as docs/112 (`g_pool_53d0c`) and docs/109 (`g_dat_53ea8`).

## Fix

`g_tab_53d08` is deleted (with `g_pool_53d0c` already gone), and the event play now reads the real pool with
the shipped 1-based mapping, counting anything outside the 100 entries instead of reading past it:

```c
uint32_t ev = g_dat_63364;
if (ev >= 1u && ev <= 100u && g_dat_53d0c[ev - 1u]) { fn_04185e(g_dat_53d0c[ev - 1u]); g_sfx_plays++; }
else g_sfx_oob++;
g_dat_63364 = 0;
```

## Red before / green after (drive `ep1_kill3`, 14000 frames, `NIGHT_ACTTRACE=1`)

```
before: the play argument was an unfilled table -> always null; 0 sounds reachable
after:  element-event sounds: played=74  outside-pool=0
shipped reference (sampled at 6.5 Hz, so a floor): DAT_00063364 fires 4 times, ids {1, 19} -> pool[0], pool[18]
```

`make -C port test` ALL PASS, `make selftest` ALL PASS.

## Still to verify for this item

The rig has not yet A/B'd the *audio* of a combat window (container mixer capture vs the port's PCM sink, per
docs/103) -- that is the remaining presence proof for "missing punch SFX"; the state-side evidence (a sound
object actually reaches `fn_04185e` 74 times) is in place.

## Port-side audio output before/after (the rig's own PCM byte counter, same drive and seed)

The rig dump records the PCM sink's cumulative byte count per sample, so the SFX fix is visible without any
human listening -- compare two runs of `ep1_kill3` at 6 Hz (`/tmp/rig/rx1` is the pre-fix build,
`/tmp/rig/rx2` the fixed one):

```
rx1 (before): 477,970 audio bytes,  54/1308 samples emitting audio   per-60s: {0: 460804, 180: 17166}
rx2 (after): 1,135,130 audio bytes, 137/1302 samples emitting audio  per-60s: {0: 460804, 60: 36070, 120: 21642, 180: 319972, 240: 296642}
```

2.4x more audio in total, and -- the part that matters -- the extra output lands in the 120-300 s stretch,
which is exactly where this drive fights (the kill counter moves at 231 s, `NIGHT_HITTRACE` hits cluster
there). Before the fix the combat stretch was silent apart from the music track. The shipped-side mixer A/B
over the same window (`--audio-capture` + `diff_audio.py`) is the remaining presence proof for this item.
