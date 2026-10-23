# 54 — stage-transition corruption: what the headless run actually shows

The report says the screen is corrupted during stage-1 startup and self-heals after a while. Measuring it
instead of eyeballing it, with a structural proxy — mean absolute difference between neighbouring pixels
("edge-noise"; the sheared intro frames scored 58.6, clean 320x200 gameplay scores ~29-30):

| frame | earliest run (before the pitch / view / retrace fixes) | current run |
|---|---|---|
| `episode_start` | 29.39 (320x200) | 10.23 (fading in) |
| `stage_play` | 29.59 (320x200) | 30.23 (320x200) |
| `playing` | 29.03 | 29.80 |

So the scripted headless route never produced a garbage stage-startup frame — before *or* after the fixes —
and the transition frames carry the same structure as steady gameplay. Two conclusions:

1. Whatever was seen by hand is not reproduced by this script. The candidates left are the *in-flight*
   frames (the fade/composite window between the 640x480 stage card and the first 320x200 room frame,
   which is exactly where the per-step view geometry and the frame pitch matter) and anything gated on real
   timing (the retrace-paced waits and the audio device), neither of which the script samples.
2. The harness gap is real: it auto-shots on a *large* screen change and on resource lookups, so the
   transition is covered by one or two shots (`stage1.pcx_hold` -> `stage_play`) rather than by every
   presented frame.

Next step for this task: add a dense-capture window (dump every presented frame from the stage-card hold
shot until the first `stage_play` shot), then flag any frame whose edge-noise or row-continuity deviates
from its neighbours — that turns "self-heals after a while" into a per-frame series that can be compared
against the original's frames once the DOSBox capture (docs/46) is run in a terminal.
