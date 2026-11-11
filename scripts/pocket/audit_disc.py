#!/usr/bin/env python3
"""Verify a disc-read audit against the SHIPPED disc -- the gate docs/410 said was missing.

`cdda.raw` cannot judge an audio change: three runs of one binary and seed give two different md5s, because it
is a wall-clock sampling log of the mixer ring (docs/410). This gates the read path instead: cd_audit.c (built
into both hosts) records every read the port makes, and this maps each request onto TG_NIGHT.IMG and compares
the bytes. The request is a property of the read path and the disc, so it reproduces.

    cd_iso.c      -> IMG / IMG_USER   offset is already an IMG byte offset
    of_disc.c     -> TRACKnn         offset is slot-relative; mapped onto IMG through scratch/pocket/manifest.tsv
                                      (the mapping lives here, in one language, not in C as well)

    python3 scripts/pocket/audit_disc.py AUDIT [--img TG_NIGHT.IMG] [--manifest scratch/pocket/manifest.tsv]

Exit 0 = every recorded request matches the shipped disc byte for byte. Also reports COVERAGE: how much of each
track the run actually touched, because a gate nobody reaches is not a gate.
"""
import argparse
import pathlib
import sys

SECTOR = 2352


def fnv1a64(b):
    h = 14695981039346656037
    for c in b:
        h ^= c
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def requests(path):
    out = []
    for ln in open(path, 'rb').read().decode('latin1').splitlines():
        if not ln or ln.startswith('#'):
            continue
        parts = ln.split('\t')
        if len(parts) == 4:
            out.append((parts[0], int(parts[1]), int(parts[2]), parts[3].strip()))
    return out


