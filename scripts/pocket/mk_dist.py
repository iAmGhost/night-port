#!/usr/bin/env python3
"""Generate the Pocket core tree (pocket/dist/night) from NIGHT/TG_NIGHT.CUE.

The slot table is derived from the shipped CUE so the audio-track slots can
never drift from the disc layout; everything else in the tree is static APF
core JSON.  Run:  python3 scripts/pocket/mk_dist.py  (also `make -C pocket/src/night dist`)

Layout produced (the APF SD-card shape image.sh assembles):

  pocket/dist/night/
    Cores/iamghost.night/{core,data,video,audio,input,interact,variants}.json
    Assets/night/iamghost.night/NIGHT.json      <- instance (slot -> filename)
    Assets/night/common/README.md                <- what the user must put there
    Platforms/night.json

Slot plan (see docs/330 §4-5):
  1 os.bin | 2 night.ini | 3 night.elf
  4 NIGHT.RES (the 51 MB resource container)
  7         bank.ofsf (SDK sound bank, only if MIDI is ever used)
  --        (no spare slots: the firmware allows a maximum of 32 data slots per core)
  10-17     night_N.sav (nonvolatile saves, 256 KiB each)
  18..31    trackNN.bin, one slot per CDDA audio track of TG_NIGHT.IMG (flat in Assets/<platform>/common/)
"""
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
CUE = REPO / "NIGHT" / "TG_NIGHT.CUE"
# The SDK's own core templates.  interact.json's first variables are reserved by the SDK -- README:543,
# "The first 4 are reserved by the SDK (Analogizer, SNAC, video offsets).  App-specific options start at
# index 4" -- so they are copied verbatim rather than skipped.
SDK_TPL = REPO / "pocket" / "src" / "sdk" / "platforms" / "pocket" / "templates"
OUT = REPO / "pocket" / "dist" / "night"

CORE_ID = "iamghost.night"
PLATFORM = "night"
APP = "night"
AUTHOR = "iamghost"
VERSION = "0.1.29"          # shown in the Pocket core menu: bump it when the ELF on the card changes
BITSTREAM = "os25"          # indexed 2D + hardware mixer (docs/330 §5)
SAVE_SLOTS = 7              # game saves live in ids 10..16; id 17 is the log (see below)
sys.path.insert(0, str(Path(__file__).resolve().parent))
from slot_ids import AUDIO_IDS, SAVE_BASE, LOG_ID, RES_ID, BANK_ID, slots_for_tracks   # docs/339


def cue_audio_tracks(path: Path):
    """Track numbers of every AUDIO index in the CUE, in disc order."""
    tracks = []
    for line in path.read_text().splitlines():
        m = re.match(r"\s*TRACK\s+(\d+)\s+(MODE1/2352|AUDIO)\s*$", line)
        if m and m.group(2) == "AUDIO":
            tracks.append(int(m.group(1)))
    return tracks


def slots_for_tracks(tracks):
    if len(tracks) > len(AUDIO_IDS):
        raise SystemExit(f"{len(tracks)} audio tracks but only {len(AUDIO_IDS)} non-save slot ids")
    return {AUDIO_IDS[i]: f"track{t:02d}.bin" for i, t in enumerate(tracks)}


def jw(path: Path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=4) + "\n")


