#!/usr/bin/env python3
"""logo-intro evidence: compare the port's logo and opening.fli frames with the ORIGINAL's captured frames.

Method notes
  * The reference for .fli frame indices is an ffmpeg decode of the shipped scratch/res/opening.fli (one PNG per
    .fli frame, extracted with -fps_mode passthrough), so a capture can be placed at a .fli frame index by
    nearest-neighbour matching on 80x50 thumbnails, and index estimates carry an ambiguity margin (2nd-best
    distance) that we print rather than hide.
  * The logo has an independent reference too: a PIL decode of the shipped scratch/res/logo.pcx (640x480 card).
  * Channel order: the ORIGINAL-side PNGs are produced by scripts/playtest_orig_drive.py, which decodes mode-13h
    payloads as BGRX and VESA 101h payloads as RGBX (measured; docs/70). This script assumes those PNGs are
    already in RGB order.
  * Every pair reports NCC (luma), fraction of pixels within +-3 on all three channels, mean absolute error, and
    the distinct-colour histogram overlap, because NCC alone stays ~0.99 for a channel-mirrored pair (docs/70).
"""
import argparse, os, sys, json
import numpy as np
from PIL import Image

TH = (80, 50)  # thumbnail size for index matching


def rgb(path):
    return np.asarray(Image.open(path).convert('RGB')).astype(np.int16)


def thumbn(a):
    """Fade-invariant thumbnail: same as thumb() but divided by its own mean, so a frame shown at a different DAC
    fade level still matches the same reference frame (the intro .fli is played under a fading palette, and
    un-normalised matching put the ORIGINAL's frames 25.7 thumb-MAE away from every reference frame while the
    port's sat at 1.0)."""
    t = thumb(a)
    return (t / max(float(t.mean()), 1.0)).astype(np.float32)


def thumb(a):
    im = Image.fromarray(a.astype(np.uint8)).resize(TH, Image.BILINEAR)
    return np.asarray(im).astype(np.float32).reshape(-1)


def score(p, o):
    """p, o: HxWx3 int16 arrays of the same shape."""
    assert p.shape == o.shape, (p.shape, o.shape)
    lp = 0.299 * p[:, :, 0] + 0.587 * p[:, :, 1] + 0.114 * p[:, :, 2]
    lo = 0.299 * o[:, :, 0] + 0.587 * o[:, :, 1] + 0.114 * o[:, :, 2]
    a, b = lp - lp.mean(), lo - lo.mean()
    ncc = float((a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-9))
    d = np.abs(p - o)
    within = float((d.max(axis=2) <= 3).mean())
    mae = float(d.mean())
    # colour-set overlap (docs/70 lesson: NCC hides a channel mirror, this does not)
    sp = np.unique(p.reshape(-1, 3), axis=0)
    so = np.unique(o.reshape(-1, 3), axis=0)
    shared = len(np.intersect1d(sp.view([('', sp.dtype)] * 3), so.view([('', so.dtype)] * 3)))
    return ncc, within, mae, len(sp), len(so), shared



def lum(a):
    return 0.299 * a[:, :, 0] + 0.587 * a[:, :, 1] + 0.114 * a[:, :, 2]


def score_gain(p, o):
    """Fade-aware score: fit one brightness gain g (least squares on luma), apply it to the reference, then
    compare. The game fades whole screens through the DAC, so an un-faded reference and a faded capture differ
    by exactly that gain; comparing raw then reports a difference that is only the fade phase (docs/68 did the
    same for the logo envelope). Returns (NCC, gain, within+-3 after gain, MAE after gain, shared colours)."""
    lp, lo = lum(p), lum(o)
    a, b = lp - lp.mean(), lo - lo.mean()
    ncc = float((a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-9))
    g = float((lp * lo).sum() / max((lo * lo).sum(), 1e-9))
    q = np.clip(np.round(o * g), 0, 255)
    d = np.abs(p - q)
    sp = np.unique(p.reshape(-1, 3), axis=0); so = np.unique(np.round(q).reshape(-1, 3), axis=0)
    shared = len(np.intersect1d(sp.view([('', sp.dtype)] * 3), so.view([('', so.dtype)] * 3)))
    return ncc, g, float((d.max(axis=2) <= 3).mean()), float(d.mean()), len(sp), len(so), shared


