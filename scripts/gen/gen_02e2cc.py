#!/usr/bin/env python3
"""Generate fn_02e2cc (behaviour-0x26) from the verified fn_02c838 body.

All deltas were located with scratch/aidiff_insns.py (95.6% of the 1815 instructions of 0x02e2cc align
with 0x02c838 in order) and read in scratch/aidd.py listings; see scratch/NOTES_02e2cc.md.
"""
import re, sys

SRC = "src/f/f.c"
body = open(SRC).read()
i = body.index("void fn_02c838(uint8_t *e)")
j = body.index("\n}\n", i) + 3
b = body[i:j]

DOC = """/* 0x0002e2cc: the behaviour-0x26 entity AI handler (dispatcher arg 0; 6993 B, 1815 instructions).
 *
 * 95.6% of this handler's instruction stream is the stream of fn_02c838 in the same order, so the port is
 * generated from it; the deltas are all inside the swing/drop arm, the approach-spot chooser and the
 * scripted walk:
 *
 *   - the DROP arm is sprite 0x2c (not 0x2a): it latches on entity frame 1, plans Y only with the closing
 *     speed (player_y - own_y) / 20.0 (the double at DS 0x5007c), and walks frames 2..0x14 with
 *     k = frame (0x0002e635 loads entity[0x11] directly -- no frame offset, unlike 0x02c838's frame-0x32
 *     latch).
 *   - the SWING arm is sprite 0x2d (not 0x2c), latches on entity frame 1 and keeps the same
 *     slope/interpolation machine, but over 55 frames: the doubles at DS 0x50084 / 0x5008c / 0x50094 are
 *     all 55.0, and the window is 2..0x37.
 *   - the one-shot ambush gate closes at 0x46 px of X (0x3c in 0x02c838, 0x46 again in 0x027bec).
 *   - the approach-spot chooser has no entity[0x5f] phase latch.  Instead a coin flip (prng % 100 < 50,
 *     signed) picks between an 8-way draw based at 2 -- where drawing 2 or 3 triggers a 2-way sub-draw
 *     that yields 2/3 or 4/5 -- and a plain 0/1 draw.  The 6..9 rectangle arm survives but is now
 *     unreachable (0x0002ecf4).
 *   - the X field clamp runs BEFORE the too-close test (0x0002ee26 then 0x0002ee5d), i.e. the 0x02319c
 *     order, so a rejected spot is restored already clamped.
 *   - the state-4 "aim past the player" offset is +-0x32 (not +-0x46) and the state-0x6a ambush proximity
 *     test is 0x37 px (not 0x4b).
 *   - the scripted walk accepts TWO marker bytes: ',' (0x2c) as in 0x02c838 and additionally '-' (0x2d)
 *     (0x0002f96d / 0x0002f990); a marker row closes on the X gap alone, an ordinary row also needs the
 *     Y gap under ten.  The original duplicates the reached/missed machinery for the two cases; the port
 *     folds it into one reach test, which is what the duplicated code computes.
 */
"""

