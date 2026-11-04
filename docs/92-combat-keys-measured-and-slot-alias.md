# 92 — The combat keys, measured AND read from the shipped IRQ handler: the host aliased nothing, and the port's combo latches are slot-array bytes modelled as separate globals

Playtest log items "Keybinds are wrong" (README.TXT) and "Player cannot attack enemies" — input half.
Raw evidence persisted in `analysis/disasm/kbd/`: `kbd_handler_dis.txt` (the guest's INT 9 handler,
0x0003fd40-0x0003fe92, plus its installer fn_03658d-piece at 0x3feb8), `keyxrefs_full.txt` (every xref to
the DS key bytes), `kbd_family_full.txt` (decompiles of fn_034353/0343d1/03442e/034635/034735/0347e3/
0348e2/03493e). Probes: `scripts/playtest/probe_orig_combat_keys.txt` (+2, modlatches, zx_sc) with logs
/tmp/dbgauto/probe_combat{,2}.log, probe_modlatch.log, probe_zxsc.log.

## 1. What the shipped keyboard handler actually does (0x0003fd40, installed on IRQ 9 by fn_03658d)

Linear disassembly (analysis/disasm/kbd/kbd_handler_dis.txt) reads port 0x60 and, per event:

```
scan = in(0x60); idx = scan & 0x7f; break = scan & 0x80
ALIAS TABLE (0x0003fd9d-0x0003fdd9, binary search on idx):
    0x2c (Z) -> 0x1d ; 0x2d (X) -> 0x38 ; 0x2e (C) -> 0x2a ; 0x2f (V) -> 0x1d
    0x30 (B) -> 0x38 ; 0x21 (F) -> 0x2a ; 0x22 (G) -> 0x2a ; everything else keeps idx
if break:  DAT_00063e88[idx] = 0                          (0x0003fe6e)
else:
    if (DAT_00063e88[idx] != 0) {                        (already held, a make-repeat)
        DAT_00063e88[idx] = 1                            (redundant)
        if (DAT_00062c58[idx] != 0) DAT_00062cd8[idx]++  (auto-repeat counter)   (0x0003fe26)
    } else DAT_00062c58[idx] = 1                         (first make arms the slot) (0x0003fe33)
    DAT_0006339c = 0
    if (player[0x17] == 0x28) { player[0x17] = 1; player[0x11] = 0 }   (a key leaves state 0x28)
```

