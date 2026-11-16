"""The one place that decides which APF file id carries which file -- now in the tool, not the build.

`scripts/pocket/slot_ids.py` used to hold this table, which meant the only thing a Pocket user without a
compiler had to do -- put a file in a folder -- could not be checked for them: `night-extractor` had to
either duplicate the ids or say nothing about them.  The table lives here and that script re-exports it, so
`gen_disc.py` (disc_table.h), `mk_dist.py` (data.json, NIGHT.json), `deploy.sh` (its labels, from
slot_map.tsv) and `check_slots.py` (the agreement) still import the same numbers, and a build of the core is
no longer on the path between owning the game data and putting it on a card.

README:644 reserves ids 10-19 for saves ("Save 0-9 -- nonvolatile CRAM0 save slots (256 KB each)", bridge
0x20100000 with a 256 KB stride) and id 8 for the SDK.  A data file bound into the save range is served from
that 256 KiB window and every read past byte 262144 comes back short -- docs/339 has the card log that
proves it.  So audio may only sit on the ids the table leaves free for app data: 4-6 (Data 1-3, NIGHT.RES
takes 4) and anything from 20 up.

There is no arithmetic left in C to get wrong: 0.1.22 shipped `19 + track` against a manifest built as
`20 + n`, which played track13.bin for the title theme and asked for a nonexistent slot 34 for track15.
"""
from __future__ import annotations

SAVE_IDS = list(range(10, 20))
AUDIO_IDS = [5, 6] + list(range(20, 32))     # Data 2, Data 3, then everything above the reserved range
RES_ID = 4                                    # NIGHT.RES, Data 1
BANK_ID = 7                                   # bank.ofsf, the optional SoundFont bank
LOG_ID = 17                                   # nightlog.sav, the diagnostic trail
# (SAVE_BASE is gone: the port binds no game save slots -- docs/449)

CARD_DIR = "Assets/night/common"              # where these files go on the card, once the core is installed


def slots_for_tracks(audio_tracks):
    """{apf_id: filename} for the CUE's audio tracks, in disc order."""
    if len(audio_tracks) > len(AUDIO_IDS):
        raise SystemExit(f"{len(audio_tracks)} audio tracks but only {len(AUDIO_IDS)} non-save slot ids")
    return {AUDIO_IDS[i]: f"track{t:02d}.bin" for i, t in enumerate(audio_tracks)}


def id_for_track(audio_tracks, track):
    for sid, name in slots_for_tracks(audio_tracks).items():
        if name == f"track{track:02d}.bin":
            return sid
    return None
