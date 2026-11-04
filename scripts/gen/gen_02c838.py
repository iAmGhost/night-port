#!/usr/bin/env python3
"""Generate fn_02c838 from fn_027bec: same chain, but the swing arm is replaced by TWO arms --
a 0x2a Y-only dive (20.0 divisor, window 0x33..0x46) and a 0x2c line-interpolation swing whose
dominant axis is stepped over 50.0 frames (DS 0x50064/0x5006c/0x50074) and whose other component
comes from the stored line (slope [0xdb], intercept [0xe3]).  Plus: ambush distance 0x46 -> 0x3c
and the walk marker 0x2b -> 0x2c.  See docs/18-ai-dispatch-map.md."""
src = open('src/f/f.c').read()
fs = src.index("void fn_027bec(uint8_t *e)\n{")
fe = src.index("\n}\n", fs) + 3
t = src[fs:fe]

def R(a, b, tag, n=1):
    global t
    assert t.count(a) == n, (tag, t.count(a))
    t = t.replace(a, b)

R('void fn_027bec(uint8_t *e)\n{', 'void fn_02c838(uint8_t *e)\n{', 'name')

# --- the swing region -------------------------------------------------------------------------------
gs = t.index("        if (*(int32_t *)(e + 0x17) == 0x2b) {")
ge = t.index("        } else if (*(int32_t *)(e + 0x17) == 5) {")
arms = '''        if (*(int32_t *)(e + 0x17) == 0x2a) {                    /* 0x0002cb10-0x0002cbde: the drop */
            int32_t sx = (int32_t)fn_01519f(e), sy = (int32_t)fn_0151d5(e);
            if (*(int32_t *)(e + 0x11) == 0x32) {                /* 0x0002cb1d: the latch frame */
                *(int32_t *)(e + 0xd3) = (uint32_t)sx;
                *(int32_t *)(e + 0xd7) = (uint32_t)sy;
                *(int32_t *)(e + 0xcb) = (uint32_t)p_x;
                *(int32_t *)(e + 0xcf) = (uint32_t)p_y;
                /* 0x0002cb64-0x0002cb88: only ONE component is planned here -- the Y closing speed is
                 * (player_y - own_y) / 20.0 (the double at DS 0x5005c), and X is never touched. */
                *(float *)(e + 0xe7) = (float)(((double)(p_y - sy)) / blk50_f64(0x5005cu));
            } else if (*(int32_t *)(e + 0x11) > 0x32 && *(int32_t *)(e + 0x11) <= 0x46) {
                /* 0x0002cb9e-0x0002cbde: y = own_y_at_latch + (frame-0x32) * speed, rounded, moved with
                 * fn_01586b (so the room Y band still applies).  No containment test at all. */
                int32_t k = *(int32_t *)(e + 0x11) - 0x32;
                int32_t ny = (int32_t)fpu_rndint((double)k * (double)ai_fget(e, 0xe7)
                                                 + (double)*(int32_t *)(e + 0xd7));
                fn_01586b(e, (uint32_t)(ny - (int32_t)fn_0151d5(e)));
            }
        } else if (*(int32_t *)(e + 0x17) == 0x2c) {              /* 0x0002cbe3-0x0002ce79: the swing */
            int32_t sx = (int32_t)fn_01519f(e), sy = (int32_t)fn_0151d5(e);
            if (*(int32_t *)(e + 0x11) == 1) {                    /* 0x0002cbf3: the latch frame */
                *(int32_t *)(e + 0xd3) = (uint32_t)sx;
                *(int32_t *)(e + 0xd7) = (uint32_t)sy;
                *(int32_t *)(e + 0xcb) = (uint32_t)p_x;
                *(int32_t *)(e + 0xcf) = (uint32_t)p_y;
                if (p_x - sx == 0) {                             /* 0x0002cc52: vertical -> steep slope */
                    *(float *)(e + 0xdb) = 100000.0f;            /* 0x47c35000 */
                } else {
                    *(float *)(e + 0xdb) = (float)(float)(p_y - sy) / (float)(p_x - sx);
                }
                /* 0x0002cca8-0x0002ccc8: instead of per-frame velocities the handler stores the LINE
                 * through the player: entity[0xdb] = slope, entity[0xe3] = the Y intercept
                 * (player_y - slope * player_x). */
                *(float *)(e + 0xe3) = (float)((double)p_y
                                               - (double)p_x * (double)ai_fget(e, 0xdb));
            } else if (*(int32_t *)(e + 0x11) > 1 && *(int32_t *)(e + 0x11) <= 0x32) {
                /* 0x0002ccd3-0x0002ce79: the swing is walked by interpolating the ANCHOR-to-player
                 * segment over 50 frames (the doubles at DS 0x50064/0x5006c/0x50074) on the dominant
                 * axis, then reading the other axis off the stored line.  The other axis is derived
                 * from the UNROUNDED dominant coordinate.  A zero slope in the vertical case pins the
                 * target X to 100 (0x0002cdf7) instead of dividing. */
                int32_t dx = *(int32_t *)(e + 0xcb) - *(int32_t *)(e + 0xd3);
                int32_t dy = *(int32_t *)(e + 0xcf) - *(int32_t *)(e + 0xd7);
                int32_t k = *(int32_t *)(e + 0x11);
                int32_t tx, ty;
                if (abs32(dx) > abs32(dy)) {                      /* 0x0002cd22: horizontal swing */
                    double xf = ((double)dx / blk50_f64(0x50064u)) * (double)k
                                + (double)*(int32_t *)(e + 0xd3);
                    tx = (int32_t)fpu_rndint(xf);
                    ty = (int32_t)fpu_rndint(xf * (double)ai_fget(e, 0xdb) + (double)ai_fget(e, 0xe3));
                } else {                                          /* 0x0002cdaf: vertical swing */
                    double yf = ((double)dy / blk50_f64(0x5006cu)) * (double)k
                                + (double)*(int32_t *)(e + 0xd7);
                    ty = (int32_t)fpu_rndint(yf);
                    if ((int32_t)(ai_fget(e, 0xdb) != 0.0f)) {
                        tx = (int32_t)fpu_rndint((yf - (double)ai_fget(e, 0xe3))
                                                 / (double)ai_fget(e, 0xdb));
                    } else {
                        tx = 0x64;
                    }
                }
                fn_01545e(e, tx - (int32_t)fn_01519f(e));
                fn_01586b(e, (uint32_t)(ty - (int32_t)fn_0151d5(e)));
            }
'''
t = t[:gs] + arms + t[ge:]

# --- constants that differ -----------------------------------------------------------------------
R("< 0x46 &&", "< 0x3c &&", 'ambush distance')
R("mode == 0x2b ||", "mode == 0x2c ||", 'walk marker')
R("on a '.' (0x2b) row", "on a ',' (0x2c) row", 'walk marker comment')
open('/tmp/fn_02c838_body.c', 'w').write(t)
print('generated', len(t.splitlines()), 'lines')
