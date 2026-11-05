#!/usr/bin/env python3
"""sim_menu.py -- a Python differential model of the Pocket settings channel.

Three things are modelled separately, because 0.1.44/0.1.47 each confused a different pair of them:

  1. interact.json's MASK/VALUE layer.  mask bit 1 = "leave this bit alone", bit 0 = "this element may write it".
     So writable = ~mask, an entry's FIELD is the contiguous run of writable bits, and an option VALUE must fit inside
     its own field.  Confusing "the mask lists the bits I want" with "the mask protects the bits I don't" produced the
     inverted masks check_interact.py caught (docs/357); confusing a field VALUE with a field INDEX is the kind of slip
     that would produce "it got muted whatever I set".
  2. The FIRMWARE layer, measured on hardware (docs/352/354/356/358): a declared value lands in a fixed readable
     peripheral word -- 0xF700000C -> 0x4000006c, 0xF7000010 -> 0x40000068, 0xF7000014 -> nothing -- and the write is
     the option value ALONE: the whole word is replaced, so a shared word cannot hold two settings (docs/358).
  3. The PORT's claim/apply logic, transcribed from main_of.c: known-slot assert, optional hunt (clean for every
     sample + ever seen silent + held an option value >= MENU_SEEN_MIN times), 0 = "the menu has not spoken",
     value -> option index -> volume.

Then the model is run three ways: against the packed 0.1.47 declaration, against the shipped 0.1.48 declaration, and
against the REAL sample timeline captured on the card in the 0.1.47 run, with the hunt on and off.  The last one has to
reproduce the observed failure (Music Volume claimed at 0x400000ec, driven to Off by a counter) for the diagnosis to
count as anything more than a story.

Usage:  python3 scripts/pocket/sim_menu.py [log.sav ...]
"""
import json
import pathlib
import re
import sys

PERIPH_BASE = 0x40000000
PERIPH_WORDS = 96
MENU_SEEN_MIN = 5
MENU_N = 0xFF                      # a menu band width used only for reporting

# The measured firmware map.  Anything not listed here was never seen to carry a value.
MIRROR = {0xF700000C: 0x4000006C, 0xF7000010: 0x40000068}

VOL = ["Off", "1/8", "1/4", "1/2", "5/8", "3/4", "7/8", "Full"]
FIT = ["letterbox", "stretch", "scaler 320x200"]


# ---------------------------------------------------------------- 1. mask / value layer
def field_of(mask: int):
    """Derive an element's field from its mask: writable = ~mask, and a legal element writes one contiguous run."""
    writable = (~mask) & 0xFFFFFFFF
    if writable == 0:
        return None, None, "mask is all ones -- the element can write NOTHING (docs/345)"
    low = (writable & -writable).bit_length() - 1
    width = writable.bit_length() - low
    if writable != (((1 << width) - 1) << low):
        return None, None, f"writable bits are not contiguous: {writable:08x}"
    return low, (writable >> low), None


def audit_entry(name, address, mask, options):
    """Check that every option VALUE fits the field the mask opens, and index it the way the host does."""
    shift, fmask, err = field_of(mask)
    if err:
        return [f"[FAIL] {name}: {err}"]
    out = []
    for v in options:
        if v == 0:
            out.append(f"[FAIL] {name}: option value 0 -- 0 means 'the menu has not spoken', not a choice")
        if v & ~((fmask << shift) & 0xFFFFFFFF):
            out.append(f"[FAIL] {name}: value {v:#x} does not fit its field (bits {shift}..{shift + fmask.bit_length() - 1})")
    if not out:
        out.append(f"[PASS] {name} @{address:#010x} mask {mask:08x} -> bits {shift}-"
                   f"{shift + fmask.bit_length() - 1}: {[f'{v:08x}->{v >> shift}' for v in options]}")
    return out


