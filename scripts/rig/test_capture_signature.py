"""Self-test for capture_original.signature() across the formats a route can produce (docs/181 §6)."""
import importlib.util, sys
spec = importlib.util.spec_from_file_location("co", "scripts/rig/capture_original.py")
co = importlib.util.module_from_spec(spec); spec.loader.exec_module(co)

HDR = bytes(15)
def mk(w, h, pitch, payload): return HDR + payload

# 1) 32-bit BGRX mode-13h frame: one dark row band, one bright
w,h,pitch = 8,4,32
px = bytearray(pitch*h)
for y in range(h):
    for x in range(w):
        v = 0x40 if y < 2 else 0xC0
        px[y*pitch + x*4 : y*pitch + x*4+3] = bytes([v, v, v])
info = dict(width=w, height=h, pitch=pitch, pixel_format="BGRX", is_paletted=False)
s = co.signature(mk(w,h,pitch,px), info)
print("BGRX   top/bottom lum:", s[0], s[-1], "grid len", len(s))
assert s[0] == 0x40 and s[-1] == 0xC0

# 2) paletted 8bpp with an appended palette (mode 3 text: 720x400 is the real case, scaled here)
w,h,pitch = 8,4,8
idx = bytearray(pitch*h)
for y in range(h):
    for x in range(w): idx[y*pitch+x] = 1 if y < 2 else 7
pal = bytearray(768)
pal[7*3:7*3+3] = bytes([255,255,255])   # a full 256-entry palette: index 7 white
info = dict(width=w, height=h, pitch=pitch, pixel_format="INDEX8", is_paletted=True)
s = co.signature(mk(w,h,pitch, idx+pal), info)
print("PALET  top/bottom lum:", s[0], s[-1])
assert s[0] == 0 and s[-1] == 255

# 3) 16-bit 565 frame: pure red / pure green
w,h,pitch = 8,4,16
px = bytearray(pitch*h)
for y in range(h):
    v = 0xF800 if y < 2 else 0x07E0
    for x in range(w): px[y*pitch + x*2 : y*pitch + x*2+2] = bytes([v & 0xff, v >> 8])
info = dict(width=w, height=h, pitch=pitch, pixel_format="RGB565", is_paletted=False)
s = co.signature(mk(w,h,pitch,px), info)
print("565    top/bottom lum:", s[0], s[-1])
assert s[0] > 0 and s[-1] > s[0]

# 4) the crash case: reply claims 32-bit but the stride says 8bpp -> inferred, and LOUDLY reported
info_bad = dict(width=8, height=4, pitch=8, pixel_format="BGRX", is_paletted=False)
g = co.frame_geom(info_bad, 8*4)
print("MISMATCH inferred:", g[5], "bpp", g[3])
assert g[5] and g[3] == 1
s = co.signature(mk(8,4,8,bytearray(8*4)), info_bad); print("accepted as 8bpp, grid len", len(s))

# 5) genuinely impossible payload: shorter than pitch*height at any width
try:
    co.signature(mk(8,4,32,bytearray(40)), dict(width=8,height=4,pitch=32,pixel_format="BGRX",is_paletted=False))
    print("FAIL: truncated payload accepted"); sys.exit(1)
except ValueError as e:
    print("TRUNC  rejected descriptively:", str(e)[:95])

# 6) geometry helper cross-check
print("frame_geom:", co.frame_geom(dict(width=720,height=400,pitch=720,pixel_format="INDEX8",is_paletted=True), 720*400+768))
print("OK: signature() handles 32-bit, paletted, 16-bit and refuses an impossible payload")
