#!/usr/bin/env python3
"""logo-intro evidence, second pass (docs/71).

Two clean measurements, both fade-aware:

  A. logo card: find the logo window on each side by fade-envelope fit against the shipped logo.pcx, then compare
     the two plateau frames directly (no gain fit) and report the envelope.
  B. opening.fli: map every sampled frame on each side to a .fli frame index by fade-invariant (mean-normalised)
     thumbnail matching against an ffmpeg decode of the shipped .fli, report the matching confidence and the
     playback rate on each side, then compare the two sides' frames at equal .fli indices at full resolution.

Why the first pass (scripts/playtest_compare_logo_fli.py) was not enough: pairing by "nearest .fli index" is
ambiguous at 6-8 Hz sampling of a ~33 fps video, and pairing by raw thumbnail distance silently paired frames from
different parts of the video (its worst pairs had NCC 0.43). This version reports the match confidence and the
per-side fit to the reference, so a bad pairing cannot hide inside the summary.
"""
import argparse, os, sys, json
import numpy as np
from PIL import Image

TH = (80, 50)


def rgb(p):
    return np.asarray(Image.open(p).convert('RGB')).astype(np.int16)


def decode_raw(path):
    """Decode a raw payload from scripts/playtest_orig_capture_api.py. Channel order is per mode, measured:
    VESA 101h payloads are RGBX, mode-13h payloads are BGRX (docs/70). The payload row is abs(pitch)/4 pixels wide,
    which for mode 13h is the emulator's doubled 640-pixel scan, so crop to the header's mode width."""
    import struct
    b = open(path, 'rb').read()
    w, h, pitch = struct.unpack_from('<iii', b, 0)
    roww = abs(pitch) // 4
    px = np.frombuffer(b[15:15 + abs(pitch) * h], dtype=np.uint8).reshape(h, roww, 4)
    if roww > w:
        px = px[:, :w, :]
    a = px[:, :, :3].astype(np.int16)
    if h == 200:
        a = a[:, :, (2, 1, 0)]
    return a


def thum(a, norm):
    t = np.asarray(Image.fromarray(a.astype(np.uint8)).resize(TH, Image.BILINEAR)).astype(np.float32).reshape(-1)
    if norm:
        t = t / max(float(t.mean()), 1.0)
    return t


def lum(a):
    return 0.299 * a[:, :, 0] + 0.587 * a[:, :, 1] + 0.114 * a[:, :, 2]


def fit(p, o):
    """Returns (ncc, gain, within3, within8, mae) with one least-squares brightness gain applied to o."""
    lp, lo = lum(p), lum(o)
    a, b = lp - lp.mean(), lo - lo.mean()
    ncc = float((a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-9))
    g = float((lp * lo).sum() / max((lo * lo).sum(), 1e-9))
    q = np.clip(np.round(o * g), 0, 255)
    d = np.abs(p - q)
    return ncc, g, float((d.max(axis=2) <= 3).mean()), float((d.max(axis=2) <= 8).mean()), float(d.mean())


