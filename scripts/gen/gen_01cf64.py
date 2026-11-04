import re
src = open('port/src/f/f.c').read()
start = src.index('void fn_01bea8(uint8_t *e)\n{')
end = src.find("\n/* =====================================================================", start)
if end < 0: end = len(src)
body = src[start:end].rstrip() + "\n"

# --- 1. signature ---
body = body.replace('void fn_01bea8(uint8_t *e)\n{', 'void fn_01cf64(uint8_t *e, int32_t mode)\n{', 1)

# --- 2. replace the case-0x00 head + chooser (up to the room-band clamp) ---
a = body.index('    case 0x00:\n')
b = body.index('                /* LAB 0x019392: clamp the chosen spot into the current room band */')
new_case0 = '''    case 0x00:
        /* 0x01d15a -- THE RECON GATE, which none of the fn_018e88/019e48/01aec8/01bea8 siblings
         * have: on the first frame (entity[0x8b] == 0) the handler does nothing but probe whether
         * the player is within 0x41 px horizontally of it, then raises the latch and goes straight
         * to the tail. entity[0x8b] is cleared again whenever a jump/landing decision is made. */
        if (*(int32_t *)(e + 0x8b) == 0) {
            if (ai_rec_i32(ty, 0x10) == 0) {
                /* [EBP-0x18] is never written on this path -- the prng() % 0x32 draw that normally
                 * fills it lives further down in case 0x00. Uninitialised stack residue in the
                 * original; pinned to 0 so the probe box is deterministic (same latent bug as the
                 * jitter local in fn_017f13). */
                const int32_t r = 0;
                if ((int32_t)fn_01519f(e) >= p_x - 0x41 - r) {
                    if ((int32_t)fn_01519f(e) <= p_x + 0x41 + r) {
                        if ((int32_t)fn_0151d5(e) >= p_y - 10 - r % 5) {
                            (void)fn_0151d5(e);   /* 0x01d1e5: the call runs, its result is thrown
                                                 * away -- the compare at 0x01d1ea is dead code */
                        }
                    }
                }
            }
            *(int32_t *)(e + 0x8b) = 1;
            break;                                /* 0x01d1f9: straight to the tail */
        }
        if (*(int32_t *)(e + 0xaf) != 4) {
            if (*(int32_t *)(e + 0xaf) == 0x0a) {
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) >> 1;
            } else {
                int32_t j = (int32_t)prng_next() % 0x32;         /* [EBP-0x18] */
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) + j;
            }
            *(int32_t *)(e + 0x1f)  = 4;
            /* No 1-in-6 forced action 4 here (fn_019e48/01bea8 have one); the only other draw is
             * the script row selector. */
            *(int32_t *)(e + 0x113) = (int32_t)prng_next() % 10;
        }
        if (*(int32_t *)(e + 0xaf) == 4) {
            if (*(int32_t *)(e + 0x8f) == 1) {
                *(int32_t *)(e + 0x97) = ai_rec_i32(ty, 0x14) >> 1;
                *(int32_t *)(e + 0x1f) = 3;
            } else {
                if ((int32_t)fn_01519f(e) < 0 || (int32_t)fn_01519f(e) > 0x13f) {
                    *(int32_t *)(e + 0x1f) = 7;                  /* 0x01d2f0: off screen, give up */
                    return;
                }
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x14) >> 1;
                if (*(int32_t *)(e + 0xc7) == 1) {
                    /* 0x01d32a: the hiding spot is chosen by the MODE the dispatcher passed in
                     * EDX, not by a jitter-table slot: 0 = always on the player's left, 1 = always
                     * on the right, 2 = a coin flip that may not repeat the previous side. */
                    int32_t side = -1;                           /* [EBP-0x64] */
                    if (mode == 0)      side = 0;
                    else if (mode == 1) side = 1;
                    else if (mode == 2) {
                        do {
                            side = (int32_t)prng_next() % 2;
                        } while (side == *(int32_t *)(e + 0xff));
                        *(int32_t *)(e + 0xff) = (uint32_t)side;
                    }
                    /* Any other mode leaves [EBP-0x64] uninitialised in the original (unreachable
                     * through the behaviour table, which only maps 0/1/2); pinned to -1 above so
                     * neither e[0x6f] nor a spot is affected. */
                    *(int32_t *)(e + 0x6f) = (side == 0 || side == 1) ? 1u : 0u;
                    int32_t keep_x = *(int32_t *)(e + 0xbf);     /* [EBP-0x60] */
                    int32_t keep_y = *(int32_t *)(e + 0xc3);     /* [EBP-0x5c] */
                    /* The offset comes from the type's SCRIPT ROW (byte 0), not the jitter table. */
                    uint32_t off = ai_script_byte(ty, *(int32_t *)(e + 0x113), 0);
                    if (side == 0) {
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x - (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else if (side == 1) {
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x + (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    }
                    /* 0x01d436: the screen clamp runs BEFORE the distance test -- as in fn_01aec8,
                     * so a spot restored below can sit outside the clamp again. */
                    if (*(int32_t *)(e + 0xbf) < 0) {
                        *(int32_t *)(e + 0xbf) = 0x14;
                    } else if (*(int32_t *)(e + 0xbf) > 0x13f) {
                        *(int32_t *)(e + 0xbf) = 0x12c;
                    }
                    if (abs32(*(int32_t *)(e + 0xbf) - (int32_t)fn_01519f(e)) < 10 &&
                        abs32(*(int32_t *)(e + 0xc3) - (int32_t)fn_0151d5(e)) < 5) {
                        *(int32_t *)(e + 0xbf) = (uint32_t)keep_x;   /* too close: keep the old spot */
                        *(int32_t *)(e + 0xc3) = (uint32_t)keep_y;
                    } else {
                        *(int32_t *)(e + 0xc7) = 0;
                    }
                }
'''
body = body[:a] + new_case0 + body[b:]
open('/tmp/fn_01cf64_body.c','w').write(body)
print(len(body.splitlines()), "lines")
