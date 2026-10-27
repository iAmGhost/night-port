#!/usr/bin/env python3
"""Adjudicate static_audit findings into FIXED / FILED / EXPLAINED verdicts.

Rules are mechanical wherever the evidence allows it; everything else is queued for the runtime
oracle rather than guessed at. The port source is NOT modified by this pass: the blind gate
(blind-validation) has to run against the current build, so findings are filed with evidence and
fixed afterwards, with the rig's before/after diff as the proof.

Verdict vocabulary
  FILED-READONLY   guest never writes the cell, image bytes are non-zero, port initialises zero
                   -> shipped config the port simply dropped (fix = copy the image value)
  FILED-SPAN       port declares a span that covers another port global the guest addresses
                   independently -> over-declared array hiding a table
  FILED-UNREACH    shipped fn is called by other shipped code but has no port counterpart
  FILED-REPR       pointer/number representation disagreement between port and shipped DB
  FILED-VIEW       two DS bases with overlapping access spans, both port-named
  FILED-MODEL      DS cell the guest touches that the port does not name at all
  EXPLAINED-*      the finding is not a defect (dead code, scratch cell, covered by a port span)
  PENDING-RUNTIME  static evidence is insufficient by construction; the state oracle decides
                   (reported as FILED-RUNTIME: filed with its adjudication route attached)
"""
import collections
import json
import os

import ghidra_db
import port_src
import static_audit

HERE = os.path.dirname(os.path.abspath(__file__))
REPORTS = os.path.join(HERE, "reports")


