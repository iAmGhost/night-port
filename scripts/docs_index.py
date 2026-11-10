#!/usr/bin/env python3
"""Generate docs/INDEX.md.

Why an index exists: there are 400+ numbered analysis documents and no navigation. Numbering is
IMMUTABLE -- the numbers are cited from C comments, python tools and shell scripts (grep 'docs/40' in
pocket/src and scripts/), so nothing may be renumbered or deleted; organising means adding navigation
and status, never renumbering.

The Pocket-era section (>= 330) gets a hand-kept theme map, because that thread is where a reader
actually arrives. Older documents are listed with the title from their own H1, which is descriptive
enough that no separate summary is needed.

    python3 scripts/docs_index.py            # write docs/INDEX.md
    python3 scripts/docs_index.py --check    # fail if the committed index is stale
"""
import argparse
import glob
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = os.path.join(REPO, 'docs')
POCKET_START = 330

# Themes for the Pocket era only. First match wins, so order matters: a doc about the audio feed's
# cost belongs to audio even though it mentions present timings.
THEMES = [
    ('Audio: the CD-DA feed', ('cdda', 'disc', 'audio', 'music', 'ring', 'track', 'underrun', 'read')),
    ('Video and display', ('display', 'blit', 'fb_mode', 'present', 'palette', 'video',
                           'fit', 'picture', 'framebuffer', 'scaler', 'surface', 'overlay',
                           'terminal', 'logo', 'pcx', 'fli', 'stage-back')),
    ('Settings, menu, interact', ('interact', 'menu', 'settings', 'periph', 'mirror', 'mask',
                                  'volume', 'packing', 'bridge', 'probe value')),
    ('Input, saves, clock, performance', ('pad', 'input', 'key', 'save', 'slot', 'bram', 'clock',
                                          'tick', 'pump', 'perf', 'budget', 'spin')),
    ('Verification: rig, accuracy, capture', ('rig', 'frame', 'diff', 'accuracy', 'compare',
                                              'capture', 'oracle', 'sweep', 'ncc', 'witness',
                                              'route', 'drive', 'episode', 'timing', 'phase')),
    ('Packaging and deploy', ('core.json', 'deploy', 'install', 'repack', 'dist', 'slot',
                              'interact.json', 'card')),
]

ORDER = ['Port plan and host layer', 'Audio: the CD-DA feed', 'Video and display',
         'Settings, menu, interact', 'Input and saves', 'Clock, tick, performance',
         'Diagnostics and logging', 'Verification: rig, accuracy, capture', 'Packaging and deploy',
         'Other Pocket-port documents']

# Explicit themes for the Pocket era. Keyword buckets misclassify this thread badly -- "the menu value
# is READABLE" is a settings document, not an audio one -- so the era gets a hand map, which doubles as
# its table of contents. Documents outside the map fall back to the keywords above.
PORT = 'Port plan and host layer'
AUDIO = 'Audio: the CD-DA feed'
VIDEO = 'Video and display'
SET = 'Settings, menu, interact'
INPUT = 'Input and saves'
PERF = 'Clock, tick, performance'
DIAGN = 'Diagnostics and logging'
VER = 'Verification: rig, accuracy, capture'
PKG = 'Packaging and deploy'

THEME_MAP = {
    330: PORT, 331: PORT,
    332: AUDIO, 339: AUDIO, 340: AUDIO, 375: AUDIO, 376: AUDIO,
    333: INPUT, 334: INPUT, 374: INPUT, 390: INPUT, 392: INPUT,
    335: PERF, 364: PERF, 369: PERF, 371: PERF, 372: PERF,
    336: PKG, 366: PKG,
    337: DIAGN, 348: DIAGN, 368: DIAGN, 370: DIAGN, 378: DIAGN,
    338: VIDEO, 343: VIDEO,
    **{n: SET for n in (341, 342, 344, 345, 346, 347, 349, 350, 351, 352, 353, 354, 355, 356,
                        357, 358, 359, 360, 361, 362)},
    **{n: VER for n in (363, 365, 367, 373, 377, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388,
                        389, 391)},
    393: DIAGN, 394: DIAGN,
    # the audio thread proper, 0.1.73 -> 0.1.91 (docs/412 is its consolidated record)
    **{n: AUDIO for n in (395, 396, 397, 398, 402, 403, 406, 407, 408, 409, 412)},
    399: VIDEO, 400: VIDEO, 401: VIDEO, 404: VIDEO, 405: VIDEO,
    410: DIAGN, 411: DIAGN,
}


