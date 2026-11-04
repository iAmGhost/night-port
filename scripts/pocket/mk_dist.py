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
OUT = REPO / "pocket" / "dist" / "night"

CORE_ID = "iamghost.night"
PLATFORM = "night"
APP = "night"
AUTHOR = "iamghost"
VERSION = "0.1.1"          # shown in the Pocket core menu: bump it when the ELF on the card changes
BITSTREAM = "os25"          # indexed 2D + hardware mixer (docs/330 §5)
SAVE_SLOTS = 8              # saves live in ids 10..17
SLOT_BASE = 18              # first audio-track slot: track02 -> 18 ... track15 -> 31 (all ids <= 31)


def cue_audio_tracks(path: Path):
    """Track numbers of every AUDIO index in the CUE, in disc order."""
    tracks = []
    for line in path.read_text().splitlines():
        m = re.match(r"\s*TRACK\s+(\d+)\s+(MODE1/2352|AUDIO)\s*$", line)
        if m and m.group(2) == "AUDIO":
            tracks.append(int(m.group(1)))
    return tracks


def slots_for_tracks(tracks):
    return {SLOT_BASE + i: f"track{t:02d}.bin" for i, t in enumerate(tracks)}


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
        {"id": 9, "name": "Debug Log", "required": False, "parameters": "0x84", "nonvolatile": True,
         "address": "0x20300000", "size_maximum": "0x10000", "extensions": ["log"]},
    ]
    data_slots += [
        {"id": 10 + i, "name": f"Save {i}", "required": False, "parameters": "0x84", "nonvolatile": True,
         "address": "0x20100000", "address_stride": "0x40000", "size_maximum": "0x40000", "extensions": ["sav"]}
        for i in range(SAVE_SLOTS)
    ]
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

    jw(core_dir / "video.json", {"video": {"magic": "APF_VER_1", "scaler_modes": [
        {"width": 320, "height": 240, "aspect_w": 4, "aspect_h": 3, "rotation": 0, "mirror": 0},
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
    jw(core_dir / "interact.json", {"interact": {"magic": "APF_VER_1", "variables": [
        {"name": "Music Volume", "id": 100, "type": "slider_u32", "enabled": True, "persist": True,
         "address": "0xF7000100", "defaultval": 200,
         "graphical": {"signed": False, "min": 0, "max": 255, "adjust_small": 1, "adjust_large": 16}},
        {"name": "Tick Pump Hz", "id": 101, "type": "slider_u32", "enabled": True, "persist": True,
         "address": "0xF7000104", "defaultval": 490,
         "graphical": {"signed": False, "min": 60, "max": 490, "adjust_small": 1, "adjust_large": 30}},
        # Same variable openfpgaOS-Quake ships: it is the OS's own "show the terminal instead of the game"
        # switch, and the core's printf diagnostics (memstat, disc/track selection, load/save reports) go to
        # that 40x30 CP437 terminal (of_terminal.h).  Index 2 -- appended, so indexes 0/1 above keep their
        # positions for of_interact_get().
        {"name": "Pocket LCD", "id": 70, "type": "list", "enabled": True, "persist": True,
         "address": "0xF7000000", "defaultval": 0, "mask": "0xFFFCFFFF", "options": [
             {"value": "0x00000000", "name": "On"},
             {"value": "0x00010000", "name": "Off"},
             {"value": "0x00020000", "name": "Terminal"},
         ]},
    ], "messages": []}})

    inst_slots = [
        {"id": 1, "filename": "os.bin"},
        {"id": 2, "filename": f"{APP}.ini"},
        {"id": 3, "filename": f"{APP}.elf"},
        {"id": 4, "filename": "NIGHT.RES"},
    ]
    inst_slots.append({"id": 9, "filename": f"{APP}.log"})
    for i in range(SAVE_SLOTS):
        inst_slots.append({"id": 10 + i, "filename": f"{APP}_{i}.sav"})
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
        "MUSIC_VOL=200\n"
        "SFX_VOL=255\n"
        "CDDA=0\n"
        "PAD=0\n"
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
        + f"| `{APP}.log` | 9 | the host's diagnostic trail (phases, clock probe, memstat) -- read it after a run |\n"
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
