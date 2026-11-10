#!/usr/bin/env python3
import re
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
# The SDK core that runs the same bitstream this port runs: its interact.json is the base for ours.
SDK_OS_CORE = REPO / "pocket" / "dist" / "sdk" / "Cores" / "ThinkElastic.openfpgaOS"
OUT = REPO / "pocket" / "dist" / "night"

CORE_ID = "iamghost.night"
PLATFORM = "night"
APP = "night"
AUTHOR = "iamghost"
VERSION = "0.2.10"          # shown in the Pocket core menu: bump it when the ELF on the card changes
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
    # interact.json = the SDK's own openfpgaOS core file verbatim, plus NIGHT's two knobs appended as new
    # bit-fields of the same control word.  The base is the shipped core's file rather than the template because
    # dist/sdk/Cores/ThinkElastic.openfpgaOS/interact.json is the interact definition for the exact bitstream
    # this port runs (os25) and is the only definition known to work on it: eight entries -- Analogizer bit 15,
    # SNAC Adapter 0-4, SNAC Assignment 6-7, Analogizer Video Out 10-13, Analogizer 15kHz Timing 18-19, Pocket
    # LCD 16-17, and the two offset sliders at 0xF7000004 / 0xF7000008.  The template we used before is MISSING
    # "Analogizer 15kHz Timing", so our menu described a different picture of that word than the core does.
    base = json.loads((SDK_OS_CORE / "interact.json").read_text())["interact"]["variables"]
    # Two NIGHT knobs, added to that word with masks touching only bits the shipped core leaves free.
    # Analogue's rules, read properly this time (analogue.co/developer/docs/openfpga/core-definition-files/
    # interact-json):
    #   "mask: Bits set to '1' will be left untouched in the register.  Bits set to '0' may be modified by the
    #    UI element."
    #   "Each frame, all of the UI elements are read back from the core at a specified 32-bit BRIDGE address,
    #    updated with new user-adjusted values, and finally written back.  This read-modify-write cycle requires
    #    the core to have both read and write support at each desired address, unless the element is flagged as
    #    writeonly."
    # Three separate mistakes in the earlier attempts, all now traceable to those two sentences:
    #   * 0.1.32 copied Doom's mask "0xFFFFFFFF".  Under the real rule an all-ones mask leaves EVERY bit
    #     untouched, so those elements were allowed to write nothing at all -- which is exactly why they drew no
    #     selected state and could not be moved: an element that may not modify the register has no state to
    #     read back.  Doom's App Option 1/2/3 are placeholders in the same condition, and its
    #     interact_persist.json holds ids 20/40/50/60/70 (the reserved entries) and none of theirs.
    #   * 0.1.27 mounted Music Volume in bits 20-23 with option values 0x00..0x80 -- values lying OUTSIDE the
    #     field the element writes.  The core stopped booting.  Every option value now sits inside its own field.
    #   * 0.1.25/0.1.26 mounted the knobs at 0xF700000C / 0xF7000010, words no openfpgaOS core decodes, so the
    #     read half of the cycle had nothing to read.
    # Fields are chosen from what the shipped core leaves untouched -- bits 5, 8-9, 14, 20-23, 24-31 -- and
    # 20-23 is avoided because 0.1.27 showed something lives behind it.  Music Volume takes 24-26, Screen Fit
    # takes 27-28, leaving 29-31 unused.  Values are 1..5 and 1..3 placed IN the field, never 0: an untouched
    # variable reads 0 on the wire and 0 has to keep meaning "nobody chose" (docs/341).
    # The app reads the same word off the bus (of_save.c bridge_control_word()), which is how openfpgaOS's own
    # drivers and the SDK's interactdemo see platform state -- of_interact_get() demonstrably carries nothing
    # (docs/344), so the bridge itself is the channel and the ecall is the cross-check.
    # 0.1.35 turns the knobs off, and the reason is now a pattern rather than a guess.  Both builds that declared
    # an app element INSIDE 0xF7000000 failed to boot the core; every build that declared one anywhere else booted
    # fine, all day, and simply never delivered a value:
    #   0.1.25/26  0xF700000C / 0xF7000010   booted, values unreadable (0x55555555, then fixed junk)
    #   0.1.32     0x03FE0000 / 0x03FE0008   booted, mask 0xFFFFFFFF so the elements could write nothing at all
    #   0.1.27     0xF7000000 bits 20-23      core dead: card kept the previous log, wrote nothing new
    #   0.1.34     0xF7000000 bits 24-26/27-28  core dead again, with masks and values that were well formed this
    #                                          time (every option value inside its own field, none zero)
    # The two failures therefore do NOT single out a bit position -- they single out the address.  The firmware
    # rewrites every declared element's field of that word each frame, and openfpgaOS's core_top is what consumes
    # the word; an app element inside it is a periodic write into state the core owns, and the core stops running.
    # So an app element cannot live at 0xF7000000, and an app element elsewhere is inert on this bitstream.  That
    # is the whole surface: a custom core on the prebuilt openfpgaOS bitstream has no usable interact knob.  The
    # settings a player can change are night.ini and the pad combo (docs/345 §6), and what 0.1.35 measures is
    # whether the app can at least READ the word the firmware writes -- see bridge_control_word() in of_save.c.
    # Flipping this flag back on is the experiment, and it is a one-line experiment: check_interact.py now refuses
    # an app element on 0xF7000000, so re-enabling also means editing the rule that encodes the measurement.
    APP_INTERACT_KNOBS = False
    # 0.1.40 adds ONE entry whose entire purpose is a question: if this core declares a value of its own, does it
    # ever land somewhere the app can read?  The three paths that looked plausible are all closed by measurement --
    # the interact ecall answers a fixed table of junk, an app read of 0xF7000000 hangs the core, and an app element
    # IN 0xF7000000 stops the core booting.  But 0.1.38 found openfpgaOS publishing Pocket LCD at periph register
    # 0x40000080 (On/Off/Terminal all observed, bits 16-17 exactly as interact.json declares), which means the OS
    # mirrors menu state into its own peripheral page -- a page an app may read.
    # So: declare a value, move it, and watch 96 registers for one that follows it.  If a word steps 1 -> 2 -> 3 -> 4
    # in step with the menu, NIGHT's own settings can become real menu settings.  If none does, the answer is final
    # and night.ini plus the pad combo stop being a workaround and become the design.
    # Address 0xF700000C: a bridge word no openfpgaOS core decodes, which is exactly why it is SAFE to declare --
    # 0.1.25/26 mounted knobs there and the core booted and ran for hours (it was 0.1.27 and 0.1.34, both INSIDE
    # 0xF7000000, that never came up).  Mask 0xFFFFFFF0 leaves every bit untouched except 0-3, and the option values
    # 0x01..0x04 sit inside that field, which is the shape 0.1.27 and 0.1.32 violated.
    # "writeonly" is the load-bearing guess: Analogue's rule is that the read-modify-write cycle "requires the core
    # to have both read and write support at each desired address, unless the element is flagged as writeonly", and
    # an entry the bitstream does not decode has neither.  If writeonly is what makes an undecoded element movable in
    # the UI, this probe shows it in the menu itself; if the Pocket rejects the flag for a list, the core will not
    # boot and APP_INTERACT_PROBE goes back to False.
    APP_INTERACT_PROBE = True        # 0.1.42 answered the question this flag was raised to ask: the flag was
                                     # accepted, the value was held, and a register answers it.  See below.
    app_vars = ([] if not APP_INTERACT_KNOBS else [
        {"name": "Music Volume", "id": 120, "type": "list", "enabled": True, "persist": True,
         "address": "0xF700001C", "mask": "0xFFFFFF1F", "defaultval": "0x04", "options": [
             {"value": "0x01", "name": "Mute"}, {"value": "0x02", "name": "Quiet"},
             {"value": "0x04", "name": "Half"}, {"value": "0x06", "name": "Louder"},
             {"value": "0x08", "name": "Full"}]},
        {"name": "Screen Fit", "id": 121, "type": "list", "enabled": True, "persist": True,
         "address": "0xF7000020", "mask": "0xFFFFFFFC", "defaultval": "0x01", "options": [
             {"value": "0x01", "name": "Fit"}, {"value": "0x02", "name": "Fill"},
             {"value": "0x03", "name": "Stretch"}]},
    ])
    # 0.1.42 settled it (docs/352).  Two entries with DISJOINT value sets were declared, the player moved them, and
    # two ADJACENT words in openfpgaOS's own peripheral page followed, each one carrying only its own set:
    #   0x4000006c  0 -> 1 at boot (the entry's defaultval), then 1 -> 2 -> 3 -> 4, ending at 4 = the choice left on
    #               the menu.  Four moves, values 1..4, probe A's set exactly.
    #   0x40000068  silent (0) through the whole window in which A moved four times; then 0 -> 5 -> 8 -> 5 -> 8 in
    #               answer to B's alternation, ending at 8 = the choice left on the menu.  Values 5..8, B's set.
    # Separated in time, separated in value range, both ending exactly where the player left them, and the firmware's
    # own Settings/iamghost.night/Interact/_core/interact_persist.json recorded the choices as option INDICES (id 120
    # val 3 = High, id 121 val 3 = Eight).  That is the settings channel the interact ecall never carried: the OS
    # mirrors a declared value into a register the app is allowed to read.
    # So the probes stop being probes and become the settings they were always meant to be.  Their wire values move
    # to a high, deliberately odd band (0x41..0x48 and 0x51..0x53) for two reasons: no natural register chatter in
    # any captured log ever sat in those ranges, and each entry's DEFAULT value is then a sentinel the host can find
    # the slot with -- "a word that was 0 and is now 0x44" is this core's Music Volume, whatever slot the OS chose.
    # THE MENU CHANNEL MAP, measured on hardware in five runs (docs/352, 354, 355, 356, 358).  A value declared at one
    # of these bridge addresses is mirrored into this readable peripheral word -- and nothing else is mirrored at all:
    #
    #   0xF700000C  ->  0x4000006c     0.1.42 (1..4), 0.1.43 (0x41..0x48), 0.1.45 (0x41..0x48), 0.1.48/49 (0x41..0x48)
    #   0xF7000010  ->  0x40000068     0.1.42 (5..8),  0.1.43 (0x51..0x53), 0.1.45 (0x51..0x53), 0.1.48/49 (0x61..0x68)
    #   0xF7000014  ->  nothing        0.1.44 and 0.1.45: never appeared in any of the 96 words
    #
    # 0.1.45 proved it is the ADDRESS and not the declaration order (Screen Fit, declared LAST at 0xF7000010, was
    # mirrored; SFX Volume, declared SECOND at 0xF7000014, was not).  An entry mounted on an address that is not in
    # this map gets slot 0: the host then hunts, says "no peripheral slot", or does nothing -- it never guesses.
    MEASURED_MIRROR = {0xF700000C: 0x4000006C, 0xF7000010: 0x40000068}

    # TWO LAYOUTS, one flag apart, because the question of whether one word can hold several settings is still open.
    #
    #   "proven"  -- 0.1.48/0.1.49, verified on hardware.  One entry per wired address, whole-word option values.
    #   "packed"  -- 0.1.47's idea, revived.  openfpgaOS's OWN interact.json packs six entries into 0xF7000000 with
    #                disjoint masks (Enable Analogizer bit 15, SNAC Adapter bits 0-4, SNAC Controller Assignment
    #                bits 6-9, Analogizer Video Out bits 10-13, 15kHz Timing bits 18-19, Pocket LCD bits 16-17), so
    #                packing is the documented shape of a Core Options word.  What 0.1.47 measured is that the OS did
    #                NOT compose our fields -- it wrote the whole word -- and the reason docs/345's spec gives for why
    #                is the read-modify-write cycle: "each frame, all of the UI elements are read back from the core
    #                at a specified 32-bit bridge address, updated with new user-adjusted values, and finally written
    #                back", which "requires the core to have both read and write support at each desired address,
    #                unless the element is flagged as writeonly".  A writeonly element has no read-back, so there is
    #                nothing to compose INTO.  0.1.47 used writeonly -- so its whole-word result is what the spec
    #                predicts, not what packing predicts.
    #
    #                This layout drops writeonly, which is also what the user asked for (values must persist).  With
    #                a read-back the OS can merge fields, and three settings fit in one word.  The prediction is test-
    #                shaped: the mirrored word either holds music|sfx|fit at once (read-modify-write on an undecoded
    #                address works, packing is alive and 0xF7000010 frees up) or it holds one value at a time (the
    #                core's interact read is as dead as of_interact_get() looks, docs/342, and "proven" is the answer).
    # CONFIRMED on hardware with 0.1.52 (docs/362): one peripheral word, 0x4000006c, carried music|sfx|fit at the same
    # time -- 0x388 read back as Music Full at bits 0-3, SFX Full at bits 4-7 and the scaler fit at bits 8-9.  Packing
    # is the documented shape of a Core Options word after all; what 0.1.47 measured was the effect of its own
    # "writeonly" flag, which removes the read half of the read-modify-write cycle.  "proven" is kept as the fallback
    # shape (one setting per measured word) in case a future OS build stops composing.
    LAYOUT = "packed"

    if LAYOUT == "proven":
        APP_MENU_SETTINGS = [
            ("Music Volume", 130, "0xF700000C", "0xFFFFFF00", "0x44",
             [("0x41", "Off"), ("0x42", "1/8"), ("0x43", "1/4"), ("0x44", "1/2"),
              ("0x45", "5/8"), ("0x46", "3/4"), ("0x47", "7/8"), ("0x48", "Full")]),
            ("SFX Volume", 131, "0xF7000010", "0xFFFFFF00", "0x64",
             [("0x61", "Off"), ("0x62", "1/8"), ("0x63", "1/4"), ("0x64", "1/2"),
              ("0x65", "5/8"), ("0x66", "3/4"), ("0x67", "7/8"), ("0x68", "Full")]),
        ]
    else:
        APP_MENU_SETTINGS = [
            ("Music Volume", 130, "0xF700000C", "0xFFFFFFF0", "0x04",
             [("0x01", "Off"), ("0x02", "1/8"), ("0x03", "1/4"), ("0x04", "1/2"),
              ("0x05", "5/8"), ("0x06", "3/4"), ("0x07", "7/8"), ("0x08", "Full")]),
            ("SFX Volume", 131, "0xF700000C", "0xFFFFFF0F", "0x40",
             [("0x10", "Off"), ("0x20", "1/8"), ("0x30", "1/4"), ("0x40", "1/2"),
              ("0x50", "5/8"), ("0x60", "3/4"), ("0x70", "7/8"), ("0x80", "Full")]),
            ("Screen Fit", 132, "0xF700000C", "0xFFFFFCFF", "0x200",
             [("0x100", "letterbox"), ("0x200", "stretch"), ("0x300", "scaler 320x200")]),
        ]
    # No "writeonly" flag: persist:true is what stores the choice, and it already worked with writeonly set (the
    # firmware wrote Settings/iamghost.night/Interact/_core/interact_persist.json for 0.1.42's writeonly entries, and
    # in 0.1.49 the mirrored words were still carrying the last picks -- 0x48 and 0x65 -- at the first sample of the
    # next boot, before anything was touched).  Dropping writeonly is not what makes a value persist; it asks the OS
    # for the read half of the read-modify-write cycle, which is the half the os25 bitstream has never answered.
    probe_vars = [
        {"name": n, "id": i, "type": "list", "enabled": True, "persist": True,
         "address": a, "mask": m, "defaultval": d,
         "options": [{"value": v, "name": w} for v, w in opts]}
        for (n, i, a, m, d, opts) in APP_MENU_SETTINGS
    ]
    variables = base + app_vars + probe_vars
    jw(core_dir / "interact.json",
       {"interact": {"magic": "APF_VER_1", "variables": variables, "messages": []}})

    # of_interact_get() addresses variables by POSITION, so the indices this layout produced are generated
    # into a header the host compiles against; the order above is the only place they are decided.
    idx = {v["name"]: i for i, v in enumerate(variables)}
    defaults = {v["name"]: v["defaultval"] for v in variables}
    hdr = ["/* generated by scripts/pocket/mk_dist.py: indices into interact.json, which of_interact_get()",
           " * addresses by position and not by id (SDK README:543). */",
           "#ifndef OF_INTERACT_IDX_H", "#define OF_INTERACT_IDX_H"]
    pairs = [("Pocket LCD", "OF_IDX_PLCD", None)]
    const_of = {"Music Volume": "MUSIC", "SFX Volume": "SFX", "Screen Fit": "FIT"}
    for (n, iid, a, m, d, opts) in APP_MENU_SETTINGS:
        pairs.append((n, "OF_IDX_" + const_of[n], (n, a, m, d, opts)))
    pairs = tuple(pairs)
    for name, const, spec in pairs:
        hdr.append("#define %-12s %d" % (const, idx[name]))
        hdr.append("#define %-12s %s" % ("DEFAULT_" + const, defaults[name]))
        if spec is None:
            continue
        n2, addr, mask, dflt, opts = spec
        mask_i = int(mask, 16)
        writable = (~mask_i) & 0xFFFFFFFF
        low = (writable & -writable).bit_length() - 1
        width = writable.bit_length() - low
        fmask = writable >> low
        vals = sorted(int(v, 16) for v, _ in opts)
        c = const_of[n2]
        # The host reads a FIELD out of the mirror word: shift/mask straight out of the mask it declared, option values
        # shifted down into that field, and the measured peripheral word that carries them (0 if no word has ever been
        # seen to carry this address).  Declaring a layout must not require editing main_of.c as well -- the split
        # between "what the JSON says" and "what hardware was measured to do with it" is the whole lesson of 0.1.44.
        hdr.append("#define %-22s %du" % ("OF_FIELD_%s_SHIFT" % c, low))
        hdr.append("#define %-22s 0x%Xu" % ("OF_FIELD_%s_MASK" % c, fmask))
        hdr.append("#define %-22s %du" % ("OF_FIELD_%s_LO" % c, vals[0] >> low))
        hdr.append("#define %-22s %du" % ("OF_FIELD_%s_HI" % c, vals[-1] >> low))
        hdr.append("#define %-22s %du" % ("OF_FIELD_%s_DFLT" % c, int(dflt, 16) >> low))
        hdr.append("#define %-22s 0x%08Xu" % ("OF_MENU_SLOT_%s" % c, MEASURED_MIRROR.get(int(addr, 16), 0)))
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
    ini_text = (
        "[os]\n"
        "GAME=NIGHT\n"
        "INSTANCE=NIGHT\n"
        f"ELF={APP}.elf\n"
        f"VARIANT={BITSTREAM}\n"
        "ARGS=\n\n"
        "[night]\n"
        "RES=NIGHT.RES\n"
        "MUSIC_VOL=255\n"     # unity: the samples are the level, and the menu knob is what turns it down.  128 was
                         # the old default, and because the group volume used to repeat the voice volume it
                         # meant 0.50 * 0.50 = 0.25 of the recording (docs/361).
        "SFX_VOL=255\n"
        "MASTER_VOL=255\n"  # the mixer output stage; openfpgaOS-Doom sets the same 255
        "SFX_GAIN=200\n"      # sample gain through of_gain16's knee: the game's own samples are quiet,
                         # the loudest one ever measured on the board is 93.8% of full scale, and the knee
                         # cannot reach full scale before 212%, so this one cannot clip (docs/361).
        "CDDA_GAIN=150\n"     # music gain.  6 of the 14 shipped tracks already peak at 0 dBFS, so above
                         # unity this is a compressor -- quiet passages come up, loud ones get squeezed
                         # rather than crack.  100 puts the disc back at exactly its own level.

        "CDDA_OFF=0\n"      # explicit off-flags: 0 = music and pad are live
        "PAD_OFF=0\n"
        # SCREEN=0 asks for the game alone.  SCREEN=2 (the host's console over the game picture) is the
        # diagnostic view and is now something the card owner chooses, not something the dist ships with:
        # on hardware it drew the readout over the game and looked like a broken screen (docs/336, 0.1.7
        # reported dm=2 on the device because this line said SCREEN=2).
        # SCREEN: 0 game only, 1 terminal only, 2 console over the game, -1 follow the menu's Pocket LCD value.
        # It is pinned here rather than left to the menu because the interact bridge is ambiguous: five
        # variables share 0xF7000000, and a value of 0 is both "On" and "nothing was carried".
        # Pinned, on purpose.  SCREEN=-1 used to follow the Pocket LCD field read out of the interact bridge, and that bridge answers garbage on this OS (docs/342) -- a junk word with bits 16-17 set could move the app to the console by itself.  SCREEN=2 overlays the card log on the game for a hardware run; SCREEN=-1 only means something once openfpgaOS answers of_interact_get().
        "SCREEN=0\n"     # 0 = the game alone (the shipping shape). 2 = the diagnostic panel over the picture, 1 = console alone, -1 = follow the menu's Pocket LCD: SCREEN=2 is a developer's instrument, not a player setting (docs/370)
        # How the 320x200 game picture meets the 320x240 panel: 0 = 1:1 with 20+20 black bars, 1 = fill the panel by
        # showing 40 of the 200 rows twice (the default: the panel is filled, which is what the Pocket looks like),
        # 2 = ask the core's scaler for its 320x200 framebuffer mode.  The menu's Screen Fit knob and L1+B do the same
        # thing live, and 0.1.52 confirmed all three on hardware through the packed menu word (docs/362).
        "FIT=1\n"
        "BLIT_DIRTY=0\n"
            # 1 = present only the changed rows; DISABLED on the card: the display path is triple-buffered, so a skipped row stays three frames stale (uncleared pictures, flicker) -- docs/400    # 1 = present only the rows that changed into the display aperture (docs/399); 0 = the 0.1.76 shape, every row every frame
        "BLIT_SKIP=0\n"     # 1 = skip the 640x480->320x240 blit while the sampled frame is unchanged (max 8 in a row)
        "DISC_IDLE_HOOK=0\n"  # 1 = present frames while the CD read waits on SD DMA (docs/426)
        "FDUMP=2\n"  # 2 = auto-arm the fade trace at stage entry (docs/424); 1 = L1+Up only, 0 = off
        "PALETTE_AFTER_FLIP=0\n"  # 1 = upload the palette just after flip (docs/422 fade flicker)
        "PRESENT_SKIP=1\n"
        "ROW_SKIP=0\n"
        "STALL_MS=400\n"    # witness a line when no present has happened this long (docs/433)      # 1 = per-display-buffer row hashes: write only rows that buffer lacks (docs/430)  # 1 = skip the copy AND rely on the proven buffer when a FULL-surface hash (every byte, not the sampled 64th) says the surface is identical to what three display buffers already hold (docs/416); the skip starts 3 frames after each change or mode set
        "DELAY_US=0\n"     # 0 = f.c's own 500 us per delay_units() clock read. The original waits ~1270 us/read on the card hold and ~1890 on the logo hold (docs/417, upper bounds) -- only worth raising once PRESENT_SKIP stops the copy from padding the wait
        "FB_MODE=1\n"    # 1 = native 640x480 surface + row memcpy: 307 200 B / 15 771 us = 19.5 MB/s. 0 = OS 320x240 surface + software 2x2 downscale: 76 800 B / 82 657 us = 0.93 MB/s, ~1 us per output pixel (docs/401). 0.1.84/0.1.85 shipped 0 on a geometry theory and it brought the logo freeze straight back -- the user reported it, exactly as docs/401's title predicts. The corruption that made me doubt 1 was the stale-surface commit (docs/404 5), which is fixed and which lives ONLY in this branch, so 1 is both the fast shape and the one whose defect is now understood\n"
        "PUMP_AUDIO_TICK=0\n"
        "DISC_ASYNC=0\n"         # 1 = the steady-state CD-DA feed uses double-buffered async SD reads (27 sectors each, of_file_read_async); 0 = the synchronous read shape (docs/403)
        "CDDA_CARD_STOP=1\n"           # 1 = silence CD-DA while the stage<N>.pcx hold card is on screen: the game issues no CD call between 0x03b130 and 0x03b1ba, and the original is silent through that hold (docs/413)
        "DISC_RAMP=0\n"           # 0 = prime the CD-DA slice in the requesting visit; 1 = spread it over pump visits (underran: dry 5->49, audio_low 5->64, ahead_min 0.00 s -- docs/402)
        "DISC_PREFILL_FIRST=150\n"    # sectors of CD-DA read before the voices start (2 s). One-shot priming is the shape that measured dry=0 audio_low=0 ahead 0.45 s; its cost is a rare 418-424 ms fill (docs/396, docs/402)
        "DISC_MIN_WANT=12\n"    # 12 is the hard ceiling in of_cdda.c (CD_MIN_WANT_MAX). 27 used to be shipped here and silently ran as 12; docs/420 measured that a real 27 DEFERS small reads -- 36 sectors of a track went unread in the same 2000-frame route that the 12 policy read -- so the ceiling is kept and the key no longer claims otherwise
        "CDDA_STOP_FIX=1\n"     # 1 = keep the CD-DA stop the port puts at fn_000405a7; that function's bytes are INT 10h AX=0013 (a video mode set), so the original cannot issue a stop there (docs/413). 0 = the original's shape: audio keeps playing. Audible either way, so it is a knob and not a default flip (docs/421).

        "DISC_LOOKAHEAD=240\n"   # sectors of CD-DA read-ahead past the 75 sectors/s disc clock. 120 (1.6 s) was SMALLER than the 4 s ring and forced the 1.5-sector trickle reads (docs/406/407)   # wait until 12 sectors of read allowance have accumulated, then take them in one read (docs/375 6); 1 = the old read-whatever-is-due shape
        "DISC_BATCH=27\n"   # sectors per CD-DA slot read: 27 = 63 504 B, the largest single op this OS moves (docs/406)75); 1 = the old shape
        "RESTORE=1\n"   # 1 = a loaded save restores the HP bars (docs/374); 0 = nothing, 2 = state bytes + stage selectors, 4 = the key table
                "PRESENT_SYNC=0\n"    # 0 = do not wait for the flip to complete. 1 cost 18 294 -> 33 436 us on the flip stage (docs/401) and stacked on top of an already 90 ms present    # 1 = wait for each flip to complete before writing the surface again; 0 lets a write overlap scanout of a buffer that is mid-frame (torn/ghosted picture), which is what the 3-buffer display path makes possible    # wait for each flip to be presented before writing the surface again
        "TICK_HZ=490\n"
        # Experimental reads, all off.  0.1.35 shipped them unconditionally and the run died mid-boot (docs/347).
        # One line to opt in, no rebuild; a player's build performs no MMIO scan, no analogizer probe and no
        # interact census.  Bit 3 (8) used to mean "read 0xF7000000" and is retired: with DIAG=15, 0.1.37's log ends
        # at "probe: control-word read begin" -- the read never returned and the core froze (docs/349).
        "DIAG=0\n"
        # The card log: Saves/night/common/nightlog.sav, rewritten every eight lines.  It is the instrument this whole
        # port was measured with (docs/337 onwards), and it is off by default because a player has no reason to carry a
        # quarter-megabyte file they will never read.  Set LOG=1 and the next boot writes the same trail it always did.
        "LOG=0\n"
    )

    (asset_dir / "common" / f"{APP}.ini").write_text(ini_text)

    # A comment this long can swallow the entry after it: my own FB_MODE edit ate "PUMP_AUDIO_TICK=0\n" into its
    # trailing comment, and that key left the generated night.ini for a whole release -- silently, because the
    # code default then takes over and nothing downstream notices.  So the generator names the keys a run
    # depends on and refuses to ship an ini that dropped one (docs/405).
    REQUIRED = ["RES", "MUSIC_VOL", "SFX_VOL", "MASTER_VOL", "SFX_GAIN", "CDDA_GAIN", "CDDA_OFF", "PAD_OFF",
                "SCREEN", "FIT", "BLIT_DIRTY", "BLIT_SKIP", "DISC_IDLE_HOOK", "FDUMP", "PALETTE_AFTER_FLIP", "PRESENT_SKIP", "ROW_SKIP", "STALL_MS", "DELAY_US", "CDDA_STOP_FIX", "FB_MODE", "PUMP_AUDIO_TICK", "DISC_ASYNC",
                "DISC_RAMP", "CDDA_CARD_STOP", "DISC_PREFILL_FIRST", "DISC_MIN_WANT", "DISC_BATCH", "RESTORE", "PRESENT_SYNC",
                "TICK_HZ", "DIAG", "LOG", "DISC_LOOKAHEAD"]
    got = set(re.findall(r"(?m)^([A-Z_]+)=", ini_text))
    missing = [k for k in REQUIRED if k not in got]
    if missing:
        raise SystemExit("mk_dist: night.ini is missing %s -- an entry was lost to a comment or an edit. "
                         "Repair the ini block above; do not paper over it with a code default."
                         % ", ".join(missing))
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
        + f"| `nightlog.sav` | 17 | the host's diagnostic trail, opened through the SDK alias `save:7` -- only when "
          "`night.ini` says `LOG=1` (it ships off, so a player's card carries no log file) |\n"
        + f"| `{APP}.cfg` | 8 | the host's diagnostic trail (phases, clock probe, memstat) -- read it after a run |\n"
        + "\n## Settings\n\n"
          "Three of them are in **Core Options** on the device -- Music Volume, SFX Volume, Screen Fit -- all three in "
          "one bridge word, and they survive an eject and a reboot. `L1+B` cycles the panel fit live. The rest are "
          "developer knobs in this folder's `night.ini` under `[night]`:\n\n"
          "| key | default | what it does |\n|---|---|---|\n"
          "| `MUSIC_VOL` `SFX_VOL` `MASTER_VOL` | 255 | mixer stages, 0-255 |\n"
          "| `SFX_GAIN` `CDDA_GAIN` | 200 / 150 | sample gain in percent, 100-212, through a knee that cannot clip and cannot attenuate |\n"
          "| `FIT` | 1 | panel fit: 0 letterbox (20+20 black bars), 1 stretch (fills the panel), 2 the core scaler's 320x200 |\n"
          "| `SCREEN` | 0 | 0 game, 1 terminal, 2 the diagnostic panel over the game, -1 follow the menu's Pocket LCD |\n"
          "| `CDDA` `PAD` | 0 | `1` mutes the CD music / ignores the pad |\n"
          "| `TICK_HZ` | 490 | cap the game's fixed step rate, 0 = whatever the game programs |\n"
          "| `FB_MODE` | 1 | 1 = native 640x480 source framebuffer, 0 = the host's own 2x2 downscale |\n"
          "| `DISC_PREFILL_FIRST` | 150 | sectors of CD-DA read before the voices start (2 s), in one visit; the rare 418-424 ms fill is its price (docs/396, docs/402) |\n"
          "| `CDDA_CARD_STOP` | 1 | 0 = keep streaming the introduction track over the `stage<N>.pcx` hold card; 1 = silence it, as the original is (docs/413) |\n"
          "| `DISC_RAMP` | 0 | 1 = spread that priming over pump visits; it removes the big fill but the ring never got ahead on hardware (dry 5->49, audio_low 5->64, ahead_min 0.00 s, docs/402) |\n"
          "| `DISC_MIN_WANT` | 12 | sectors of read allowance to accumulate before filling; 1 = the old one-sector-at-the-disc-rate shape |\n"
          "| `DISC_BATCH` | 12 | sectors per CD-DA slot read; 1 reproduces the old one-sector-at-a-time reads |\n"
          "| `RESTORE` | 1 | what a loaded save applies: 1 HP bars, 2 state bytes + stage selectors, 4 key table, 0 nothing (2 and 7 can make the game dereference out of range mid-stage) |\n"
          "| `PUMP_AUDIO_TICK` | 0 | 0 = drain the board's DMA byte ring on every pump entry (the shape every build has used); 1 = drain it once per 490 Hz tick. The hardware A/B lever for the 267 000-pump-entries-a-frame finding (docs/369), off by default because that ring is the one the game is writing into |\n"
          "| `LOG` | 0 | 1 writes `Saves/night/common/nightlog.sav`, the diagnostic trail this port was measured with |\n"
          "| `DIAG` | 0 | which experimental bus probes a run may perform (1 periph scan, 2 analogizer, 4 interact census) |\n"
          "\nOn a desktop build the same keys are `NIGHT_MUSIC_VOL`, `NIGHT_SFX_VOL`, `NIGHT_FIT`, `NIGHT_LOG`, "
          "`NIGHT_CDDA_OFF`, `NIGHT_PAD_OFF` and so on, and the desktop build logs to `scratch/pocket/night.log` "
          "by default because the rig reads it as its witness.\n"
        + "\nIn-game save keys (hold L1): `L1+Select` saves the current slot, `L1+Start` loads it, `L1+A` "
          "cycles slots 0-9. On a desktop build the same is `--save N --save-at FRAME` / `--load N`.\n"
    )

    print(f"wrote {OUT}: {len(inst_slots)} instance slots, {len(data_slots)} data slots "
          f"({len(tracks)} audio tracks -> slots {min(ts)}-{max(ts)})")


if __name__ == "__main__":
    main()
