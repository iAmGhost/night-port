#!/usr/bin/env python3
"""Capture the ORIGINAL game's playtest frames under DOSBox Staging, for port comparison.

The port's harness (port/src/app/main.c, docs/46) takes screenshots named after game events from a
route script. This tool plays the same route against the original:

  * input    -> DOSBox Staging's AUTOTYPE (scripted keyboard entry into the running DOS program).
                Its clock is a fixed PACE per button with ',' as an extra PACE delay, so route events
                are snapped to PACE multiples (both sides then see the same cadence).
  * capture  -> the "Screenshot" action (raw framebuffer PNGs) fired from the host with Cmd+F5 at the
                route's `shot` times, into [capture] capture_dir.
  * naming   -> captures are renamed to the port's names (NNNN_<event>.png) in the order they are taken,
                so scripts/playtest_compare.py can pair them 1:1.

Usage: python3 scripts/playtest_orig.py scripts/playtest/route_episode1.txt scratch/orig [--lead 6]
"""
import argparse, os, subprocess, sys, time

PACE = 0.5                      # seconds per AUTOTYPE button (the shared cadence)
DOSBOX = os.environ.get("DOSBOX", "dosbox-staging")
KEYMAP = {                      # route key name -> AUTOTYPE button name
    "enter": "enter", "space": "space", "esc": "esc", "tab": "tab",
    "up": "up", "down": "down", "left": "left", "right": "right",
    "ctrl": "ctrl", "shift": "shift",
}


def parse_route(path):
    events = []
    for line in open(path):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        f = line.split()
        if len(f) >= 3 and f[1].lower() == "shot":
            events.append((int(f[0]), "shot", f[2]))
        elif len(f) >= 3 and f[1].lower() == "key" and f[3].lower() in ("down", "1"):
            events.append((int(f[0]), "key", f[2]))
    events.sort()
    return events


def autotype_buttons(events):
    """keys -> AUTOTYPE button list, one button per PACE, ',' filling the gaps"""
    buttons, shots, last = [], [], 0.0
    for t_ms, kind, arg in events:
        t = t_ms / 1000.0
        if kind == "shot":
            shots.append((t, arg))
            continue
        name = KEYMAP.get(arg.lower())
        if not name:
            continue
        gap = max(0, round((t - last) / PACE) - 1)
        buttons += [","] * gap + [name]
        last = t
    return " ".join(buttons), shots


def conf_text(cue_root, out_dir, buttons, route_wait):
    return f"""[dosbox]
machine = svga_s3
memsize = 16

[cpu]
core = normal
cycles = auto

[capture]
capture_dir = {out_dir}
default_image_capture_formats = raw

[sdl]
fullscreen = false
window_size = 640x480

[autoexec]
mount c {cue_root}
imgmount d {cue_root}/TG_NIGHT.CUE -t iso
c:
autotype -w {route_wait:.1f} -p {PACE} {buttons}
night
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("route")
    ap.add_argument("out", nargs="?", default="scratch/orig")
    ap.add_argument("--root", default=os.path.expanduser("~/Git/night-port/NIGHT"))
    ap.add_argument("--lead", type=float, default=4.0, help="seconds before the first scripted key")
    ap.add_argument("--tail", type=float, default=3.0)
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()

    events = parse_route(a.route)
    buttons, shots = autotype_buttons(events)
    conf = os.path.abspath("scripts/rig/conf/dosbox_playtest.conf")
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    for f in os.listdir(out):                       # start from an empty capture dir
        if f.endswith(".png") or f.startswith("SCREEN"):
            os.remove(os.path.join(out, f))
    open(conf, "w").write(conf_text(a.root, out, buttons, a.lead))
    print(f"autotype: {buttons or '(none)'}")
    print(f"shots: {[f'{t:.1f}s {n}' for t, n in shots]}")
    if a.dry_run:
        print(open(conf).read())
        return 0

    log = open(os.path.join(out, "dosbox.log"), "w")
    proc = subprocess.Popen([DOSBOX, "--noprimaryconf", "--conf", conf],
                            stdout=log, stderr=subprocess.STDOUT)

    def host_key(name):                             # fire a DOSBox mapper action from the host
        # macOS "keystroke \"5\"" is the character 5, not F5 -- function keys need key codes:
        # F5 = 96 (screenshot), F9 = 101 (shutdown).
        code, mods = {"screenshot": (96, "using command down"),
                      "shutdown": (101, "using control down")}[name]
        subprocess.run(["osascript", "-e",
                        f'tell application "System Events" to key code {code} {mods}'], check=False)

    t0 = time.time()
    for t, name in shots:                           # screenshot at each route event
        time.sleep(max(0.0, t0 + t - time.time()))
        host_key("screenshot")
        print(f"  screenshot at {t:6.1f}s -> {name}")
    time.sleep(a.tail)
    host_key("shutdown")
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()
    log.close()

    caps = sorted([f for f in os.listdir(out) if f.endswith("-raw.png") or f.endswith(".png")],
                  key=lambda f: os.path.getmtime(os.path.join(out, f)))
    if len(caps) != len(shots):
        print(f"warning: {len(caps)} captures for {len(shots)} shot events", file=sys.stderr)
    for (t, name), src in zip(shots, caps):
        seq = "%04d" % shots.index((t, name))
        os.replace(os.path.join(out, src), os.path.join(out, f"{seq}_{name}.png"))
    print(f"captured {min(len(caps), len(shots))} frames into {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
