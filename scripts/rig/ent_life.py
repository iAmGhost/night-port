#!/usr/bin/env python3
"""ent_life.py -- do the two sides differ in how many entities appear, or in how long they live?

docs/161 §5 measured that the port carries more live records than the shipped game over the same anchored
route (985 vs 827 record-samples; 659 vs 508 of them type-1 enemies), and pop_align.py pinned the first
divergence (t=174875 ms: shipped 3 live, port 4, the extra one a type-2 record with the +0x30 dying flag set).
Two causes explain that difference and they need different fixes:

  (a) the port SPAWNS more entities            -> the gates in fn_03e20f / the per-room cap g_tab_528b4
  (b) entities LIVE LONGER in the port         -> damage, hp drain, or the removal path (fn_0112fd, +0x30)

This script separates them by identity-linking records across samples (docs/115's rule: nearest predecessor of
the same type within 48 px) and reporting, per side, the number of distinct identities, their lifetimes in
milliseconds, and how they end.  Lifetimes are quoted in ms, never in samples, because the shipped dump is
change-triggered (~5 Hz effective) while the port dumps at 10 Hz (docs/115's cadence warning).

Field offsets are docs/125's constructor layout: +0x00 type byte, +0x01 x, +0x09 y, +0x17 state, +0x29 hp
(word), +0x30 dying flag.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

DS_LO = 0x50000
MAX_STEP = 48          # docs/115: identity link radius in px


def s32(v):
    return None if v is None else (v - 0x100000000 if v >= 0x80000000 else v)


def field(blk, off, w):
    if off + w > len(blk):
        return None
    return int.from_bytes(blk[off:off + w], "little")


def track_debt(prev, blk):
    """Value history for the two damage-accumulation words docs/118 §8 identified as the gate that decides
    whether damage reaches +0x29: the debt word at +0x23 and the +0x56 accumulator."""
    out = {}
    for off, w in ((0x23, 2), (0x56, 4)):
        hist = list((prev or {}).get(off, []))
        v = field(blk, off, w)
        v = s32(v) if w == 4 else (v - 0x10000 if v is not None and v >= 0x8000 else v)
        hist.append((None, v))
        out[off] = hist
    return out


def unfilled(blk):
    return len(blk) < 0x40 or blk[0] >= 0x40 or not any(blk[:0x40])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--anchor", default="0x63e14:67")
    a = ap.parse_args()
    ds, val = (int(x, 0) for x in a.anchor.split(":"))

    O, pcl = load_orig(a.orig)
    P, pcells = load_port(a.port)
    names = [c["name"] if isinstance(c, dict) else c for c in pcl]
    cmap = {c["name"]: c for c in pcells}

    def blocks_o(sample):
        hb = sample[2] if len(sample) > 2 else b""
        if not hb:
            return []
        blk = len(hb) // len(names)
        return [hb[names.index(n) * blk:(names.index(n) + 1) * blk]
                for n in names if n.startswith("ent")]

    def blocks_p(sample):
        blob = sample[1]
        out = []
        for n, c in cmap.items():
            if not n.startswith("ent"):
                continue
            ro = c.get("rec_offset")
            out.append(blob[ro:ro + c.get("rec_bytes", 0x160)] if ro is not None else b"")
        return out

    def track(side, blocks_of, tag):
        live = []                      # entities alive in the current sample
        seen = {}                      # id -> latest state of every identity ever tracked (docs/161 §7):
                                       # censusing `live` alone would only ever describe the LAST sample
        tstamps = []                   # every sample time that had live records
        nxt = 0
        n_samples = 0
        flags = collections.defaultdict(collections.Counter)      # type -> +0x30 value -> sample count
        hps = collections.defaultdict(collections.Counter)        # type -> +0x29 value -> sample count
        states = collections.defaultdict(collections.Counter)     # type -> +0x17 state -> sample count
        for s in sorted(side, key=lambda x: x[0]):
            rows = [b for b in blocks_of(s) if not unfilled(b)]
            if not rows:
                # An empty sample means the list was EMPTY: every predecessor is gone.  Skipping the sample
                # instead (the first version) left `prev` standing for an arbitrary stretch of time and let a
                # record match another one across a 62-second gap -- which is how a 5-sample identity came to
                # look like a 62-second lifetime.  docs/115's proximity caveat, in the same family.
                live = []
                continue
            n_samples += 1
            t = s[0]
            tstamps.append(t)
            for b in rows:
                flags[b[0]][field(b, 0x30, 1)] += 1
                states[b[0]][s32(field(b, 0x17, 4))] += 1
                hps[b[0]][field(b, 0x29, 2)] += 1
            prev = list(live)
            live = []
            used = set()
            for b in rows:
                ty, x, y = b[0], s32(field(b, 0x01, 4)), s32(field(b, 0x09, 4))
                st, hp, fl = s32(field(b, 0x17, 4)), field(b, 0x29, 2), field(b, 0x30, 1)
                best, bd = None, None
                for i, e in enumerate(prev):
                    if e is None or i in used or e["type"] != ty:
                        continue
                    if e["t1"] > t:
                        continue
                    d = max(abs((x or 0) - (e["x"] or 0)), abs((y or 0) - (e["y"] or 0)))
                    if d <= MAX_STEP and (bd is None or d < bd):
                        best, bd = i, d
                if best is None:
                    rec = dict(id=nxt, type=ty, t0=t, t1=t, n=1, maxhp=hp, hp=hp, state=st, flag=fl,
                               x=x, y=y, spawns=1, trace=[(t, hp)] if hp is not None else [],
                               states_seen=[st], debt=track_debt(None, b))
                    nxt += 1
                else:
                    used.add(best)
                    e = prev[best]
                    drops = e.get("drops", 0) + (1 if (hp is not None and e["hp"] is not None and hp < e["hp"]) else 0)
                    tr = list(e.get("trace") or [])
                    if hp is not None and (not tr or tr[-1][1] != hp):
                        tr.append((t, hp))
                    rec = dict(e, t1=t, n=e["n"] + 1, maxhp=max(e["maxhp"] or 0, hp or 0), hp=hp, drops=drops,
                               trace=tr,
                               state=st, flag=fl, x=x, y=y, spawns=0,
                               states_seen=list(e.get("states_seen") or []) + [st],
                               debt=track_debt(e.get("debt"), b))
                    prev[best] = None      # consumed
                live.append(rec)
            for e in live:
                seen[e["id"]] = e
            # An entity whose record is absent from this sample is GONE: carrying unmatched predecessors
            # forward would model disappearance as continuation and make every lifetime look infinite.
        # hp transition trace for any identity that ever carries hp (docs/125: +0x29 is a WORD and is the
        # field damage lands on).  Step SIZES are the interesting part: docs/118 §8 left the debt words
        # (+0x23 / +0x56) as the gate that decides whether damage reaches +0x29 at all.
        print(f"== {tag} hp transitions")
        for i, e in sorted(seen.items()):
            tr = e.get("trace") or []
            if len(tr) < 2:
                continue
            ch = [(t0, h0, t1, h1) for (t0, h0), (t1, h1) in zip(tr, tr[1:]) if h1 != h0]
            if not ch:
                continue
            steps = collections.Counter(h1 - h0 for _, h0, _, h1 in ch)
            print(f"   id={i} type={e['type']:#04x} hp steps: {sorted(steps.items(), key=lambda kv: kv[0])}")
            for t0, h0, t1, h1 in ch[:12]:
                print(f"        t={t0:>7} hp {h0} -> {h1} (d={h1 - h0:+})")

        by_type = collections.defaultdict(list)
        for e in seen.values():
            by_type[e["type"]].append(e)
        # Self-policing guard (docs/162 §11).  Links only ever form between adjacent samples, so an identity
        # that spans far more time than its sample count allows is a CONTRADICTION, not a long-lived entity --
        # the 62273 ms / 5-sample "record" that made me write and then retract a lifetime claim.  Flag it here
        # instead of discovering it by hand next time.
        spans = sorted(e["t1"] - e["t0"] for e in seen.values())
        gaps = sorted(b - a for a, b in zip(tstamps, tstamps[1:])) or [100]
        med_gap = gaps[len(gaps) // 2]
        bad = [e for e in seen.values()
               if e["n"] > 1 and (e["t1"] - e["t0"]) > (e["n"] + 1) * med_gap * 3]
        print(f"== {tag}: {n_samples} samples with live records, {nxt} distinct identities "
              f"(median sample gap {med_gap} ms)")
        if bad:
            print(f"   SELF-CONTRADICTION: {len(bad)} identities span more time than their link count permits "
                  f"-- lifetime numbers from this run are NOT usable:")
            for e in sorted(bad, key=lambda e: e["t1"] - e["t0"], reverse=True)[:5]:
                print(f"      id={e['id']} type={e['type']:#04x} span={e['t1'] - e['t0']} ms in {e['n']} samples")
        for ty, rows in sorted(by_type.items()):
            life = [(e["t1"] - e["t0"]) for e in rows]
            nsamp = [e["n"] for e in rows]
            ended = collections.Counter()
            for e in rows:
                ended["flag30" if e["flag"] else ("hp0" if (e["hp"] or 0) == 0 else "alive-at-end")] += 1
            life.sort()
            fl = flags[ty].most_common(8)
            print(f"        +0x30 flag census over live samples: {fl}")
            print(f"        +0x29 hp values: {hps[ty].most_common(8)}")
            dr = sorted(e.get("drops", 0) for e in rows)
            for lab, off, w in (("+0x23 debt word", 0x23, 2), ("+0x56 accumulator", 0x56, 4)):
                cc = collections.Counter()
                ntr = 0
                for e in rows:
                    seq = (e.get("debt") or {}).get(off, [])
                    for _, v in seq:
                        cc[v] += 1
                    for i in range(1, len(seq)):
                        if seq[i][1] != seq[i - 1][1]:
                            ntr += 1
                if cc:
                    print(f"        {lab}: value census {cc.most_common(6)}  transitions {ntr}")
            print(f"        hp-decrease events per identity: min={dr[0]} median={dr[len(dr)//2]} max={dr[-1]} "
                  f"total={sum(dr)}  (max hp seen {max((e['maxhp'] or 0) for e in rows)})")
            print(f"   type {ty:#04x}: {len(rows)} identities  lifetime ms: min={life[0]} "
                  f"median={life[len(life)//2]} max={life[-1]}  mean samples={sum(nsamp)/len(nsamp):.1f}")
            print(f"        how they leave the list: {dict(ended)}")
            long_lived = [e for e in rows if (e["t1"] - e["t0"]) > 5000 and ty != 0]
            if ty == 0x02 and long_lived:
                print(f"        type-2 records alive MORE than 5 s (shipped: never, docs/162):")
                for e in sorted(long_lived, key=lambda e: e["t1"] - e["t0"], reverse=True)[:6]:
                    st = collections.Counter(e.get("states_seen") or [])
                    print(f"          id={e['id']} t {e['t0']}..{e['t1']} ({e['t1'] - e['t0']} ms, "
                          f"{e['n']} samples) last state={e['state']:#x} flag={e['flag']} states={st.most_common(5)}")

    t_o = t_p = None
    po = cmap.get("g_dat_%05x" % ds, {}).get("offset")
    for s in sorted(O, key=lambda x: x[0]):
        o = ds - DS_LO
        if int.from_bytes(s[1][o:o + 4], "little") == val:
            t_o = s[0]
            break
    for s in sorted(P, key=lambda x: x[0]):
        if int.from_bytes(s[1][po:po + 4], "little") == val:
            t_p = s[0]
            break
    if t_o is None or t_p is None:
        sys.exit(f"anchor {ds:#x} == {val} not found")
    shift = t_o - t_p
    print(f"anchor {ds:#x} == {val}: shipped t={t_o} ms, port clock shifted by {shift} ms")
    track(O, blocks_o, "ORIG")
    print()
    track([(t + shift, *rest) for t, *rest in P], blocks_p, "PORT")


if __name__ == "__main__":
    main()
