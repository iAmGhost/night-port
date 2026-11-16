#!/usr/bin/env python3
"""Encode the disc's CD-DA into the compressed tracks the browser port streams instead of raw sectors.
docs/463.

    make -C wasm audio            # 128 kbps, the default
    make -C wasm audio KBPS=96    # thinner
    make -C wasm audio TRACKS="02 03"

Why this exists: the browser port reads CD-DA from HTTP Range windows, so the whole disc costs whatever
the game actually plays -- but the whole disc is 604 MB of 44.1 kHz stereo PCM and one song is 8-46 MB of
it. Encoded at 128 kbps constant-bitrate MP3 it is 16 KB/s instead of 176 KB/s, which is 11x less and,
more to the point for a phone on a mobile connection, 3.8 MB for a four-minute track instead of 42 MB.

Why MP3 and not something denser: constant bitrate is a clock. Sample S sits at byte
first_byte + S * bytes_per_sec / rate, so a Range window can start decoding anywhere and the game's LBA
arithmetic survives unchanged. Formats that would need less bandwidth (Opus, AAC) carry no such map, and
the only decoder they have in a browser wants the whole file in one piece.

What this writes, and why the port needs it: <out>/trackNN.mp3, and <out>/tracks.tsv. The encoded length
of a track says nothing about how much music is in it, and cd_iso needs the Red Book length of every
track -- it bounds a track's reads with it, and the last track on the disc has no following INDEX to
bound it with. So the index states it, per track, along with what the encoder reported and what a decoder
gets back out. Run past the files, it is a manifest; read by wasm/src/wasm_mp3.c at boot, it is the only
claim about track geometry the lossy path has. The port treats a tree with no tracks.tsv as a tree with no
lossy audio and streams raw sectors, exactly as it did before this existed.

The delay column is measured, not assumed: an MP3 encoder pads the head of a track with its encoder delay
and only tells gapless decoders, in a tag. The probe cross-correlates the decoded head against the raw
sectors it was encoded from and reports the offset that lines them up.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

SECTOR = 2352
SAMPLES_PER_SECTOR = 588


def audio_tracks(cue_path: str) -> list[int]:
    """The AUDIO track numbers, in disc order, from the CUE."""
    out = []
    track = None
    with open(cue_path, encoding="utf-8") as f:
        for line in f:
            m = re.match(r"\s*TRACK\s+(\d+)\s+(\S+)", line)
            if m:
                track = int(m.group(1))
                if m.group(2) == "AUDIO":
                    out.append(track)
                else:
                    track = None
    return out


def size(path: str) -> int:
    return os.path.getsize(path)


def run(cmd: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True)


def encode(src: str, dst: str, kbps: int, rate: int, channels: int, pad: int) -> None:
    """Encode one track, with `pad` samples of silence after it if `pad` is not 0.

    The pad is off by default, and the reason it is an option at all is worth keeping. An MP3 encoder puts
    its filterbank's warm-up -- the delay -- in front of the music, and a port that throws that padding away
    to put sample 0 where the CD had it takes the same number of samples off the back of the timeline. So
    "does the file reach the end of its track" is a real question, and the first pass at this script
    answered it with arithmetic on numbers the probe had capped at the track's own length -- which made 12
    of 14 tracks look short, including on files that a straight decode showed reaching five frames past the
    last sector. The question is now asked by walking the stream past the end and stopping at the first
    frame it cannot decode (nk_mp3_probe.c phase 4, and checked here). If a future encoder does run short,
    this is the lever that fixes it; `--pad 2304` and re-encode.
    """
    cmd = [
        "ffmpeg", "-hide_banner", "-v", "error", "-y",
        "-f", "s16le", "-ar", str(rate), "-ac", str(channels), "-i", src,
    ]
    filt = None
    if pad > 0:
        cmd += ["-f", "lavfi", "-i", f"anullsrc=r={rate}:cl={'stereo' if channels == 2 else 'mono'}"]
        filt = (f"[1]atrim=duration={pad / rate:.8f},asetpts=PTS-STARTPTS[p];"
                f"[0][p]concat=n=2:v=0:a=1[a]")
    cmd += ["-c:a", "libmp3lame", "-b:a", f"{kbps}k", "-ac", str(channels), "-ar", str(rate),
            "-id3v2_version", "0", "-write_id3v1", "0", "-f", "mp3"]
    if filt:
        cmd += ["-filter_complex", filt, "-map", "[a]"]
    else:
        cmd += ["-map", "0:a"]
    cmd.append(dst)
    r = run(cmd)
    if r.returncode != 0:
        sys.exit(f"ffmpeg failed for {src}: {r.stderr.strip()}")


def probe(probe_bin: str, mp3: str, raw: str) -> dict[str, str]:
    """What nk_mp3_probe saw: the stream's own geometry, and how far off the clock is."""
    r = run([probe_bin, mp3, "--raw", raw])
    if r.returncode != 0:
        sys.exit(f"probe failed for {mp3}: {r.stderr.strip() or r.stdout.strip()}")
    text = r.stdout
    fields = {}
    m = re.search(r"rate=(\d+) ch=(\d+) kbps=(\d+) first_byte=(\d+) frames=(\d+) samples=(\d+)", text)
    if not m:
        sys.exit(f"probe said nothing usable about {mp3}:\n{text}")
    fields.update(rate=m.group(1), channels=m.group(2), kbps=m.group(3), first=m.group(4),
                  frames=m.group(5), samples=m.group(6))
    m = re.search(r"map_err_max=(\d+)", text)
    fields["map_err_max"] = m.group(1) if m else "?"
    m = re.search(r"resyncs=(\d+) fifo_drop=(\d+)", text)
    fields["resyncs"], fields["fifo_drop"] = (m.group(1), m.group(2)) if m else ("?", "?")
    m = re.search(r"head delay=(\d+) samples \(([0-9.]+) ms\), mean \|difference\| ([0-9.]+)", text)
    if not m:
        sys.exit(f"probe gave no head delay for {mp3}:\n{text}")
    fields["lag"], fields["lag_ms"], fields["lag_sad"] = m.group(1), m.group(2), m.group(3)
    # Every seek the probe ran mid-track: the residual lag it had to slide by to line the decoded samples
    # up with the sectors, and how far apart the two were once slid. A clock that is good shows lag 0 and a
    # mean difference at codec-loss level; a clock that is one frame off anywhere shows loud noise, because
    # a whole 26 ms of the wrong music is being compared. The last seek of a track usually lands on its
    # silent tail and reads 0/0, which is why these are reported rather than asserted one by one.
    fields["seeks"] = re.findall(r"seek to raw sample \d+: pos=\d+ \(asked \d+\), "
                                 r"residual lag=(-?\d+) \(mad ([0-9.]+)\)", text)
    if not fields["seeks"]:
        sys.exit(f"probe ran no seek test for {mp3}:\n{text}")
    m = re.search(r"tail: the file answers for (\d+) samples past the end", text)
    if not m:
        sys.exit(f"probe ran no tail test for {mp3}:\n{text}")
    fields["tail"] = int(m.group(1))
    return fields


