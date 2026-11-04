# 336 — Pocket: the core's JSON did not match the shape the firmware expects

Hardware round: the core boots and draws the logo, but nothing this core writes ever reaches the card, the
Core Options menu lists every variable and refuses to change any of them, and the terminal never appears even
when the menu's own persisted value says "Terminal".

## What the card said

```
Assets/night/common/night.elf   md5 0b0cc8fac76268949cd6a3c…   == pocket/.obj/night/app.elf   (the run's build)
Assets/night/common/night.log   0 B   (timestamp = the deploy, not the run)
Saves/night/common/night.log    0 B
night_0.sav … night_7.sav       0 B   (both locations)
Cores/iamghost.night/           os25.rbf_r 2 135 596 B, loader.bin, core/data/input/interact/video/audio/variants.json
```

Settings the firmware *did* keep — `Settings/iamghost.night/Interact/_core/interact_persist.json`:

```json
{"id":100,"type":"slider_u32","val":0},   {"id":101,"type":"slider_u32","val":60},
{"id":71,"type":"list","val":2},          {"id":70,"type":"list","val":2}
```

`val 2` on both lists is the "Game + log" / "Terminal" entry, and the sliders sit pinned at their minimums.
So the firmware stores interact values; the menu would not move them, and the app never got a usable value.

## Two structural mismatches, both against the SDK's own files

Compared with `Cores/ThinkElastic.quake/` on the same card (the same openfpgaOS family, menus working):

**1. interact.json was missing the reserved variables.** SDK README:543:

> Variable indices match `interact.json` order. **The first 4 are reserved by the SDK (Analogizer, SNAC, video
> offsets). App-specific options start at index 4.**

`src/sdk/platforms/pocket/templates/interact.json` has six variables (Analogizer, SNAC Adapter, SNAC Controller
Assignment, Analogizer Video Out at `0xF7000000` with masks, then Video H/V Offset sliders at `0xF7000004` and
`0xF7000008`); Quake ships those plus "Pocket LCD" (`id 70`, mask `0xFFFCFFFF`) at index 4.  Ours shipped four
variables total — and put its own sliders **on `0xF7000004` / `0xF7000008`, the SDK's reserved Video H/V Offset
words**.  `mk_dist.py` now copies the template's variables verbatim, appends Quake's Pocket LCD entry, then this
core's three, and emits `of_interact_idx.h` (indices 7/8/9 plus their defaultvals) because `of_interact_get()`
addresses by position, not id.

**2. the save slots shared one address.** Ours declared eight slots at `0x20100000` with
`"address_stride": "0x40000"`.  The SDK template and Quake give each slot its own address
(`0x20100000, 0x20140000, … 0x20340000`); `address_stride` is not a key the Pocket's JSON parser is documented
to expand.  Eight nonvolatile slots on one address is a plain explanation for "no slot ever accepted a write".
`mk_dist.py` emits explicit addresses now.

Also confirmed as *not* the problem: `core.json` (`target_product: Analogue Pocket`, `chip32_vm: loader.bin`,
`cores[0].filename` matching the shipped `os25.rbf_r`), `variants.json` (Quake's `variant_list` is empty too),
`video.json`/`audio.json`/`input.json` shapes, and the top-level key shape of every file — all identical to
Quake's.

## Settings precedence, restated

Because the menu is not a reachable knob on this core, `night.ini`'s `[night]` section is the settings channel:
`RES`, `MUSIC_VOL`, `SFX_VOL`, `CDDA`, `PAD`, `SCREEN`.  A menu value is applied only when it differs from its
`defaultval`, so an unwired `of_interact_get()` answering 0 cannot silently mute music (`MUSIC_VOL=200`) or drop
the tick pump.

## Still open

- whether the interact menu becomes editable with the reserved entries present (the test this document is written
  for, before the fix is known good);
- whether writes land in `Assets/night/common/night.log` or `Saves/night/common/night.log` once each slot has its
  own address;
- **the terminal remains unexplained**: `of_video_set_display_mode(OF_DISPLAY_OVERLAY)` (of_video.h:43-45) was
  requested at boot and reasserted every `present()`, with a boot banner printed, and no text appeared on the
  LCD.  Next step is a channel that cannot fail: draw the diagnostics into the port's own framebuffer
  (`g_video_ram`, an 8×8 font plus a forced palette entry) — the logo already proves that path reaches the screen.

## Round 4: the reserved entries fixed the menu, and two more faults fell out

After 0.1.2 shipped with the template's variables present, **every Core Options variable became editable except
"NIGHT Screen"** — Music Volume moved (persisted `id 100 val 94`), Tick Pump moved, Pocket LCD moved.  The
persisted file still read `id 102 val 2` for NIGHT Screen, the one entry whose address was `0xF700014` and whose
mask was `0xFFFFFFFF`: every working list in the SDK template and in Quake lives on `0xF7000000` with a mask that
leaves the other occupants' bits alone (`mask` = bits the variable does *not* own — Analogizer 15, SNAC 0-4 and
6-9, Analogizer Video Out 10-13, Pocket LCD 16-17).  A list off that word with a hole-free mask is the one shape
the menu refused to move.

That also explains the missing terminal, which had nothing to do with the terminal:

**1. The port overrode the user's choice, 60 times a second.**  "Pocket LCD" (`id 70`, bits 16-17 of
`0xF7000000`) *is* the screen switch — it is what openfpgaOS-Quake ships, and the card had it persisted at
`val 2` = **Terminal**.  My redundant "NIGHT Screen" knob could not be moved, so `of_setting_screen()` stayed 0
and `present()` called `of_video_set_display_mode(OF_DISPLAY_FRAMEBUFFER)` every frame, stomping the bitstream's
terminal mode.  **NIGHT Screen is deleted**; the host now mirrors Pocket LCD: Terminal → `OF_DISPLAY_TERMINAL`,
On/Off → `OF_DISPLAY_FRAMEBUFFER`, with `night.ini`'s `SCREEN=0/1/2` as the fallback when the bridge carries
nothing (1 = terminal, 2 = game with the log overlaid).

**2. The log lived on a slot that is not writable.**  SDK README:466: *"10 persistent save slots (256 KB each),
mapped to APF file IDs 10-19. Slot 8 is reserved for SDK/shared config… Dirty save files are committed through
the bridge when closed."*  My diagnostic log was **slot 9** — neither a save slot (10-19) nor the config slot
(8).  `fopen("night.log","w")` answered with a usable `FILE *`, every line was written to it, `fflush` succeeded,
and the file on the card stayed at 0 bytes through three runs.  The log is now **slot 8 / `night.cfg`** at the
template's own config address `0x20380000`, which is where openfpgaOS-Quake's working `quake.cfg` lives.

Also adopted from README:440-448, which the port had been skipping: *"Apps register filenames at startup, then
use standard C file I/O"* — `of_file_slot_register(id, name)` before the `fopen`, now done for `night.cfg` and
for each `night_N.sav` before a save read or write.

Deployed as 0.1.3 (`night.elf` md5 `63cdfa05…`, 29 data slots, max id 31, `night.cfg` pre-created in
`Assets/night/common/` and `Saves/night/common/`), repo-root `make test` still `== ALL PASS ==`.
