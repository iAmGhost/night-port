# 05 — Game Logic & State Machine

Startup → menu → stage flow, the combat/EXP system, and resolution of every
open item in `docs/03-formats.md` §10. Function addresses are from the Ghidra
12.1.3 LE load of `NIGHT.EXE` (see `docs/01-executable.md`).

## 1. Boot & top-level state machine

Entry `FUN_000420bc` (DOS4GW runtime) → … → **`FUN_0003af18`** (game main).

```
FUN_0003af18:
  FUN_0003ae8e()      // low-level init (video/mem)
  FUN_0003658d()      // audio init
  FUN_0003690b()      // master data load: night.res dir + exptab.dat
  FUN_00010c3e()      // CD-present gate (cdinsert.pcx loop, fingerprint 0xC51) → docs/06-cdda.md
  FUN_0004297a(_), FUN_00041ec3(_)

  do {                                // LAB_0003af69 — outer "attract" loop
      FUN_00038a20()                  // pre-open prep
      FUN_00038ae0()                  // OPENING: opening.fli (Cinepak) + CD track 1
      r = FUN_0003a224()              // MENU; returns 1=exit, 3=replay-opening, else=play
      if (r == 1) { FUN_00037306(); return; }      // quit
  } while (r == 3);

  // --- start a run ---
  FUN_00037b03(0)                     // stage load (see §3)
  DAT_00063354 = DAT_0006334c         // current stage-record pointer
  DAT_00063350 = DAT_0006334c
  DAT_00063398 = byte[DAT_0006334c + 0x11]   // stage variant flag (0/1)
  p = FUN_00041bdc(0x147)             // load resource #327
  p = FUN_00010efc(p, ...)            // build player state
  DAT_00053c7c = DAT_00053c84 = p     // live player struct
  *(int*)(p+1)  = DAT_000639dc + (DAT_00063398 ? 0x15e : -0x1e)  // spawn x by variant

  do {                                // LAB_0003b05a — per-stage body
      if (FUN_0003f15f() == 1) {      // (re)enter VESA mode / frame tick
          if (DAT_00052f2c[episode])  // episode has pre-stage cutscene slides
              FUN_00010b1d(0xb);      //   play CD track 11 (combat)
          for (i < DAT_00052f2c[episode]) {
              sprintf(buf, "%ds%d.pcx", episode, i)   // e.g. "1s0.pcx"
              FUN_0003628e(buf, 0)                   // load+show slide PCX
              ... FUN_00035dfc() (wait) ...
          }
          FUN_00010dde()
          sprintf(buf, "stage%d.pcx", episode)
          FUN_0003628e(buf, 0)                   // load stage background
          <0xC00-iter delay>
      }
      FUN_000405a7()                     // CD poll / get-current-time
      while (true) {                     // inner frame loop
          r = FUN_0003a829()             // run one game frame (combat, §4)
          FUN_00010dde()
          if (r == 1)  break;            // stage cleared → next stage
          if (r == 2)  { FUN_000376c8(); continue; }   // new wave / entity reset
          if (r == 0)  { DAT_00063370 = 1; FUN_00037347();
                         DAT_00063398 = 0; goto LAB_0003af69; }  // back to menu
          if (r == -1) { FUN_00010dde(); FUN_00037306();
                         FUN_00042394("\nThank You For Playing NIGHT\n"); return; }  // game over
          goto LAB_0003b05a;             // re-enter stage body
      }
      <advance episode, goto LAB_0003b05a>   // (FUN_0003ace9 …)
  } while (…)
```

### States (addresses)
| state | function | notes |
|-------|----------|-------|
| init / data | `FUN_0003690b` | loads `night.res` + `exptab.dat` |
| CD gate | `FUN_00010c3e` | `cdinsert.pcx` loop |
| opening | `FUN_00038ae0` | `opening.fli` Cinepak + CD track 1 |
| menu | `FUN_0003a224` | 1=exit, 3=replay opening, else=play; also plays CD track 9 (0x3a442) |
| stage load | `FUN_00037b03` | `.wal` walkmap + `.spr` + `.stg` |
| stage body | `LAB_0003b05a` / `FUN_000376c8` | cutscene slides, `stage%d.pcx` bg, entities |
| frame / combat | `FUN_0003a829` | per-frame; returns 1 clear / 2 wave / 0 menu / -1 gameover |
| combat sub | `FUN_00038f3d`, `FUN_00038445` | fight update + UI (CD tracks 11/12) |
| game over | `FUN_00037306` + `FUN_00042394` | "Thank You For Playing NIGHT" |

