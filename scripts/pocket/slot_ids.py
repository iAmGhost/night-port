"""The one place that decides which APF file id carries which file.

README:644 reserves ids 10-19 for saves ("Save 0-9 -- nonvolatile CRAM0 save slots (256 KB each)", bridge
0x20100000 with a 256 KB stride) and id 8 for the SDK.  A data file bound into the save range is served from
that 256 KiB window and every read past byte 262144 comes back short -- docs/339 has the card log that proves
it.  So audio may only sit on the ids the table leaves free for app data: 4-6 (Data 1-3, NIGHT.RES takes 4)
and anything from 20 up.

gen_disc.py writes this into disc_table.h (the host's runtime map), mk_dist.py writes it into data.json and
the instance manifest, and check_slots.py asserts the two agree.  There is no arithmetic left in C to get
wrong: 0.1.22 shipped `19 + track` against a manifest built as `20 + n`, which played track13.bin for the
title theme and asked for a nonexistent slot 34 for track15.
"""

SAVE_IDS = list(range(10, 20))
AUDIO_IDS = [5, 6] + list(range(20, 32))     # Data 2, Data 3, then everything above the reserved range
RES_ID = 4                                    # NIGHT.RES, Data 1
BANK_ID = 7                                   # bank.ofsf, the optional SoundFont bank
LOG_ID = 17                                   # nightlog.sav, the diagnostic trail
SAVE_BASE = 10                                # night_0.sav .. night_6.sav on Save 0-6


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
