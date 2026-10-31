# 275 — Item 2b: posts attributed at the call site — 79 + 45 = 124, self-consistent, and it *narrows* the defect away from posting volume

## 1. Corrected instrument (posts counted at each call site, per docs/274 §2)

Canonical drive `scripts/rig/drives/ep1_kill3.txt` (150 s ✓, room confirmed entered: the per-room wipe witness fired
exactly once ✓), `NIGHT_ACTTRACE=1`:

```
fn_0347e3 entries = 3,076,377          armed_ticks = 1,691,327        case_hits (0x1d/0x2a/0x2c/0x36/0x38) = 316
posts from fn_034735 (f.c:8857) = 79   posts from fn_034635 (f.c:9759/9760/9762) = 45
fn_034353 total entries = 124          <-- 79 + 45 = 124, arithmetically closed against the call graph
```

The counters now agree with the call graph ✓, so docs/274 §2's inconsistency is resolved ✓ and the numbers are quotable ✓

## 2. What that changes: the port's *posting rate* is not badly off — the ring's persistence is

Convert to per-second on the same 150 s window (docs/272 §2's dump-side census, `/tmp/rig/rb8` ✓ same drive ✓):

| | shipped | port |
|---|---|---|
| ring cursor advances | 162 / 150 s = **1.08 /s** | 101 / 150 s = **0.67 /s** (posts counted live: 124 / 150 s = **0.83 /s** ✓ consistent with 0.67 + the `fn_0348e2` advances ✓) |
| ring slots holding a non-zero code | **86 %** | **33 %** |
| consumed-flag sum over 10 slots | 6-7 | 10 (all consumed) in 60 % of samples |
| ≥2 adjacent `0x1d` codes in the window | **73 %** | 33 % |

Posting volume is within a factor of ~1.3 ✓ — so **posting starvation is not the primary defect** ✓ The gap is what the
ring *looks like afterwards*: the shipped keeps 86 % of its slots carrying codes and leaves 3-4 slots unconsumed, while
the port's ring is two-thirds empty and fully consumed ✓ Slots are zeroed by `fn_000348e2` (`0x34901 mov byte
[eax+0x13f14], 0` ✓ "push a null entry and advance" ✓, 39 shipped call sites vs 31 port sites ✓ — and docs/271 §5 showed
a count difference can be a factoring artifact ✓, so the 31-vs-39 count is not yet a verdict ✓) and consumed by
`fn_0003272f`'s 8 `mov byte [eax+0x13f1e], 1` stores ✓

## 3. Next measurement (bounded)

1. Count `fn_0348e2` and `fn_03442e` entries in the port under the same witness (two more call-site counters ✓) and
   compare their ratio to posts — if `fn_0348e2` entries vastly exceed posts ✓, the port is overwriting ring slots with
   code 0 faster than it fills them, which is exactly the 33 %-fill / all-consumed signature ✓
2. Re-derive the shipped side of the same ratio offline: its cursor advances (1.08 /s ✓) split into
   `fn_034353`-type advances (level written from `DAT_00063f08` ✓, consumed flag cleared ✓ — visible as slots with
   `DAT_00063f1e == 0` ✓, 3-4 per sample ✓) versus `fn_0348e2`-type advances (code 0 ✓ — visible as slots with
   `DAT_00063f14 == 0` ✓, 601 of 4280 slots ✓ i.e. 14 % ✓ vs the port's 10,100 of 15,000 = 67 % ✗✓✓) — the shipped's
   own dump already says its null-entry pushes are ~5× rarer than the port's ✓
3. Only then decide the fix, and gate it on: ring fill rising toward 86 % ✓, consumed sum leaving the all-10 mode ✓,
   ≥2 adjacent `0x1d` windows rising toward 73 % ✓, `DAT_00062d58` producing `0x34/0x35/0x36` ✓, then the item-2b green
   gate (`kind==1` settle ✓, `+0x32 = 10` ✓, `player[+0x52]` rising ✓) ✓
