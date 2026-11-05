# 358 — packing is dead (the OS writes the whole word) and a settings hunt grabbed two counters

Core 0.1.47 tested docs/357's idea.  Both halves of the answer are in the card log, and the second half explains the
symptom the player sees: *"screen fit works but sfx, music act weird — muted whatever value I set."*

## 1. A mask protects the core's bits; it does not make the firmware compose values

Three entries were mounted in disjoint fields of `0xF700000C` (music bits 0–3, sfx bits 4–7, fit bits 8–9).  The
mirrored word `0x4000006c` decoded over the run:

| t | word | music | sfx | fit | what happened |
|---|---|---|---|---|---|
| 9.6 s | `00000300` | 0 | 0 | 3 | picked Screen Fit = scaler |
| 29.8 s | `00000001` | 1 | 0 | 0 | picked Music = Off — **fit's field erased** |
| 38.4 s | `00000008` | 8 | 0 | 0 | picked Music = Full |
| 54.2 s | `00000200` | 0 | 0 | 2 | picked Screen Fit = stretch — **music erased** |
| 81.3 s | `00000070` | 0 | 7 | 0 | picked SFX = 7/8 — **music erased again** |

Every write lands as the option value *alone*: the other fields go to 0.  So the OS does not read-modify-write an
interact value, and a mask is a statement about **the core's own registers** (which bits the core may change), not a
statement about how the firmware stores a menu choice.  `writeonly` — the flag that makes an undecoded address usable
by an app at all — is exactly what forbids the firmware from reading the word it is about to write.

**The scarce unit is the address, exactly as docs/356 said.** `0xF700000C → 0x4000006c` and `0xF7000010 → 0x40000068`,
one entry each.

## 2. The mute was a wrong register, not a wrong translation

The value translation is correct, and spelling it out was worth it:

```
Music 0x41 0x42 0x43 0x44 0x45 0x46 0x47 0x48  ->  vol 0 36 72 109 145 182 218 255
SFX   0x61 0x62 0x63 0x64 0x65 0x66 0x67 0x68  ->  vol 0 36 72 109 145 182 218 255
Fit   0x51 0x52 0x53                            ->  letterbox / stretch / scaler
```

What was wrong is **where the port read from**.  Only Screen Fit was claimed on evidence
(`@0x4000006c … the word hardware carried it in on`); Music and SFX were claimed by the fallback hunt:

```
menu: Music Volume slot CLAIMED t=15.8s @0x400000ec found by field cleanliness (word 00000007, field 7 = 7/8)
menu: SFX Volume   slot CLAIMED t=15.8s @0x400000dc found by field cleanliness (word 00000242, field 4 = 1/2)
menu: Music Volume = 7/8 (field 7 of word 00000007) applied -> music_vol=218
menu: Music Volume field in slot @0x400000ec reads 0 again -- the value from night.ini stands
menu: Music Volume = Off (field 1 of word 00000001) applied -> music_vol=0        <- the mute
```

