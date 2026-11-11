# 452 — After 0.2.23 the game got worse, and it was the card, not the code

**Baseline:** `0.2.23` (`8f02247`, tagged `approved-baseline`) — now on the card, on branch `approved-0.2.23`.
**Instrumented line:** `main` / tag `instrumented-0.2.28` (`bb6f0cc`).

> *"Yes, this version feels a lot better. changes after this version made game worse."*

That verdict lands on the same commit whose logging I had just spent four boots mourning (docs/451), and it makes me retract a claim I made an hour earlier: I said nothing behavioural shipped after `0.2.23`. **Something made it worse, and I had said it couldn't.** So here is what actually differed between the build the player just played and the build they had been living with.

## 1. The rollback moved two variables, not one

Rolling to `0.2.23` changed the **code** *and* the **card's `night.ini`**. Only one of them was a candidate for "worse".

Code, `0.2.23 → 0.2.28`: `+63` lines in the shared `src/f/f.c`, `+89` in `main_of.c`. Read line by line, they are counters and section timers behind flags — `g_room_calls`, `g_room_drawn`, `g_room_us[5]`, `room_now_us()` which short-circuits to `0` before touching the clock when `g_room_prof == 0`. Nothing in that set changes what the game decides or when it decides it.

Card, every boot since **`0.2.16`** (`0cd5b69`, docs/441), set by `devini.sh` and re-set after each deploy:

```
DIAG=160  ROOM_PROFILE=1  FADELOG=1  FDUMP=2  LOG_COMMIT=32
PRESENT_CAP_MS=25    <-- caps how often a frame may be presented while waiting
DELAY_US=500         <-- pay the guest 0.5 ms per DOS clock read
```

`docs/441` introduced those last two **as instruments that change game time**, on purpose: a hold's wall length is *reads × poll cost*, so to pay the game its real per-read time you add 0.5 ms per read. For a game whose clock is real (docs/450: its ~50 updates/s is CPU-independent, i.e. clock-paced), **adding time inside the wait loop slows the loop's iterations** — which for content whose animation advances per drawn frame is exactly slow motion. That is the mechanism, it was on the card for five versions, and the player was the only instrument because the log was dead (docs/451).

So the most likely truth: the felt regression was my tuning card, shipped to myself as if it were the game.

## 2. One real code defect did turn up in the audit

`fn_038098` (the room pump), as of `0.2.28`:

```c
if (g_room_prof == 0) { const char *rp = getenv("NIGHT_ROOM_PROFILE");
                        if (rp && *rp && *rp != '0') g_room_prof = 1; }
```

guarded by `== 0`, so on the Pocket — where profiling is normally off — it calls `getenv` **on every entry to the game's hottest loop, forever**. `getenv` is a linear `strcmp` walk of the environment. It is probably small, and probably not what the player felt; but an instrument whose off-state costs work is the failure mode this project has named before (docs/411: a diagnostic can be the pathology). The sibling three lines above it does it right (`static int s_frate = -1;` — read once). **Fix:** read the env once, same pattern.

## 3. The test that has not been run, and would close this without a rollback

`0.2.28` with the **release** ini — `DIAG=0 ROOM_PROFILE=0 FADELOG=0 FDUMP=0 DELAY_US=0 PRESENT_CAP_MS=0` — is a build differing from `0.2.23` only by counters behind flags and a working log path. If it feels the same, the code is innocent and the ini was the whole story; if it feels worse, the code is guilty and it bisects cheaply (`0.2.24` is a commit, `0.2.25/26` is the room-pump instrumentation, `0.2.27` is a counter in `palette_push`).

That single boot is the difference between "we lost something in the code" and "we lost the plot because the instrument was on the patient". It is worth one boot whenever the game gets touched again — and never again while a player is judging feel.

## 4. Rules this verdict buys

1. **A tuned card is not the game.** `devini.sh` exists so a card can be measured, and it prints *this is a TEST card — do not ship it*. Judging feel on one is comparing my instruments to my instruments. Feel is judged on `LOG=0 DIAG=0` or not at all.
2. **A boot must self-report its timing shape.** Every log line is meaningless if the reader doesn't know `DELAY_US`/`PRESENT_CAP_MS` were `500`/`25` for it. On the instrumented line the settings line should print the active timing knobs, so a number can never again be quoted off a build whose shape wasn't stated.
3. **Keep `approved-baseline` (`8f02247`) as the thing to beat.** Not as a museum piece — as the reference a candidate must feel *at least* as good as, judged on a quiet card. Any change that can't clear that bar isn't a change.
4. **Feel first, numbers second.** Five docs (`446`–`450`) measured holds, fades, DAC rates and update rates against the original while the player was quietly getting a worse game. Every one of those measurements was taken on the tuned card. When the only clean instrument is a person, use it before the log exists.
