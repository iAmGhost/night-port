# 231 — Item 4 fixed: the card key-wait `fn_00035d5d` was a no-op standin, and its "degenerate spin" note was an inverted read of the shipped jumps

Fix task: `fix-keywait`.  Red detection was committed first (docs/209 `ab96d85`, docs/217 `d705c2f`, gate report
docs/226 `4ea273d`).

## 1. Root cause, from the shipped bytes

`fn_00035d5d` (0x00035d5d, 158 B, 0x35d5d-0x35dfb) is a **press-and-release gate** over the four measured game-key
latches of docs/92 (0x63ea4 enter, 0x63ea5 punch, 0x63ec0 kick, 0x63ec1 space).  Prologue:

```
0x35d5d  push 0x1c ; call 0x418ee        ; Borland stack-check thunk: 0x418ee = xchg [esp+4],eax; call 0x41901
                                          ; and 0x41901 = _CHKSP (stack head vs DAT_00053394, SS vs DAT_0005333c)
                                          ; -> host-side bookkeeping, not game logic
```

Body, three phases (each chain is "if any latch is set, jump to the re-spin arm"):

```
A) 0x35d75-0x35d9b   cmp [0x63ec1]/[0x63ea4]/[0x63ea5]/[0x63ec0], 0
                     any set  -> 0x35d92/0x35d9d -> `jmp 0x35d75`      ; re-spin
                     all clear -> 0x35d9f                              ; leave: a held key has been drained
B) 0x35d9f-0x35dc5   same four tests; the all-clear path reaches 0x35dc9 `jmp 0x35d9f`   ; KEEP spinning
                     any latch set leaves via 0x35dcb                                       ; a PRESS arrived
C) 0x35dcb-0x35df1   same four tests; any latch still set reaches 0x35de8 / 0x35df3 `jmp 0x35dcb` ; re-spin
                     all clear -> 0x35df5 pop ebp/edi/esi/edx/ecx/ebx ; ret               ; the RELEASE drained
```

The port had

```c
void fn_00035d5d(void) {}      /* "4-flag DMA/channel busy-wait ... the second and third checks JUMP BACK
                                * (re-spin) when all four are clear ... would loop forever (degenerate spin)
                                * ... no observable effect. No-op standin." */
```

That comment is an **inverted read of phase B**: B spins *while all four are clear*, which is precisely "wait for
a press", and C spins *while any is set*, which is "wait for the release".  Nothing loops forever; the routine
consumes exactly one press-and-release.

Why this is item 4: `fn_038f3d` (the level-up / status screen) calls the card display and then this gate

* intro card: 0x000391b1 `fn_035f1b("help1.pcx", 0)` -> 0x000391d3 `call 0x35d5d` (port `f.c:25775`), taken when
  `DAT_000633a4 == 0`, i.e. on every first visit of the screen;
* help-page card: 0x0003948c `fn_035f1b("help%d.pcx", 1)` -> 0x00039491 `call 0x35d5d` (port `f.c:25828`);
* the "EXP not enough" card: 0x00039625-2f `fn_035f1b("help8.pcx", 1)` -> 0x0003964f `call 0x35d5d`
  (port `f.c:25840`), reached from the buy path when the purchase is refused.

With the gate a no-op, every one of those cards fell through to the next frame -- "the level-up introduction
advances automatically without a keypress, and the 'EXP not enough' message likewise".  It is the same mechanism
docs/97 fixed for the slide-show's sibling gate `fn_00035dfc` (press -> 0, ESC -> 1); no new gate is invented
here, the shipped one is modelled.  `fn_010c3e`'s "insert the disc" prompt (`0x0010ca2`, port `f.c:5675`) uses the
same gate and now waits for a key like the shipped does.

## 2. The change

`src/f/f.c`: the stub replaced by the three phases, one `while` per shipped phase, each over the same four
latches through `sys_key_spin()`:

```c
void fn_00035d5d(void) {
    while (g_dat_63ec1 || g_dat_63ea4 || g_dat_63ea5 || g_dat_63ec0)   /* A: 0x35d75-0x35d9d */
        sys_key_spin();
    while (!(g_dat_63ec1 || g_dat_63ea4 || g_dat_63ea5 || g_dat_63ec0)) /* B: 0x35d9f-0x35dc9 */
        sys_key_spin();
    while (g_dat_63ec1 || g_dat_63ea4 || g_dat_63ea5 || g_dat_63ec0)   /* C: 0x35dcb-0x35df3 */
        sys_key_spin();
}
```

No timeouts, no ESC escape hatch: the shipped's phase B ignores `DAT_00063e89` (ESC), so a card only advances on
one of the four game keys ✓ the port now matches.

## 3. Suite: the tests that had been pinning the no-op

