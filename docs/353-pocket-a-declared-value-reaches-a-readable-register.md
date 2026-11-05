# 353 — a declared value reaches a register the app can read: the settings channel exists

Core 0.1.42 measured it, core 0.1.43 uses it.  This is the result docs/342–352 spent six builds and two dead cores
looking for.

## 1. The run

Two entries with **disjoint** value sets were declared, one at a time, and the 96-word peripheral page was sampled
every second with wall-time stamps:

| entry | address | wire values | what the player did |
|---|---|---|---|
| Probe Value (id 120) | `0xF700000C` | 1,2,3,4 = None/Low/Mid/High | left on **High** |
| Probe Value B (id 121) | `0xF7000010` | 5,6,7,8 = Five/Six/Seven/Eight | left on **Eight** |

Two words in openfpgaOS's own peripheral page answered, and each one carried only its own entry's value set:

```
periph move [small] @0x4000006c t= 9.4s: 00000000 -> 00000001     (the entry's defaultval, at boot, untouched by the player)
named t=20.0s: candidate A @0x4000006c = 00000002 (was 00000001)
named t=26.1s: candidate A @0x4000006c = 00000003 (was 00000002)
named t=33.6s: candidate A @0x4000006c = 00000004 (was 00000003)     ← and it stays 4 for the rest of the run

periph move [small] @0x40000068 t=45.6s: 00000000 -> 00000005
periph move [small] @0x40000068 t=60.4s: 00000005 -> 00000008
periph move [small] @0x40000068 t=66.2s: 00000008 -> 00000005
periph move [small] @0x40000068 t=73.3s: 00000005 -> 00000008        ← and it stays 8: Eight, as left

cand @0x4000006c: 4 moves, values 1..4  [probe A range only]
cand @0x40000068: 4 moves, values 5..8  [probe B range only]
census t=75.3s: 3 candidate(s) of 96 registers
```

The two are **separated in time** (`0x40000068` was `0` through every moment probe A moved, and `0x4000006c` was
frozen from 33.6 s on while `0x40000068` stepped four times), **separated in value range** (1..4 versus 5..8, which is
why two disjoint sets were declared), and **each ends exactly where the player left its menu entry** (4 = High, 8 =
Eight).  `0x4000006c` and `0x40000068` are adjacent words.

The third candidate the census printed, `cand @0x40000018: 15 moves, values 1..5`, is what noise looks like: 15 moves
at ~1 s spacing, a range overlapping both probe sets, no relationship to any action.

## 2. The firmware's own record agrees — and reveals what `persist` stores

`/Volumes/Pocket/Settings/iamghost.night/Interact/_core/interact_persist.json`:

```json
{"id": 120, "type": "list", "val": 3},   {"id": 121, "type": "list", "val": 3}
```

Both `3`, which is neither 4 nor 8 — because **the persist file stores the option *index*, not the wire value**: index
3 of None/Low/Mid/High is High (wire `0x04`), index 3 of Five/Six/Seven/Eight is Eight (wire `0x08`).  That is worth
knowing for anyone reading a persist file as evidence of what a core received: it says which option was chosen, not
what landed on the bus.