subs = [
    ("void fn_02c838(uint8_t *e)", "void fn_02e2cc(uint8_t *e)"),

    # --- drop arm ---
    ("        if (*(int32_t *)(e + 0x17) == 0x2a) {                    /* 0x0002cb10-0x0002cbde: the drop */",
     "        if (*(int32_t *)(e + 0x17) == 0x2c) {                    /* 0x0002e5a4-0x0002e660: the drop */"),
    ("            if (*(int32_t *)(e + 0x11) == 0x32) {                /* 0x0002cb1d: the latch frame */",
     "            if (*(int32_t *)(e + 0x11) == 1) {                   /* 0x0002e5b1: the latch frame */"),
    ("                /* 0x0002cb64-0x0002cb88: only ONE component is planned here -- the Y closing speed is\n"
     "                 * (player_y - own_y) / 20.0 (the double at DS 0x5005c), and X is never touched. */\n"
     "                *(float *)(e + 0xe7) = (float)(((double)(p_y - sy)) / blk50_f64(0x5005cu));",
     "                /* 0x0002e5f4-0x0002e618: only ONE component is planned here -- the Y closing speed is\n"
     "                 * (player_y - own_y) / 20.0 (the double at DS 0x5007c), and X is never touched. */\n"
     "                *(float *)(e + 0xe7) = (float)(((double)(p_y - sy)) / blk50_f64(0x5007cu));"),
    ("            } else if (*(int32_t *)(e + 0x11) > 0x32 && *(int32_t *)(e + 0x11) <= 0x46) {\n"
     "                /* 0x0002cb9e-0x0002cbde: y = own_y_at_latch + (frame-0x32) * speed, rounded, moved with\n"
     "                 * fn_01586b (so the room Y band still applies).  No containment test at all. */\n"
     "                int32_t k = *(int32_t *)(e + 0x11) - 0x32;",
     "            } else if (*(int32_t *)(e + 0x11) > 1 && *(int32_t *)(e + 0x11) <= 0x14) {\n"
     "                /* 0x0002e61e-0x0002e660: y = own_y_at_latch + frame * speed, rounded, moved with\n"
     "                 * fn_01586b (so the room Y band still applies).  No containment test at all. */\n"
     "                int32_t k = *(int32_t *)(e + 0x11);"),

    # --- swing arm ---
    ("        } else if (*(int32_t *)(e + 0x17) == 0x2c) {              /* 0x0002cbe3-0x0002ce79: the swing */",
     "        } else if (*(int32_t *)(e + 0x17) == 0x2d) {              /* 0x0002e66b-0x0002e8f0: the swing */"),
    ("            if (*(int32_t *)(e + 0x11) == 1) {                    /* 0x0002cbf3: the latch frame */",
     "            if (*(int32_t *)(e + 0x11) == 1) {                    /* 0x0002e67b: the latch frame */"),
    ("                /* 0x0002cca8-0x0002ccc8: instead of per-frame velocities the handler stores the LINE",
     "                /* 0x0002e730-0x0002e750: instead of per-frame velocities the handler stores the LINE"),
    ("            } else if (*(int32_t *)(e + 0x11) > 1 && *(int32_t *)(e + 0x11) <= 0x32) {\n"
     "                /* 0x0002ccd3-0x0002ce79: the swing is walked by interpolating the ANCHOR-to-player\n"
     "                 * segment over 50 frames (the doubles at DS 0x50064/0x5006c/0x50074) on the dominant",
     "            } else if (*(int32_t *)(e + 0x11) > 1 && *(int32_t *)(e + 0x11) <= 0x37) {\n"
     "                /* 0x0002e75b-0x0002e8f0: the swing is walked by interpolating the ANCHOR-to-player\n"
     "                 * segment over 55 frames (the doubles at DS 0x50084/0x5008c/0x50094) on the dominant"),
    ("                if (abs32(dx) > abs32(dy)) {                      /* 0x0002cd22: horizontal swing */\n"
     "                    double xf = ((double)dx / blk50_f64(0x50064u)) * (double)k",
     "                if (abs32(dx) > abs32(dy)) {                      /* 0x0002e77a: horizontal swing */\n"
     "                    double xf = ((double)dx / blk50_f64(0x50084u)) * (double)k"),
    ("                } else {                                          /* 0x0002cdaf: vertical swing */\n"
     "                    double yf = ((double)dy / blk50_f64(0x5006cu)) * (double)k",
     "                } else {                                          /* 0x0002e807: vertical swing */\n"
     "                    double yf = ((double)dy / blk50_f64(0x5008cu)) * (double)k"),
    ("                 * from the UNROUNDED dominant coordinate.  A zero slope in the vertical case pins the\n"
     "                 * target X to 100 (0x0002cdf7) instead of dividing. */",
     "                 * from the UNROUNDED dominant coordinate.  A zero slope in the vertical case pins the\n"
     "                 * target X to 100 (0x0002e852) instead of dividing. */"),

    # --- ambush gate ---
    ("    if (*(int32_t *)(e + 0x7f) == 1) {                           /* 0x00026896-0x000268f7: one-shot ambush */\n"
     "        if (abs32((int32_t)fn_01519f(e) - p_x) < 0x3c &&",
     "    if (*(int32_t *)(e + 0x7f) == 1) {                           /* 0x0002e8f8-0x0002e959: one-shot ambush */\n"
     "        if (abs32((int32_t)fn_01519f(e) - p_x) < 0x46 &&"),

    # --- chooser draw ---
    ("                int32_t keep_x = *(int32_t *)(e + 0xbf);         /* [EBP-0x64] */\n"
     "                int32_t keep_y = *(int32_t *)(e + 0xc3);         /* [EBP-0x60] */\n"
     "                /* 0x0002847e-0x000284d6: the entity[0x5f] PHASE LATCH returns (0x01df8b in the switch\n"
     "                 * family had it, 0x02319c dropped it).  Phase 1 draws a script-row side slot (0/1) and\n"
     "                 * sets the latch; phase 0 draws an absolute-rectangle slot (6..9) and clears it.  The\n"
     "                 * latch is written INSIDE the re-roll loop, so a rejected slot redraws in the other phase. */\n"
     "                for (;;) {\n"
     "                    if (*(int32_t *)(e + 0x5f) != 0) {\n"
     "                        slot = (int32_t)prng_next() % 4 + 6;\n"
     "                        *(int32_t *)(e + 0x5f) = 0;\n"
     "                    } else {\n"
     "                        *(int32_t *)(e + 0x5f) = 1;\n"
     "                        slot = (int32_t)prng_next() % 2;\n"
     "                        *(int32_t *)(e + 0x5f) = 1;              /* 0x000284cc: re-written, harmlessly */\n"
     "                    }\n"
     "                    if (slot != *(int32_t *)(e + 0xff)) break;\n"
     "                }",
     "                int32_t keep_x = *(int32_t *)(e + 0xbf);         /* [EBP-0x60] */\n"
     "                int32_t keep_y = *(int32_t *)(e + 0xc3);         /* [EBP-0x5c] */\n"
     "                /* 0x0002eacc-0x0002eb53: no entity[0x5f] latch here.  A coin flip (prng % 100 < 50, the\n"
     "                 * remainder keeps the dividend's sign) picks between an 8-way draw based at 2 -- where a\n"
     "                 * 2 or 3 result triggers a 2-way sub-draw, turning it into 2/3, or into 4/5 after being\n"
     "                 * rewritten to 4 -- and a plain 0/1 draw.  So the reachable slots are 0..5. */\n"
     "                for (;;) {\n"
     "                    if ((int32_t)prng_next() % 100 < 50) {\n"
     "                        slot = (int32_t)prng_next() % 8 + 2;\n"
     "                        if (slot == 2) {\n"
     "                            slot += (int32_t)prng_next() % 2;\n"
     "                        } else if (slot == 3) {\n"
     "                            slot = 4;\n"
     "                            slot += (int32_t)prng_next() % 2;\n"
     "                        }\n"
     "                    } else {\n"
     "                        slot = (int32_t)prng_next() % 2;\n"
     "                    }\n"
     "                    if (slot != *(int32_t *)(e + 0xff)) break;\n"
     "                }"),

    # --- slot arms: 6..9 now dead ---
    ("                } else {\n"
     "                    /* 0x00028713-0x00028799: slots 6..9 use the SAME rectangles as ABSOLUTE field\n"
     "                     * coordinates -- no player term at all (the \"absolute jitter\" of 0x01df8b). */",
     "                } else {\n"
     "                    /* 0x0002ed14-0x0002ed9a: slots 6..9 use the SAME rectangles as ABSOLUTE field\n"
     "                     * coordinates -- no player term at all (the \"absolute jitter\" of 0x01df8b).  This\n"
     "                     * handler's draw only yields 0..5, so the arm is kept for fidelity but is dead. */"),

    # --- too-close test / X clamp order ---
    ("                /* 0x0002879f-0x000287f7: the too-close test runs FIRST here; a rejected spot is restored\n"
     "                 * and the entity[0xc7] clear is skipped, as elsewhere in the family. */\n"
     "                if (abs32(*(int32_t *)(e + 0xbf) - (int32_t)fn_01519f(e)) < 10 &&\n"
     "                    abs32(*(int32_t *)(e + 0xc3) - (int32_t)fn_0151d5(e)) < 5) {\n"
     "                    *(int32_t *)(e + 0xbf) = (uint32_t)keep_x;\n"
     "                    *(int32_t *)(e + 0xc3) = (uint32_t)keep_y;\n"
     "                } else {\n"
     "                    *(int32_t *)(e + 0xc7) = 0;\n"
     "                }\n"
     "                /* 0x00028806-0x00028833: the X clamp comes AFTER the too-close test (0x02319c: before),\n"
     "                 * so a rejected spot is restored unclamped. */\n"
     "                if (*(int32_t *)(e + 0xbf) < 0) {\n"
     "                    *(int32_t *)(e + 0xbf) = 0x14;\n"
     "                } else if (*(int32_t *)(e + 0xbf) >= 0x140) {\n"
     "                    *(int32_t *)(e + 0xbf) = 0x12c;\n"
     "                }",
     "                /* 0x0002ee26-0x0002ee56: the X field clamp runs FIRST here (the 0x02319c order; 0x027bec\n"
     "                 * and 0x02c838 clamp after testing), so a spot rejected by the too-close test below is\n"
     "                 * restored on top of an already-clamped value. */\n"
     "                if (*(int32_t *)(e + 0xbf) < 0) {\n"
     "                    *(int32_t *)(e + 0xbf) = 0x14;\n"
     "                } else if (*(int32_t *)(e + 0xbf) >= 0x140) {\n"
     "                    *(int32_t *)(e + 0xbf) = 0x12c;\n"
     "                }\n"
     "                /* 0x0002ee5d-0x0002eeb7: too close to the entity -- restore, and skip the entity[0xc7]\n"
     "                 * clear, as elsewhere in the family. */\n"
     "                if (abs32(*(int32_t *)(e + 0xbf) - (int32_t)fn_01519f(e)) < 10 &&\n"
     "                    abs32(*(int32_t *)(e + 0xc3) - (int32_t)fn_0151d5(e)) < 5) {\n"
     "                    *(int32_t *)(e + 0xbf) = (uint32_t)keep_x;\n"
     "                    *(int32_t *)(e + 0xc3) = (uint32_t)keep_y;\n"
     "                } else {\n"
     "                    *(int32_t *)(e + 0xc7) = 0;\n"
     "                }"),

    # --- state 4: aim past the player ---
    ("            } else if (r == 4) {                                 /* 0x0002741c-0x00027489: aim past the player */",
     "            } else if (r == 4) {                                 /* 0x0002f4da-0x0002f54e: aim past the player */"),
    ("                if ((int32_t)fn_01519f(pl) - (int32_t)fn_01519f(e) < 0)\n"
     "                    *(int32_t *)(e + 0xb3) = (uint32_t)(*(int32_t *)(e + 0xb3) + 0x46);\n"
     "                else\n"
     "                    *(int32_t *)(e + 0xb3) = (uint32_t)(*(int32_t *)(e + 0xb3) - 0x46);",
     "                if ((int32_t)fn_01519f(pl) - (int32_t)fn_01519f(e) < 0)\n"
     "                    *(int32_t *)(e + 0xb3) = (uint32_t)(*(int32_t *)(e + 0xb3) + 0x32);\n"
     "                else\n"
     "                    *(int32_t *)(e + 0xb3) = (uint32_t)(*(int32_t *)(e + 0xb3) - 0x32);"),

    # --- state 0x6a proximity ---
    ("                    if (abs32((int32_t)fn_01519f(e) - p_x) < 0x4b &&",
     "                    if (abs32((int32_t)fn_01519f(e) - p_x) < 0x37 &&"),

    # --- scripted walk: two marker bytes ---
    ("                if (*(int32_t *)(e + 0xab) < 0) {\n"
     "                    /* 0x00029264-0x00029420: script col 1 selects the reach test -- on a ',' (0x2c) row\n"
     "                     * only the X gap matters, otherwise the Y gap gets TEN pixels (0x02319c also uses ten\n"
     "                     * but has no marker rows).  Advancing a row does not re-latch the anchors. */\n"
     "                    int32_t cell = ai_script_byte(ty, *(int32_t *)(e + 0x113), 0);\n"
     "                    int32_t mode = ai_script_byte(ty, *(int32_t *)(e + 0x113), 1);\n"
     "                    int reached = abs32((int32_t)fn_01519f(e) - p_x) < cell &&\n"
     "                                  (mode == 0x2c || abs32((int32_t)fn_0151d5(e) - p_y) < 10);",
     "                if (*(int32_t *)(e + 0xab) < 0) {\n"
     "                    /* 0x0002f939-0x0002fbf2: script col 1 selects the reach test, and this handler has\n"
     "                     * TWO marker bytes -- ',' (0x2c, as in 0x02c838) and '-' (0x2d, 0x0002f96d/0x0002f990)\n"
     "                     * -- both of which close on the X gap alone; an ordinary row also needs the Y gap\n"
     "                     * under ten.  The original duplicates the reached/missed machinery per case (the\n"
     "                     * marker case at 0x0002f999, the ordinary one at 0x0002fabd); folding it into one\n"
     "                     * reach test is what that duplication computes.  Advancing a row does not re-latch\n"
     "                     * the anchors. */\n"
     "                    int32_t cell = ai_script_byte(ty, *(int32_t *)(e + 0x113), 0);\n"
     "                    int32_t mode = ai_script_byte(ty, *(int32_t *)(e + 0x113), 1);\n"
     "                    int reached = abs32((int32_t)fn_01519f(e) - p_x) < cell &&\n"
     "                                  (mode == 0x2c || mode == 0x2d ||\n"
     "                                   abs32((int32_t)fn_0151d5(e) - p_y) < 10);"),
]

for old, new in subs:
    n = b.count(old)
    if n != 1:
        sys.exit("substitution matched %d times:\n%s" % (n, old[:120]))
    b = b.replace(old, new, 1)

open("scratch/body_02e2cc.c", "w").write(DOC + b)
print("wrote scratch/body_02e2cc.c (%d bytes)" % len(DOC + b))