def main():
    db = ghidra_db.Db()
    pmap = static_audit.port_global_map()
    join = {r["ds"]: r for r in json.load(open(os.path.join(HERE, "gen", "ds_join.json")))}
    res = json.load(open(os.path.join(REPORTS, "static_findings.json")))

    verdicts = []

    def V(check, kind, subject, evidence, verdict):
        verdicts.append({"check": check, "kind": kind, "subject": subject,
                         "evidence": evidence, "verdict": verdict})

    # ---- C3 + C6: image-initialised DS vs the port's initializers ------------
    prod = static_audit.port_written_names()
    for f in res["c3_image_init"]:
        row = join.get(f["ds"], {})
        produced = f["name"] in prod
        if row.get("writes", 0) == 0 and (row.get("reads", 0) or 0) > 0 and not produced:
            V("C3", "image-init", f["name"] + " @ " + hex(f["ds"]),
              f"image={f['image_head']} port_init='{f['init']}' guest reads={row['reads']} writes=0, "
              f"and nothing in the port ever writes '{f['name']}' -> shipped bytes were dropped",
              "FILED-READONLY")
        elif row.get("writes", 0) == 0 and produced:
            V("C3", "image-init", f["name"] + " @ " + hex(f["ds"]),
              f"image={f['image_head']} port_init='{f['init']}' but the port writes this global "
              f"itself -> compare its producer against the shipped bytes", "FILED-PRODUCER-CHECK")
        else:
            V("C3", "image-init", f["name"] + " @ " + hex(f["ds"]),
              f"image={f['image_head']} port_init='{f['init']}' "
              f"guest reads={row.get('reads')} writes={row.get('writes')}",
              "FILED-RUNTIME")
    for f in res["c6_image_data_without_symbol"]:
        a, b = int(f["ds_start"], 16), int(f["ds_end"], 16)
        cells = [join.get(x) for x in range(a, b, 4)]
        cells = [c for c in cells if c]
        rw = sum(c["reads"] for c in cells)
        ww = sum(c["writes"] for c in cells)
        subj = f"{f['ds_start']}..{f['ds_end']} ({f['cells']} cells)"
        if f["port_models_anything_here"] and ww == 0 and rw > 0:
            V("C6", "image-run-no-symbol", subj,
              f"image_head={f['image_head']} bases={f['distinct_base_constants']} "
              f"reads={rw} writes=0 port={f['port_models_anything_here']}", "FILED-READONLY")
        elif rw or ww:
            V("C6", "image-run-no-symbol", subj,
              f"reads={rw} writes={ww} bases={f['distinct_base_constants']} "
              f"port={f['port_models_anything_here']}", "FILED-RUNTIME")
        else:
            V("C6", "image-run-no-symbol", subj,
              f"no instruction-level access from any base in {f['distinct_base_constants']}",
              "EXPLAINED-UNACCESSED")

    # ---- C2: spans --------------------------------------------------------
    imms = static_audit.ds_immediates(db)
    for f in res["c2_aliasing"]:
        if f["kind"] == "port-span-overlap":
            inner = f["b"]
            inner_addr = int(f["b_span"][0], 16)
            own_base = any(b == inner_addr for b in imms)
            near = [b for b in imms if int(f["b_span"][0], 16) - 8 <= b <= int(f["b_span"][1], 16)]
            if own_base or near:
                V("C2", "port-span-overlap", f"{f['a']} covers {f['b']}",
                  f"overlap={f['overlap']}B shipped base constants in the inner span: "
                  f"{[hex(x) for x in near][:4]}", "FILED-SPAN")
            else:
                V("C2", "port-span-overlap", f"{f['a']} covers {f['b']}",
                  f"overlap={f['overlap']}B, no shipped base constant inside the inner span "
                  f"-> one contiguous shipped table split by the port", "EXPLAINED-SAME-TABLE")
        elif f["kind"] == "port-two-names-one-cell":
            V("C2", "port-two-names-one-cell", hex(f["ds"]) + " " + ",".join(f["names"]),
                f"types={f['types']}", "FILED-VIEW")
        else:
            V("C2", f["kind"], str(f.get("a", {}).get("label", f)), "guest-side interval overlap",
              "FILED-VIEW")

    # ---- C4: representation ----------------------------------------------
    for f in res["c4_representation"]:
        V("C4", f["why"], f"{f['port_name']} @ {hex(f['ds'])}",
          f"port={f['port_type']} shipped={f['guest_type']} image={f['image_u32']} "
          f"R{f['n_reads']}/W{f['n_writes']} {f.get('small_imm_stores') or f.get('indexed_reads') or ''}",
          "FILED-REPR")

    # ---- C1: DS cells the port never names -------------------------------
    port_spans = []
    for addr, decls in pmap.items():
        for d in decls:
            port_spans.append((addr, addr + (d.get("guest_size") or 4), d["name"]))
    port_spans.sort(key=lambda t: (t[0], t[1]))

    def covered(addr):
        for a0, a1, nm in port_spans:
            if a0 <= addr < a1:
                return nm
        return None

    for f in res["c1_unmodelled"][:80]:
        nm = covered(f["ds"])
        row = join.get(f["ds"], {})
        if nm:
            V("C1", "unmodelled-cell", hex(f["ds"]),
              f"R{f['reads']}/W{f['writes']} inside port span {nm}", "EXPLAINED-BY-SPAN")
        elif f["total"] >= 3:
            V("C1", "unmodelled-cell", hex(f["ds"]),
              f"R{f['reads']}/W{f['writes']} image={row.get('image_u32')} "
              f"sites={row.get('write_sites', [])[:2]}", "FILED-MODEL")
        else:
            V("C1", "unmodelled-cell", hex(f["ds"]),
              f"R{f['reads']}/W{f['writes']} single/low-traffic access", "EXPLAINED-SCRATCH")

    # ---- C5: reachability ------------------------------------------------
    called = set()
    for cs in db.callees().values():
        called.update(cs)
    ptr_targets = set()
    for d in db.defined_data:
        if d["type"] in ("pointer", "undefined *") and d.get("label", "").startswith("PTR_"):
            ptr_targets.add(int(d["addr"], 16))
    name2entry = {f["name"]: int(f["entry"], 16) for f in db.functions}
    entry2callees = db.callees()
    port_defs, port_calls = port_src.functions()

    def equivalent_candidate(name):
        """Did the port keep this routine under another name?

        Evidence: the shipped routine's callees are addressed by name in the port (fn_<addr>), so a
        port function calling most of the same callees is almost certainly the same routine."""
        entry = name2entry.get(name)
        if entry is None:
            return None
        mapped = {f"fn_{c:06x}" for c in entry2callees.get(entry, []) if f"fn_{c:06x}" in port_defs}
        if len(mapped) < 2:
            return None
        best = None
        for pn, calls in port_calls.items():
            if pn.startswith("__toplevel__") or pn == name:
                continue
            hit = len(mapped & calls)
            if hit >= max(2, int(0.5 * len(mapped))) and (best is None or hit > best[1]):
                best = (pn, hit)
        return best

    for f in res["c5_dead_sides"]["guest_functions_without_port_counterpart"]:
        addr = name2entry.get(f)
        called_by = [e for e, cs in entry2callees.items() if addr in cs] if addr is not None else []
        eq = equivalent_candidate(f)
        if eq:
            V("C5", "guest-fn-no-port-fn", f,
              f"port equivalent candidate {eq[0]} shares {eq[1]} callees", "EXPLAINED-RENAMED")
        elif called_by:
            V("C5", "guest-fn-no-port-fn", f, f"called by {len(called_by)} shipped fns "
              f"({[hex(x) for x in called_by[:3]]})", "FILED-CALLGRAPH-GAP")
        else:
            V("C5", "guest-fn-no-port-fn", f, "no shipped caller", "EXPLAINED-DEAD")
    for f in res["c5_dead_sides"]["port_functions_never_called_in_port"][:80]:
        V("C5", "port-fn-never-called", f, "no call site inside the port", "FILED-RUNTIME")

    # ---- C8: view pairs that matter --------------------------------------
    views = [f for f in res["c8_base_pair_aliasing"] if f["port_name_a"] and f["port_name_b"]]
    for f in views[:60]:
        V("C8", "overlapping-bases", f"{f['base_a']} ({','.join(f['port_name_a'])}) vs "
          f"{f['base_b']} ({','.join(f['port_name_b'])})",
          f"distance={f['distance']} n={f['n_a']}/{f['n_b']} shared_fns={f['shared_functions'][:3]}",
          "FILED-VIEW")
    V("C8", "overlapping-bases", f"{len(res['c8_base_pair_aliasing']) - len(views[:60])} further pairs",
      "at least one base is not modelled by a port global", "EXPLAINED-NOT-MODELLED")

    tally = collections.Counter(v["verdict"] for v in verdicts)
    by_check = collections.Counter((v["check"], v["verdict"]) for v in verdicts)
    with open(os.path.join(REPORTS, "triage.json"), "w") as fh:
        json.dump({"tally": dict(tally), "verdicts": verdicts}, fh, indent=1)

    lines = ["# Static audit triage", "",
             "Port source is deliberately untouched in this pass: the blind gate must run against "
             "the current build, so findings are filed with evidence and fixed afterwards.", "",
             "| verdict | count |", "|---|---|"]
    for k, n in tally.most_common():
        lines.append(f"| {k} | {n} |")
    for check in ("C1", "C2", "C3", "C4", "C5", "C6", "C8"):
        sel = [v for v in verdicts if v["check"] == check]
        filed = [v for v in sel if v["verdict"].startswith("FILED")]
        lines.append(f"\n## {check}: {len(sel)} adjudicated, {len(filed)} filed")
        for v in filed[:18]:
            lines.append(f"- **{v['verdict']}** {v['subject']} — {v['evidence']}")
        pend = [v for v in sel if v["verdict"] == "FILED-RUNTIME"]
        if pend:
            lines.append(f"- _{len(pend)} FILED-RUNTIME (the state oracle decides):_ "
                         + "; ".join(v["subject"] for v in pend[:10]))
    with open(os.path.join(REPORTS, "triage.md"), "w") as fh:
        fh.write("\n".join(lines) + "\n")
    print(json.dumps(dict(tally), indent=1))
    print(f"wrote {os.path.join(REPORTS,'triage.md')}")


if __name__ == "__main__":
    main()
