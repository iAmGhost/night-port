# 360 — packing is the documented shape after all; the missing half is the read

The user's point is correct and it reopens docs/358's conclusion in a useful direction.  openfpgaOS's own
`interact.json` — the file this port copies verbatim for entries 0–7 — packs **six** elements into one bridge word with
disjoint masks:

| element | field of `0xF7000000` | mask | option values |
|---|---|---|---|
| Enable Analogizer | bit 15 | `0xFFFF7FFF` | `0x00000000`, `0x00008000` |
| SNAC Adapter | bits 0–4 | `0xFFFFFFE0` | `0x00`, `0x10`, `0x11` |
| SNAC Controller Assignment | bits 6–9 | `0xFFFFFC3F` | `0x0`, `0x40`, `0x80` |
| Analogizer Video Out | bits 10–13 | `0xFFFFC3FF` | `0x0 … 0x3C00` |
| Analogizer 15kHz Timing | bits 18–19 | `0xFFF3FFFF` | `0x0`, `0x00040000`, `0x00080000` |
| Pocket LCD | bits 16–17 | `0xFFFCFFFF` | `0x0`, `0x00010000`, `0x00020000` |

Six settings, one address, no value outside its own field.  So packing is not something I invented and it is not
forbidden by the format — docs/358's "the scarce unit is the address" was too strong, and 0.1.47's whole-word result
needs a reason.  Analogue's own wording has it (docs/345):

> Each frame, all of the UI elements are **read back from the core** at a specified 32-bit bridge address, updated with
> new user-adjusted values, and finally written back.  This read-modify-write cycle **requires the core to have both
> read and write support at each desired address, unless the element is flagged as `writeonly`**.

## 1. Why the shipped entries can share a word and ours could not

Those six fields belong to openfpgaOS itself: the bitstream's `core_top` implements that register, so the read half of
the cycle has something to read and the merge is possible.  An app element is a different story:

| version | what was declared | read half | outcome |
|---|---|---|---|
| 0.1.25/26 | knobs at `0xF700000C` / `0xF7000010`, **not** writeonly | nothing answers an undecoded address | core booted and ran for hours; the menu could neither show nor move the entries |
| 0.1.27 | Music Volume in bits 20–23 of **`0xF7000000`**, not writeonly | the control word, which an app must not touch | core never booted |
| 0.1.32 | knobs at `0x03FE0000/08` with mask `0xFFFFFFFF` | mask 1 = untouched → the element may write nothing | visible in the menu, unselectable |
| 0.1.34 | Music Volume bits 24–26 and Screen Fit bits 27–28 of **`0xF7000000`**, not writeonly | same as 0.1.27 | core never booted |
| 0.1.42 → 0.1.49 | `writeonly` entries at `0xF700000C` / `0xF7000010` | skipped, by design | menu movable, value mirrored into `0x4000006c` / `0x40000068` |

The distinction 0.1.47 never tested is the one the spec draws: **`writeonly` is what removes the read**, and without a
read there is nothing to merge a field *into*.  0.1.47 packed three `writeonly` entries and observed the whole word being
replaced — which is precisely what a write-only write looks like.  That is consistent with packing being fine and with
`writeonly` being the reason it did not compose.

## 2. Core 0.1.50: packed, and no `writeonly`

Both changes at once, because they are the same question.  Three entries on `0xF700000C`:

| entry | field | mask | option values |
|---|---|---|---|
| Music Volume | bits 0–3 | `0xFFFFFFF0` | `0x01 … 0x08` |
| SFX Volume | bits 4–7 | `0xFFFFFF0F` | `0x10 … 0x80` |
| Screen Fit | bits 8–9 | `0xFFFFFCFF` | `0x100 … 0x300` |

`writeonly` is gone (the user asked for it, and it is the flag that decides whether a read happens).  `persist: true`
stays, exactly as the shipped entries have it.  The log answers in one boot:

| mirrored word `0x4000006c` shows | meaning |
|---|---|
| `0x278` — music 8 **and** sfx 7 **and** fit 2 at once | read-modify-write works on an undecoded address: packing is alive, three settings fit in one word, and `0xF7000010 → 0x40000068` frees up for a fourth |
| one value at a time, other fields zeroed | there is no read-back for an app element, `writeonly` was load-bearing, and `LAYOUT = "proven"` (0.1.49) is the shipping layout |
| the core does not boot | declaring a non-writeonly element at an address this bitstream does not answer is itself the failure — 0.1.25/26's shape, and the two dead control-word builds say what the worst case is |

`sim_menu.py` now models both hypotheses side by side.  With composition on, this is the payoff picture:

```
pick SFX Volume=0x70 -> word 4000006c: 00000208 -> 00000278   [music=8 sfx=7 fit=2]
    SFX Volume slot CLAIMED @4000006c (word 00000278, field 7 = 7/8)
```

and with the measured whole-word write it is docs/358's table again, where every pick erases the others.

## 3. The layout is generated now, which is the actual engineering win

`mk_dist.py` emits the host's menu geometry from the same interact.json it writes, so changing a layout is a JSON
change and not a hunt through `main_of.c`:

```c
#define OF_FIELD_MUSIC_SHIFT   0u
#define OF_FIELD_MUSIC_MASK    0xFu
#define OF_FIELD_MUSIC_LO      1u          /* option values shifted down into the field */
#define OF_FIELD_MUSIC_HI      8u
#define OF_MENU_SLOT_MUSIC     0x4000006Cu /* docs/356's measured mirror map applied per entry address */
```

`MEASURED_MIRROR = {0xF700000C: 0x4000006C, 0xF7000010: 0x40000068}` is the only place the measured map lives, and an
entry mounted on any other address gets slot 0 — hunt, or report "no peripheral slot", never a guess.  `LAYOUT =
"packed" | "proven"` switches between the two shipping shapes.  Two small faults this created and fixed on the spot: an
undeclared Screen Fit briefly had **two** definitions of `OF_FIELD_FIT_*` in `main_of.c` (the inert default and the real
one) so the checker read whichever came first — the row is now simply absent when the JSON does not declare it; and
`check_interact.py` had to learn to cross-check the generated header instead of hand-written constants.

## 4. Persistence: declared, not yet proven for this core

`persist: true` is on every entry, in the shipped style.  The 0.1.42 run proved the firmware *does* persist our
declarations — `Settings/iamghost.night/Interact/_core/interact_persist.json` recorded `id 120 val 3` and `id 121 val 3`
(option **indices**, not wire values).  Right now that file is absent from the card: the ids changed to 130/131/132 and
the folder was pruned, so persistence across boots has to be re-demonstrated rather than assumed.  The test is two
boots with no menu touches:

1. boot, set the three entries, eject → the Settings file should reappear with ids 130/131/132;
2. boot again **without touching Core Options** → the log's first samples should already show `0x4000006c` carrying the
   previous word (`menu: … CLAIMED … the word hardware carried it in on` naming the same values), and the applied
   volumes should match them.

If it does not reappear, persistence needs `defaultval`-shaped workarounds and that is a separate finding.

Card holds core 0.1.50, `DIAG=11`.  Build clean, `check_interact.py` ALL PASS, fixture suite ALL PASS, `src/f`
untouched.
