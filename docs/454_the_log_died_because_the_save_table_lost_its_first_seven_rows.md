# 454 — The log died because the save *table* lost its first seven rows, not because of its name

**Build:** `0.3.1` on `fix-save-table` (from `player-0.3.0` = docs/452's approved-feel line). Card log lands again.

## The boots, in order

Every row is one physical boot. `md5` identifies the card file, so a repeated hash means nothing was written.

| build | save ids declared | log opens | result |
|---|---|---|---|
| `0.2.22` | `10`–`17` | `save:7` | **lands** — `930cb1e5…`, 99 135 B |
| `0.2.23` | `17` | `save:7` | silent — `6c1ee2d2…` |
| `0.2.24` | `17` | `save:7` | silent — same bytes |
| `0.2.26` | `17` | `save:7` | silent — same bytes |
| `0.2.27` | `17` | `save:7` | silent — same bytes |
| `0.2.28` | `17` | `slot:17`, `nightlog.sav`, `save:7` | silent — same bytes |
| `0.3.0` | `17` | `slot:17` first | silent — same bytes |
| **`0.3.1`** | **`10`–`17`** | `slot:17` first | **lands** — `10967361…`, 70 063 B, `log: opened "slot:17"` |

The log's own `data.json` entry — `{"id": 17, "name": "Log Save", "parameters": "0x84", "nonvolatile": true, "address": "0x202C0000", "size_maximum": "0x40000", "extensions": ["sav"]}` — is **byte-identical** across every row of that table. So is `NIGHT.json`'s `{"id": 17, "filename": "nightlog.sav"}` binding. The only thing that changed between the two landing builds and the six silent ones is that slots `10`–`16` exist.

## What it was, and what it wasn't

The OS does not appear to attach a save slot that the table never reaches. Declare `17` alone and `fopen` still answers with a `FILE *`, writes still succeed, `fflush` still succeeds, commits still "happen" — and nothing ever reaches the card. There is no error, because from the app's side nothing failed.

That is why renaming the log (`save:7` → `slot:17`) changed nothing: **the name was never the failure.** It is also why four "VOID BOOT" alerts were not evidence of a cached old core running: there was no stale process, just a stale file. Every conclusion drawn from those boots was reading `0.2.21` bytes.

Two earlier diagnoses get corrected here, both mine:

- **docs/451** said the break was the ordinal alias `save:7` losing its position when the seven save slots were deleted, plus the fallback list (`night_0.sav … night_6.sav`) being emptied. The observation (silence begins exactly at `0.2.23`) was right; the mechanism was wrong. Ordinals and fallbacks were irrelevant — the file was never reachable at all.
- **docs/453** withdrew the `nightlive.txt` marker for writing an undeclared name. That reasoning still stands on its own terms, but the marker would not have distinguished our two states, because both "log failed" and "no app" produce an absent file when the *window itself* is inert.

## Why I trust the shape, not just the result

The hypothesis was formed from the card, not from imagination: both cores on it that write saves declare the save table **contiguously**, every slot at its own `0x40000` stride —

```
ThinkElastic.quake   nonvolatile: 8(shared), 10..19   each at 0x20100000 + 0x40000*n
ThinkElastic.doom    nonvolatile: 9(settings), 10..19
```

— and the deleted line in `0.2.23` was exactly that table. `0.2.22`, which had `10`–`17`, lands a log **today, on this card, with the seven save files physically absent** — which also kills "the files went missing" as an explanation: they are missing in both cases.

## What ships

`0.3.1` = the approved-feel build (docs/452) + the log path + the restored save *table*. **The save feature remains gone** (docs/449): no UI, no `L1` combos, no blob, and nothing in the game or the host ever opens `night_0.sav … night_6.sav`. They are declared and bound so the table is contiguous, and that is all they do. Slot count is 28, against Analogue's 32-slot cap (`check_slots.py` enforces the cap).

`check_slots.py:check_log_open_name()` now requires all of:

- the log's first open name is `slot:<id>`, and that id is in `10..19`, never `8`;
- the id is declared, nonvolatile, not `required: true`;
- **every id from `10` up to the log slot is declared** — the contiguity rule, with the eleven-boot story in its failure message;
- every other name in the chain is either the same slot id or a name `slot_map.tsv` really binds;
- no duplicate ids, at most 32 slots.

```
[PASS] log chain ['"slot:17"', '"nightlog.sav"', '"save:7"'] all bind slot 17
       (nonvolatile, not required, 0x40000); 28 slots, cap 32
```

## The lesson, which is the same lesson again

`README.md` says the writable app save slots are "mapped to APF file IDs 10-19", and I read that as a range I was free to sample from. The two working cores on the card read it as a table. When a platform is documented as reserving a structure, the safe assumption is that it **walks** that structure — and that a hole in it produces no diagnostic, because from the application's side every call returned success.

Also: for eleven boots, the only evidence that anything was wrong was a *repeated hash*, and I nearly lost that signal too — `logsave.sh`'s VOID guard exists only because an earlier boot's stale file had already been mistaken for a fresh measurement. The cheapest instrumentation in this project has been `md5` on a file the app writes. Put it in the tool you already run.