It also shows the boot value in the register (`1` at t=9.4 s, probe A's `defaultval`) rather than the persisted choice
from the previous run (`3` → Mid) — so this firmware writes the JSON default into the page at boot and does **not**
replay the persisted value into it.  Which is convenient for the host (see §4): the sentinel it is looking for is
predictable.

## 3. What this does not overturn

The three hardware facts paid for with dead or frozen cores all still hold, and they are why this route is the
surviving one:

1. an app `interact.json` element **at `0xF7000000`** prevents the core booting (0.1.27, 0.1.34) — `check_interact.py`
   still refuses it;
2. an app **read** of `0xF7000000` hangs the core (0.1.37) — `bridge_control_word()` stays deleted;
3. `of_interact_get()` carries nothing (0.1.28/29/32/36/41/42): in this very run it answered `interact[8]=5d04d515`,
   `interact[9]=54154cc1` — fixed junk, while the menu entries at those same indices demonstrably moved registers.

The channel is not the ecall and not the control word.  It is openfpgaOS's peripheral page, which any app may read.

> **Corrected by docs/354.** Two readings in this document were wrong, and 0.1.43's run is what caught them.  (1) The
> mirror word is **zero until the player actually changes the entry** -- it is not filled with `defaultval` at boot.
> 0.1.41's `0x4000006c = 1` at 9.4 s was the player having already moved that entry, not the firmware writing option
> 1. (2) A claim rule built on "a word that was 0 and is now the default value" therefore never fires in a run where
> the player never chooses that default, which is exactly why 0.1.43 booted, showed the values landing, and applied
> nothing.  The mechanism described in §1-§3 is confirmed and stronger than claimed here; the host's slot-discovery
> rule in §4 was wrong and is replaced in docs/354.

## 4. Core 0.1.43: the probes become the settings they were always meant to be

| entry | id | address | mask | wire values | names |
|---|---|---|---|---|---|
| **Music Volume** | 130 | `0xF700000C` | `0xFFFFFF00` | `0x41..0x48` | Off, 1/8, 1/4, **1/2** (default), 5/8, 3/4, 7/8, Full |
| **Screen Fit** | 131 | `0xF7000010` | `0xFFFFFF00` | `0x51..0x53` | **Letterbox** (default), Stretch, Scaler 320 |

Values moved into a high, deliberately odd band for two reasons: no natural register in any captured log ever sat in
`0x41..0x48` or `0x51..0x53`, and each entry's **default value becomes a sentinel the host can find its slot with**.

The *layout rule is still unknown* — probe A landed at `0x4000006c` and probe B at `0x40000068`, which is not
declaration order, not id order, and not the element's own address — so the host does not assume one.  It hunts:

```c
/* hunt: 0 -> our own defaultval, and nobody else's slot already */
if (v != m->dflt || g_menu_prev[i] != 0u) continue;
...
NLOG("menu: %s slot FOUND t=%u.%us at @0x%08x (was 0, now its default %02x; range %02x..%02x)\n", ...);
```

Once a slot is claimed:

* every `settings_beat()` (100 ms) reads that one word; a value inside the range that differs from the last one is
  decoded and applied — `level*255/7` for volume through `of_setting_apply_music_vol()`, `0/1/2` for fit through
  `of_setting_apply_fit()`, which feeds the existing `OF_SET_VOL` / `OF_SET_FIT` application path (CDDA volume, the SFX
  mixer group, `fit_apply("menu")`);
* a slot that leaves its range for five consecutive samples is **distrusted and dropped** — a register that can hold
  values the menu cannot write was never the slot.  That is the false-positive guard, and it is why the option ranges
  are disjoint and unusual;
* the census now reports only registers whose every observed value lies inside a menu option range
  (`[music option range]` / `[fit option range]`), which is what a mirror can look like and nothing else.

`of_settings_poll()` stays empty and `settings_beat()` becomes
`of_settings_poll() | menu_slots_scan()` (the scan is `#ifndef OF_PC`, so the PC shim is untouched).

The pad combo keeps working and does not fight the menu: a slot only applies when its **value changes**, so cycling
fit with L1+B stays local until the player moves the menu entry.

## 5. What the next run has to show

1. Two `menu: ... slot FOUND` lines naming two addresses — ideally `0x4000006c` and `0x40000068` again, which would
   also say the slot assignment is stable across boots.
2. A `settings: music_vol=NN applied from the menu` line when Music Volume is moved, and the panel changing when
   Screen Fit is moved (Stretch vs Letterbox is visible on the 320×200 picture).
3. No `slot lost` lines.

If those appear, NIGHT has live menu settings on hardware for the first time, and `night.ini` stops being the settings
surface and becomes what it always was in this port: the developer's instrument (docs/341).  SFX volume, CDDA
on/off and the `FB_MODE` 640×480-vs-downscale choice can each be added the same way, one entry and one sentinel each —
and `FB_MODE` as a menu entry is the cheap way to finally measure both 640×480 routes on one card cycle, which docs/350
§3 still lists as unmeasured (640×480 presents mean 21,344 µs against 320×200 mean 3,608 µs).
