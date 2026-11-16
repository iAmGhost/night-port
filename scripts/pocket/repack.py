#!/usr/bin/env python3
"""Fill a Pocket core's data slots from whatever game data someone actually owns.

WHAT IT WRITES (scratch/pocket/, i.e. regenerable; the repo only carries this script)

  NIGHT.RES              the resource container, byte-for-byte (slot 4)
  cd/trackNN.bin         one CD-DA track per file: raw 2352-byte frames, no header (slots 5, 6, 20..31)
  manifest.tsv           what was written, from what byte range, with the slot id (audit_disc.py reads it)
  slot_map.tsv           the same rows, in the shape a person copying files to a card wants
  disc.json              the table of contents the core compiles in, and the geometry behind it

WHY PER-TRACK FILES: the SDL host reads the whole disc through one TG_NIGHT.IMG (src/cd/cd_iso.c,
fseek+fread).  On the Pocket a data slot is a file, and the game asks for a *track* (fn_010b1d resolves
a TOC index to an LBA and a sector count), so the disc is cut along its own track boundaries: one slot
per audio track, which is the mapping scripts/pocket/mk_dist.py writes into data.json and
pocket/src/night/of_disc.c resolves at run time.  Both read the ids from scripts/night-extractor's
slots.py, so there is one table and not two.

WHY THE INPUT IS NOT A PATH ANYMORE: everything below the "find TG_NIGHT.CUE, stat TG_NIGHT.IMG" step
used to assume the data arrives as that pair.  It does not always: it arrives as a zip from an archive
site, or as files someone unzipped, or as MUSIC.RES + SOUND.RES from a fan repack, or as NIGHT.RES plus
nothing, or as one big image with no cue at all.  Those are shapes of the *same* data, and the place
that knows how to tell them apart -- and where each track's audio really sits inside each of them -- is
`night_extractor.source`, the module the browser port's byod.js mirrors (docs/465).  This script now
asks it, so the card gets the same bytes the browser and `make extract` get.

Run:  python3 scripts/pocket/repack.py --data TG_NIGHT.CUE TG_NIGHT.IMG
      python3 scripts/pocket/repack.py --data ~/Downloads/dosbox.zip
      python3 scripts/pocket/repack.py                 (the disc this repo has; `make -C pocket/src/night data`)
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts" / "night-extractor"))   # the input layer (docs/465)
sys.path.insert(0, str(REPO / "scripts"))                        # disc_loc.py (docs/455)

from night_extractor import slots                                  # one slot table for every port
from night_extractor.disc import SECTOR
from night_extractor.extract import write_stream
from night_extractor.source import DISC_SECTORS, RES_CRC, default_paths, disc_cue, open_source, res_stream

OUT_DEFAULT = REPO / "scratch" / "pocket"

MAN_COLS = ("slot", "file", "bytes", "track", "sectors", "img_first_sector", "disc_lba", "sha1_1MiB")


def sha1_head(path: Path, n: int = 1 << 20) -> str:
    """A cheap fingerprint of the first MiB: enough to notice a cut that went wrong at its start."""
    h = hashlib.sha1()
    with path.open("rb") as f:
        read = 0
        while read < n:
            b = f.read(min(1 << 20, n - read))
            if not b:
                break
            h.update(b)
            read += len(b)
    return h.hexdigest()[:12]


def write_res(out: Path, src, force: bool) -> dict:
    """The one container the core boots from, from whichever of the two places the data keeps it.

    A fan repack hands over `NIGHT.RES` itself; the original disc's only copy is inside the installer's
    payload, the ISO9660 file `NIGHT/NIGHT.DAT` (docs/455) -- and this is the step the DOS installer used
    to be responsible for, which is why the card can be filled without a Windows box or an emulator.
    """
    from night_extractor.source import res_stream
    dst = out / "NIGHT.RES"
    crc = None
    if src.res is not None:
        m = src.res
        with m.stream() as stream:
            got, sha, wrote = write_stream(dst, stream, 0, m.size, force, None)
        note = f"{m.base} as handed over ({m.label()})"
        crc = m.crc
    else:
        found = src.res_in_image
        if found is None:
            sys.exit("[game] no NIGHT.RES to write -- neither handed over nor inside the image")
        e = next((x for x in found["index"] if x["name"].upper() == "NIGHT.RES"), None)
        if e is None:
            sys.exit(f"[game] {found['name']} holds {found['count']} entries and none of them is NIGHT.RES")
        print(f"[game] {found['name']} at volume LBA {found['lba']}: entry {e['name']!r} at +{e['off']}, "
              f"{e['size']:,} B -- the byte range SETUP.EXE installed")
        stream, closer = res_stream(src.img, found, src.cache_dir)
        try:
            got, sha, wrote = write_stream(dst, stream, e["off"], e["size"], force, None)
        finally:
            if closer:
                closer()
        note = f"{found['name']} entry at +{e['off']}, installed by SETUP.EXE"
    row = {"slot": slots.RES_ID, "file": "NIGHT.RES", "bytes": got, "sha1_1MiB": sha1_head(dst),
           "note": note, "sha256": sha}
    print(f"       {'wrote' if wrote else 'kept '} NIGHT.RES {got:>12,} B  slot {slots.RES_ID}  "
          f"sha256 {sha[:12]}")
    if crc is not None and crc != RES_CRC:
        print(f"       note: CRC-32 {crc:08x}, not the {RES_CRC:08x} this port was verified against")
    return row


def write_audio(out: Path, src, force: bool):
    """Cut each CD-DA track to its own file: raw 2352-byte frames, no header, no decoding.

    The slice positions come from `source.plan_tracks`, the one place that knows the difference between a
    disc image (the cue's numbers are the disc's) and a fan rip (they are almost the disc's, and two
    tracks are not the disc's at all).  So the same 14 files come out of TG_NIGHT.IMG and out of a
    repack's MUSIC.RES -- with the honest caveat the extractor prints: a fan rip's audio matches the
    shipped disc byte for byte on some tracks and drifts inside others (docs/466), so a fan card plays
    this disc's music and not this disc's bytes.
    """
    plan = src.plan
    if not plan:
        sys.exit("[audio] the data handed over carries no CD-DA to cut -- the core would run silent")
    tracks = [t["track"] for t in plan]
    slot_of = {t: slots.id_for_track(tracks, t) for t in tracks}
    cd_dir = out / "cd"
    cd_dir.mkdir(parents=True, exist_ok=True)
    rows = []
    print(f"[audio] {len(plan)} CD-DA tracks -> cd/trackNN.bin (raw {SECTOR}-byte frames, no header)")
    cut = [t for t in plan if t.get("clipped")]
    if cut:
        print(f"       note: {', '.join('track%02d' % t['track'] for t in cut)} cut short at the next "
              f"track's start -- the layout of this image, not the disc's lengths (docs/466)")
    fh, closer = src.img.reader(src.cache_dir)
    try:
        for t in plan:
            name = f"track{t['track']:02d}.bin"
            got, sha, wrote = write_stream(cd_dir / name, fh, t["off"], t["len"], force, None)
            rows.append({"slot": slot_of[t["track"]], "file": f"cd/{name}", "bytes": got,
                         "track": t["track"], "sectors": t["sectors"],
                         "img_first_sector": t["off"] // SECTOR, "disc_lba": t["lba"],
                         "sha1_1MiB": sha1_head(cd_dir / name), "sha256": sha})
            print(f"       {'wrote' if wrote else 'kept '} {name} {got:>12,} B  {t['sectors']:>7} sect  "
                  f"{t['sectors'] / 75:>6.1f} s  slot {slot_of[t['track']]}  "
                  f"img[{t['off'] // SECTOR}..{t['off'] // SECTOR + t['sectors'] - 1}]  sha256 {sha[:12]}")
    finally:
        if closer:
            closer()
    return rows


def write_disc_json(out: Path, src) -> Path:
    """Hand the compiled-in disc table the cue it should use, and the geometry that goes with it.

    The game asks in the *disc's* LBAs whatever the file on the card turns out to be, so for a fan rip or
    a bare image the table comes from this port's own table of contents and the lead-out from the disc's
    own length (`DISC_SECTORS`) -- not from the byte length of a rip that begins with 174 header sectors
    and ends wherever the rip ended.  `of_disc.c` answers the disc gate's 7-byte header and the TOC
    records from these numbers, so the game sees the disc it expects and the bytes come from the slots.
    """
    own = src.fan or src.own_toc
    # The same decision `extract.part_cue` makes, and for the same reason: a fan rip's cue numbers describe
    # the rip (its track 02 sits at sector 174), and the game asks in the disc's LBAs (37262).  Handing that
    # cue to the table generator would bake the rip's coordinates into the binary and play every track from
    # the wrong place -- which is why the cue the *game* gets is this port's own table of contents instead.
    cue = src.cue_text if src.cue_out is not None else disc_cue()
    img_sectors = DISC_SECTORS if own else src.img.size // SECTOR
    d = {"cue": cue,
         "cue_from": ("this port's own table of contents" if own else
                      f"{src.cue.base if src.cue else 'the cue that came with the data'}"),
         "img": src.img.base,
         "img_path": str(getattr(src.img, "path", None) or (src.img.cont.file if src.img.cont else "")),
         "img_sectors": img_sectors,
         "img_sectors_from": ("this disc's own length: the image on the card is not the disc's length"
                              if own else f"{src.img.base} size / {SECTOR}"),
         "fan": bool(src.fan), "own_toc": bool(src.own_toc)}
    p = out / "disc.json"
    p.write_text(json.dumps(d, indent=1, sort_keys=True) + "\n")
    print(f"[disc] wrote {p.name}: {d['cue_from']}, {img_sectors:,} sectors ({d['img_sectors_from']})")
    return p


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--data", nargs="+", metavar="PATH",
                    help="a zip, or CUE + image, or NIGHT.RES alone, or a fan repack's files -- "
                         "any shape; default: the disc this repo has")
    ap.add_argument("--cue", help="the CUE sheet to use, when the data holds more than one")
    ap.add_argument("--img", help="the image to use, when the data holds more than one")
    ap.add_argument("-o", "--out", default=str(OUT_DEFAULT), help="output directory "
                                                                  f"(default: {OUT_DEFAULT})")
    ap.add_argument("--cache", help="where a compressed image is unpacked once for random access")
    ap.add_argument("--force", action="store_true", help="rewrite files that already look right")
    ap.add_argument("--no-gen", action="store_true",
                    help="skip gen_disc.py (the header is regenerated by `make disc` anyway)")
    args = ap.parse_args()

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    src = open_source(args.data or default_paths(), cue=args.cue, img=args.img,
                      cache_dir=args.cache).describe()

    rows = [write_res(out, src, args.force)]
    rows += write_audio(out, src, args.force)
    disc_json = write_disc_json(out, src)

    # The card's flat file list, and the byte-range map audit_disc.py maps slot offsets back onto.
    (out / "slot_map.tsv").write_text(
        f"# generated by scripts/pocket/repack.py.  Copy these into {slots.CARD_DIR}/ on the card, flat: "
        "the name in column 2 is the name the core asks for, with no cd/ prefix.\n"
        + "slot\tfile\tbytes\ttrack\tsha256\n"
        + "".join("\t".join(str(r.get(c, "")) for c in ("slot", "file", "bytes", "track", "sha256")) + "\n"
                  for r in rows))
    (out / "manifest.tsv").write_text(
        "# generated by scripts/pocket/repack.py -- do not edit.  img_first_sector maps a slot-relative\n"
        "# offset back onto the image the data came out of (scripts/pocket/audit_disc.py).  slot is the APF\n"
        "# data-slot id from scripts/night-extractor/night_extractor/slots.py, the same table mk_dist.py\n"
        "# writes into data.json.\n"
        + "\t".join(MAN_COLS) + "\n"
        + "".join("\t".join(str(r.get(c, "")) for c in MAN_COLS) + "\n" for r in rows))

    total = sum(r["bytes"] for r in rows)
    print(f"total: {total:,} B in {len(rows)} files -> {out}")
    print(f"card : copy NIGHT.RES and cd/*.bin into {slots.CARD_DIR}/ (see slot_map.tsv)")

    if not args.no_gen:
        subprocess.run([sys.executable, str(REPO / "scripts" / "pocket" / "gen_disc.py"),
                        "--json", str(disc_json)], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
