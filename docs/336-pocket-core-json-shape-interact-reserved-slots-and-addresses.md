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
