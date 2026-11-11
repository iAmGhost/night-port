# 451 — The card log went silent with the save-feature removal; the log's name was an ordinal


> **MECHANISM CORRECTED BY docs/454.** The observation here is right -- the silence begins exactly at
> `0.2.23` -- but the cause is not the ordinal alias `save:7`, and not the emptied fallback list. The
> log slot `17` is byte-identical across the working and silent builds; what `0.2.23` deleted was save
> slots `10`-`16`, and the OS does not appear to attach a save slot the save table never reaches.
> Renaming the log to `slot:17` (the fix proposed and shipped here) changed nothing. Read the tooling
> sections as current, and this section as history.
**Build:** 0.2.27 → **0.2.28** · **Date:** 2026-11-11 · **Status:** log path rebuilt, guard added; the confirming boot is the next one

## What went missing, and how long it was missing

The card's diagnostic trail stopped landing. Not degraded — **absent**, and invisible as absent:

| archive | size | md5 | banner |
|---|---|---|---|
| `…171637.log` | 87 616 B | `6c1ee2d2bf593d5df5d31891cf3b4881` | core 0.2.21 |
| `…171905-void-cadence8.log` | 87 616 B | `6c1ee2d2bf593d5df5d31891cf3b4881` | core 0.2.21 |
| `…182953-retry.log` | 87 616 B | `6c1ee2d2bf593d5df5d31891cf3b4881` | core 0.2.21 |
| `…184312-0.2.27-fade.log` | 87 616 B | `6c1ee2d2bf593d5df5d31891cf3b4881` | core 0.2.21 |

Four archives, one file — the bytes the 0.2.21 run wrote, in `Saves/night/common/nightlog.sav`. Between them: three deploys (0.2.24, 0.2.26, 0.2.27), each verified against the build tree by `verify_card.py`, each ejected cleanly, each booted.

The failure is worse than "the log is old". It is **a measurement channel that fails silently while everything around it keeps reporting success.** The deploys said OK. The card said 0.2.27. `logsave.sh` screamed VOID — that guard (`logsave.sh`, added 2026-11-11) is the only reason this was caught in four boots instead of forty; without it I would have read a stale 0.2.21 file and written a conclusion about 0.2.27.

## The mechanism

`of_log.c:38` (before the fix):

```c
#define LOG_NAME   "save:7"   /* the SDK save alias for APF slot 17 (save_0/save:0 is id 10) */
```

and the SDK's own words on what that means (`src/apps/save_test.h:16`):

> The four equivalent paths the kernel accepts for a save: the registered filename, `save:N`, `save_N`, and `slot:10+N`.

`0.2.23` deleted the save feature — the game never had one (docs/374) — which changed the slot declarations:

```
before 0.2.23   nonvolatile slots: 8    ids 10 11 12 13 14 15 16 17
after  0.2.23   nonvolatile slots: 1    id 17 only
```

and emptied the card's save directory from eight files to one.

Slot **17 itself survived** — `data.json` still declares `{"id": 17, "name": "Log Save", "parameters": "0x84", "nonvolatile": true, "size_maximum": "0x40000"}`. What did not survive is the chain of last resorts behind the alias:

- the fallbacks were `night_0.sav … night_6.sav` (`of_log.c:139-146`), the seven files the removal deleted, so the third and fourth chances were gone;
- `LOG_FALLBACKS` is now `{ -1 }`, so `of_log_open()` returns `-1` on the first iteration of its own guard;
- and a total failure reported itself by **`printf`** — which on hardware goes nowhere.

Whether the kernel resolves `save:7` from the *declarations* (fixed `slot:10+N`) or from the files it *discovers* in the save window decides which of those is the proximate cause. **The sources do not say, and this doc will not pretend.** What is settled is the design error, and it is independent of that: *the diagnostic trail keyed its name on the save layout, and had no audible failure mode.*

## The fix

1. **Open by explicit slot id.** `LOG_NAME_0 "slot:17"` first, `save:7` second (kept — it is the name every log that ever landed was opened on), then the bare names. An id names a slot; `save:N` names a position in a list someone can delete.
2. **Record every attempt.** `g_attempts` collects `open "<name>" -> WRITABLE|FAILED` for each try, whether or not the log opens, reachable via `of_log_attempts()`.
3. **`nightlive.txt` — a liveness proof that cannot depend on the log path.** Written every boot by a plain `fopen` (with a `.sav` fallback, because the slot declaration says `"extensions": ["sav"]` and a marker failing for a naming reason would recreate the exact silence this is meant to remove). It states the outcome plus the attempt list. Three states are now distinguishable in one mount:

   | `nightlive.txt` | `nightlog.sav` | meaning |
   |---|---|---|
   | fresh | fresh | healthy |
   | fresh, "LOG OPEN FAILED" | frozen | the save window is writable; **the log path specifically** failed — and the attempts say how |
   | absent | frozen | the app never reached this line, or the window is unwritable |

4. **A guard in the generator's own check.** `check_slots.py:check_log_open_name()` reads **both** the C source and `data.json`: `LOG_NAME_0` must be `slot:<id>`, that id must be declared, nonvolatile, and not `required: true`. Self-test, on the exact regression:

   ```
   [FAIL] of_log.c LOG_NAME_0 is "save:7": must be the explicit "slot:<id>" form --
          save:N is an ordinal and breaks when the save layout changes (docs/451)
   ```

## Why nothing saw it

Every check in the repo verified the card against the build tree — files, bytes, ini keys, slot ids, even the log-report parser's fixtures. None of them read the **source** and the **data declarations** together, so a consumer going quiet after a generator change was structurally invisible. This is the project's recurring shape, restated with a new twist: the last five self-hits were a *report* lying about data that existed. **This is the first where the data never existed and no report was wrong.** Silence is not a passing grade.

## Also worth keeping

The persist files were a red herring worth naming: `Settings/iamghost.night/{Interact,Input,Video}/_core/*_persist.json` moved while `nightlog.sav` did not, which looked like proof of a successful boot with a dead log. Their mttime disagrees with the log's by the card's clock offset (~3 h), so they support no conclusion. A clue whose timestamps are inconsistent with itself is not a clue.

## Reproduce

```sh
python3 scripts/pocket/check_slots.py                    # log-name / data.json cross-check
sed -i '' 's/"slot:17"/"save:7"/' pocket/src/night/of_log.c
python3 scripts/pocket/check_slots.py                    # 1 FAIL -- the guard bites
scripts/pocket/logsave.sh --note any-name                # VOID guard vs card core.json
ls -la /Volumes/Pocket/Saves/night/common/               # nightlive.txt is the liveness proof
```
