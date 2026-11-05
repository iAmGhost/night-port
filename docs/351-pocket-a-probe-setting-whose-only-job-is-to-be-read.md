# 351 — a probe setting on purpose: declare a value, then look for the register that carries it

Core 0.1.40.  docs/350 proved one thing worth building on: openfpgaOS mirrors a menu choice into **its own peripheral
page**, at `0x40000080`, where an app may read it (Pocket LCD On/Off/Terminal all observed there, in the bits
`interact.json` declares).  Every other route to a setting is closed by measurement — the interact ecall answers a
fixed table of junk, an app read of `0xF7000000` hangs the core, an app element *inside* `0xF7000000` stops the core
booting.

So the open question is exactly one wide: **if this core declares a setting of its own, does its value reach a
register the app can read?**  The answer decides whether NIGHT ever gets real menu settings (volume, fit) or whether
`night.ini` + the L1+B pad combo is the permanent design.  Answering it needs a setting that exists only to be
measured.

## 1. The entry

`interact.json` grows a ninth entry, after the eight the shipped os25 core ships verbatim:

```json
{"name": "Probe Value", "id": 120, "type": "list", "enabled": true, "persist": true, "writeonly": true,
 "address": "0xF700000C", "mask": "0xFFFFFFF0", "defaultval": "0x01",
 "options": [{"value":"0x01","name":"None"}, {"value":"0x02","name":"Low"},
             {"value":"0x03","name":"Mid"},  {"value":"0x04","name":"High"}]}
```

Each choice is deliberate, because each of them is a mistake already paid for:

* **`0xF700000C` — a bridge word no openfpgaOS core decodes.**  That is not a guess picked at random; it is the safe
  class.  0.1.25/26 mounted knobs there and the core booted and ran for hours.  The two cores that never came up
  (0.1.27, 0.1.34) put app elements *inside* `0xF7000000`, and `check_interact.py` now hard-refuses that address.
  An undecoded address means the write goes nowhere — which is fine for a probe: the question is not "does the core
  consume it" but "does the OS echo it somewhere readable".
* **`mask 0xFFFFFFF0` — writable bits 0-3 only.**  Analogue's rule is inverted from what this port assumed for three
  builds: a `1` in the mask means *leave that bit alone*.  An all-ones mask lets the element write nothing, which is
  why 0.1.32's menu entry could not be moved.  This mask lets the element write exactly its own four bits.
* **values `0x01..0x04` — inside the field, and none of them 0.**  0.1.27 offered `0x00..0x80` for a 4-bit field
  (values outside the field it writes) and the core refused to boot; 0 also has to stay free because an untouched
  variable reads 0 on the wire and the host needs 0 to mean "nobody chose".
* **`writeonly: true` — the load-bearing guess.**  Analogue's rule: the read-modify-write cycle "requires the core to
  have both read and write support at each desired address, **unless the element is flagged as `writeonly`**", in
  which case the firmware keeps the value in its own JSON and writes it to the core without reading it back.  An
  address the bitstream does not decode has *neither* read nor write support — which is very likely why every
  previous attempt at an app element was immovable in the UI.  `writeonly` is the one flag that removes the
  read-back requirement.  It is also the risk: if the Pocket's JSON validation rejects `writeonly` on a `list`, the
  core will not boot at all, and `APP_INTERACT_PROBE` goes back to `False` (one line, one build).

## 2. The host side

* `probe_watch()` runs in the pump every second: `of_interact_probe(OF_IDX_PROBE, &v)`, extract `(v >> PROBE_SHIFT) &
  PROBE_MASK`, log on change, and **bracket the change with a full 96-word peripheral snapshot**.  It is not gated on
  `DIAG` — it is one ecall a second, and it is what the run is for.
* Every periph artefact now carries a wall-time stamp, which is what makes a correlation possible at all:
  `periph t=42.5s 0080: 00000000 00000000 …`, `periph move @0x4000000c t=43.0s: 00000000 -> 00000002`,
  `probe t=43.0s: interact[8] err=0 val=00000002 field=2`.
* The generated header carries `OF_IDX_PROBE 8`, and `check_interact.py` cross-checks the host's `PROBE_SHIFT` /
  `PROBE_MASK` against the JSON field in both directions (`ALL PASS`), plus the standing rules: app elements never at
  `0xF7000000`, never an all-ones mask, never a value outside its own field, never 0.

## 3. The run, and what would settle it

Card is at core 0.1.40, `DIAG=11`, `SCREEN=0`, `FB_MODE=1`.

1. Boot, let the game come up.
2. Core Options → **Probe Value**: `None → Low → Mid → High`, **holding each about ten seconds** so at least one
   timestamped dump is taken while it sits there.  No clock needed — the sequence itself is the marker.
3. Play normally in between; eject when the run is over.

Three outcomes, each worth the boot:

| what the log shows | what it means |
|---|---|
| a periph word steps `1 → 2 → 3 → 4` across those timestamps | **openfpgaOS echoes declared values into a readable page.**  NIGHT's volume and fit become real menu settings, wired to that register. |
| the menu entry exists but cannot be changed | `writeonly` did not remove the read-back requirement at an undecoded address — the UI needs a core that answers reads, which a custom core on a prebuilt bitstream cannot be.  Settings surface is closed by construction. |
| the menu entry changes but nothing moves in any register | the write is accepted and discarded: the bridge only decodes fields its own core_top owns.  Closed, and this is now a measured statement rather than an inference. |

A fourth, cheaper finding comes free: whether the Pocket accepts `writeonly` on a `list` at all.  If the core fails to
boot (no boot marker in the log, no `nightlog.sav` growth) that is the answer, and the fix is one flag.

## 4. What this does not touch

`SCREEN=-1` already follows the one menu value proven readable (`0x40000080`), `night.ini` still carries every real
setting, and `DIAG=0` remains the shipping value.  Nothing in the game's behaviour depends on the probe: if it reads
nothing, the port behaves exactly as 0.1.39 did.
