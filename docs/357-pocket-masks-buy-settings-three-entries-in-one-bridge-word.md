# 357 — masks buy settings: three menu entries mounted in one bridge word

docs/356 concluded "two mirrored addresses means two menu settings".  That was wrong in a way worth writing down,
because the fix is sitting in a field the port had already read the spec of (docs/345) and never used for this purpose:

> A mask bit of **1** is left untouched; a bit of **0** may be modified by that UI element.

If the OS writes only the bits an element is allowed to touch, then **the unit that is scarce is the bit, not the
address** — so several entries may share one address as long as their writable bits are disjoint.  Core 0.1.47 mounts
three entries on `0xF700000C`, the address docs/352/354/355/356 all saw mirrored into the readable word `0x4000006c`:

| field | entry | option values | mask |
|---|---|---|---|
| bits 0–3 | Music Volume | `0x01 … 0x08` | `0xFFFFFFF0` |
| bits 4–7 | SFX Volume | `0x10 … 0x80` | `0xFFFFFF0F` |
| bits 8–9 | Screen Fit | `0x100 … 0x300` | `0xFFFFFCFF` |

The host reads **one** peripheral word and extracts three fields, so the whole settings surface is one MMIO read per
100 ms once claimed.  Ten bits of the word are used; twenty-two are free, and `0xF7000010 → 0x40000068` is now unused
entirely — a fourth group (CDDA on/off, `FB_MODE`) has a place to live.

## 1. The mask arithmetic the checker caught

My first masks were `0xFFFF00FF` for bits 4–7 and `0xFFFF3FFF` for bits 8–9 — written as if a mask listed the writable
bits instead of the protected ones, which is the exact inversion docs/345 exists to prevent.  The checker rejected it
four ways at once:

```
[FAIL] 0xF700000C: bits 8 written by both 'SFX Volume' and 'Screen Fit'
[FAIL] SFX Volume offers [16, 32, ... 128] outside its own field (mask 0xFFFF00FF writes only [8..15])
       -- 0.1.27's shape, and it bricked a boot
[FAIL] SFX Volume: host SFX_SHIFT=4, interact.json's field says 8
```

The rule `writable = ~mask`, `mask = 0xFFFFFFFF ^ ((1 << width) - 1) << low` is now stated in `mk_dist.py`'s table
comment.  Correct masks: `0xFFFFFFF0`, `0xFFFFFF0F`, `0xFFFFFCFF`.  `check_interact.py` re-derives each field from the
JSON and cross-checks it against the host's `*_SHIFT` / `*_MASK` constants in both directions, which is what caught it
before a card was involved — the same net that caught the all-ones masks (0.1.32–35) and the `0xF7000000` mount (0.1.27,
0.1.34).

## 2. What the field rule changes in the host

Docs/354's two hard-won rules survive, but they become **per field** instead of per word:

* **Silence means "not spoken yet".** A field of 0 is the "menu has not spoken for this setting" state, exactly as a
  whole word used to be — which is why no option value is 0 and why `MUSIC_VOL=128` from `night.ini` stands while a
  field reads 0.  Reading one word for three entries means Music Volume's silence says nothing about SFX Volume's.
* **A candidate must have been seen silent before it can be claimed.** Re-checked per field, so the constant that fooled
  0.1.44 (`0x4000003c` sitting at `0x63` from boot) is still rejected: its music field is 3 and its SFX field is 6 from
  the first sample, and neither was ever 0.
* **Cleanliness is a field test.** A word is clean for an entry only if that entry's field only ever showed 0 or one of
  that entry's option values.  Two entries may now claim the *same* word — the old "somebody else claimed it" veto is
  gone, since disjoint fields make sharing legitimate rather than a conflict.

Logging changed to match: a claim now prints the word and the field it was read out of
(`word 00000143, field 3 = 1/4 at bits 0-3`), and a lost slot says which field misbehaved.

## 3. What is hypothesis and what is measurement

* Measured: the two-word mirror map (`0xF700000C → 0x4000006c`, `0xF7000010 → 0x40000068`, nothing else), four runs.
* Measured: `writeonly` entries at an undecoded bridge address do reach the mirror (docs/352), and the Pocket UI accepts
  them.
* **Not yet measured: whether the OS honours a per-element field on a shared address.** `writeonly` is documented to
  skip the read-back — which is also what makes a read-modify-write impossible for the firmware if it does not keep the
  rest of the word.  So 0.1.47 is an experiment, and it reports its own result:
  * all three settings react → the word is combined per field, the menu holds three settings in one address, and the
    remaining word is available for a fourth group;
  * one setting reacts while the other two fields stay 0 → the OS writes the whole word, packing is dead, and the port
    reverts to docs/356's layout (one entry per wired address, two settings) by deleting two table rows in `mk_dist.py`;
  * the log says which it is, because `menu: … CLAIMED` / `… field … reads 0 again` lines name the word and the field.

Build clean, `check_interact.py` ALL PASS, fixture suite ALL PASS (`src/f` untouched), card holds 0.1.47 with `DIAG=11`
and 11 interact entries (8 shipped + 3 packed).
