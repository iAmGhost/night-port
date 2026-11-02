#!/usr/bin/env python3
"""Request-to-audible delay of the port's CD-DA stream, measured from the dump (docs/307).

Instrument: NIGHT_CDDADBG=1 with --rig-dump writes `cdda_pipe.jsonl`, one line per event
  device      freq/channels of the opened device
  play        cdda_play() ran (the game's track request reached the board); avail = SDL_GetAudioStreamAvailable()
  flush       the branch of cdda_play that flushes a live stream; avail = old-track bytes still queued
  first_feed  the first pump batch after a play; pos = sectors handed to SDL so far
  stop        cdda_stop()
  pipe        throttled (40 ms) sample: avail / pushed / pulled / pos / budget

Definitions used here, all in device-format bytes (avail is SDL_GetAudioStreamAvailable, i.e. bytes in the
stream that the device has NOT taken yet):
  queued ahead of a switch = avail at the play line (or at its flush line, whichever is larger)
  switch delay             = that / (freq * channels * 2)   seconds of audio the device must still eat
  feed rate                = pushed bytes / 2352 / elapsed wall seconds, against 75 sectors/s = real time
  frame coupling           = t(first_feed) - t(play)
  underrun samples         = pipe lines with a live track, pos < sectors, and avail == 0
"""
import json
import os
import sys

SECTOR = 2352


def load(d):
    p = os.path.join(d, "cdda_pipe.jsonl") if os.path.exists(os.path.join(d, "cdda_pipe.jsonl")) \
        else os.path.join(d, "port", "cdda_pipe.jsonl")
    if not os.path.exists(p):
        sys.exit(f"{d}: no cdda_pipe.jsonl (run with NIGHT_CDDADBG=1)")
    return [json.loads(l) for l in open(p) if l.strip()]


def analyse(d):
    rows = load(d)
    dev = next(r for r in rows if r["ev"] == "device")
    bps = max(1, dev["freq"] * dev["channels"] * 2)
    plays = [i for i, r in enumerate(rows) if r["ev"] == "play"]
    out = {"dir": d, "dev": dev, "bps": bps, "switches": [], "max_avail": 0, "underrun": 0, "live_pipe": 0}

    for i in plays:
        r = rows[i]
        prev_flush = rows[i - 1] if i and rows[i - 1]["ev"] == "flush" else None
        queued = max(r["avail"], prev_flush["avail"] if prev_flush else 0)
        ff = next((x for x in rows[i + 1:i + 40] if x["ev"] == "first_feed"), None)
        out["switches"].append(dict(
            t_ms=r["t_ms"], index=r["index"], had_flush=prev_flush is not None, queued=queued,
            delay_s=queued / bps, coupling_ms=(ff["t_ms"] - r["t_ms"]) if ff else None,
            first_batch=ff["pos"] if ff else 0))

    pipes = [r for r in rows if r["ev"] == "pipe"]
    if pipes:
        out["max_avail"] = max(r["avail"] for r in pipes)
        span = (pipes[-1]["t_ms"] - pipes[0]["t_ms"]) / 1000.0
        out["pushed_sectors"] = pipes[-1]["pushed"] / SECTOR
        out["feed_rate_sps"] = out["pushed_sectors"] / span if span > 0 else 0
        out["pulled_sectors"] = pipes[-1]["pulled"] / SECTOR
        out["drain_rate_sps"] = out["pulled_sectors"] / span if span > 0 else 0
        out["pipe_lines"] = len(pipes)
        out["underrun"] = sum(1 for r in pipes if r["pos"] < r.get("sectors", 1 << 30) and r["avail"] == 0
                              and r["pushed"] > 0)
    return out


def main():
    runs = [analyse(d) for d in sys.argv[1:]]
    for a in runs:
        print(f"== {a['dir']}  device freq={a['dev']['freq']} ch={a['dev']['channels']} "
              f"({a['bps']} B/s)")
        for s in a["switches"]:
            print(f"   t={s['t_ms']:>7} ms  index={s['index']:<3} flush={'yes' if s['had_flush'] else 'NO '} "
                  f"queued={s['queued']:>9} B  delay={s['delay_s']:>5.2f} s  "
                  f"coupling={s['coupling_ms']} ms  first_batch={s['first_batch']} sectors")
        if a["pipe_lines"]:
            print(f"   avail peak {a['max_avail']} B = {a['max_avail']/a['bps']:.2f} s of audio")
            print(f"   pushed {a['pushed_sectors']:.0f} sectors at {a['feed_rate_sps']:.2f} sectors/s "
                  f"({a['feed_rate_sps']/75:.2f}x real time)")
            print(f"   device drained {a['pulled_sectors']:.0f} sectors at {a['drain_rate_sps']:.2f} "
                  f"sectors/s ({a['drain_rate_sps']/75:.2f}x)")
            print(f"   underrun samples (live track, avail==0): {a['underrun']} of {a['pipe_lines']} pipe lines")
    if len(runs) > 1:
        print("== summary")
        print(f"{'run':30} {'switches':>8} {'max delay s':>11} {'peak avail s':>12} {'feed x':>7} {'drain x':>8} {'underrun':>9}")
        for a in runs:
            mx = max((s["delay_s"] for s in a["switches"]), default=0.0)
            print(f"{a['dir']:30} {len(a['switches']):>8} {mx:>11.2f} {a['max_avail']/a['bps']:>12.2f} "
                  f"{a['feed_rate_sps']/75:>7.2f} {a['drain_rate_sps']/75:>8.2f} {a['underrun']:>9}")


if __name__ == "__main__":
    main()
