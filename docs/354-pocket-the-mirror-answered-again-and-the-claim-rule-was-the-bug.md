# 354 — the mirror answered again with different values, and the port's claim rule was the bug

Cores 0.1.43 (the measurement) and 0.1.44 (the fix).  docs/353 §1–§3 stand; docs/353 §4 was wrong and now carries a
correction notice.

## 1. The mechanism confirmed a second time, with a different value band

0.1.43 mounted two entries at the same addresses as 0.1.42 but with completely different option values — Music Volume
`0x41..0x48`, Screen Fit `0x51..0x53`.  The **same two words** answered, carrying the new values exactly:

```
named t=45.2s: menu slot 6c @0x4000006c = 00000047 (was 00000000)     7/8 chosen
named t=56.2s: menu slot 6c @0x4000006c = 00000041 (was 00000047)     Off
named t=88.3s: menu slot 6c @0x4000006c = 00000048 (was 00000041)     Full
periph move  @0x4000006c t=94.3s: 00000048 -> 00000041                Off -- the value the player left it on

periph move  @0x40000068 t=64.2s: 00000000 -> 00000052                Stretch
periph move  @0x40000068 t=65.2s: 00000052 -> 00000053                Scaler 320
periph move  @0x40000068 t=72.2s: 00000053 -> 00000052                Stretch
periph move  @0x40000068 t=81.3s: 00000052 -> 00000051                Letterbox
```

`0x4000006c` ↔ the entry at `0xF700000C`, `0x40000068` ↔ the entry at `0xF7000010`.  Same words as 0.1.42, values from
a band 0.1.42 never used.  The channel is not a coincidence and not a quirk of small numbers: **openfpgaOS writes a
declared interact value into a fixed word of its peripheral page, and an app may read that word.**

## 2. Two things this corrected

**The word is 0 until the player moves the entry.**  0.1.43's registers sat at `00000000` for the first 45 and 64
seconds — until the player chose something.  docs/353 read 0.1.41's `0x4000006c = 1` at the first sample as "the
firmware writes the JSON defaultval at boot", and 0.1.43 disproves it: with the default `0x44` never chosen, `0x4000006c`
never held `0x44`.  What 0.1.41 actually showed is the player having already moved that entry before the 9.4 s sample.
Consequence for the design: `0` is a meaningful third state — *the menu has not spoken this run* — and it must mean
"keep the value from `night.ini`", not "the option called zero".

**The port therefore applied nothing.**  The claim rule was

```c
if (v != m->dflt || g_menu_prev[i] != 0u) continue;      /* hunt: 0 -> our own defaultval */
```

which only fires on the exact transition *0 → default value*.  The player chose `0x47`, `0x41`, `0x48` — never `0x44` —
so nothing was ever claimed, no `menu:` line was printed, `music_vol` stayed at the ini's 128, and the music played at
full volume no matter what the menu said.  The user's report — "I changed the volume, tried various values, finally
OFF, and it didn't turn the music off" — is exactly what that code does with a correct peripheral signal.

## 3. Why a naive "is the value in range?" rule is also wrong

Two registers in the page are monotonic counters that **climb straight through the option bands**:

```
periph move @0x4000010c t=47.2s -> 0x41        (music option value, coincidence)
periph move @0x4000010c t=75.3s: 00000076 -> 0000007e -> 0x82 -> 0x8a -> 0x8c ...
periph move @0x40000150 t=49.2s -> 0x42, t=51.2s -> 0x53
```

A rule that claimed "the first word whose value is inside the option range" would have grabbed `0x4000010c` and called
a counter Music Volume.  And 0.1.43's own census showed the other half of the trap — it printed

```
cand @0x40000004: 29 moves, values 27138643..-74608374  [music option range]
census t=45.2s: 43 candidate(s) of 96 registers
```

because the range test compared a **signed** int against a free-running counter above 2^31: `hi` printed as
`-74608374` satisfies `hi <= 0x48`.  A 32-bit lesson paid for in a log line.

## 4. Core 0.1.44: assert what was measured, verify it, and treat 0 as silence

* **Known slots.**  `OF_MENU_SLOT_MUSIC = 0x4000006c`, `OF_MENU_SLOT_FIT = 0x40000068` — the two words hardware
  carried in on, in both runs.  The layout rule is still unknown, and the port does not invent one.
* **Verification, not trust.**  A word is *clean* for an entry only if **every sample it has ever shown** is `0` or one
  of that entry's own option values, and it has been seen holding a real option value at least 5 samples (0.5 s at the
  100 ms beat).  `0x4000010c` fails the first test within seconds; `0x4000006c` passes both.  A claimed slot that
  shows anything outside that set is dropped with `menu: ... slot lost`.
* **Fallback hunt** uses the same cleanliness rule rather than a range match, so a counter can never be claimed.
* **`0` means silence**: the slot reads 0 → `menu: ... reads 0 again — the value from night.ini stands`, and no setting
  is applied.  So a player who never opens Core Options gets exactly the 0.1.41 behaviour, and the ini stays the
  developer's instrument.
* **Read budget.**  A full 96-word sweep costs 96 peripheral reads and `settings_beat()` runs every 100 ms, while a
  640×480 present already costs 21 ms of a 33 ms frame.  The sweep now runs at 2 Hz while any slot is unclaimed and
  stops completely once both are claimed: after that a beat is one read per setting.
* Census rewritten on the cleanliness rule, so it reports the real candidates instead of 45.

Expected next log:

```
menu: Music Volume slot CLAIMED t=... @0x4000006c the word hardware carried it in on (value 47 = 7/8, options 41..48)
menu: Music Volume = 7/8 (47) applied: music volume
settings: music_vol=227 applied from the menu
menu: Screen Fit = Letterbox (51) applied: panel fit
census t=...: 2 candidate(s) of 96 registers; claimed: Music Volume, Screen Fit
```

and no `slot lost` line.  If `music_vol` changes and the CD music actually goes quiet on `Off`, then NIGHT has live
menu settings on hardware — the first time in this port — and `night.ini` returns to being what it always was in the
SDL host: a developer's instrument (docs/341).

## 5. 0.1.44 also adds the entry that answers the layout question

A third entry went in at the next word of the bridge page, with a third disjoint band:

```
SFX Volume (id 132)  address 0xF7000014  mask 0xFFFFFF00  values 0x61..0x68 = Off,1/8,1/4,1/2,5/8,3/4,7/8,Full
```

It has no known slot — it has never been observed — so the port hunts for it with the cleanliness rule and `night.ini`'s
`SFX_VOL` stands until something is claimed.  Where its value lands is the structural answer 0.1.42 and 0.1.43 could
not give with two points: `0xF700000C` → `0x4000006c` and `0xF7000010` → `0x40000068` are *reversed and adjacent*, 0x14
bytes from the control word's mirror at `0x40000080`.  If `0xF7000014` lands at `0x40000064` the page carries variables
in a descending slot table; at `0x40000070` an ascending one; anywhere else and the OS allocates slots, which is
exactly why the host hunts instead of assuming.  It is also the setting this port most obviously lacks: the game's
sound effects currently have no menu control at all, only `night.ini`.

## 6. The remaining shape of the unknown
* `of_interact_get()` still answers fixed junk at every index (`interact[8]=5d04d515`, `interact[9]=54154cc1` in this
  run) while those same entries demonstrably move registers, so the SDK's documented settings API remains unusable on
  this OS build; the peripheral page is the workaround, and it works.
* The two dead routes stay closed: an app element at `0xF7000000` (0.1.27, 0.1.34) and an app read of `0xF7000000`
  (0.1.37).