Those two words are peripheral state, not mirrors: `0x400000dc` walks `0x20d → 0x242 → 0x247 → 0x2cb` (a counter), and
`0x400000ec` flips between 0, 1 and 7.  With a 4-bit field, a small wandering value *looks* like a menu: it is 0 at
boot (so it passes "was it ever silent"), its field stays inside 1..8 for many samples (so it passes "held an option
value ≥5 times"), and nothing else is visible in those bits (so it passes cleanliness).  The census agreed that the
hunt was unreliable — 9 candidates out of 96 registers.  The claim happened at t=15.8 s, before the player touched
Music Volume at all, and once an entry has a slot the hunt stops — so the wrong register stuck for the whole run.

Meanwhile the correct word sat there answering only the entry that had been claimed correctly:

```
periph t=29.8s 0060: 00000000 00000000 00000001 00000000 ...   ^ 0x6c carries Music Volume; 0x68 silent
```

## 3. The rule 0.1.48 ships: a known mirror address is the only place to read

```c
if (!m->slot && m->known) continue;   /* an entry whose mirror address is known may ONLY be read there */
```

The hunt survives only for entries whose address is unknown — which is what the hunt is for, and it is now clearly
labelled as a guess.  A known word that never answers is **reported** (`has no peripheral slot after 60 s`) rather than
replaced by whatever register happens to look menu-shaped.  This is the same lesson docs/354 wrote in a weaker form: an
inference about *where* hardware carries a value may only be used where the measurement was made.

Layout restored to docs/356's, which is the layout with two proven claims behind it:

| entry | bridge address | mirror word |
|---|---|---|
| Music Volume | `0xF700000C` | `0x4000006c` |
| SFX Volume | `0xF7000010` | `0x40000068` |
| Screen Fit | — | L1+B during play, `night.ini FIT` |

The field machinery stays (shift/mask per entry, every test run on the field) because it costs nothing and makes a claim
falsifiable — with masks back at `0xFFFFFF00` a field simply *is* the whole word.

## 4. What a third menu setting would take

Nothing about masks, addresses beyond these two, or the interact API.  The remaining lever is finding a **third
mirrored address**, and the only way to look is to mount something on one and read the page afterwards: `0xF7000014` is
measured dead twice; `0xF7000018` and `0xF700001C` have never been tried.  That is one card cycle and it is the only
open question on the settings surface; until it is answered the honest shipping configuration is two menu settings plus
the L1+B fit combo, and 0.1.48 ships that.

## 5. Simulated in Python before spending another card cycle

`scripts/pocket/sim_menu.py` models the three layers separately, because 0.1.44 and 0.1.47 each confused a different
pair of them.  Run it with `python3 scripts/pocket/sim_menu.py scratch/pocket/log_0147.sav`:

1. **mask / value layer** — derives each element's field from its mask (`writable = ~mask`, one contiguous run) and
   checks every option value fits that field, is non-zero, and indexes the way the host indexes it.
2. **firmware layer** — the measured map plus the measured *whole-word* write: `word[mirror] = value`, not
   `value | (prev & ~field)`.
3. **port layer** — `main_of.c`'s logic transcribed: known-slot assert, optional hunt (clean + ever silent +
   `seen >= 5`), 0 = "not spoken", value → index → volume.

**Packing, simulated** (three entries, one word):

```
pick Music Volume=0x8 -> word 4000006c: 00000001 -> 00000008   [music=8 sfx=0 fit=0]
    Music Volume = Full -> music_vol=255
pick Screen Fit=0x200 -> word 4000006c: 00000008 -> 00000200   [music=0 sfx=0 fit=2]
    Music Volume field reads 0 -- night.ini stands
    Screen Fit = stretch -> fit=1
```

**Shipped layout, simulated** (one entry per word) keeps both settings alive at once:

```
pick Music=0x48 -> 0x4000006c: 00000043 -> 00000048   [music@4000006c=48 sfx@40000068=64]
    Music Volume = Full -> music_vol=255
pick SFX=0x61   -> 0x40000068: 00000064 -> 00000061   [music@4000006c=48 sfx@40000068=61]
    SFX Volume = Off -> sfx_vol=0
```

**Replay of the real 0.1.47 log**, folded into 129 sample instants with the core's 2 Hz sweep interpolated:

| rule | claims | consequence |
|---|---|---|
| hunt allowed (0.1.47) | `Screen Fit @0x4000006c` ✓, then `Music Volume @0x40000000` (word `00000003` = "1/4"), `SFX Volume @0x4000001c` (word `00000040` = "1/2") | two settings sit on peripheral state that has nothing to do with the menu |
| known mirror only (the rule 0.1.48 ships, applied here to the same packed declaration) | `Screen Fit`, `Music Volume` and `SFX Volume` all at `0x4000006c`, and nothing else | `music_vol=255 / 0 / 255`, `sfx_vol` follows the picks -- but each pick erases the others, which is §1 all over again |

The card's own claims were `fit @0x4000006c t=12.9s`, `music @0x400000ec t=15.8s`, `sfx @0x400000dc t=15.8s` — the
model picks different garbage registers (its sample history is thinner: the log records 129 instants, the core actually
swept at 2 Hz) but the *class* of failure reproduces exactly: with the hunt enabled, a word whose field only ever shows
in-range values and which was 0 once at boot is indistinguishable from a mirror, and it gets claimed.

### The simulator's own mask/value bug, worth recording

The first version of the replay claimed nothing but the correct words, which was wrong in the *other* direction.  The
sweep was written

```python
for i, v in enumerate(words):        # words is a dict {index: value}
```

and `enumerate()` over a dict yields `(position, index)` — so every cleanliness/seen table was being fed register
**indices** as if they were register **values**: word 15 looked like the value `0x0f`, which sits inside a 1..8 option
band.  `words.items()` is the fix.  A mask-vs-value, index-vs-value confusion was indeed at the bottom of this thread —
twice, once in the C (`docs/357`'s inverted masks) and once in the Python that was meant to check it.  Which is the
argument for writing the model down rather than reasoning about the claim rules in prose: the second bug was invisible
in the analysis and obvious in the run.
