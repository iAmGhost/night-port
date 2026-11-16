"""Building the run tree: streaming copies that hash as they go, the three parts, the manifests, verify.

Every file is written to `.part` and renamed, and every one carries its sha256 into MANIFEST.tsv, so
"the tree I have is the tree that was verified" is a question `--verify` answers instead of a hope.
"""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

from . import slots
from .disc import SECTOR, DataTrack, IsoStream, decode_container_dir
from .source import RES_CRC, disc_cue, res_stream
from .wav import seconds, write_wav

CHUNK = 1 << 20

def sha256_file(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for block in iter(lambda: f.read(CHUNK), b""):
            h.update(block)
    return h.hexdigest()


def load_manifest(out: Path):
    """The previous run's MANIFEST.tsv as rows -- so a partial run can keep the parts it did not touch.

    `--only audio` used to rewrite MANIFEST.tsv from its own rows alone, which dropped the game rows:
    the next `--verify` then verified 14 files of 21, and load_prev() lost the hashes that make a re-run
    cost one stat.  A manifest describes the tree, not the last invocation.
    """
    man = out / "MANIFEST.tsv"
    if not man.exists():
        return []
    rows = [l.split("\t") for l in man.read_text().splitlines() if l.strip() and not l.startswith("#")]
    if len(rows) < 2:
        return []
    hdr = rows[0]
    out_rows = []
    for r in rows[1:]:
        d = dict(zip(hdr, r))
        if "kind" in d and "file" in d:
            out_rows.append(d)
    return out_rows


def load_prev(out: Path):
    """{file: (bytes, sha256)} from the previous run's manifest, so a kept file costs one stat."""
    return {r["file"]: (int(r["bytes"]), r["sha256"]) for r in load_manifest(out)
            if r.get("bytes", "").isdigit()}


def write_stream(dst: Path, src, off: int, size: int, force: bool, prev=None):
    """Copy `size` bytes from `src` at `off` into dst -> (bytes, sha256, wrote).

    Idempotent by size: an existing file of the right length, whose hash the previous manifest knows,
    is neither read nor rewritten unless --force, so a re-run costs one stat per file.  --verify is
    what re-hashes.  Written to .part then renamed, so an interrupted run cannot leave a half-written
    NIGHT.RES that the next run would trust.
    """
    if dst.exists() and dst.stat().st_size == size and not force:
        if prev and dst.name in prev and prev[dst.name][0] == size:
            return size, prev[dst.name][1], False
        return size, sha256_file(dst), False
    dst.parent.mkdir(parents=True, exist_ok=True)
    tmp = dst.with_suffix(dst.suffix + ".part")
    h = hashlib.sha256()
    if off:
        src.seek(off)                                     # a deflated member has nowhere to seek to
    left = size
    with tmp.open("wb") as g:
        while left:
            block = src.read(min(CHUNK, left))
            if not block:
                raise EOFError(f"short read at image byte {off}: {left} bytes missing")
            g.write(block)
            h.update(block)
            left -= len(block)
    tmp.replace(dst)
    return size, h.hexdigest(), True



def part_game(out: Path, src, args, prev):
    """The container the game loads, from whichever of the two places the visitor's data keeps it.

    Either someone handed over `NIGHT.RES` itself -- a fan repack does, and so does an already-extracted
    tree -- or their data is the original disc, whose only copy of it is inside the installer's payload, the
    ISO9660 file `NIGHT/NIGHT.DAT` (docs/455).  The file wins when both are present: it is the thing the
    person pointed at, and a payload behind it is the same bytes by a longer address.

    The installer's payload holds the whole install set -- `NIGHT.EXE`, `DOS4GW.EXE`, `README.TXT`,
    `NIGHT.ICO`, `NIGHT.BAT` and `NIGHT.RES` -- which is the installer's job, and doing the installer's job
    is the reason this script exists (docs/455).  The port boots from none of them but the container, and
    neither does the Pocket core, but writing all six is what makes "no installer needed" a measured claim
    instead of a partial one.
    """
    rows = []
    if src.res is not None:
        m = src.res
        with m.stream() as stream:
            got, sha, wrote = write_stream(out / "NIGHT.RES", stream, 0, m.size, args.force, prev)
        note = f"{m.base} as handed over ({m.label()})"
        if m.crc is not None and m.crc != RES_CRC:
            note += f" -- CRC-32 {m.crc:08x}, not the {RES_CRC:08x} this port was verified against"
        print(f"       {'wrote' if wrote else 'kept '} NIGHT.RES {got:>12,} B  sha256 {sha[:12]}"
              f"   ({m.label()})")
        rows.append(dict(kind="game", file="NIGHT.RES", bytes=got, sha256=sha, source=m.name,
                         slot=slots.RES_ID, note=note))
        if src.res_in_image is not None:
            print(f"       (the image carries one too, inside {src.res_in_image['name']}: the file you "
                  f"pointed at wins)")
        return rows

    found = src.res_in_image
    if found is None:
        sys.exit("[game] no NIGHT.RES to write -- neither handed over nor inside the image")
    print(f"[game] {found['name']} at volume LBA {found['lba']}, {found['size']:,} bytes: {found['count']} "
          f"entries")
    stream, closer = res_stream(src.img, found, src.cache_dir)
    try:
        for e in found["index"]:
            ship = e["name"].upper()          # the disc's spelling, not the container index's (see below)

            got, sha, wrote = write_stream(out / ship, stream, e["off"], e["size"], args.force, prev)
            rows.append(dict(kind="game", file=ship, bytes=got, sha256=sha,
                             slot=slots.RES_ID if ship.upper() == "NIGHT.RES" else "",
                             source=f"NIGHT.DAT@{found['lba']} +{e['off']}..+{e['off'] + got - 1}",
                             note=f"NIGHT.DAT entry {e['name']!r} at +{e['off']}, {e['size']} B -- "
                                  f"installed by SETUP.EXE"))
            spelled = f"  (the container index spells it {e['name']})" if ship != e["name"] else ""
            print(f"       {'wrote' if wrote else 'kept '} {ship:12s} {got:>12,} B  sha256 {sha[:12]}{spelled}")
    finally:
        if closer:
            closer()
    return rows


def part_audio(out: Path, src, args, prev):
    """Cut each CD-DA track to its own file: raw 2352-byte frames, no header, no decoding.

    The slice positions come from `source.plan_tracks`, which is the one place that knows the difference
    between a disc image (the cue's numbers are the disc's) and a fan rip (they are almost the disc's, and
    two tracks are not the disc's at all) -- so the same 14 files come out of TG_NIGHT.IMG and out of a repack's
    MUSIC.RES, and `verify` can say so with a hash.

    The files go under `cd/` -- the names the port itself boots from (`NIGHT_CD_DIR`) -- and every row
    carries the Pocket data-slot id that file is bound to, from `slots.py`.  Those same files, copied flat
    into the card's `Assets/night/common/`, are the whole of what the Pocket core needs: the same installer
    job, the same cut music, one more place to put it (docs/330).

    `--wav` writes each track a second time under `wav/`, with a RIFF header on the same bytes.  A fan
    repack's music is only as good as the offsets it was sliced at, and a hash says `same` or `different`
    without saying whether anyone would notice -- a WAV says that in the one language a person can check.
    """
    plan = src.plan
    rows = []
    if not plan:
        print("[audio] nothing to cut -- the data handed over carries no audio, so the run is silent")
        return rows
    tracks = [t["track"] for t in plan]
    # keyed by track, because that is what a row has; slots_for_tracks() is keyed by slot id and is what
    # mk_dist.py and deploy.sh read.
    slot_of = {t: slots.id_for_track(tracks, t) for t in tracks}
    print(f"[audio] {len(plan)} CD-DA tracks -> cd/trackNN.bin (raw {SECTOR}-byte frames, no header)")
    fh, closer = src.img.reader(src.cache_dir)
    try:
        for t in plan:
            name = f"track{t['track']:02d}.bin"
            got, sha, wrote = write_stream(out / "cd" / name, fh, t["off"], t["len"], args.force, prev)
            rows.append(dict(kind="audio", file=f"cd/{name}", bytes=got,
                             sha256=sha, track=t["track"], sectors=t["sectors"], disc_lba=t["lba"],
                             img_first_sector=t["off"] // SECTOR,
                             slot=slot_of[t["track"]],
                             source=f"{src.img.base} byte {t['off']}..{t['off'] + t['len'] - 1}",
                             note="raw CD-DA frames; 2352 B = 588 audio frames"))
            line = (f"       {'wrote' if wrote else 'kept '} {name} {got:>12,} B  {t['sectors']:>7} sect  "
                      f"{seconds(t['sectors']):>6.2f} s  slot {slot_of[t['track']]}  sha256 {sha[:12]}")
            if args.wav:
                wb, wsha = write_wav(out / "wav" / f"track{t['track']:02d}.wav", fh, t["off"],
                                     t["sectors"], args.force)
                line += f"  -> wav/track{t['track']:02d}.wav ({wsha[:8]})"
            print(line)
    finally:
        if closer:
            closer()
    cut = [t for t in plan if t.get("clipped")]
    if cut:
        print(f"[audio] {len(cut)} track(s) cut short where the next track's audio begins in this image "
              f"({', '.join('track%02d at %d sect' % (t['track'], t['sectors']) for t in cut)}) -- the layout of "
              f"a fan rip's image, where the disc's own length would run into its neighbour (docs/466)")
    if len(plan) >= 2:                             # the disc gate's own number, printed not assumed
        gap = plan[1]["lba"] - plan[0]["lba"]
        print(f"[audio] track {plan[1]['track']:02d} - track {plan[0]['track']:02d} = {gap} sectors "
              f"(0x{gap:x}) -- the difference fn_010c3e compares")
    return rows


def part_cue(out: Path, src, args):
    """The table of contents worth writing down, which is not always the one that was handed over.

    A disc's own cue is copied verbatim: it is 669 bytes and the port answers MSCDEX from it.  A fan rip's
    cue numbers describe the fan image and not the disc, and a bare image brings no numbers at all, and the
    game asks in the disc's LBAs either way -- so what gets written is this port's own table of contents,
    under the name every downstream reader looks for.  `disc_table.h`, the SDL host and `gen_disc.py` then
    all agree with the bytes on disk without anyone re-typing a sector number.
    """
    if src.cue_out is not None:
        name, text, note = src.cue.base, src.cue_text, "the cue the data came with: the TOC src/cd/cd_iso.c "             "answers MSCDEX from"
    else:
        name, text = "TG_NIGHT.CUE", disc_cue()
        note = ("this port's own table of contents: the data handed over brings none the game can use "
                + ("(a fan rip's cue describes the fan image, not the disc)" if src.fan else
                   "(the image came alone)" if src.own_toc else ""))
    cue_path = getattr(src.cue, "path", None)      # a member inside a zip has no path of its own
    if cue_path is not None and Path(cue_path).resolve() == (out / name).resolve():
        return []                                   # already the file itself
    dst = out / name
    data = text.encode("latin1") if isinstance(text, str) else text
    if dst.exists() and dst.stat().st_size == len(data) and not args.force:
        print(f"[cue ] kept  {name}")
    else:
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(data)
        print(f"[cue ] wrote {name} ({len(data)} B)")
    return [dict(kind="cue", file=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                 source=str(src.cue.base if src.cue_out is not None else "the port's own table of contents"),
                 note=note)]


# The audio rows carry four columns more than the other kinds.  They are part of MAN_COLS rather than
# dropped, because MANIFEST.tsv is also what a partial run reads back to keep the parts it did not
# write -- with them missing, cd/manifest.tsv would lose the track/sector/LBA columns after a
# `--only game`.
MAN_COLS = ("kind", "file", "bytes", "sha256", "source", "note", "track", "sectors", "disc_lba",
            "img_first_sector", "slot")
KIND_ORDER = ("game", "audio", "cue")          # the order a manifest lists its parts in
CD_COLS = ("slot", "file", "bytes", "track", "sectors", "disc_lba", "img_first_sector", "sha256")


def write_manifests(out: Path, rows):
    (out / "MANIFEST.tsv").write_text(
        "# generated by scripts/extract_disc.py -- do not edit; --verify re-hashes against this\n"
        + "\t".join(MAN_COLS) + "\n"
        + "".join("\t".join(str(r.get(c, "")) for c in MAN_COLS) + "\n" for r in rows))
    # What goes in which data slot, for whoever is putting files on a card.  deploy.sh reads the build's
    # own slot_map.tsv, generated alongside data.json; this is the same table, and it exists so a run of
    # `uv run night-extractor` is enough to fill a card, with no core build in the loop (docs/465).
    pocket = [r for r in rows if r.get("slot")]
    if pocket:
        (out / "slot_map.tsv").write_text(
            f"# generated by scripts/extract_disc.py.  Copy these files into {slots.CARD_DIR}/ on the card;"
            " the name in column 2 is the name the core asks for, flat -- no cd/ prefix.\n"
            + "\t".join(("slot", "file", "bytes", "track", "sha256")) + "\n"
            + "".join("\t".join(str(r.get(c, "")) for c in ("slot", "file", "bytes", "track", "sha256"))
                      + "\n" for r in pocket))
    cd = out / "cd"
    if cd.is_dir():
        (cd / "manifest.tsv").write_text(
            "# generated by scripts/extract_disc.py.  A TRACKnn audit records offsets relative to the\n"
            "# track file; img_first_sector + offset/2352 maps one onto TG_NIGHT.IMG, which is what\n"
            "# scripts/pocket/audit_disc.py does.  slot is the APF data-slot id from slots.py -- the same\n"
            "# table scripts/pocket/mk_dist.py writes into data.json, so this tree's cd/ files can go straight\n"
            "# onto a card's Assets/night/common/ (docs/465).\n"
            + "\t".join(CD_COLS) + "\n"
            + "".join("\t".join(str(r.get(c, "")) for c in CD_COLS) + "\n"
                      for r in rows if r["kind"] == "audio"))



def verify(out: Path) -> int:
    man = out / "MANIFEST.tsv"
    if not man.exists():
        print(f"FAIL no manifest at {man}")
        return 1
    rows = [l.split("\t") for l in man.read_text().splitlines() if l.strip() and not l.startswith("#")]
    hdr, rows = rows[0], rows[1:]
    bad = miss = 0
    for r in rows:
        d = dict(zip(hdr, r))
        p = out / d["file"]
        if not p.exists():
            print(f"MISS {d['file']}"); miss += 1; continue
        got = sha256_file(p)
        size_ok = p.stat().st_size == int(d["bytes"])
        ok = got == d["sha256"] and size_ok
        print(f"{'OK  ' if ok else 'FAIL'} {d['file']:24s} {p.stat().st_size:>10} B"
              f"{'' if ok else '   got ' + got[:12] + (' size' if not size_ok else '')}")
        bad += 0 if ok else 1
    print(("OK   %d files verified against %s" % (len(rows), man.name)) if not bad
          else f"FAIL {bad} of {len(rows)} differ ({miss} missing)")
    return 0 if not bad else 1


def cross_check_installed(out: Path, rows, res_dir: Path):
    """Say plainly whether the extracted install set equals the installed copy the repo already ships.

    'Replaces the installer' is a claim about bytes, so it is measured here rather than asserted.  The
    the container's index and the disc disagree about case, and the tree now keeps the disc's; matching
    stays case-insensitive, which is exactly the DOS behaviour the installer ran under.
    """
    if not res_dir.is_dir() or not (res_dir / "NIGHT.RES").exists():
        print(f"\nno installed copy in {res_dir}/ to cross-check against (--res to point at one; a zip "
              f"or a bare container has nothing to compare with)")
        return
    installed = {p.name.upper(): p for p in res_dir.iterdir() if p.is_file()}
    print("\nthe install set, against the installed copy in %s/:" % res_dir)
    for r in (x for x in rows if x["kind"] == "game"):
        b = installed.get(r["file"].upper())
        if not b:
            print(f"  {r['file']:12s} no installed copy to compare")
            continue
        same = r["bytes"] == b.stat().st_size and r["sha256"] == sha256_file(b)
        print(f"  {r['file']:12s} {'byte-identical' if same else 'DIFFERS (extract %d B vs installed %d B)' % (r['bytes'], b.stat().st_size)}")