Background naming: pre-stage slides are `%ds%d.pcx` (episode, index), the stage
floor is `stage%d.pcx` — episode number is 1-based in the filename
(`DAT_00063370` starts at 1).

## 2. Controls (README.TXT, EUC-KR decoded)

Genre: **street-fighting action** (스트리트 격투 액션). Protagonist: **건 (Gun)**.

| key | action |
|-----|--------|
| arrow keys | move |
| SHIFT | jump |
| CTRL **or** Z | punch |
| ALT **or** X | kick |
| SPACE | crouch / sit |
| ESC | pause & options |

Alternative layout: SHIFT/Z/X = jump/punch/kick, SPACE = sit. NUMLOCK must be
OFF (README warns the protagonist otherwise keeps jumping).

## 3. Stage load (`FUN_00037b03`)

Reads the episode's `.stg` (record reader `FUN_0003e0a0`, 81-byte tagged
records, count in `DAT_00063e14`) and dispatches per tag:

- **tag 1** — ensure entity loaded: `idx = *(u32*)(rec+1)`. If
  `DAT_000639ec[idx] == 0`, load the entity's `.spr` via the name table
  `PTR_s_gun_spr_00051d88[idx]` (e.g. `gun.spr`, `boss1.spr`, `ena-1.spr`) and
  its `.act` via `PTR_s_gun_act_00052198[idx]`; cache in `DAT_000639ec[idx]` /
  `DAT_00053818[idx]`.
- **tag 5** — spawn/anchor point: writes a 5×u32 tuple into
  `DAT_0005d0b4 + n*5` (x, z/y, w, …; half-pixel coords, see §10.6), then
  zeroes the tag byte.
- **tags 0/2/3/6** — not consumed in `FUN_00037b03`; they carry per-entity
  runtime parameters interpreted later by the frame handler `FUN_0003a829` /
  wave init `FUN_000376c8` (see §10.2).

After the `.stg` pass, `FUN_00037b03` reads the `.wal` walkmap into the
collision grid (`DAT_00053e9c`, stride 0x88, 0x65=101 rows; each row has two
1-byte flags + a 10×10 tile block) — this is the walkable/solid map.

## 4. Combat & EXP / 근성치 (tenacity)

- Per-frame combat is `FUN_0003a829` (called once per iteration of the inner
  loop). It drives player input (§2), enemy AI, hit resolution, and the two
  music variants (CD 12 combat / 13 boss, `FUN_00038f3d` + `FUN_00038445`).
- Techniques (기법) are defined in `.key` files (see §10.4) — a combo of
  direction/attack tokens with per-step timing. The player's input stream is
  matched against these to trigger named techniques.
- **EXP table** = `exptab.dat` (ASCII, §10.5). The protagonist **건** gains
  EXP/근성치 (tenacity) from fights; the README describes an EXP/power-up
  system and the 근성치 meter. Table rows are per-episode thresholds.
- Stage cleared when `FUN_0003a829` returns 1 (next episode) or −1 (game
  over / "Thank You For Playing").

## 5. §10 open items — resolution

### 10.1 `.act` header fields + param arrays — RESOLVED
Record reader `FUN_00010181`. A `.act` file = `u32 count`, then `count`
records, each exactly **109 + 10n** bytes where `n` = the record's frame/beat
count (`*piVar2`):
- **109-byte fixed header** (fields read at fixed offsets: leading `u32 n`,
  then u16/u8/u32 scalars, plus a 6×u16 block at byte 74, etc.).
- `u16[n]` array — **sprite frame index** per beat (stored at struct +0x6d).
- `u32[n]` array — per-beat **delay/duration** (struct +0x71).
- `u32[n]` array — per-beat **x-offset/position or event** (struct +0x75).

`109 + 2n + 4n + 4n = 109 + 10n` ✓ (matches the exact-to-EOF parse of all 55
files). The two `u32[n]` semantics (delay vs offset) are inferred from the
allocation sizes and the animation consumer; the frame-index array is
confirmed. Exact per-field naming of the 109-byte header is best-effort (the
reader scatters reads, so a full field-by-field map needs the consumer
`FUN_000376c8` traced field by field) — **structure fully resolved, header
field names partially**.

