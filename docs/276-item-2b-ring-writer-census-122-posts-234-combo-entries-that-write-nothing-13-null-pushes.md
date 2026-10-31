# 276 — Item 2b: the ring-writer census — 122 posts, 234 combination-helper entries that write nothing, 13 null pushes; docs/275 §2's hypothesis is wrong

## 1. Measured (canonical `ep1_kill3` 150 s, `NIGHT_ACTTRACE=1`, counters at function entry)

```
fn_0347e3 entries        = 3,132,379        armed_ticks = 200,130 (the arming window is short ✓)
posts  fn_034353         = 122   (78 via f.c:8857 from fn_034735, 44 via f.c:9759/9760/9762 from fn_034635)
fn_03442e entries        = 234
fn_0348e2 entries        = 13        <-- "push a null entry and advance" is RARE in the port
```

`122 + 13 = 135` cursor-advancing writes ✓ against the dump-side count of 101 observed cursor `+1` transitions in
`/tmp/rig/rb8` (sampling at 10 Hz misses sub-sample pairs ✓ consistent ✓) — the ring bookkeeping closes arithmetically ✓

## 2. Retracted: "the port overwrites ring slots with code 0 five times more often than the shipped"

docs/275 §2 proposed that ✓ — **it is wrong** ✗: the port calls `fn_0348e2` (the null-entry push, shipped
`0x34901 mov byte [eax+0x13f14], 0` ✓) only **13 times** in 150 s ✓ The 67 % null-code slots in the port's ring are not
null *pushes*; they are the ring's initial zeroed state plus posts that never happened ✓ — do not read a state census as
an event census ✓ (docs/264 §7's rule again ✓)

## 3. What the 234 `fn_03442e` entries do (and do not) do

`fn_03442e` is the combination poster: it writes `0x67/0x68/0x69` into `DAT_00063f14[cursor]` **only when a branch
fires** (`if (z != 1) return;` ✓), and only then clears the consumed flag, records the level, advances the cursor and
calls `fn_0343d1` (which is the slot-clearing recycler ✓). Neither side's ring contains `0x67/0x68/0x69`
(shipped histogram `{0x1d:2401, 0x48:618, 0:601, 0x66:513, 0x64:89, 0x50:58}` ✓, port `{0:10100, 0x1d:3041, 0x48:860,
0x66:425, 0x64:296, 0x50:278}` ✓) ⇒ **the combination branches never satisfy on either side** ✓ — so 234 entries are
mostly early returns ✓ and `fn_0343d1`'s slot-clearing is not what erases the armed chain keys either ✓

## 4. The remaining divergence, and the measurement that can settle it (event-based, not state-based)

Everything both sides put into the ring came from `fn_034353` posts ✓. The shipped's ring is 86 % filled with those
codes and holds 3-4 unconsumed slots in most samples ✓, the port's is 33 % filled and fully consumed ✓ — so the shipped
*posted materially more often than the port's 122*, but a state census cannot give its post **count** ✓

Admissible both-sides measurement, already available offline in `rb8`: **count transitions of
`DAT_00063f14[slot]` from 0 → non-zero between consecutive shipped samples** ✓ (that is a post *event* in the shipped's
own dump ✓, sampled at its dump cadence ✓) and compare it against the port's 122 live counter plus the same transition
count in the port's dump ✓ If the shipped's transition count is several times the port's ✓, posting volume is the defect
after all and the fix target is the arming path (`keymap.c:63`'s `held == 0` gate ✓ vs the shipped's INT 09
`0x3fe12/0x3fe26/0x3fe33` make stream ✓); if they are comparable ✓, the defect is on the consumption side
(`fn_03272f`'s eight `mov byte [eax+0x13f1e], 1` stores ✓ consuming faster than the shipped does) ✓