So the whole DS key model is ONE table, `DAT_00063e88[scancode&0x7f]` (held byte) plus the slot array
`DAT_00062c58[scancode&0x7f]` (pump-armed) plus the repeat counters `DAT_00062cd8[idx]` — every byte
docs/91 measured is just a table slot: 0x63e89=+0x01 Esc, 0x63ea4=+0x1c Enter, 0x63ea5=+0x1d Ctrl,
0x63eb2=+0x2a LShift, 0x63eb4=+0x2c (Z's pre-alias byte; the alias means the handler never writes it),
0x63ebe=+0x36 RShift, 0x63ec0=+0x38 LAlt, 0x63ec1=+0x39 Space, 0x63ec3=+0x3b F1.
**There are no "any key" bytes at all** — the docs/60 guess that DAT_00063eb2/0x63ebe are an any-key pair
is wrong; they are exactly the two SHIFT held-bytes. The key-wait loops (fn_038cde 0x38d2d-76 etc.) poll
the whole byte group {+0x01, +0x1c, +0x2a, +0x36, +0x38, +0x39, +0x1d} because that group is "was any key
held" — every reader in the port already reads that exact group (f.c 14807, 14842, 24679, ...).

Alias meaning in the README's own terms: Z=Ctrl (펀치/punch), X=Alt (킥/kick), C=Shift (점프/jump), and the
shipped code also aliases V=Ctrl, B=Alt, F/G=Shift. RCtrl/REnter/RAlt arrive as E0-prefixed codes whose
second byte is 0x1d/0x1c/0x38, so they land on the same slots — measured (probe 2: rightctrl→0x63ea5,
rightalt→0x63ec0, rightshift→0x63ebe which is its OWN slot +0x36).

## 2. Measured confirmation (container, live episode-1 room; docs/91 harness ops)

| key held | ONLY byte set in DS 0x63e80+0x68 | table slot |
|---|---|---|
| leftshift | 0x63eb2 | +0x2a |
| rightshift | 0x63ebe | +0x36 |
| leftctrl / rightctrl / z | 0x63ea5 | +0x1d |
| leftalt / rightalt / x | 0x63ec0 | +0x38 |
| space | 0x63ec1 (+ slot 0x62c58[0x39]) | +0x39 |

Hold-window dumps of 0x62c58 (probe 3) show the slot array bytes at +0x1d (0x62c75), +0x2a (0x62c82),
+0x38 (0x62c90) go 0->1 while ctrl/shift/alt are held, and Z's aliased slot shows its repeat counter at
0x62cd8+0x1d = 0x62cf5 — exactly the handler's arm-then-INC behaviour.

## 3. Port bugs this exposes

1. **fn_03442e's "combo latches" are slot-array bytes modelled as separate globals.** The decompile reads
   DAT_00062c75/0x62c82/0x62c8e/0x62c90 — that is DAT_00062c58[0x1d/0x2a/0x36/0x38]. The port defines
   `g_dat_62c75`...`g_dat_62c90` as independent `uint8_t` (f.c 7670-7673) that NOTHING writes, while the
   pump arms `g_dat_62c58[idx]`. So the combo-marker stamps 0x67/0x68/0x69 (Ctrl+Alt, Ctrl+Shift, Alt+Shift
   pairs — the action chains in the .act tables key on them via fn_03272f's chain match) never fire, and
   single-modifier attacks route through fn_034735's state-4 stamp of the RAW scancode marker ('*' 0x2a,
   '6' 0x36, '8' 0x38, 0x1d) into the ring DAT_00063f14. Fix: alias them into the array (same shape as the
   docs/91 DAT_00053eb8 alias).
2. **The host never aliases Z/X/C/V/B/F/G**, and main.c's SDL switch mapped no ALT and no letter keys at
   all — only LCTRL and LSHIFT. With aliases + array arming, jump/punch/kick reach the shipped resolver.
3. **The host's break handling re-wrote the slot to 0**; the shipped break only clears the held byte
   DAT_00063e88[idx] (the slot decays through fn_0343d1/pump). keymap.c must match.
4. The blanket "set 0x63eb2/0x63ebe on every non-arrow make" (docs/60 guess) must go: those bytes are the
   two shifts. Every key writes its own table slot; the wait loops cover the group.

## 4. Host changes made (this task)

- `src/app/keymap.c`: one measured table writer — alias map {0x2c->0x1d, 0x2d->0x38, 0x2e->0x2a,
  0x2f->0x1d, 0x30->0x38, 0x21->0x2a, 0x22->0x2a}, first make arms g_dat_62c58[idx], repeat makes
  INC g_dat_62cd8[idx], break clears only the held byte; the player-state-0x28 exit and DAT_0006339c=0
  stay host-side as the handler does them.
- `src/f/f.c`: g_dat_62c75/82/8e/90 become `uint8_t *const` aliases into g_dat_62c58 (0x1d/0x2a/0x36/0x38).
- `src/app/main.c`: SDL map gains LSHIFT/RSHIFT/LCTRL/RCTRL/LALT/RALT/Z/X/C (DOS set-1 codes), pt_sc gains
  alt/altgr/z/x/c names.
- `test/test_f1.c`: the key-map table now pins the handler alias map + measured bytes.

## 5. Reproduce

```sh
# original side (container up, live ep-1 room):
export DOSBOX_API_TOKEN=$(cat /tmp/dbgauto/.token)
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_combat_keys3 --drive scripts/playtest/probe_orig_combat_keys.txt --hz 1
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_modlatch --drive scripts/playtest/probe_orig_modlatches.txt --hz 1
# Ghidra side (headless on a copy of the ppk project, the GUI session holds the lock):
cp -R ~/Ghidra/ppk.gpr ~/Ghidra/ppk.rep /tmp/ghproj/
JAVA_HOME=/opt/homebrew/opt/openjdk@21 /opt/homebrew/Cellar/ghidra/12.1.3/libexec/support/analyzeHeadless \
  /tmp/ghproj ppk -process NIGHT.EXE -scriptPath scripts/ghidra_scripts -postScript DumpKeyXrefs.java
# (also DumpRangeDis2.java for the handler, DumpKbdFamily.java for the pump family)
# port side:
cd port && ./test/test_f1 | grep -A120 "key map:"
# scripted live-room run (witnesses ring stamps + player-state transitions):
SDL_VIDEODRIVER=dummy NIGHT_KEYTRACE=1 ./test/night \
  --script ../scripts/playtest/route_combat_keys.txt --shots DIR --no-audio --frames 12500
```

Measured run output (2026-10-27, /tmp/dbgauto/port_combat2.log): every README action fires through BOTH
its keys --

```
ring stamp marker=0x2a -> player state 0x1 -> 0x83   (SHIFT jump; C aliases to 0x2a)
ring stamp marker=0x1d -> player state 0x1 -> 0x33   (CTRL punch)
ring stamp marker=0x1d -> player state 0x1 -> 0x33   (Z punch, via the alias)
ring stamp marker=0x38 -> player state 0x1 -> 0x3d   (ALT kick)
ring stamp marker=0x38 -> player state 0x1 -> 0x3d   (X kick, via the alias)
(SPACE arms slot 0x39 / held byte 0x63ec1; it is deliberately NOT in the pump switch -- fn_03272f's
 dash branch reads the held bytes g_dat_63ec1/0x63ea5/0x63ec0 directly, 0x000334cc-0x0003358f)
```

Shots `idle_mid / jump_shift / punch_ctrl / punch_z / kick_alt / kick_x / crouch_space` in the run's
--shots dir show the poses changing (player-region diff 229-3814 px); `punch_ctrl.bmp` shows the hero
mid-extension against the boxing enemies.

Also fixed on the way (docs/92): pt_sc's letter fallback computed DOS codes linearly from SDL scancodes,
which are HID usage order -- "z" resolved to 0x37. It is now the real A-Z set-1 table (A 0x1e .. Z 0x2c).

## 6. Still open

- DAT_00063eb4 (+0x2c, fn_03272f's dash/attack latch read next to +0x39): the alias table prevents the
  handler from ever writing it; no probe ever saw it set. Who writes it, if anyone, is unresolved —
  flagged, not guessed. (fn_03272f's dash branch also arms on plain Space, which is measured.)
- The 0x2c case in fn_0347e3's pump switch is dead after aliasing (kept as shipped).