* `test_fn_035d_spin` (test_f1.c) asserted `fn_035d5d: no side effect on flags` after a call with nothing
  scripted.  It now pins the three phases with the scripted key timeline, and the first check is a *negative
  control for the stub*: a key is held on entry and its release is scripted at tick 2 -- the scripted release is
  only ever applied while the gate spins, so a no-op returns with the latch still set and the check fails.
  Three checks: drains a key held on entry; returns only after press **and** release with all four latches clear;
  an idle entry still consumes one press-and-release.
* `test_fn_038f3d` scenario A and `test_fn_03d244` (both calls) reached the intro card and therefore the gate:
  their timelines now supply one press-and-release of **space** (`0x63ec1`, chosen so the confirm latch stays clear
  and no purchase happens) before the loop-exit latch, exactly as the shipped's card consumes one key.
* `test_fn_010c3e`'s second scenario (disc wrong twice, then right) shows `cdinsert.pcx` twice, so its timeline now
  supplies two press-and-release pairs.  Before this change those two tests **hung** (the suite timed out at 500 s,
  2 289 checks in) which is itself the witness that the gate really waits now.
* `make test`: **== ALL PASS ==**, rc=0, **3 069** checks, 0 failures (the suite was 3 067 before; the two
  added checks are the `fn_035d5d` phase checks).

## 4. Red before / green after, same drive, same rig

New drive `scripts/rig/drives/ep1_lumenu3.txt`: the `ep1_lumenu` shape (selection pokes at 94.5 / 95 s), the
harness warp `NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=5600` entering `fn_038f3d`, **one** press at 98 s, then
a 78 s key-free gap to a final key at 176 s (which also keeps the 2.5 s playtest-idle quit of docs/228 §3b from
cutting the screen short).  `NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_MENUBG=1`,
`stdbuf -o0 -e0`, 12 000 frames, `--rig-hz 5`, dumps `/tmp/rig/kw3_before` (pre-fix binary
`/tmp/prefix/test/night` at `4ea273d`) and `/tmp/rig/kw3_after` (fixed build).

New oracle `scripts/rig/card_hold.py`: MD5 over each frame's 40x30 luminance signature from `frames.bin`, windows
taken from the drive's own key-down lines, so a window is key-free *because the drive says so*.

The level-up screen really was entered in both runs (`markers.jsonl`: `res levelup.spr`, `res levelup2.pcx` at
tick 467 / t=106 400 ms before, tick 469 / t=99 017 ms after).  Its card window is the key-free gap 99-176 s:

| build | samples in window | distinct picture runs | longest hold | new pictures / s |
|---|---|---|---|---|
| before (no-op gate) | 9 283 | **1 478** | 38 samples = **0.32 s** | **19.19** |
| after (gate modelled) | 9 242 | **53** | 9 139 samples = **76.14 s** | **0.69** |

A picture now survives the whole key-free stretch and only changes when a key arrives: the gate consumes one
press-and-release per card, exactly as docs/97's sibling gate `fn_00035dfc` does for the slide-show, and the
shipped side holds 4.5-7.5 s per card in the same kind of window (docs/217 §3, `orig_kw2`: 54 consecutive
identical samples) ✓ same mechanism on both sides, no newly invented wait.

**No collateral change** in the neighbouring key-free windows of the same runs (the gameplay churn is the control):

| window | before | after |
|---|---|---|
| 89-98 s (gameplay, before the screen) | 496 runs, 55.11 / s, longest 0.12 s | 492 runs, 54.67 / s, longest 0.18 s |
| 177-206 s (after the final key) | 35 runs, 1.21 / s, longest 27.37 s | 36 runs, 1.27 / s, longest 26.77 s |

## 5. docs/65 rows added

| claim | verdict | evidence |
|---|---|---|
| `fn_00035d5d` is a DMA/channel busy-wait that would spin forever, so it is a no-op standin | **no: it is a press-and-release card gate; the "jumps back when all clear" reading is phase B waiting for a press, docs/231 §1** | 0x35d9f-0x35dc9 (all clear -> `jmp 0x35d9f`, any set -> 0x35dcb), 0x35dcb-0x35df1 (any set -> `jmp 0x35dcb`, all clear -> 0x35df5 ret) |
| Item 4's auto-advancing cards are cursor logic (`up_ctr`/`down_ctr`) | **partly superseded: the cards advance because the wait call itself was stubbed, docs/231 §1-§2** | 0x391b1/0x391d3 intro, 0x3948c/0x39491 help page, 0x39625-2f/0x3964f help8 "EXP not enough" |
| A suite timeout after a faithful change is a regression | **no: it is the witness that the gate now waits, docs/231 §3** | `fn_038f3d` / `fn_03d244` / `fn_010c3e` fixtures hung until their timelines supplied press-and-release pairs |
