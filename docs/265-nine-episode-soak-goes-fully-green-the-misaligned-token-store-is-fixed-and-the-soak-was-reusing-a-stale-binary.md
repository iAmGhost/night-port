# 265 — The nine-episode soak goes fully green for the first time: the standing misaligned pointer store is fixed, and the soak was reusing a stale binary

## 1. The standing UBSan report, fixed

Every stage of every soak since docs/224 §6 carried one report:

```
src/f/f.c:7374:9: runtime error: store to misaligned address 0x000100e730f9 for type 'char *',
                  which requires 8 byte alignment
```

`fn_04589e` (the tokenized command-line builder ✓ `f.c:7391+` ✓) lays out, in one heap block: the copied command
string, a dword (`DAT_0005338c`), then the token-pointer array, then a terminating entry ✓ The shipped's array
entries are 4 bytes ✓ the port's are host `char *` = 8 bytes ✓ (a widening the port documents deliberately ✓) — but
the widened array started at `tail + 4` ✓ which is only 4-aligned ✓, so every token write was an 8-byte store through
a 4-aligned `char **` ✓ (the misaligned address ending in `9` ✓ = 4 mod 8 ✓) ✓

Fix: start the widened array at the first pointer-aligned address at or after `tail + 4` and enlarge the allocation by
the worst-case 7 bytes of padding ✓

```c
uint32_t buf_off = heap_malloc(arr + 4u + nbytes + 7u);
char **toks = (char **)(((uintptr_t)(tail + 4) + 7u) & ~(uintptr_t)7u);
```

The unit test that asserted `tok = tail + 4` exactly (`port/test/test_f1.c:4223` ✓ "out[count] terminator written" ✓)
now derives the same aligned start ✓ — the fixture encoded the port's own layout, not the shipped's ✓, and the other
assertions (in-place tokenization ✓ the appended dword ✓ the two token pointers ✓ the DAT_64090/94 mirrors ✓) are
unchanged ✓ `make -C port test`: **ALL PASS** ✓

## 2. Rig defect: the soak did not build the binary it soaked

`scripts/rig/asan_soak.sh` used `BIN=${BIN:-./port/test/night-asan}` with no build step ✓, and after today's edits the
reported line `f.c:7374` **was a blank line** in the current source ✓ (`awk 'NR==7374'` ✓) — proof the ASan binary was
stale ✓ So docs/262's "9/9 rc=0 with 1 UBSan report per episode" measured an *older build* ✓ superseded ✓ The script
now runs `make -C port asanapp` before soaking (disable with `SOAK_BUILD=0` ✓) and fails loudly on a build error ✓ —
the same class of defect as docs/181 §5 ("the `test` target must rebuild the app binary") and docs/181 §5's witness-string
rule ✓

## 3. Result after both fixes

```
OUT=/tmp/rig/soak266 JOBS=4 CAP=300 FRAMES=4000 scripts/rig/asan_soak.sh   (binary built by the script)
stage 1..9: rc=0  ubsan=0  asan=0  segv=0  frames presented: 4000
```

**The goal's soak condition (9/9 rc=0 with 0 UBSan/ASan/SEGV reports) is met for the first time in this project** ✓
(`make -C port test` ALL PASS in the same working tree ✓)

## 4. Caveat to apply to docs/264 §5-§7 numbers

The `NIGHT_ACTTRACE` censuses quoted there (targets offered/matched ✓ "level too high" ✓ "len != k" ✓ the
`DAT_63f28`/ring/cursor probes added in `f.c` + `main.c` ✓) came from **ad-hoc `./port/test/night` runs without
`--rig-dump`** ✓ Run-to-run variation on such runs is already established (docs/191 §1, docs/239 §4 ✓): the ring's
consumed flags read `0,1,1,1,0,1,0,1,1,1` in one such run and `0,0,…,0` in the next ✓, rejection counts moved
(552 → 394 → 386 "level too high" ✓, 4132 → 3244 → 3166 "len != k" ✓) ✓ Those numbers are directional evidence for
*where to look* ✓, not quotable before/after evidence ✓ Quotable before/after for any fn_03272f change must come from
`scripts/rig/route.sh` with `--rig-dump`, repeated at least twice per side ✓, and the cursor comparison in docs/264 §7
is additionally **basis-mismatched** (the port's histogram counts `fn_03272f` entries ✓ 4857 of them ✓, the shipped's
counts DS samples ✓ 244 of them ✓) → the two are not comparable as stated ✓ and must be re-measured on one basis ✓

Item 2b remains open on its green gate (a settled `kind==1` enemy ✓ `+0x32 = 10` ✓ `player[+0x52]` rising ✓); two
fixes are landed with rig evidence (docs/254 clamp guard ✓, docs/263 slot selection ✓) and the probe set
(`act_level_*`, `act_ring_*`, `act_cursor_hist` ✓) is in place for the next measurement ✓