def main() -> None:
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.dirname(os.path.dirname(here))
    ap = argparse.ArgumentParser()
    ap.add_argument("--cue", default=os.path.join(repo, "extracted", "TG_NIGHT.CUE"))
    ap.add_argument("--src", default=os.path.join(repo, "extracted", "cd"))
    ap.add_argument("--out", default=os.path.join(repo, "wasm", "dist", "data", "audio"))
    ap.add_argument("--kbps", type=int, default=128)
    ap.add_argument("--rate", type=int, default=44100)
    ap.add_argument("--channels", type=int, default=2)
    ap.add_argument("--probe", default=os.path.join(repo, "build", "nk_mp3_probe"))
    ap.add_argument("--pad", type=int, default=0,
                    help="samples of silence encoded after each track (0 = none; see encode())")
    ap.add_argument("--min-tail", type=int, default=1152,
                    help="how far past the end of a track its MP3 has to reach, in samples")
    ap.add_argument("--tracks", default="", help='subset, e.g. "02 03"')
    ap.add_argument("--no-probe", action="store_true")
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()

    tracks = audio_tracks(a.cue)
    if a.tracks:
        want = {int(t, 16) if len(t) == 2 and not t.isdigit() else int(t) for t in a.tracks.split()}
        tracks = [t for t in tracks if t in want]
    os.makedirs(a.out, exist_ok=True)

    if not a.no_probe and not os.path.isfile(a.probe):
        sys.exit(f"no probe at {a.probe}: build it with `make -C wasm probe`")

    rows: list[tuple[int, dict[str, object]]] = []
    total_raw = total_enc = 0
    for t in tracks:
        raw = os.path.join(a.src, f"track{t:02d}.bin")
        dst = os.path.join(a.out, f"track{t:02d}.mp3")
        if not os.path.isfile(raw):
            print(f"track {t:02d}: no {raw} -- skipped", file=sys.stderr)
            continue
        sectors = size(raw) // SECTOR
        if size(raw) % SECTOR:
            sys.exit(f"{raw} is not a whole number of 2352-byte sectors")
        if os.path.isfile(dst) and not a.force:
            print(f"track {t:02d}: {dst} exists (--force to re-encode)")
        else:
            encode(raw, dst, a.kbps, a.rate, a.channels, a.pad)
        enc = size(dst)
        total_raw += size(raw)
        total_enc += enc
        f = (probe(a.probe, dst, raw) if not a.no_probe else
             {"rate": str(a.rate), "channels": str(a.channels), "kbps": str(a.kbps),
              "samples": "0", "lag": "0", "lag_sad": "-", "map_err_max": "-", "seeks": "-"})
        # What the port must be told, in the units it thinks in.
        rows.append((t, {
            "mp3_bytes": enc,
            "sectors": sectors,
            "kbps": int(f["kbps"]),
            "rate": int(f["rate"]),
            "channels": int(f["channels"]),
            "decoded_samples": int(f["samples"]),
            "delay": int(f["lag"]),
        }))
        seek_lags = [int(x[0]) for x in f["seeks"]]
        seek_sad = [float(x[1]) for x in f["seeks"]]
        # Does the file reach the end of the track it was made from, and does it have room past it? The
        # port drops the encoder's head padding off the front, which costs the same number of samples off
        # the back, and a track whose MP3 stops early plays its last sectors as silence. Measured, not
        # assumed: the probe walks the stream past the raw track's last sample and stops at the first
        # frame it cannot decode.
        if f["tail"] < a.min_tail and not a.no_probe:
            print(f"track {t:02d}: THE MP3 STOPS {f['tail']} SAMPLES PAST THE END OF THE TRACK and "
                  f"{a.min_tail} were required -- raise --pad (now {a.pad}) and re-encode", file=sys.stderr)
        # 0.02 of full scale is comfortably above what 128 kbps costs a track, so anything louder there is
        # the wrong music being compared. A seek that lands in a track's silent tail scores mad 0 at every
        # lag, so its reported lag means nothing and is not held against the file.
        loud = [(l, m) for l, m in zip(seek_lags, seek_sad) if m > 0.02]
        if loud:
            print(f"track {t:02d}: SEEK TESTS FAILED ({[f'lag {l} mad {m:.5f}' for l, m in loud]}) -- "
                  f"the bytes in {dst} do not answer to the clock the index will state",
                  file=sys.stderr)
        print(f"track {t:02d}: {size(raw) / 1e6:7.1f} MB raw -> {enc / 1e6:6.2f} MB mp3 "
              f"({size(raw) / enc:4.1f}x)  {f['kbps']}k {f['rate']}Hz {f['channels']}ch "
              f"decoded={f['samples']} delay={f['lag']} ({f['lag_ms']} ms, {f['lag_sad']} fs)  "
              f"clock_err={f['map_err_max']}B resyncs={f['resyncs']} fifo_drop={f['fifo_drop']} "
              f"tail=+{f['tail']} samples past the track  "
              f"seek_lag<={max(abs(x) for x in seek_lags)} seek_mad<={max(seek_sad):.5f} fs")

    if not rows:
        sys.exit("nothing encoded")

    index = os.path.join(a.out, "tracks.tsv")
    with open(index, "w", encoding="utf-8") as f:
        f.write("# nk-audio 1 -- what wasm/src/wasm_mp3.c reads to answer Red Book reads out of MP3.\n")
        f.write("# track  mp3_bytes  redbook_sectors  bitrate_kbps  sample_rate  channels  "
                "decoded_samples  head_delay_samples\n")
        f.write(f"# encoded from {a.src} at {a.kbps} kbps constant bitrate, "
                f"{a.rate} Hz, {a.channels} channel(s) by wasm/scripts/encode_audio.py\n")
        f.write("# decoded_samples is what a decoder gets out of the file through the track's last Red Book "
                "sector;\n# the head padding sits in front of them, and every file here was checked to "
                "reach at least one\n# MP3 frame past the track's last sample (analysis/probes/nk_mp3_probe.c).\n")
        for t, r in rows:
            f.write(f"{t} {r['mp3_bytes']} {r['sectors']} {r['kbps']} {r['rate']} {r['channels']} "
                    f"{r['decoded_samples']} {r['delay']}\n")
    print(f"index: {index} ({len(rows)} tracks)  "
          f"{total_raw / 1e6:.0f} MB raw -> {total_enc / 1e6:.0f} MB mp3 "
          f"({total_raw / total_enc:.1f}x)")


if __name__ == "__main__":
    main()