### 10.2 `.stg` tags 0/2/3/6 — RESOLVED (structure) / consumer located
`FUN_00037b03` handles only tag 1 (load) and tag 5 (anchor). Sample records
from `stage1.stg` show the other tags carry parameter u32s:
- tag 2: `idx, 0, u32, u32` (e.g. `02 00… 04 00… f6 05… 8a 00…`)
- tag 3: `0, u32×4` (four position/size values)
- tag 6: `idx, u32×5` (e.g. `06 29… 00… f8 05… 70 00… 40 06… c6 00…`)
These are **per-entity spawn parameters** (type/index + transform) consumed by
the frame/wave path (`FUN_0003a829` / `FUN_000376c8`), not by the loader.
Structure resolved; exact field roles for 2/3/6 are best-effort (not needed to
render stage-1 geometry — tag 1 + tag 5 give the spawns the slice uses).

### 10.3 `.chr` 116-byte record layout — RESOLVED (structure) / partial semantics
File = `u16 count`(=103) + 103 × 116-byte records (parses exact-to-EOF for all
9 files). Only ~10 records are non-zero per file (the defined
characters/episodes for that stage). Offsets seen in `night1.chr`:
`+0,+4` (u32 pair, present in 10 recs), `+8,+12` (9 recs), then six u32 slots
at `+20/+32/+40/+52/+60/+72/+80/+92/+100` and a u32 at `+112`. Values in the
`+52…+100` slots cluster in the 0x2900–0x3500 range (IDs/offsets). These are
**per-character episode stats** (sprite refs, EXP/tenacity values). Structure
resolved; per-field names best-effort — **not required for the stage-1 slice**
(stage render uses `.stg`/`.act`/`.wal`, not `.chr`).

### 10.4 `.key` token→scancode table — RESOLVED (format) / mapping best-effort
ASCII, CRLF. Layout:
```
<count>\r\n
repeated <count> times:
  <id> <n> <c1> … <cn>\r\n     ; technique id (u16), n steps, n key letters
  <t1> … <tn-1> 0\r\n          ; per-step timing, 0-terminated
  <a> <b>\r\n
  \r\n
  1\r\n
  (padded blank lines to a fixed record size)
```
Observed tokens: `a b c f u` (e.g. `115 5 b b f u a`, `113 5 a a a a a`,
`112 1 a`). The letters are **technique input tokens** (directions +
punch/kick/jump). The physical keys are documented in §2 (arrows, SHIFT,
CTRL/Z, ALT/X, SPACE); the exact letter→scancode map lives in the `.key`
loader (traced during `port-stage1` input work). **Format fully resolved; the
letter table is best-effort** — enough to model the combo system, exact
tokens resolved when implementing input.

### 10.5 `exptab.dat` layout — RESOLVED
ASCII, CRLF, **6 rows × 5 integers** (space-separated), e.g.
`300 1000 4000 5000 5000`. Parsed as plain text by the data loader
(`FUN_0003690b`). Rows = per-episode; columns = EXP/tenacity thresholds
(base, growth, cap…). Values are small (1000–5000), consistent with EXP
amounts. **Fully resolved** (it is plain text; semantics = EXP table per
README's EXP/근성치 system).

### 10.6 320×200 → 640×480 compositing — RESOLVED (path) / kernel best-effort
- `FUN_0003f3be` is **VESA `int 0x10 AH=0x4F05` (set window start)**: it moves
  the 64 KB A0000 window through the linear framebuffer using window table
  `DAT_00063e50[]`, page index `DAT_00063e5c`, and shift `DAT_00063e4c`. This
  is how the 640×480 (307200 B) PCX backgrounds and final frame are written
  across 5×64 KB window pages.
- Sprites are drawn by `FUN_0003bb42` into a **320×200 off-screen buffer
  (stride 0x140)**, clipped to its bounds, using half-pixel world coordinates
  (`.stg` tag-5 anchors and `.wal` grid are in this space).
- The 320×200 sprite layer is then composed onto the 640×480 surface as a
  **2× nearest-neighbor upscale** (320→640, 200→400/480). No dither/pattern
  mode was found in the `FUN_0003f3be` callers. The exact upscale kernel
  (plain 2× vs a specific pattern) is best-effort — **path and scale
  resolved; kernel is plain 2× for the port**.

---
**Summary:** state machine, controls, stage load, and combat/EXP flow are
resolved with function addresses. Every §10 item is resolved at the structural
level; the few remaining best-effort sub-details (`.act` header field names,
`.stg` 2/3/6 field roles, `.chr` field names, `.key` letter table, upscale
kernel) are explicitly not required to build the stage-1 vertical slice and
will be confirmed as their consumers are traced during porting.
