#!/usr/bin/env python3
"""Hits per attack make, measured from the dump only (docs/306).

The user's report (2026-11-02): one attack make lands several hits on the same enemy at once.  The oracle
counts, per attack make, how many separate hit events one and the same victim record absorbs.

Why not hp: +0x29 never changes on the canonical route (0 decreases in 1500 samples of /tmp/rig/rb15), so the
dump now carries the two fields the shipped's hit path actually touches -- +0x23 (the term the port's own DMG
witness at f.c:25457 calls `debt`) and +0x56 (the damage pool).

A "make" is a transition of the player record's +0x17 into an attack state (0x33..0x36 = 51..54) from a state
outside that set; the window runs while the player stays in the set.  A "hit event" is a fall of a victim's
+0x23 between two consecutive samples in which that victim is present.

Identity: (record offset, kind, span).  A change of kind or span at the same offset starts a new identity, so
a re-used slot (docs/247, docs/251) is not counted as the same enemy.  Per docs/246, slot reuse can also make
+0x23 look like it fell when a slot was reassigned: the quoted numbers therefore include the *net* lowest
+0x23 per identity, not only event counts.

  python3 scripts/rig/hits_per_attack.py /tmp/rig/h306/head1 /tmp/rig/h306/base1
"""
import json
import os
import sys
from collections import Counter, defaultdict

ATTACK = {0x33, 0x34, 0x35, 0x36}


def i16(v):
    return ((v & 0xFFFF) ^ 0x8000) - 0x8000


def load(d):
    p = os.path.join(d, "port", "records.jsonl")
    if not os.path.exists(p):
        p = os.path.join(d, "records.jsonl")
    if not os.path.exists(p):
        sys.exit(f"{d}: no records.jsonl")
    out = []
    for l in open(p):
        r = json.loads(l)
        for e in r.get("ents") or []:
            # the port's own DMG witness reads the debt term as a WORD (f.c:25457 uses ua16 at +0x23), so the
            # dump's 32-bit read is normalized back to that width here.
            e["d23"] = i16(e.get("d23", 0))
            e["hp"] = i16(e.get("hp", 0))
        out.append(r)
    return out


def census(d):
    recs = load(d)
    live = {}                      # identity -> last sample
    inst = 0                       # identity-instance counter
    hits = defaultdict(list)       # (identity, window) -> [tick, old, new]
    makes = []                     # (start_tick, end_tick, states)
    win_of = {}                    # tick -> window index
    cur = None
    for r in recs:
        tick = r["tick"]
        po = r.get("player")
        ents = r.get("ents") or []
        pst = None
        for e in ents:
            if po is not None and e["o"] == po:
                pst = e["st"] & 0x7FFFFFFF
                break
        inatk = pst in ATTACK
        if inatk:
            if cur is None:
                cur = [tick, tick, []]
                makes.append(cur)
            cur[1] = tick
            cur[2].append(pst)
            win_of[tick] = len(makes) - 1
        else:
            cur = None

        ident_now = set()
        for e in ents:
            base = (e["o"], e["type"], e["n"])
            if base in live and (live[base]["kind"] != e["type"] or live[base]["n"] != e["n"]):
                del live[base]                      # slot rewritten whole: new enemy
            if base not in live:
                inst += 1
                live[base] = dict(kind=e["type"], n=e["n"], d23=e["d23"], pool=e["pool"],
                                  tick=tick, lo=e["d23"], seen=1)
                ident_now.add(base)
                continue
            L = live[base]
            L["seen"] += 1
            ident_now.add(base)
            w = win_of.get(tick)
            if e["d23"] < L["d23"] and w is not None:
                hits[(base, w)].append((tick, L["d23"], e["d23"]))
            L["d23"] = e["d23"]
            L["pool"] = e["pool"]
            L["lo"] = min(L["lo"], e["d23"])
        for k in [k for k in live if k not in ident_now]:
            del live[k]                             # left the entity list: identity ends

    per_window = defaultdict(int)
    for (ident, w), ev in hits.items():
        per_window[w] += len(ev)
    dist = Counter(len(v) for v in hits.values())
    multi = {k: v for k, v in hits.items() if len(v) >= 2}
    return dict(recs=len(recs), makes=len(makes), identities=inst, pairs=len(hits),
                events=sum(len(v) for v in hits.values()), dist=dist, multi=multi,
                per_window=per_window, hits=hits, makes_list=makes)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    runs = {}
    for d in sys.argv[1:]:
        runs[d] = census(d)
        c = runs[d]
        print(f"== {d}")
        print(f"   samples={c['recs']}  attack makes={c['makes']}  enemy identities={c['identities']}")
        print(f"   (make,victim) pairs with >=1 hit={c['pairs']}  hit events={c['events']}")
        print(f"   hits per (make,victim) distribution: {sorted(c['dist'].items())}")
        print(f"   makes that hit the same victim >=2 times: {len({w for (_, w) in c['multi']})}")
        mx = sorted(c["multi"].items(), key=lambda kv: -len(kv[1]))[:5]
        for (ident, w), ev in mx:
            mk = c["makes_list"][w]
            seq = ", ".join(f"t{t}:{a}->{b}" for t, a, b in ev)
            print(f"     victim o={ident[0]} kind={ident[1]} span={ident[2]} window ticks {mk[0]}..{mk[1]} "
                  f"states={mk[2]} hits={len(ev)}  {seq}")
    print("== summary")
    print(f"{'run':34} {'makes':>6} {'events':>7} {'pairs':>6} {'multi':>6} {'max/victim':>10}")
    for d, c in runs.items():
        multi = len({w for (_, w) in c['multi']})
        mx = max(c['dist']) if c['dist'] else 0
        print(f"{d:34} {c['makes']:>6} {c['events']:>7} {c['pairs']:>6} {multi:>6} {mx:>10}")


if __name__ == "__main__":
    main()
