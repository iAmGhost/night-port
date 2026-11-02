# 300 — straight-down walking: blind-detected, then root-caused to my own docs/291 guard fix

Date: 2026-11-02.  Playtest report: `docs/manual-playtest/04-2026-11-02.md` item 3 —
"the player can't move straight downward (right+down works, but straight down doesn't)".
This item is *outside* the goal's six items; it is a regression **I introduced** (docs/291, commit b1fd1ba).

## 1. Blind detection (rig-only, before any fix, on the build that had the defect)

New drive `scripts/rig/drives/ep1_downonly.txt`: bring-up, then phase A holds **Down alone** for 40 s
(85–126 s), then phase B holds **Right+Down** (128–148 s) as the control the playtest says works.
Dump `/tmp/rig/do1` (port side, pinned clock, `player_recs.jsonl` sidecar, phases mapped through
`markers.jsonl`).

| phase | samples | player Y | Y-change events | states |
|---|---|---|---|---|
| A — DOWN held alone | 390 | **172 → 172** | **0** | 1 (idle) 357, 12 21, 11 12 |
| B — RIGHT + DOWN | 180 | 172 → 195 | 1 | 2 (walk) 109, 21 30 |
| pre — bring-up/idle | 191 | 172 → 172 | 0 | 0 128, 1 51 |

Down alone moves the player in **no axis**: Y is constant and the state never leaves 1 (idle).  The
diagonal walks (state 2) and changes Y.  That is the reported symptom, measured without a human.

## 2. Which byte is which key

`port/src/app/keymap.c:47-48` (the port's own BIOS-scancode map, `main.c:546` agrees):

| scancode | key | cell |
|---|---|---|
| `0x48` | arrow UP | `g_dat_63ed0` |
| `0x4b` | arrow LEFT | `g_dat_63ed3` |
| `0x4d` | arrow RIGHT | `g_dat_63ed5` |
| `0x50` | **arrow DOWN** | **`g_dat_63ed8`** |

The four cells were labelled in the port as "select-held key byte", "mirrored select-held key byte",
"reversed vertical-direction flag", "plain vertical-direction enable" — four names, none of which says
which arrow it is.  That labelling is why docs/291 went wrong.

## 3. The shipped's admission chain, read off the disassembly

`LAB_0003284e` (the walking body shared by states 1, 2 and 0x28) starts at `0x3284e call 0x3493e`
(read the key) and then:

```
0x32856  cmp byte [0x13ed8], 0   ; DS 0x63ed8 = DOWN
0x3285d  jne 0x32868  -> 0x32868 jmp 0x32873 -> body at 0x32882     DOWN admits the body
0x3285f  cmp byte [0x13ed0], 0   ; UP     ; je 0x3286a ; fallthrough -> 0x32868 -> body
0x3286a  cmp byte [0x13ed3], 0   ; LEFT   ; je 0x32875 ; fallthrough 0x32873 jmp 0x32882 -> body
0x32875  cmp byte [0x13ed5], 0   ; RIGHT  ; je 0x3294d              no arrow -> body SKIPPED
```

Inside the body the two movers are called per axis:

```
0x328b2  cmp [0x13ed0],0 ; je 0x328cc
0x328bb..0x328c5          UP    -> call 0x1586b(record, -step)     fn_01586b = the VERTICAL mover
0x328cc  cmp [0x13ed8],0 ; je 0x328e2
0x328d5..0x328dd          DOWN  -> call 0x1586b(record, +step)
0x328e2  cmp [0x13ed3],0 ; je 0x32908
0x328eb..0x328f5          LEFT  -> call 0x1545e(record, -step); 0x328ff mov [record+0x1b],1
0x32908  cmp [0x13ed5],0                                        fn_01545e = the HORIZONTAL mover
```

So **Down is the first key the shipped tests**, and `fn_01586b` is the *vertical* mover — not the
horizontal one.  Several port comments have the axes backwards, which is how a "horizontal keys only"
guard looked plausible.

## 4. Retraction

docs/291 §2 claimed: "0x3285f-0x3287c is a je-chain … DAT_00063ed8 is never tested here — it appears only
inside the vertical sub-branch at 0x328cc. The port's guard also admitted 63ed8, so a jump/up key press
alone walked the player and forced state 2."  **Both halves are wrong**: the chain begins at `0x32856` and
its first test is `63ed8` (Down); `0x328cc` is Down's *own* arm, and it calls the vertical mover.  The
guard that docs/291 "fixed" was correct before docs/291 touched it, and docs/292's red/green pair for it
(Y distribution going from `{172:580, 195:91}` to `{172:912}`, i.e. *removing* vertical movement) measured
the defect it created, not a fix.  docs/291 §2 and docs/292's guard-fix pair are retracted.

## 5. Fix and green measurement

`port/src/f/f.c`, `lab_0003284e`: the guard now reproduces the shipped's four-arrow chain in order, and
the four key-cell declarations are relabelled to the arrows they hold:

```c
if (g_dat_63ed8 != 0 || g_dat_63ed0 != 0 ||                     /* 0x00032856-0x3287c */
    g_dat_63ed3 != 0 || g_dat_63ed5 != 0) {
```

Green run `/tmp/rig/do4` (same drive, same pinned clock, both sides — the shipped side needed the
dosbox-automation tarball re-provisioned, §6):

| phase | before (`do1`) | after (`do4`) |
|---|---|---|
| A — DOWN alone | Y 172→172, 0 Y-events, state 1 × 357 | **state 2 (walk) × 336**, Y reached 195 |
| B — RIGHT+DOWN | Y 172→195, state 2 × 109 | unchanged: Y 172→195, state 2 × 116 |

Phase A went from "never enters the walking body" to "walks" ✓ the diagonal is untouched ✓ no
side-effect on phase B ✓

## 6. Rig repair that this run required

`/tmp/dbgauto` had lost the *extracted* dosbox-automation directory (the tarball path alone is not
enough — `scripts/rig/bringup.sh:21` copies `dosbox-automation-0.85.1-7567ec4-linux-x86_64/`), so two
runs aborted with `calibrate failed` after the port half had already produced a 59 MB dump.  Redownloaded
and **extracted** the 0.85.1 tarball; `route.sh` then completed end-to-end (`/tmp/rig/do4`: 3562 original
frames, 220 screen witnesses, 271 activity-profile cells all SAME).  A port-only run is still usable for
port-side claims, but it silently loses the shipped side — worth checking `calib1.log` before believing a
route produced two sides.

## 7. Standing rules this item produced

- A key cell whose name encodes a guess ("select-held", "reversed vertical-direction") is an invitation to
  misread a guard.  Name such cells after the physical input the port's own keymap assigns them.
- Before "fixing" a guard by *removing* a test that the shipped performs, disassemble the guard's first
  instruction, not the one that matches the symptom you are chasing.  docs/291 started reading at
  `0x3285f`, three bytes late, and lost the whole first arm.
- A red/green pair that *reduces* the amount of movement measured is a warning sign, not a pass.
