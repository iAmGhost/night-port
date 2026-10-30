#!/usr/bin/env python3
"""uncalled_port_fns.py -- port functions with NO call site, and whether the shipped binary can reach them.

The audit already asks the guest->port direction (which shipped functions the fuzz census never enters, docs/120
§12).  It never asked the reverse question this turn needed: the port *defines* `fn_04933a` and `fn_04589e` and
nothing ever calls them (docs/179), while `call_sites.py` reports zero direct callers in the shipped binary too.
That combination is a signature, not a coincidence: the shipped program reaches these through a function-pointer
table, so a port that models no table entry can never reach them either, and every cell they publish stays empty
(DS 0x53451, 0x64090, 0x64094).

Usage: uncalled_port_fns.py [--json out.json]
"""
import json, os, re, subprocess, sys, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
SRC = [os.path.join(ROOT, "port", "src", d) for d in ("f", "app", "gen")]


def port_functions():
    """Every function the port defines, with how many times its name appears in the sources."""
    hits = collections.Counter()
    defined = {}
    for d in SRC:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.endswith(".c"):
                continue
            p = os.path.join(d, fn)
            txt = open(p, errors="ignore").read()
            for m in re.finditer(r"\b([A-Za-z_][A-Za-z0-9_]{2,})\s*\([^;{}]*\)\s*\{", txt):
                defined.setdefault(m.group(1), []).append("%s:%d" % (fn, txt[:m.start()].count("\n") + 1))
            for m in re.finditer(r"\b([A-Za-z_][A-Za-z0-9_]{2,})\s*\(", txt):
                hits[m.group(1)] += 1
    return defined, hits


def shipped_callers(name):
    """How many CALL edges the shipped function graph has into a function, by name."""
    out = subprocess.run([sys.executable, os.path.join(HERE, "call_sites.py"), "--target", "0x" + name[3:]],
                         capture_output=True, text=True, cwd=HERE).stdout
    m = re.search(r"callers of 0x[0-9a-f]+: (\d+)", out)
    return int(m.group(1)) if m else None


def main():
    defined, hits = port_functions()
    rows = []
    for name, where in sorted(defined.items()):
        if not name.startswith("fn_"):
            continue
        total = hits.get(name, 0)
        # a definition line itself counts as one occurrence; zero calls means total == 1
        if total > 1:
            continue
        rows.append({"port_fn": name, "defined_at": where[0], "occurrences": total,
                     "shipped_direct_callers": shipped_callers(name)})
    print("port functions with no call site: %d" % len(rows))
    for r in rows:
        print("  %-14s %-16s shipped direct callers: %s"
              % (r["port_fn"], r["defined_at"], r["shipped_direct_callers"]))
    if "--json" in sys.argv:
        p = sys.argv[sys.argv.index("--json") + 1]
        json.dump(rows, open(p, "w"), indent=1)
        print("wrote %s" % p)


if __name__ == "__main__":
    main()