def deg(a):
    return len(np.unique(a.reshape(-1, 3), axis=0)) < 4 or float(lum(a).mean()) < 6.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port-dir', default='/tmp/nightport/st1fix')
    ap.add_argument('--orig-dir', default='/tmp/dbgauto/orig_boot')
    ap.add_argument('--fli-frames', default='scratch/fli_frames')
    ap.add_argument('--logo', default='scratch/res/logo.pcx')
    ap.add_argument('--out', default='docs/manual-playtest/08-shots')
    ap.add_argument('--orig-raw', default='', help='directory of raw payloads from playtest_orig_capture_api.py (decoded here, per-mode channel order)')
    ap.add_argument('--json', default='')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rep = {}

    def port_frames():
        out = []
        for ln in open(os.path.join(a.port_dir, 'manifest.tsv')).read().splitlines()[1:]:
            f = ln.split('\t')
            p = os.path.join(a.port_dir, '%04d_%s.bmp' % (int(f[0]), f[3]))
            if os.path.exists(p):
                out.append(dict(seq=int(f[0]), t=int(f[1]), event=f[3], res=f[4], path=p))
        return out

    def img(r):
        return r['arr'] if 'arr' in r else rgb(r['path'])

    def orig_frames():
        if a.orig_raw:
            out = []
            for ln in open(os.path.join(a.orig_raw, 'manifest.tsv')).read().splitlines()[1:]:
                f = ln.split('\t')
                q = os.path.join(a.orig_raw, f[9])
                out.append(dict(seq=int(f[0]), t=int(f[1]), path=q, arr=decode_raw(q)))
            return out
        out = []
        for ln in open(os.path.join(a.orig_dir, 'events.tsv').replace('events', 'events') if False else os.path.join(a.orig_dir, 'events.tsv')).read().splitlines()[1:]:
            f = ln.split('\t')
            out.append(dict(seq=int(f[0]), t=int(f[1]), event=f[2], path=os.path.join(a.orig_dir, f[7])))
        return out

    PF, OF = port_frames(), orig_frames()

    # ---- delimit the opening.fli window on each side by card anchors, not by wall time ----
    # The .fli intro sits between the company-logo card and the title card, both of which are VESA 640x480 cards we
    # have independent decodes of. Without this, the original's mode-13h frames (which also include the attract demo
    # and later gameplay) were being matched against .fli references and the whole side scored as garbage
    # (median thumb distance 0.288 vs the port's 0.043).
    cards = {}
    for n in ('logo', 'title1', 'title3'):
        fp = os.path.join('scratch/res', n + '.pcx')
        if os.path.exists(fp):
            cards[n] = rgb(fp)

    def card_hit(im):
        for n, c in cards.items():
            if im.shape == c.shape:
                ncc, g, w3, w8, mae = fit(im, c)
                if ncc > 0.99:
                    return n
        return None

    def fli_window(frames):
        seq = []
        for r in frames:
            im = img(r)
            seq.append((r, im.shape[:2], card_hit(im)))
        # Runs of consecutive 320x200 frames, in time order. The card anchors work for the logo on both sides but
        # not for the title (the original's title card is animated, so NCC>0.99 against a still decode fails), so the
        # window is chosen geometrically: the first 320x200 run that begins after the logo card ends. That run is the
        # .fli on both sides; the attract demo and gameplay runs are later and are excluded.
        logo_end = max((r['t'] for r, sh, c in seq if c == 'logo'), default=-1)
        runs, cur = [], []
        for r, sh, c in seq:
            if sh == (200, 320):
                cur.append(r)
            else:
                if cur:
                    runs.append(cur); cur = []
        if cur:
            runs.append(cur)
        print('   320x200 runs: %s' % [(g[0]['t'], g[-1]['t'], len(g)) for g in runs])
        sel = next((g for g in runs if g[0]['t'] > logo_end and len(g) >= 10), [])
        return [(r, (200, 320)) for r in sel if not deg(img(r))]
    PW, OW = fli_window(PF), fli_window(OF)
    print('   fli window: port %d frames t=%s..%s | original %d frames t=%s..%s'
          % (len(PW), PW[0][0]['t'] if PW else '-', PW[-1][0]['t'] if PW else '-',
             len(OW), OW[0][0]['t'] if OW else '-', OW[-1][0]['t'] if OW else '-'))

    # ---------------- A. logo card ----------------
    logo = rgb(a.logo)
    def envelope(frames, want640):
        env = []
        for r in frames:
            im = img(r)
            if im.shape != logo.shape or deg(im):
                continue
            ncc, g, w3, w8, mae = fit(im, logo)
            if ncc > 0.99 and 0.0 < g <= 1.3:
                env.append((r['t'], g, im, ncc, w3))
        env.sort()
        return env
    pe, oe = envelope(PF, True), envelope(OF, True)
    print('A. logo card')
    print('   port:   %d frames fit logo.pcx (NCC>0.99), gain %.3f..%.3f, t=%d..%d'
          % (len(pe), min(x[1] for x in pe), max(x[1] for x in pe), pe[0][0], pe[-1][0]) if pe else 'none')
    print('   origin: %d frames fit logo.pcx (NCC>0.99), gain %.3f..%.3f, t=%d..%d'
          % (len(oe), min(x[1] for x in oe), max(x[1] for x in oe), oe[0][0], oe[-1][0]) if oe else 'none')
    if pe and oe:
        gp = max(x[1] for x in pe); go = max(x[1] for x in oe)
        p = min(pe, key=lambda x: abs(x[1] - go))          # port frame at the original's brightest fade level
        o = max(oe, key=lambda x: x[1])
        d = np.abs(p[2] - o[2])
        row = dict(port_t=p[0], orig_t=o[0], port_gain=p[1], orig_gain=o[1],
                   within3=float((d.max(axis=2) <= 3).mean()), within8=float((d.max(axis=2) <= 8).mean()),
                   mae=float(d.mean()))
        for nm, e in (('port', pe), ('original', oe)):
            x = max(e, key=lambda y: y[1])
            dd = np.abs(x[2] - logo)
            print('   %-9s brightest logo frame: t=%d gain=%.3f  vs asset: within+-3 %.4f within+-8 %.4f MAE %.2f'
                  % (nm, x[0], x[1], float((dd.max(axis=2) <= 3).mean()), float((dd.max(axis=2) <= 8).mean()), float(dd.mean())))
        print('   plateau pair: port t=%d gain=%.3f vs original t=%d gain=%.3f -> within+-3 %.4f within+-8 %.4f MAE %.3f'
              % (row['port_t'], row['port_gain'], row['orig_t'], row['orig_gain'], row['within3'], row['within8'], row['mae']))
        # how long each side holds the logo (frames with gain >= 0.9 of its own max)
        def hold(e):
            top = max(x[1] for x in e)
            ts = [x[0] for x in e if x[1] >= 0.9 * top]
            return ts[-1] - ts[0]
        row['port_hold_ms'], row['orig_hold_ms'] = hold(pe), hold(oe)
        print('   hold at >=90%% of peak fade: port %d ms, original %d ms' % (row['port_hold_ms'], row['orig_hold_ms']))
        rep['logo'] = row
        cv = np.zeros((logo.shape[0] // 2, logo.shape[1] * 2 + 1, 3), np.uint8)
        cv[:, :logo.shape[1]] = p[2].astype(np.uint8)[::2]
        cv[:, logo.shape[1] + 1:] = o[2].astype(np.uint8)[::2]
        Image.fromarray(cv).save(os.path.join(a.out, 'logo_plateau_port_vs_original.png'))

    # ---------------- B. opening.fli ----------------
    names = sorted(os.listdir(a.fli_frames))
    R = np.stack([thum(rgb(os.path.join(a.fli_frames, n)), True) for n in names])
    # de-duplicate identical reference thumbnails so "nearest frame" is not inside a flat run
    keep = [i for i in range(len(names)) if i == 0 or np.abs(R[i] - R[i - 1]).mean() > 0.02]
    Rd, idxs = R[keep], [int(n.split('.')[0]) - 1 for n in (names[i] for i in keep)]
    print('B. opening.fli: %d reference frames, %d distinct' % (len(names), len(Rd)))

    def match(frames, size):
        got = []
        for r in frames:
            im = img(r)
            if im.shape[:2] != size or deg(im):
                continue
            t = thum(im, True)
            d = np.abs(Rd - t).mean(axis=1)
            k = int(np.argmin(d))
            got.append(dict(seq=r['seq'], t=r['t'], idx=idxs[k], d=float(d[k]),
                            gap=float(np.partition(d, 1)[1] - d[k]), im=im, path=r['path']))
        return got

    po = match([r for r, sh in PW], (200, 320))
    oo = match([r for r, sh in OW], (200, 320))
    def summ(name, xs):
        if not xs:
            print('   %s: no frames' % name); return
        conf = [x for x in xs if x['d'] < 0.10 and x['gap'] > 0.01]
        print('   %-9s sampled=%3d  confident idx matches=%3d (%.0f%%)  median thumb dist %.3f  idx range %s  distinct idx %d'
              % (name, len(xs), len(conf), 100.0 * len(conf) / len(xs), float(np.median([x['d'] for x in xs])),
                 (min(x['idx'] for x in conf), max(x['idx'] for x in conf)) if conf else '-', len({x['idx'] for x in conf})))
        if len(conf) >= 4:
            ts = np.array([x['t'] for x in conf], float); ix = np.array([x['idx'] for x in conf], float)
            A = np.vstack([ts, np.ones_like(ts)]).T
            slope, _ = np.linalg.lstsq(A, ix, rcond=None)[0]
            print('             playback rate %.2f .fli frames/s over the confident samples' % (slope * 1000.0))
        return conf
    pconf, oconf = summ('original', oo), summ('port', po)
    rep['fli'] = dict(orig_sampled=len(oo), port_sampled=len(po),
                      orig_confident=len(oconf), port_confident=len(pconf),
                      orig_median_dist=float(np.median([x['d'] for x in oo])) if oo else None,
                      port_median_dist=float(np.median([x['d'] for x in po])) if po else None)

    # pair at equal .fli index; require confidence on both sides and a real index gap margin
    pairs = []
    for o in oconf:
        cands = [p for p in pconf if p['idx'] == o['idx']]
        if not cands:
            continue
        p = min(cands, key=lambda x: abs(x['t'] - o['t']))
        ncc, g, w3, w8, mae = fit(p['im'], o['im'])
        pairs.append((o['idx'], p['t'], o['t'], ncc, g, w3, w8, mae))
    if pairs:
        w = np.array([[x[5], x[3], x[6]] for x in pairs])
        print('   pairs at identical .fli index: n=%d  within+-3: median %.4f  >=0.99: %d  | NCC median %.4f  | within+-8 median %.4f'
              % (len(pairs), np.median(w[:, 0]), int((w[:, 0] >= 0.99).sum()), np.median(w[:, 1]), np.median(w[:, 2])))
        for x in sorted(pairs, key=lambda y: -y[5])[:5]:
            print('      idx %4d port t=%6d orig t=%6d  within+-3 %.4f within+-8 %.4f NCC %.4f gain %.3f MAE %.2f'
                  % (x[0], x[1], x[2], x[5], x[6], x[3], x[4], x[7]))
        rep['fli']['pairs'] = [list(x) for x in pairs]
        for x in sorted(pairs, key=lambda y: -y[5])[:3]:
            p = next(z for z in pconf if z['idx'] == x[0] and z['t'] == x[1])
            o = next(z for z in oconf if z['idx'] == x[0] and z['t'] == x[2])
            cv = np.zeros((200, 641, 3), np.uint8)
            cv[:, :320] = p['im'].astype(np.uint8); cv[:, 321:] = o['im'].astype(np.uint8)
            Image.fromarray(cv).save(os.path.join(a.out, 'fli_idx%04d_port_vs_original.png' % x[0]))
    if a.json:
        json.dump(rep, open(a.json, 'w'), indent=1)


if __name__ == '__main__':
    main()