def doc_meta(path):
    """(number, title, first body line) from a document's own text."""
    name = os.path.basename(path)
    m = re.match(r'(\d+)-', name)
    num = int(m.group(1)) if m else 0
    try:
        text = open(path, encoding='utf-8', errors='replace').read(4000)
    except OSError:
        return num, name, ''
    h = re.match(r'\s*#\s*(.+)', text)
    title = h.group(1).strip() if h else os.path.splitext(name)[0]
    # strip the leading "NNN —" from the H1 when present; the number is already the row label
    title = re.sub(r'^\s*\d+\s*[—-]\s*', '', title)
    body = re.sub(r'^\s*#.*$', '', text, count=1, flags=re.M)
    first = ''
    for line in body.split('\n'):
        s = line.strip()
        if s and not s.startswith(('>', '```', '|', '!', '*', '-', '    ')):
            first = s
            break
    return num, title, first[:150]


def theme(num, title, name):
    if num in THEME_MAP:
        return THEME_MAP[num]
    if num < POCKET_START:
        return None
    blob = (title + ' ' + name).lower()
    for label, keys in THEMES:
        if any(k in blob for k in keys):
            return label
    return 'Other Pocket-port documents'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()

    metas = [doc_meta(p) for p in glob.glob(os.path.join(DOCS, '*.md'))]
    metas.sort(key=lambda t: t[0])
    metas = [m for m in metas if m[0] > 0]          # 00-tasks.md and unnumbered notes are excluded

    out = []
    out.append('# Document index')
    out.append('')
    out.append('Generated by `scripts/docs_index.py` — do not edit by hand; re-run the script.')
    out.append('')
    out.append('**Document numbers are immutable.** They are cited from C comments, python tools and shell')
    out.append('scripts (`grep -rn "docs/40" pocket/src scripts/`), so organising means adding navigation and')
    out.append('status lines, never renumbering or deleting. When a document is superseded, the newer one')
    out.append('links back and this index says so; the older file stays where its citations point.')
    out.append('')
    out.append('## Where to start')
    out.append('')
    starts = [
        (412, 'the consolidated CD-DA record — if you are here about audio, this is the one file'),
        (367, 'the consolidated port record — what ported, what diverged, how to reproduce'),
        (330, 'the Pocket port plan: target, budgets, mapping'),
    ]
    for num, why in starts:
        out.append('- **docs/%d** — %s' % (num, why))
    out.append('- `pocket/INSTALL.md` — installing the core on a card')
    out.append('- `AGENTS.md` — repo layout, tools, how to analyse')
    out.append('')

    # Pocket era, grouped
    pocket = [m for m in metas if m[0] >= POCKET_START]
    if pocket:
        out.append('## The Pocket port (docs/%d–%d)' % (pocket[0][0], pocket[-1][0]))
        out.append('')
        buckets = {}
        for num, title, first in pocket:
            name = None
            # theme on title + the doc's own filename
            for n in os.listdir(DOCS):
                if n.startswith('%d-' % num):
                    name = n
                    break
            buckets.setdefault(theme(num, title, name or ''), []).append((num, title, first))
        ordered = ORDER + [l for l in buckets if l not in ORDER]
        for label in ordered:
            rows = buckets.get(label)
            if not rows:
                continue
            out.append('### %s' % label)
            out.append('')
            for num, title, first in sorted(rows):
                out.append('- **%d** — %s' % (num, title))
                if first:
                    out.append('  <sub>%s</sub>' % first)
            out.append('')

    # Legacy, compact
    legacy = [m for m in metas if m[0] < POCKET_START]
    if legacy:
        out.append('## The DOS port and original-game analysis (docs/1–%d)' % legacy[-1][0])
        out.append('')
        out.append('The function-to-function port of `NIGHT.EXE` and the reverse engineering behind it. Listed')
        out.append('by number; titles are self-describing.')
        out.append('')
        for num, title, first in sorted(legacy):
            out.append('- **%d** — %s' % (num, title))
        out.append('')

    text = '\n'.join(out) + ''
    dest = os.path.join(DOCS, 'INDEX.md')
    if args.check:
        old = open(dest, encoding='utf-8').read() if os.path.exists(dest) else ''
        if old != text:
            print('INDEX.md is stale; run: python3 scripts/docs_index.py', file=sys.stderr)
            return 1
        return 0
    open(dest, 'w', encoding='utf-8').write(text)
    print('wrote %s (%d docs, %d lines)' % (dest, len(metas), text.count('\n')))
    return 0


if __name__ == '__main__':
    sys.exit(main())