def diff(p1, p2, t2s):
    """Compare two audits as SETS of (kind, sector-aligned request, hash).

    Run to run, this port's reads arrive in different batch sizes (the prefill, DISC_MIN_WANT, how far the
    voice ran ahead), so an exact sequence comparison would report a difference that is only timing. The set of
    sector-granular requests is the thing a change to the read path actually moves: a wrong stop, a wrong
    start sector, a sector never read, or bytes that differ.
    """
    if not p2:
        print('FAIL --diff needs a second audit file'); return 2
    def canon(rs):
        s = {}
        for kind, off, n, h in rs:
            if kind.startswith('TRACK'):
                if off % SECTOR or n % SECTOR:
                    s.setdefault((kind, off, n), []).append(h)
                    continue
                for i in range(n // SECTOR):
                    s.setdefault((kind, off + i * SECTOR, SECTOR), []).append(h)
            else:
                s.setdefault((kind, off, n), []).append(h)
        return s
    A, B = canon(requests(p1)), canon(requests(p2))
    ka, kb = set(A), set(B)
    print('A %s: %d requests -> %d sector-granular' % (p1, len(requests(p1)), len(ka)))
    print('B %s: %d requests -> %d sector-granular' % (p2, len(requests(p2)), len(kb)))
    only_a, only_b = sorted(ka - kb), sorted(kb - ka)
    print('only in A: %d   only in B: %d   shared: %d' % (len(only_a), len(only_b), len(ka & kb)))
    for k in only_a[:6]:
        print('   A only: %s off=%d n=%d' % k)
    for k in only_b[:6]:
        print('   B only: %s off=%d n=%d' % k)
    bad = 0
    for k in sorted(ka & kb):
        if sorted(A[k]) != sorted(B[k]):
            bad += 1
            if bad <= 5:
                print('   SAME REQUEST, DIFFERENT BYTES: %s off=%d n=%d' % k)
    if bad:
        print('FAIL %d shared requests returned different bytes' % bad); return 1
    if only_a or only_b:
        print('NOTE request sets differ; every shared request matched byte for byte')
        return 0
    print('OK the two audits request the identical byte-identical set')
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('audit')
    ap.add_argument('audit2', nargs='?')
    ap.add_argument('--diff', action='store_true', help='compare two audits as SETS of requests (batching and order differ run to run; the set is what a change to the read path moves)')
    ap.add_argument('--img', default=None,
                    help='the raw 2352-byte-frame image (default: the disc this repo has, docs/455)')
    ap.add_argument('--manifest', default='scratch/pocket/manifest.tsv')
    a = ap.parse_args()

    # track -> first sector in the IMG (the CUE's PREGAP is already carried in the manifest).
    # Comment lines are skipped: scripts/extract_disc.py's manifest carries its own explanation, and
    # reading its first line as the header turned every request into "track N not in manifest". 
    man = pathlib.Path(a.manifest)
    t2s = {}
    if man.exists():
        rows = [l.split('\t') for l in man.read_text().splitlines() if l.strip() and not l.startswith('#')]
        hdr = rows[0]
        for r in rows[1:]:
            d = dict(zip(hdr, r))
            if d.get('track', '').isdigit() and d.get('img_first_sector', '').isdigit():
                t2s[int(d['track'])] = (int(d['img_first_sector']), int(d['sectors']))
    else:
        print('FAIL manifest %s missing (scripts/pocket/repack.py)' % man); return 1

    if a.diff:
        return diff(a.audit, a.audit2, t2s)

    img = pathlib.Path(a.img)
    if not img.exists():
        print('FAIL shipped IMG missing at %s' % img); return 1
    imgsz = img.stat().st_size

    f = open(a.audit, 'rb')
    data = f.read().decode('latin1').splitlines()
    imgf = open(img, 'rb')
    n = bad = 0
    first_bad = []
    cov = {}          # track -> set of IMG sectors touched
    kinds = {}
    for ln in data:
        if not ln or ln.startswith('#'):
            continue
        parts = ln.split('\t')
        if len(parts) != 4:
            continue
        kind, off, nb, hsh = parts[0], int(parts[1]), int(parts[2]), parts[3]
        n += 1
        kinds[kind] = kinds.get(kind, 0) + 1
        if kind.startswith('TRACK'):
            tr = int(kind[5:])
            if tr not in t2s:
                bad += 1
                first_bad.append('%s: track %d not in manifest' % (ln, tr)); continue
            first, secs = t2s[tr]
            if off % SECTOR or nb % SECTOR:
                bad += 1
                first_bad.append('%s: offset/length not sector-aligned' % ln); continue
            base = first + off // SECTOR
            s0 = (first + off // SECTOR) * SECTOR
            img_off, img_len = s0, nb
            cov.setdefault(tr, set()).update(range(base, base + nb // SECTOR))
        else:
            img_off, img_len = off, nb
        if img_off + img_len > imgsz:
            bad += 1
            first_bad.append('%s: past end of IMG (%d > %d)' % (ln, img_off + img_len, imgsz)); continue
        imgf.seek(img_off)
        blob = imgf.read(img_len)
        if '%016x' % fnv1a64(blob) != hsh.strip():
            bad += 1
            if len(first_bad) < 5:
                first_bad.append('%s: BYTES DIFFER at IMG offset %d len %d' % (ln, img_off, img_len))
    imgf.close()

    print('requests verified: %d  kinds: %s' % (n, ', '.join('%s=%d' % kv for kv in sorted(kinds.items()))))
    for tr in sorted(cov):
        first, secs = t2s[tr]
        got = len(cov[tr])
        print('  track %02d coverage: %d / %d sectors (%.0f%%)' % (tr, got, secs, 100.0 * got / secs if secs else 0))
    for b in first_bad[:5]:
        print('  %s' % b)
    if bad:
        print('FAIL %d of %d requests do not match the shipped disc' % (bad, n))
        return 1
    if n == 0:
        print('FAIL nothing recorded -- set NIGHT_CD_AUDIT=... and re-run')
        return 1
    print('OK every recorded read is byte-identical to %s' % img)
    return 0


if __name__ == '__main__':
    sys.exit(main())
