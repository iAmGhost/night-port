# 453 — The Pocket's "files" are slots; a name nothing declares fails silently, not loudly

**Build:** `0.3.0` on branch `approved-0.2.23` = the approved-feel build (`8f02247`, docs/452) plus the log-path fix and nothing else.

The warning that produced this doc: *be careful with file I/O, openfpgaOS/Analogue Pocket is really picky (reserved file IDs and more); consult the README.* Correct, and it caught a bug I had just written.

## What the README actually says

`~/Git/openfpgaSDK/README.md`:

| fact | where |
|---|---|
| Save files are **10 persistent slots, 256 KB each, mapped to APF file IDs 10-19** | :460 |
| **Slot 8 is reserved for SDK/shared config** — never app data or saves | :463, :648 |
| *"The aliases `save_0`–`save_9` and `save:0`–`save:9` remain available **for compatibility**"* | :480 |
| Preferred route is **the save filename from the instance JSON** — `fopen("MyGame_0.sav", "wb")` | :468 |
| **Dirty save files are committed through the bridge when closed** | :464 |
| Slot table: `0` game selector, `1` `os.bin`, `2` `<app>.ini`, `3` app ELF, `4-6` data, `7` sound bank, `8` shared config, **`10-19` Save 0-9** | :630-644 |
| The **filename registry is populated by a directory scan** | :654 |
| `save_test.h:16` — *"the four equivalent paths the kernel accepts for a save: the registered filename, `save:N`, `save_N`, and `slot:10+N`"*; `:162-168` proves a **write to a read-only data slot (`slot:2`) is denied** — i.e. writes in 10-19 are the supported case | `src/apps/save_test.h` |

## What I had just written, and why it goes away

The `0.2.28` log fix (docs/451) added a liveness marker: `fopen("nightlive.txt", "wb")`, falling back to `nightlive.sav`.

Both names are **undeclared**. Nothing in the instance/data JSON binds either name to a slot, and the registry comes from a directory scan of files that *do* bind. So the marker's write would fail — and, worse, it would fail *quietly*, which is the exact failure mode that cost four boots and two weeks of blind tuning in docs/451. An instrument whose own I/O can silently fail is not an instrument.

There is a second hazard. If the scan binds by discovery order, **adding a file to the save window can move other files' ordinals** — and `save:N` is an ordinal. Inventing a file there is precisely how `save:7` came to mean nothing after `0.2.23`. So the marker is withdrawn from the player build, not reworked: to be legitimate it would need a declared slot of its own (`18`), which is a slot-layout change, which is the class of change that broke logging in the first place. The question it answers ("did the app run at all?") can wait for the lab line, where a slot-layout experiment can be verified instead of assumed.

## The chain that ships

```c
static const char *names[] = { LOG_NAME_0, LOG_NAME_2, LOG_NAME };   /* "slot:17", "nightlog.sav", "save:7" */
```

Three names, **all binding slot 17**, in the order the README's preferences and our own hardware history agree on: explicit id first (immune to ordinals), declared filename second (the README's preferred route), compat alias third (the only name proven to land on hardware). `"log.sav"` is removed from the chain — it was never declared, so it was never a real route.

## The guard, extended to the whole surface

`check_slots.py:check_log_open_name()` now resolves the macros in the open chain to their literal strings and requires all of:

- first name is `slot:<id>`;
- `10 <= id <= 19`, and never `8` (SDK-owned);
- that id is declared in `data.json`, `nonvolatile`, **not** `required: true` (a required log would fail the launch);
- every other name in the chain either names the same slot id, or is a bare name that `slot_map.tsv` actually binds;
- no duplicate slot ids, and at most 32 slots (firmware cap).

Self-tests, both of which must bite:

```
[FAIL] of_log.c opens the log by "save:7": the first name must be the explicit "slot:<id>" form --
       save:N is an ordinal and breaks when the save layout changes (docs/451)
[FAIL] log chain tries ""log.sav"", which slot_map.tsv binds to nothing: the registry binds declared
       names, and a name that does not bind fails silently (docs/451)
```

Verified on the ELF that ships — the only file names it carries are declared ones:

```
$ strings -a night.elf | grep -E "^(slot:17|save:7|nightlog\.sav|nightlive\..*|log\.sav)$"
nightlog.sav
save:7
slot:17
```