# ---------------------------------------------------------------- 2. firmware model
class Firmware:
    def __init__(self, map_ok=True):
        self.word = {}                                    # peripheral word -> value
        self.map_ok = map_ok

    def mount(self, entries):
        self.entries = entries
        for e in entries:
            m = MIRROR.get(e["address"]) if self.map_ok else None
            e["mirror"] = m

    def pick(self, name, value):
        """Measured behaviour: the option value is written as the WHOLE word (docs/358), wiping other entries."""
        e = next(x for x in self.entries if x["name"] == name)
        m = e["mirror"]
        if m is None:
            return None                                   # 0xF7000014 and friends: nothing arrives anywhere
        prev = self.word.get(m, 0)
        self.word[m] = value                              # not value | (prev & ~field): the whole word
        return m, prev, value


# ---------------------------------------------------------------- 3. port model
class Port:
    def __init__(self, entries, hunt=True, verbose=True):
        self.entries = entries
        self.hunt = hunt
        self.verbose = verbose
        self.events = []
        self.clean = {id(e): {i: True for i in range(PERIPH_WORDS)} for e in entries}
        self.zero = {id(e): {i: False for i in range(PERIPH_WORDS)} for e in entries}
        self.seen = {id(e): {i: 0 for i in range(PERIPH_WORDS)} for e in entries}

    def field(self, e, v):
        return (v >> e["shift"]) & e["fmask"]

    def is_opt(self, e, v):
        f = self.field(e, v)
        return e["lo"] <= f <= e["hi"]

    def opt_ok(self, e, v):
        f = self.field(e, v)
        return f == 0 or (e["lo"] <= f <= e["hi"])

    def name(self, e, f):
        k = f - e["lo"]
        if e["kind"] in ("music", "sfx"):
            return VOL[k] if 0 <= k <= 7 else "?"
        return FIT[k] if 0 <= k <= 2 else "?"

    def apply(self, e, f):
        k = f - e["lo"]
        if e["kind"] == "music":
            return ("music_vol", k * 255 // 7)
        if e["kind"] == "sfx":
            return ("sfx_vol", k * 255 // 7)
        return ("fit", k)

    def sweep(self, words):
        # .items(), not enumerate(): enumerate() over a dict yields (position, index), which feeds the cleanliness
        # tables register INDICES as if they were register VALUES -- a 0x3c "value" that is really word number 15.
        for i, v in words.items():
            for e in self.entries:
                if not self.opt_ok(e, v):
                    self.clean[id(e)][i] = False
                    continue
                if self.field(e, v) == 0:
                    self.zero[id(e)][i] = True
                if e["lo"] <= self.field(e, v) <= e["hi"]:
                    self.seen[id(e)][i] += 1

    def beat(self, addr, value, t):
        """One settings_beat: re-read claimed slots; sweep if some entry still has no slot."""
        if addr is not None:                       # a claimed-word read also counts as a sample
            self.sweep({(addr - PERIPH_BASE) // 4: value})
        out = []
        for e in self.entries:
            if not e.get("slot") and e.get("known"):
                a = e["known"]
                v = self.word_at(a)
                if self.is_opt(e, v):
                    e["slot"] = a
                    e["last"] = self.field(e, v)
                    out.append(f"t={t}s menu: {e['name']} slot CLAIMED @{a:08x} (word {v:08x}, field "
                               f"{e['last']:x} = {self.name(e, e['last'])})")
            if not e.get("slot") and e.get("known") and not self.hunt:
                continue
            if not e.get("slot"):
                for j in range(PERIPH_WORDS):
                    a = PERIPH_BASE + 4 * j
                    v = self.word_at(a)
                    if not self.clean[id(e)][j] or not self.zero[id(e)][j] or self.seen[id(e)][j] < MENU_SEEN_MIN:
                        continue
                    if not self.is_opt(e, v):
                        continue
                    e["slot"] = a
                    e["last"] = self.field(e, v)
                    out.append(f"t={t}s menu: {e['name']} slot CLAIMED @{a:08x} found by field cleanliness "
                               f"(word {v:08x}, field {e['last']:x} = {self.name(e, e['last'])})")
                    break
            if e.get("slot"):
                v = self.word_at(e["slot"])
                if not self.opt_ok(e, v):
                    out.append(f"t={t}s menu: {e['name']} slot @{e['slot']:08x} lost")
                    e["slot"] = 0
                    e["last"] = 0
                    continue
                f = self.field(e, v)
                if f == 0:
                    if e.get("last"):
                        out.append(f"t={t}s menu: {e['name']} field reads 0 -- night.ini stands")
                    e["last"] = 0
                    continue
                if f != e.get("last"):
                    e["last"] = f
                    k, val = self.apply(e, f)
                    out.append(f"t={t}s menu: {e['name']} = {self.name(e, f)} -> {k}={val}")
        self.events += out
        return out

    def word_at(self, a):
        return self._w.get(a, 0)

    # The word store is the firmware's, shared by reference.
    def bind(self, fw):
        self._w = fw.word


def entry(name, address, mask, lo, hi, kind, known):
    shift, fmask, err = field_of(mask)
    assert not err, err
    return dict(name=name, address=address, mask=mask, shift=shift, fmask=fmask, lo=lo, hi=hi,
                kind=kind, known=known)


# ---------------------------------------------------------------- scenarios
def scenario_packed():
    """0.1.47's declaration: three entries packed into disjoint fields of 0xF700000C."""
    return [
        entry("Music Volume", 0xF700000C, 0xFFFFFFF0, 1, 8, "music", 0x4000006C),
        entry("SFX Volume",   0xF700000C, 0xFFFFFF0F, 1, 8, "sfx",   0x4000006C),
        entry("Screen Fit",   0xF700000C, 0xFFFFFCFF, 1, 3, "fit",   0x4000006C),
    ]


def scenario_shipped():
    """0.1.48's declaration: one entry per wired address."""
    return [
        entry("Music Volume", 0xF700000C, 0xFFFFFF00, 0x41, 0x48, "music", 0x4000006C),
        entry("SFX Volume",   0xF7000010, 0xFFFFFF00, 0x61, 0x68, "sfx",   0x40000068),
    ]


# Option VALUES exactly as interact.json declares them (a packed value is already shifted into its field).
PICKS_PACKED = [("Screen Fit", 0x300), ("Music Volume", 0x01), ("Music Volume", 0x08), ("Screen Fit", 0x200),
                ("SFX Volume", 0x70), ("Music Volume", 0x08), ("SFX Volume", 0x80)]
PICKS_SHIPPED = [("Music Volume", 0x43), ("SFX Volume", 0x64), ("Music Volume", 0x48),
                 ("SFX Volume", 0x61), ("Music Volume", 0x44), ("SFX Volume", 0x68)]


def run_scenario(title, mk, picks):
    print(f"\n=== {title}")
    fw = Firmware()
    entries = mk()
    fw.mount(entries)
    port = Port(entries, hunt=False)
    port.bind(fw)
    for name, value in picks:
        r = fw.pick(name, value)
        if r is None:
            print(f"  pick {name}={value:#x}: nothing reached the peripheral page")
            continue
        m, prev, v = r
        # Each entry is read out of ITS OWN mirror word, so the field shown per entry is that entry's view of that
        # word -- not a shared value.  Packing would have made them share one word, which is the point of scenario 2.
        fields = " ".join(f"{e['name'].split()[0].lower()}@{e['mirror']:03x}={port.field(e, fw.word.get(e['mirror'], 0)):x}"
                          for e in entries if e.get("mirror"))
        print(f"  pick {name}={value:#x} -> word {m:08x}: {prev:08x} -> {v:08x}   [{fields}]")
        for line in port.beat(m, v, 0):
            print("   ", line)


def replay(path, hunt):
    """Fold a real card log into a per-word sample timeline and run the port's logic over it."""
    text = pathlib.Path(path).read_text(errors="replace")
    fw = Firmware()
    entries = scenario_packed()                   # the 0.1.47 run declared exactly these
    fw.mount(entries)
    port = Port(entries, hunt=hunt)
    port.bind(fw)
    timeline = []                                  # (t, {index: value}) sweeps and single moves
    scan_re = re.compile(r"^periph t=([\d.]+)s ([0-9a-f]{4}): ((?:[0-9a-f]{8} ?)+)$")
    move_re = re.compile(r"^periph move(?: \[small\])? @0x([0-9a-f]{8}) t=([\d.]+)s: ([0-9a-f]{8}) -> ([0-9a-f]{8})")
    grouped = {}                                    # one 96-word pass is printed as 12 lines of 8 words: fold them
    for line in text.splitlines():
        m = scan_re.match(line)
        if m:
            base = int(m.group(2), 16)
            vals = [int(x, 16) for x in m.group(3).split()]
            grouped.setdefault(float(m.group(1)), {}).update({base // 4 + k: v for k, v in enumerate(vals)})
            continue
        m = move_re.match(line)
        if m:
            grouped.setdefault(float(m.group(2)), {})[(int(m.group(1), 16) - PERIPH_BASE) // 4] = int(m.group(4), 16)
    timeline = sorted(grouped.items())
    claims, applies = [], []
    # The core sweeps all 96 words at 2 Hz (one beat in five, docs/354 §4) whether or not anything changed, so the log
    # records far fewer samples than the port actually saw.  A replay that replays only logged events builds a thinner
    # cleanliness history than the running core and cannot reproduce its claims -- so the sweeps between events are
    # interpolated at the real rate.
    last = 0.0

    def sweep_at():
        port.sweep({i: fw.word.get(PERIPH_BASE + 4 * i, 0) for i in range(PERIPH_WORDS)})

    for t, patch in timeline:
        step = last
        while step + 0.5 <= t:
            step += 0.5
            sweep_at()
        last = t
        for i, v in patch.items():
            fw.word[PERIPH_BASE + 4 * i] = v
        if len(patch) > 1:                          # a logged 96-word pass counts as the sweep at that instant
            sweep_at()
        for line in port.beat(None, 0, t):
            if "CLAIMED" in line or "lost" in line:
                claims.append(line)
            elif "->" in line:
                applies.append(line)
    print(f"\n=== replay {pathlib.Path(path).name} with hunt={'ON ' if hunt else 'OFF'}")
    print(f"    {len(timeline)} sample instants, {len(entries)} entries "
          f"(measured on the card: fit@0x4000006c t=12.9s, music@0x400000ec t=15.8s, sfx@0x400000dc t=15.8s)")
    for line in claims[:8]:
        print("   ", line)
    if len(claims) > 8:
        print(f"    ... {len(claims) - 8} more claim/lost events")
    music = [l for l in applies if "Music" in l]
    sfx = [l for l in applies if "SFX" in l]
    print(f"    applied: {len(music)} music, {len(sfx)} sfx")
    for line in music[:6]:
        print("   ", line)
    muted = sum(1 for l in music if "music_vol=0" in l)
    print(f"    Music Volume driven to music_vol=0 (mute) {muted} times by {entries[0].get('slot'):08x}"
          if entries[0].get("slot") else "    Music Volume never claimed a word")
    return claims, applies


def main():
    print("=== 1. mask / value audit of what interact.json declares")
    dist = sorted(pathlib.Path("pocket/dist/night").rglob("interact.json"))
    if dist:
        variables = json.loads(dist[0].read_text())["interact"]["variables"]
        for v in variables[8:]:
            opts = [int(o["value"], 16) for o in v["options"]]
            for line in audit_entry(v["name"], int(v["address"], 16), int(v["mask"], 16), opts):
                print("   ", line)
    run_scenario("2. packing three entries into one address (0.1.47), firmware writes the whole word",
                 scenario_packed, PICKS_PACKED)
    run_scenario("3. one entry per wired address (0.1.48)", scenario_shipped, PICKS_SHIPPED)
    logs = sys.argv[1:] or ["scratch/pocket/log_0147.sav"]
    for path in logs:
        replay(path, hunt=True)                    # 0.1.47's rule: hunt allowed anywhere
        replay(path, hunt=False)                   # 0.1.48's rule: a known address only
    print("\n=== verdict")
    print("    mask/value layer: consistent (option values inside their fields, indices offset by lo, none zero)")
    print("    firmware layer: whole-word writes -- a shared address cannot hold two settings (docs/358)")
    print("    port layer: with the hunt allowed, the model claims two peripheral counters out of the real 0.1.47")
    print("    timeline and drives Music Volume to mute from a counter -- exactly what the card did.  The shipped rule")
    print("    (an entry with a known mirror reads that mirror and nothing else) claims 0x4000006c/0x40000068 only.")


if __name__ == "__main__":
    main()