def main():
    tracks = cue_audio_tracks(CUE)
    if not tracks:
        sys.exit(f"no AUDIO tracks found in {CUE}")
    ts = slots_for_tracks(tracks)
    if max(ts) > 63:
        sys.exit(f"audio slots reach {max(ts)}; too high for the APF slot table")

    core_dir = OUT / "Cores" / CORE_ID
    asset_dir = OUT / "Assets" / PLATFORM

    jw(core_dir / "core.json", {
        "core": {
            "magic": "APF_VER_1",
            "metadata": {
                "platform_ids": [PLATFORM],
                "shortname": APP,
                "description": "NIGHT (1996, CD-ROM) on openfpgaOS",
                "author": AUTHOR,
                "url": "",
                "version": VERSION,
                "date_release": "1996-01-01",
            },
            "framework": {
                "target_product": "Analogue Pocket",
                "version_required": "2.2",
                "sleep_supported": False,
                "dock": {"supported": True, "analog_output": False},
                "hardware": {"link_port": True, "cartridge_adapter": 0},
            },
            "cores": [
                {"name": "os25", "id": 0, "filename": f"{BITSTREAM}.rbf_r", "chip32_vm": "loader.bin"},
            ],
        }
    })

    data_slots = [
        {"id": 0, "name": "Game", "required": True, "parameters": 275, "extensions": ["json"]},
        {"id": 1, "name": "OS Binary", "required": False, "parameters": 0, "extensions": ["bin"], "deferload": True},
        {"id": 2, "name": "OS Config", "required": False, "parameters": 0, "extensions": ["ini"]},
        {"id": 3, "name": "Application", "required": False, "parameters": 0, "extensions": ["elf"], "deferload": True},
        {"id": 4, "name": "NIGHT.RES", "required": False, "parameters": 0, "extensions": ["res", "bin"], "deferload": True},
        # No spare data slots here on purpose.  Analogue caps a core at 32 data slots -- "A maximum of 32
        # data slots is allowed" (analogue.co developer docs, core-definition-files/data-json) -- and this
        # core ships 14 audio-track slots plus 10 saves plus the OS quartet, so the spares (5, 6, 8) went:
        # with them declared the plan was 33 and the Pocket refused the instance with
        # "Instance Can't match slot ID" / "framework instance load error" before the core even started.
        {"id": 7, "name": "Sound Bank", "required": False, "parameters": 0, "extensions": ["ofsf"], "deferload": True},
        # The host's own diagnostic trail (docs/335 §4): the Pocket has no console and the core-side terminal
        # mode was not reachable on hardware, so every phase line is also written here and the card is read
        # back afterwards.
        # Slot 8 is the SDK's shared-config slot -- the one openfpgaOS-Quake writes quake.cfg to, and the
        # README's save section names it explicitly ("Slot 8 is reserved for SDK/shared config"), while the
        # writable app save slots are "mapped to APF file IDs 10-19".  A log on id 9 sits in neither category,
        # which is the plain explanation for fopen("night.log","w") answering yes and the file staying at 0
        # bytes through three hardware runs.
    ]
    data_slots += [
        {"id": 10 + i, "name": f"Save {i}", "required": False, "parameters": "0x84", "nonvolatile": True,
         # The SDK template and openfpgaOS-Quake give every save slot its own address.  address_stride is not
         # something the Pocket's JSON parser is documented to expand, and eight slots sharing one address is
         # a plain explanation for no slot ever accepting a write.
         "address": "0x%08X" % (0x20100000 + 0x40000 * i),
         "size_maximum": "0x40000", "extensions": ["sav"]}
        for i in range(SAVE_SLOTS)
    ]
    data_slots.append(
        {"id": 17, "name": "Log Save", "required": False, "parameters": "0x84", "nonvolatile": True,
         # The trail lives in an app save slot, not in slot 8.  README:462 is explicit -- "Slot 8 is reserved
         # for SDK/shared config and is not an app save slot", while the writable app save slots are "mapped to
         # APF file IDs 10-19" -- and README:478 says the same saves are reachable through the aliases
         # "save_0".."save_9" / "save:0".."save:9", which is exactly how the SDK's own test writes one
         # (src/apps/testdemo/test_saves.c:16 fopen("save:9","wb")).  Six runs of a log on slot 8 wrote nothing.
         "address": "0x%08X" % (0x20100000 + 0x40000 * 7), "size_maximum": "0x40000", "extensions": ["sav"]})
    for sid in sorted(ts):
        data_slots.append({"id": sid, "name": ts[sid].split("/")[-1], "required": False,
                           "parameters": 0, "extensions": ["bin"], "deferload": True})
    # The firmware limit, enforced here rather than discovered on hardware.
    if len(data_slots) > 32:
        sys.exit(f"data.json would declare {len(data_slots)} data slots; Analogue allows a maximum of 32 "
                 f"(drop spare slots or pack the audio tracks into fewer files)")
    if max(s["id"] for s in data_slots) > 31:
        sys.exit("a data slot id exceeds 31; the plan is renumbered wrong (ids stay in 0..31 here)")
    jw(core_dir / "data.json", {"data": {"magic": "APF_VER_1", "data_slots": data_slots}})

    # The SDK's own core (dist/sdk/Cores/ThinkElastic.openfpgaOS/video.json, and the pocket template) offers
    # eight scaler modes, 640x480 among them.  Ours declared 320x240 only -- which is why asking the OS for a
    # 640x480 source framebuffer "succeeded" (rc=0, 41 ms) and showed nothing: the core's display path had no
    # scaler mode to put it on the LCD with.  A source mode the scaler cannot map is a black screen.
    modes = [(320, 240, 4, 3), (320, 200, 4, 3), (320, 224, 10, 7), (320, 256, 5, 4),
             (320, 288, 10, 9), (400, 300, 4, 3), (256, 240, 4, 3), (640, 480, 4, 3)]
    jw(core_dir / "video.json", {"video": {"magic": "APF_VER_1", "scaler_modes": [
        {"width": w, "height": h, "aspect_w": aw, "aspect_h": ah, "rotation": 0, "mirror": 0}
        for (w, h, aw, ah) in modes
    ]}})
    jw(core_dir / "audio.json", {"audio": {"magic": "APF_VER_1"}})
    jw(core_dir / "variants.json", {"variants": {"magic": "APF_VER_1", "variant_list": []}})
    jw(core_dir / "input.json", {"input": {"magic": "APF_VER_1", "controllers": [{
        "type": "default",
        "mappings": [
            {"id": 0, "name": "A", "key": "pad_btn_a"},
            {"id": 1, "name": "B", "key": "pad_btn_b"},
            {"id": 2, "name": "X", "key": "pad_btn_x"},
            {"id": 3, "name": "Y", "key": "pad_btn_y"},
            {"id": 10, "name": "L", "key": "pad_trig_l"},
            {"id": 11, "name": "R", "key": "pad_trig_r"},
            {"id": 20, "name": "Start", "key": "pad_btn_start"},
            {"id": 21, "name": "Select", "key": "pad_btn_select"},
        ]}]}})
    # interact.json assembled the way openfpgaOS-Quake's working core has it: the SDK's six reserved
    # variables in template order, then the firmware's Pocket LCD switch, then this core's own options.
    # Ours used to be the whole file -- four variables, with the sliders parked on 0xF7000004/0xF7000008,
    # which are the SDK's reserved Video H/V Offset words.  On hardware every entry was listed in Core
    # Options and refused to change, while Settings/iamghost.night/Interact/_core/interact_persist.json kept
    # values the menu never moved (docs/336).
    reserved = json.loads((SDK_TPL / "interact.json").read_text())["interact"]["variables"]
    pocket_lcd = {"name": "Pocket LCD", "id": 70, "type": "list", "enabled": True, "persist": True,
                  "address": "0xF7000000", "defaultval": 0, "mask": "0xFFFCFFFF", "options": [
                      {"value": "0x00000000", "name": "On"},
                      {"value": "0x00010000", "name": "Off"},
                      {"value": "0x00020000", "name": "Terminal"}]}
    app_vars = [
        # Both knobs are lists, not sliders, and that is a decision about the bridge rather than the menu.
        # A variable is a word in the interact bridge, and the card's Settings/…/interact_persist.json showed
        # what an untouched slider looks like there: Music Volume 0, Tick Pump 0 (which the UI then clamps to
        # its min and shows as 60).  0 is a legal volume -- mute -- so "the bridge carries nothing" and "the
        # user asked for silence" were the same number, and the host could not tell them apart.  Named options
        # with no 0 in them end that: of_save.c accepts only values it can name.
        # 0.1.26 put this at 0xF700000C and it reached the host as nothing.  The Pocket's own doc for
        # interact.json says the firmware "each frame ... read back from the core at a specified 32-bit BRIDGE
        # address, updated with new user-adjusted values, and finally written back" -- a read-modify-write at an
        # address the core has to answer.  openfpgaOS declares only 0xF7000000 (six bit-packed lists), 0xF7000004
        # and 0xF7000008 (the two video offsets): 0xF700000C is a word its bitstream never decodes, so the write
        # landed nowhere and of_interact_get kept reading 0.  Pocket LCD works because bits 16-17 of 0xF7000000
        # are real openfpgaOS state.  So the knob moves into that word, in bits 10-13, which the SDK core's own
        # masks leave free: Analogizer bit 15, SNAC Adapter 0-4, SNAC Assignment 6-7, Video Out 8-9, 15kHz
        # Timing 18-19, Pocket LCD 16-17.  Bits 10-13 are NOT free -- "Analogizer Video Out" has its field there
        # (its mask 0xFFFFC3FF is zero exactly at 10-13) -- so this knob sits at bits 20-23, which nothing in the
        # SDK core writes.  The mask 0xFF0FFFFF is the sentence "only bits 20-23 are mine".
        # Screen Fit: how 320x200 meets the 320x240 panel.  Letterbox is the default -- the bars are what the
        # port has always drawn and the safe choice; Stretch duplicates 40 of the 200 rows to fill the panel;
        # Scaler asks the core for its declared 320x200 framebuffer mode and lets the FPGA do the stretch.
        {"name": "Screen Fit", "id": 101, "type": "list", "enabled": True, "persist": True,
         "address": "0xF7000010", "defaultval": 1, "options": [
             {"value": 1, "name": "Letterbox"},
             {"value": 2, "name": "Stretch"},
             {"value": 3, "name": "Scaler 320x200"}]},
        {"name": "Music Volume", "id": 100, "type": "list", "enabled": True, "persist": True,
         # 0.1.27 put this INSIDE 0xF7000000 (bits 20-23, mask 0xFF0FFFFF) and the core stopped booting: the card
         # kept the 0.1.26 log and wrote nothing new, i.e. the app never got as far as opening the log slot.  That
         # address is openfpgaOS's own control word -- Analogizer / SNAC / Pocket LCD are decoded from it by the
         # bitstream -- and Analogue's doc says the firmware rewrites every declared element's field "each frame".
         # A menu knob that writes a field of the control word every frame is not a knob, it is a periodic poke at
         # core state.  Back to 0xF700000C: 0.1.26 booted with that address all day, so it is proven harmless even
         # though nothing decodes it, and the values are small integers so the host can read them if the OS ever
         # does publish them.
         "address": "0xF700000C", "defaultval": 3, "options": [
             {"value": 1, "name": "Off"},
             {"value": 2, "name": "Low"},
             {"value": 3, "name": "Medium"},
             {"value": 4, "name": "High"},
             {"value": 5, "name": "Full"}]},
        # No tick-rate knob in the menu, on purpose.  It was a measurement aid (docs/335: the port's waits are
        # clock deadlines, so halving the tick rate changes how many ticks a fade or timer gets -- that is not
        # "slow motion", it is a different game), and on the card it was one more number a player had to
        # interpret.  It stays available where a developer reads it: night.ini TICK_HZ=... / NIGHT_TICK_HZ.
        # 0.1.25 shipped it as a list; 0.1.26 ships without it.
        # What an openfpgaOS app can act on is of_video_set_display_mode() (of_video.h:43-45):
        # OF_DISPLAY_TERMINAL / OF_DISPLAY_FRAMEBUFFER / OF_DISPLAY_OVERLAY.  "Game + log" is the diagnostic
        # view that makes a hardware run readable without a card reader.
    ]
    variables = reserved + [pocket_lcd] + app_vars
    jw(core_dir / "interact.json",
       {"interact": {"magic": "APF_VER_1", "variables": variables, "messages": []}})

    # of_interact_get() addresses variables by POSITION, so the indices this layout produced are generated
    # into a header the host compiles against; the order above is the only place they are decided.
    idx = {v["name"]: i for i, v in enumerate(variables)}
    defaults = {v["name"]: v["defaultval"] for v in variables}
    hdr = ["/* generated by scripts/pocket/mk_dist.py: indices into interact.json, which of_interact_get()",
           " * addresses by position and not by id (SDK README:543). */",
           "#ifndef OF_INTERACT_IDX_H", "#define OF_INTERACT_IDX_H"]
    pairs = (("Music Volume", "OF_IDX_MUSIC"), ("Pocket LCD", "OF_IDX_PLCD"),
             ("Screen Fit", "OF_IDX_FIT"))
    for name, const in pairs:
        hdr.append("#define %-12s %d" % (const, idx[name]))
        hdr.append("#define %-12s %s" % ("DEFAULT_" + const, defaults[name]))
    # ...and how many there are.  of_interact_get() past the declared count is an index the OS has no entry for;
    # a diagnostic that reads the whole table must stop at the end of it.
    hdr.append("#define %-12s %d" % ("OF_INTERACT_COUNT", len(variables)))
    # The screen knob IS the firmware's own Pocket LCD variable: bits 16-17 of 0xF7000000, which the openfpgaOS
    # bitstream decodes (openfpgaOS-Quake ships this same entry).  A separate "NIGHT Screen" list was both
    # redundant and inert -- the menu would not move it -- and the host forced OF_DISPLAY_FRAMEBUFFER every
    # frame from its stuck default, overriding the user's "Terminal" choice 60 times a second (docs/336).
    for val, name in ((0x00000000, "GAME"), (0x00010000, "LCD_OFF"), (0x00020000, "TERMINAL")):
        hdr.append("#define %-22s 0x%08X" % ("OF_PLCD_" + name, val))
    hdr.append('#define %-22s \"%s\"' % ("OF_CORE_VERSION", VERSION))
    hdr.append("#endif")
    (REPO / "pocket" / "src" / "night" / "of_interact_idx.h").write_text("\n".join(hdr) + "\n")

    inst_slots = [
        {"id": 1, "filename": "os.bin"},
        {"id": 2, "filename": f"{APP}.ini"},
        {"id": 3, "filename": f"{APP}.elf"},
        {"id": 4, "filename": "NIGHT.RES"},
    ]
    for i in range(SAVE_SLOTS):
        inst_slots.append({"id": 10 + i, "filename": f"{APP}_{i}.sav"})
    inst_slots.append({"id": 17, "filename": "nightlog.sav"})   # the diagnostic trail, written by of_log.c
    for sid in sorted(ts):
        inst_slots.append({"id": sid, "filename": ts[sid]})
    if len(inst_slots) > 32:
        sys.exit(f"NIGHT.json would bind {len(inst_slots)} data slots; Analogue allows a maximum of 32")
    jw(asset_dir / CORE_ID / "NIGHT.json", {
        "instance": {"magic": "APF_VER_1", "variant_select": {"id": 666, "select": False},
                     "data_slots": inst_slots}
    })

    jw(OUT / "Platforms" / f"{PLATFORM}.json", {
        "platform": {"category": "Computer", "name": "NIGHT", "year": 1996, "manufacturer": AUTHOR}
    })

    # The OS config slot (2) is what the chip32 loader reads to pick the
    # bitstream and what the app itself reads for its own knobs (Quake does the
    # same with [quake] CONFIG=/PAK_... keys).
    (asset_dir / "common").mkdir(parents=True, exist_ok=True)
    (asset_dir / "common" / f"{APP}.ini").write_text(
        "[os]\n"
        "GAME=NIGHT\n"
        "INSTANCE=NIGHT\n"
        f"ELF={APP}.elf\n"
        f"VARIANT={BITSTREAM}\n"
        "ARGS=\n\n"
        "[night]\n"
        "RES=NIGHT.RES\n"
        "MUSIC_VOL=128\n"     # the menu's Medium default, so the two agree until the menu is moved
        "SFX_VOL=255\n"
        "CDDA_OFF=0\n"      # explicit off-flags: 0 = music and pad are live
        "PAD_OFF=0\n"
        # SCREEN=0 asks for the game alone.  SCREEN=2 (the host's console over the game picture) is the
        # diagnostic view and is now something the card owner chooses, not something the dist ships with:
        # on hardware it drew the readout over the game and looked like a broken screen (docs/336, 0.1.7
        # reported dm=2 on the device because this line said SCREEN=2).
        # SCREEN: 0 game only, 1 terminal only, 2 console over the game, -1 follow the menu's Pocket LCD value.
        # It is pinned here rather than left to the menu because the interact bridge is ambiguous: five
        # variables share 0xF7000000, and a value of 0 is both "On" and "nothing was carried".
        "SCREEN=-1\n"     # follow the menu's Pocket LCD choice; SCREEN=0/1/2 pins it for a card-only run
        # How the 320x200 game picture meets the 320x240 panel: 0 = 1:1 with 20+20 black bars (the default),
        # 1 = fill the panel by showing 40 of the 200 rows twice, 2 = ask the core's scaler for its 320x200
        # framebuffer mode.  The menu's Screen Fit knob and L1+B do the same thing live; this is the route that
        # is known to work today (docs/342: the interact bridge has not been readable on hardware).
        "FIT=0\n"
        "BLIT_SKIP=0\n"     # 1 = skip the 640x480->320x240 blit while the sampled frame is unchanged (max 8 in a row)
        "FB_MODE=1\n"       # 1 = native 640x480 source framebuffer when the core offers that scaler mode, 0 = software 2x2
        "PRESENT_SYNC=0\n"    # wait for each flip to be presented before writing the surface again
        "TICK_HZ=490\n"
    )

    # The id map, published once so nothing downstream re-derives it: deploy.sh labels its copies from this
    # file, and of_disc.c of_disc_track_slot() is the runtime copy of the same rule.  It exists because the ids
    # are not cosmetic -- 10-19 are save-window slots and a data file bound there cannot be read past 256 KB.
    (OUT / "slot_map.tsv").write_text(
        "4\tNIGHT.RES\n"
        + "".join(f"{10 + i}\t{APP}_{i}.sav\n" for i in range(SAVE_SLOTS))
        + f"17\tnightlog.sav\n"
        + "".join(f"{sid}\t{ts[sid]}\n" for sid in sorted(ts))
    )

    readme = asset_dir / "common" / "README.md"
    readme.parent.mkdir(parents=True, exist_ok=True)
    readme.write_text(
        "# NIGHT (openfpgaOS Pocket core) — data to put in this folder\n\n"
        "Generate everything below from your own copy of the game:\n\n"
        "```sh\nmake -C pocket/src/night data     # -> scratch/pocket/, then `make deploy`\n```\n\n"
        "| file | slot | what it is |\n|---|---|---|\n"
        "| `NIGHT.RES` | 4 | the shipped 51 MB resource container, copied verbatim |\n"
        + "".join(f"| `{fn}` | {sid} | CDDA track {int(fn[-6:-4])} of TG_NIGHT.IMG, raw 44.1 kHz stereo s16 |\n"
                  for sid, fn in sorted(ts.items()))
        + f"| `{APP}_0..{SAVE_SLOTS - 1}.sav` | 10-{9 + SAVE_SLOTS} | save slots, created by the game on the first save |\n"
        + f"| `nightlog.sav` | 17 | the host's diagnostic trail, opened through the SDK alias `save:7` |\n"
        + f"| `{APP}.cfg` | 8 | the host's diagnostic trail (phases, clock probe, memstat) -- read it after a run |\n"
        + "\nSettings live in this folder's `night.ini` under `[night]`: `MUSIC_VOL`/`SFX_VOL` 0-255, "
          "`CDDA=1` mutes the CD music, `PAD=1` ignores the pad (edit the file on the card). On a desktop "
          "build the same knobs are `NIGHT_MUSIC_VOL`, `NIGHT_SFX_VOL`, `NIGHT_CDDA_OFF`, `NIGHT_PAD_OFF`.\n"
        + "\nIn-game save keys (hold L1): `L1+Select` saves the current slot, `L1+Start` loads it, `L1+A` "
          "cycles slots 0-9. On a desktop build the same is `--save N --save-at FRAME` / `--load N`.\n"
    )

    print(f"wrote {OUT}: {len(inst_slots)} instance slots, {len(data_slots)} data slots "
          f"({len(tracks)} audio tracks -> slots {min(ts)}-{max(ts)})")


if __name__ == "__main__":
    main()
