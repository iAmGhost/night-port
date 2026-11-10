#!/usr/bin/env python3
"""The report layer has hit itself seven times in one stretch of this project (docs/433, docs/446), including
twice inside the docs/446 section alone. Each one was a wrong VERDICT, not a wrong number -- which is worse,
because a wrong number gets re-read and a wrong verdict gets believed. This file is the cheapest thing that
could catch that: fixture logs whose verdict is known, so the reporter can be wrong before a card run is.
"""
import re, subprocess, sys, tempfile, os

SPLIT = ("split wall={wall}ms pump=39ms pres=322ms disc=28ms aud=27ms rest=929ms frames={fr}"
         " pump_n=225 res_calls=114 res_kb=549 | clk=2048 clkmax=4 cap={cap} noflip={nf}"
         " flipnw={fnw} hold=40ms max=40ms\n")

SPLIT_NOFLDW = ("split wall={wall}ms pump=39ms pres=322ms disc=28ms aud=27ms rest=929ms frames={fr}"
                  " pump_n=225 res_calls=114 res_kb=549 | clk=2048 clkmax=4 cap={cap} noflip={nf}"
                  " hold=40ms max=40ms\n")   # 0.2.18 shape: no flipnw= field at all

def boot(core):
    return 'core %s: boot marker\n' % core

# The reporter drops every second that has no geometry attached (it reports per phase), so a fixture without
# `view=` lines prints no invariant section at all and the test passes by never testing anything. That took
# three rounds to find, which is itself the argument for having this file.
def run(core, caps, geo='view=1 320x200 sig=a784676e chg=156'):
    out = [boot(core), geo + '\n']
    for i, (cap, nf, fnw) in enumerate(caps):
        out.append(geo + '\n')
        out.append(SPLIT.format(wall=1000 * (i + 1), fr=30 * (i + 1), cap=cap, nf=nf, fnw=fnw))
    return ''.join(out)

HEALTHY  = run('0.2.20', [(12, 5, 0), (20, 9, 0), (30, 22, 0)])
# A flip without a write: the exact shape of the docs/445 bug.
# Two samples, because the reporter reports *deltas* between split lines and bails on a run with one.
VIOLATED = run('0.2.20', [(0, 0, 0), (0, 0, 7)]) + (
    'INVARIANT flip without write: f=31 skip=0 smp=0 pal_pending=0 -- a buffer is being shown that '
    'nobody wrote (docs/446)\n')
# A core too old to report the field. Saying "flipnw=0" here would be an invariant declared satisfied from
# a number that is not in the file.
SILENT = ('core 0.2.18: boot marker\n' +
          ''.join('view=1 320x200 sig=a784676e chg=156\n' +
                  SPLIT_NOFLDW.format(wall=1000 * (i + 1), fr=30 * (i + 1), cap=c, nf=nf)
                  for i, (c, nf) in enumerate([(12, 5), (20, 9)])))

def verdict(text):
    with tempfile.NamedTemporaryFile("w", suffix=".log", delete=False) as f:
        f.write(text); path = f.name
    out = subprocess.run([os.path.join(os.path.dirname(__file__), "logreport.py"), path],
                         capture_output=True, text=True).stdout
    os.unlink(path)
    m = re.search(r"## docs/446 invariant: (.*)", out)
    return m.group(1) if m else "<no invariant section>"

CASES = [(HEALTHY, "OK"), (VIOLATED, "VIOLATED"), (SILENT, "NOT MEASURED")]
bad = 0
for text, want in CASES:
    got = verdict(text)
    ok = got.startswith(want)
    bad += 0 if ok else 1
    print("%-11s want %-12s got %-72s %s" % (want.lower(), want, got, "[PASS]" if ok else "[FAIL]"))
print("\ntest_logreport: %d/%d" % (len(CASES) - bad, len(CASES)))
sys.exit(1 if bad else 0)
