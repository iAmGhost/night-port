"""The APF slot ids, re-exported from `night_extractor.slots` -- one table, both callers.

The table itself is in the tool (docs/465): `night-extractor` has to be able to say which slot a file is
bound to for someone who will never compile the core, and a second copy of the table would have been the
price of that.  This file stays so `gen_disc.py`, `mk_dist.py` and `check_slots.py` keep importing the
names they always did.

The import works three ways, because this script runs under all three: an installed/editable package, the
repo's own venv (`uv run`), and bare `python3` with no install at all -- the last one is why scripts/
extract_disc.py exists too.
"""
from __future__ import annotations

import sys
from pathlib import Path

try:
    from night_extractor.slots import (AUDIO_IDS, BANK_ID, CARD_DIR, LOG_ID, RES_ID, SAVE_IDS,
                                       id_for_track, slots_for_tracks)
except ImportError:                                     # no install: use the tree this script sits in
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "night-extractor"))
    from night_extractor.slots import (AUDIO_IDS, BANK_ID, CARD_DIR, LOG_ID, RES_ID, SAVE_IDS,
                                       id_for_track, slots_for_tracks)

__all__ = ["SAVE_IDS", "AUDIO_IDS", "RES_ID", "BANK_ID", "LOG_ID", "CARD_DIR",
           "slots_for_tracks", "id_for_track"]
