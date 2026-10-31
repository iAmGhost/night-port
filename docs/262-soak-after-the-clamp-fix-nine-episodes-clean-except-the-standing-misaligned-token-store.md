# 262 — Nine-episode ASan/UBSan soak after the clamp fix: 9/9 rc=0, 0 ASan, 0 SEGV, 1 standing UBSan report per episode

`OUT=/tmp/rig/soak248 JOBS=4 CAP=300 FRAMES=4000 scripts/rig/asan_soak.sh` on HEAD `2617af1` (i.e. with the
`fn_01586b` clamp-guard fix in place ✓ docs/254 ✓):

```
stage 1 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 2 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 3 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 4 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 5 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 6 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 7 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 8 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
stage 9 rc=0 ubsan=1 asan=0 segv=0 frames presented: 4000
```

* **No new fault from the clamp fix** ✓ (the fix touched `fn_01586b`'s Y store ✓); 9/9 `rc=0` ✓ 0 ASan reports ✓
  0 SEGV ✓ across all nine episodes ✓
* **Not contract-green yet:** the goal's `gate3-regression` contract asks for *0* UBSan reports ✓ and every episode
  still emits exactly one ✓ the standing report docs/224 §6 / docs/227 §6 recorded as appearing on every route
  including ones that never reach a kill popup ✓ now reported at **`src/f/f.c:7374:9: runtime error: store to
  misaligned address 0x0001023a70f9 for type 'char *', which requires 8 byte alignment`** ✓ -- the line moved from
  7360 to 7374 purely because the `NIGHT_RECBUILD` witness added ~14 lines above it ✓ (`docs/251` ✓) -- so it is the
  same defect ✓ not a new one ✓
* Site: `fn_04589e` (the tokenized command-line builder ✓ `f.c:7375+` ✓ its model writes 8-byte host `char *`
  entries into an array the shipped lays out with **4-byte** entries ✓ -- the same 32->64-bit adaptation problem the
  port solved elsewhere by storing *heap offsets* or by `memcpy`-based accessors (`rec_ptr_get`/`rec_ptr_set` ✓
  `f.c:169-172` ✓) ✓ the port's own comment at `f.c:7352-7356` already states that rule for the record pointer
  fields ✓ it was never applied to this array ✓

Fix candidate for the soak's last outstanding report, stated without implementing it here: model the token array as
`uint32_t` heap offsets (the shipped's own entry width ✓) rather than host `char *` ✓ so the store is 4-byte like the
shipped's ✓ and the gate is the soak's `ubsan=0` line for all nine episodes ✓ plus the suite staying ALL PASS ✓