def degenerate(a):
    """True for frames that carry no information (flat: a fade-to-black or hold frame) -- pairing those at an
    ambiguous index would be vacuous."""
    return len(np.unique(a.reshape(-1, 3), axis=0)) < 4 or float(lum(a).mean()) < 6.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port-dir', default='/tmp/nightport/st1fix')
    ap.add_argument('--orig-dir', default='/tmp/dbgauto/orig_fix')
    ap.add_argument('--fli-frames', default='scratch/fli_frames')
    ap.add_argument('--logo', default='scratch/res/logo.pcx')
    ap.add_argument('--out', default='docs/manual-playtest/08-shots')
    ap.add_argument('--json', default='')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rep = {'logo': {}, 'fli': {}}

    # ---------- 1. logo: port vs original vs shipped logo.pcx ----------
    logo = rgb(a.logo)
    print('logo.pcx decode: %s' % (logo.shape,))

    def manifest_rows(d):
        rows = []
        for ln in open(os.path.join(d, 'manifest.tsv')).read().splitlines()[1:]:
            f = ln.split('\t')
            rows.append(dict(seq=int(f[0]), t_ms=int(f[1]), event=f[3], resource=f[4], W=int(f[7]), H=int(f[8])))
        return rows

    def orig_rows(d):
        rows = []
        for ln in open(os.path.join(d, 'events.tsv')).read().splitlines()[1:]:
            f = ln.split('\t')
            rows.append(dict(seq=int(f[0]), t_ms=int(f[1]), event=f[2], label=f[3], mode=f[5], bios=f[6], file=f[7]))
        return rows

    port = manifest_rows(a.port_dir)
    orig = orig_rows(a.orig_dir)

    # port logo shots: 640x480 shots whose resource is logo.pcx
    ps = [r for r in port if r['W'] == 640 and r['H'] == 480 and r['resource'] == 'logo.pcx']
    ps.sort(key=lambda r: r['t_ms'])
    best_p = None
    for r in ps[:12]:
        p = rgb(os.path.join(a.port_dir, '%04d_%s.bmp' % (r['seq'], r['event'])))
        if p.shape != logo.shape or degenerate(p):
            continue
        s = score_gain(p, logo)
        if best_p is None or s[2] > best_p[0][2]:
            best_p = (s, r, p)
    print('port logo shot vs shipped logo.pcx (gain-fit): seq=%d t=%d  NCC=%.4f gain=%.3f within+-3=%.4f MAE=%.3f colours %d/%d shared %d'
          % (best_p[1]['seq'], best_p[1]['t_ms'], *best_p[0]))
    rep['logo']['port_vs_asset'] = dict(zip(('ncc', 'gain', 'within3', 'mae', 'cport', 'casset', 'shared'), best_p[0]))

    os_ = [r for r in orig if r['mode'] == '640x480']
    best_o = None
    for r in os_:
        o = rgb(os.path.join(a.orig_dir, r['file']))
        if o.shape != logo.shape or degenerate(o):
            continue
        s = score_gain(o, logo)
        if best_o is None or s[2] > best_o[0][2]:
            best_o = (s, r, o)
    if best_o:
        print('original logo frame vs shipped logo.pcx (gain-fit): seq=%d t=%d %s  NCC=%.4f gain=%.3f within+-3=%.4f MAE=%.3f colours %d/%d shared %d'
              % (best_o[1]['seq'], best_o[1]['t_ms'], best_o[1]['file'], *best_o[0]))
        rep['logo']['orig_vs_asset'] = dict(zip(('ncc', 'gain', 'within3', 'mae', 'corig', 'casset', 'shared'), best_o[0]))
        s = score_gain(best_p[2], best_o[2])
        print('PORT logo frame vs ORIGINAL logo frame (gain-fit): NCC=%.4f gain=%.3f within+-3=%.4f MAE=%.3f colours %d/%d shared %d' % s)
        rep['logo']['port_vs_orig'] = dict(zip(('ncc', 'gain', 'within3', 'mae', 'cport', 'corig', 'shared'), s),
                                          port_seq=best_p[1]['seq'], port_t=best_p[1]['t_ms'],
                                          orig_seq=best_o[1]['seq'], orig_t=best_o[1]['t_ms'])
        # gain-matched pairing: the screens fade, so "the same moment" means the same fade level. Take the
        # original's plateau frame and the port frame whose fitted gain is closest, then compare with NO gain fit.
        port_env = []
        for r in ps:
            p = rgb(os.path.join(a.port_dir, '%04d_%s.bmp' % (r['seq'], r['event'])))
            if p.shape == logo.shape and not degenerate(p):
                port_env.append((score_gain(p, logo)[1], r, p))
        orig_env = []
        for r in os_:
            o = rgb(os.path.join(a.orig_dir, r['file']))
            if o.shape == logo.shape and not degenerate(o):
                orig_env.append((score_gain(o, logo)[1], r, o))
        gm = max((g for g, _, _ in orig_env), key=lambda x: x) if orig_env else 1.0
        gstar = max(orig_env, key=lambda x: x[0]) if orig_env else None
        pstar = min(port_env, key=lambda x: abs(x[0] - gstar[0])) if (port_env and gstar) else None
        if gstar and pstar:
            d = np.abs(pstar[2] - gstar[2])
            lvl = dict(orig_gain=gstar[0], port_gain=pstar[0], orig_seq=gstar[1]['seq'], orig_t=gstar[1]['t_ms'],
                       port_seq=pstar[1]['seq'], port_t=pstar[1]['t_ms'],
                       within3=float((d.max(axis=2) <= 3).mean()), within8=float((d.max(axis=2) <= 8).mean()),
                       mae=float(d.mean()))
            print('logo at matched fade level (gain orig=%.3f port=%.3f): within+-3 %.4f  within+-8 %.4f  MAE %.3f'
                  % (lvl['orig_gain'], lvl['port_gain'], lvl['within3'], lvl['within8'], lvl['mae']))
            rep['logo']['gain_matched_pair'] = lvl
            canvas = np.zeros((logo.shape[0] // 2, logo.shape[1] * 2 + 1, 3), np.uint8)
            canvas[:, :logo.shape[1]] = pstar[2].astype(np.uint8)[::2]
            canvas[:, logo.shape[1] + 1:] = gstar[2].astype(np.uint8)[::2]
            Image.fromarray(canvas).save(os.path.join(a.out, 'logo_gainmatched_port_vs_original.png'))
        rep['logo']['env_gains'] = dict(orig=[[round(g, 3), r['seq'], r['t_ms']] for g, r, _ in orig_env],
                                        port=[[round(g, 3), r['seq'], r['t_ms']] for g, r, _ in port_env])
        pair = np.zeros((logo.shape[0] // 2, logo.shape[1] * 2, 3), np.uint8)
        pair[:, :logo.shape[1]] = best_p[2].astype(np.uint8)[::2]
        pair[:, logo.shape[1]:] = best_o[2].astype(np.uint8)[::2]
        Image.fromarray(pair).save(os.path.join(a.out, 'logo_port_vs_original.png'))

    # ---------- 2. opening.fli: index matching, then pair at the same index ----------
    fdir = a.fli_frames
    names = sorted(os.listdir(fdir))
    refs = [thumb(rgb(os.path.join(fdir, n))) for n in names]
    R = np.stack(refs)
    # collapse runs of identical reference thumbnails: the .fli opens with a long static stretch, so "nearest
    # reference frame" is otherwise meaningless there (gap 0.00 in the first printout below says exactly that).
    uniq, first = [], []
    for i in range(len(names)):
        if i and np.abs(R[i] - R[i - 1]).mean() < 0.05:
            continue
        uniq.append(i)
    Rall = R
    Rn = R / np.maximum(R.mean(axis=1, keepdims=True), 1.0)
    R = R[uniq]
    Rnu = Rn[uniq]
    print('fli reference frames: %d extracted, %d distinct thumbnails' % (len(names), len(R)))

    def index_of(path):
        t = thumb(rgb(path))
        tn = thumbn(rgb(path))
        d = np.abs(Rnu - tn).mean(axis=1)
        order = np.argsort(d)
        k = int(order[0])
        dr = np.abs(R - thumb(rgb(path))).mean(axis=1)
        return uniq[k], float(dr[k]), float(d[order[1]]), float(d[order[2]] - d[order[1]])

    # original .fli captures (mode 13h, bios 19) and port .fli shots (resource opening.fli / event fli*)
    of = [r for r in orig if r['mode'] == '320x200' and r['bios'] == '19']
    pf = [r for r in port if r['W'] == 320 and r['H'] == 200 and
          (r['resource'] == 'opening.fli' or r['event'].startswith('fli'))]
    print('original fli captures: %d   port fli shots: %d' % (len(of), len(pf)))

    rows = []
    for r in of:
        i, d0, d1, gap = index_of(os.path.join(a.orig_dir, r['file']))
        rows.append(('orig', r['seq'], r['t_ms'], r['file'], i, d0, d1, gap))
    for r in pf:
        p = os.path.join(a.port_dir, '%04d_%s.bmp' % (r['seq'], r['event']))
        i, d0, d1, gap = index_of(p)
        rows.append(('port', r['seq'], r['t_ms'], os.path.basename(p), i, d0, d1, gap))
    for side, seq, t, f, i, d0, d1, gap in rows[:6] + rows[-3:]:
        print('  %-5s seq=%4d t=%6d  fli_idx=%4d  thumbMAE=%5.2f  2nd=%5.2f gap=%5.2f  %s' % (side, seq, t, i, d0, d1, gap, f))

    oi = [(r[3], r[2], r[4], r[5], r[7]) for r in rows if r[0] == 'orig']
    pi = [(r[3], r[2], r[4], r[5], r[7]) for r in rows if r[0] == 'port']
    mono = lambda xs: all(b[2] >= a[2] for a, b in zip(xs, xs[1:]))
    print('index monotone: orig=%s port=%s   orig idx range=%s port idx range=%s'
          % (mono(oi), mono(pi), (oi[0][2], oi[-1][2]), (pi[0][2], pi[-1][2])))
    print('thumb MAE to reference: orig median %.2f max %.2f | port median %.2f max %.2f'
          % (np.median([x[3] for x in oi]), max(x[3] for x in oi),
             np.median([x[3] for x in pi]), max(x[3] for x in pi)))
    rep['fli']['index'] = dict(orig=[[x[0], x[1], x[2]] for x in oi], port=[[x[0], x[1], x[2]] for x in pi])

    # pair at the nearest index and score at full resolution
    # Pair by direct thumbnail best match (fade-invariant), not by index proximity: the index estimate from a
    # 6-8 Hz sample of a ~33 fps video is ambiguous, and the index table is used only to report coverage below.
    pthumbs = []
    for f, t, i, dm, gap in pi:
        pthumbs.append(thumbn(rgb(os.path.join(a.port_dir, f))))
    P = np.stack(pthumbs)
    pairs, saved, skipped = [], 0, 0
    for (f, t, i, dm, gap), otn in zip(oi, [thumbn(rgb(os.path.join(a.orig_dir, x[0]))) for x in oi]):
        dd = np.abs(P - otn).mean(axis=1)
        k = int(np.argmin(dd))
        cand = pi[k]
        if dd[k] > 0.60:
            skipped += 1
            continue
        p = rgb(os.path.join(a.port_dir, cand[0]))
        o = rgb(os.path.join(a.orig_dir, f))
        if degenerate(o) or degenerate(p):
            skipped += 1
            continue
        s = score_gain(p, o)
        pairs.append((i, cand[1], t, s))
        if saved < 3 and s[1] > 0:
            canvas = np.zeros((200, 641, 3), np.uint8)
            canvas[:, :320] = p.astype(np.uint8)
            canvas[:, 321:] = o.astype(np.uint8)
            Image.fromarray(canvas).save(os.path.join(a.out, 'fli_idx%04d_port_vs_original.png' % i))
            saved += 1
    if pairs:
        w = np.array([[x[3][2], x[3][0], x[3][3]] for x in pairs])
        hist = np.histogram(w[:, 0], bins=[0, 0.001, 0.5, 0.9, 0.99, 0.999, 1.0001])[0]
        print('fli within+-3 histogram (bins 0, <.001, .5, .9, .99, .999, 1.0]: %s' % list(hist))
        worst = sorted(pairs, key=lambda x: x[3][2])[:3]
        for i, pt, ot, ss in worst:
            print('   worst pair idx %4d port t=%6d orig t=%6d: within+-3 %.4f NCC %.4f gain %.3f MAE %.2f' % (i, pt, ot, ss[2], ss[0], ss[1], ss[3]))
        print('fli pairs at same index (non-degenerate, gain-fit): n=%d skipped=%d  within+-3 median %.4f best %.4f | NCC median %.4f | MAE median %.2f'
              % (len(pairs), skipped, np.median(w[:, 0]), w[:, 0].max(), np.median(w[:, 1]), np.median(w[:, 2])))
        for i, pt, ot, s in sorted(pairs, key=lambda x: -x[3][2])[:6]:
            print('   idx %4d  port t=%6d orig t=%6d  within+-3 %.4f  NCC %.4f  gain %.3f  MAE %.2f  colours %d/%d shared %d'
                  % (i, pt, ot, s[2], s[0], s[1], s[3], s[4], s[5], s[6]))
        rep['fli']['pairs'] = [[i, pt, ot] + list(s) for i, pt, ot, s in pairs]

    if a.json:
        json.dump(rep, open(a.json, 'w'), indent=1)
    return 0


if __name__ == '__main__':
    sys.exit(main())
