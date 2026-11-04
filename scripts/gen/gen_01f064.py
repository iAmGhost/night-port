src = open('port/src/f/f.c').read()
start = src.index('void fn_01bea8(uint8_t *e)\n{')
end = src.find("\n/* =====================================================================", start)
if end < 0: end = len(src)
body = src[start:end].rstrip() + "\n"
body = body.replace('void fn_01bea8(uint8_t *e)\n{', 'void fn_01f064(uint8_t *e)\n{', 1)
a = body.index('    case 0x00:\n')
mark = "                /* LAB 0x019392: clamp the chosen spot into the current room band */"
b = body.index(mark)
new = '''    case 0x00:
        if (*(int32_t *)(e + 0xaf) != 4) {
            if (*(int32_t *)(e + 0xaf) == 0x0a) {
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) >> 1;
            } else {
                int32_t j = (int32_t)prng_next() % 0x32;         /* [EBP-0x58] region */
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) + j;
            }
            *(int32_t *)(e + 0x1f)  = 4;
            if ((int32_t)prng_next() % 6 == 1) *(int32_t *)(e + 0xaf) = 4;   /* the siblings' 1-in-6 */
            *(int32_t *)(e + 0x113) = (int32_t)prng_next() % 10;
        }
        if (*(int32_t *)(e + 0xaf) == 4) {
            if (*(int32_t *)(e + 0x8f) == 1) {
                *(int32_t *)(e + 0x97) = ai_rec_i32(ty, 0x14) >> 1;
                *(int32_t *)(e + 0x1f) = 3;
            } else {
                if ((int32_t)fn_01519f(e) < 0 || (int32_t)fn_01519f(e) > 0x13f) {
                    *(int32_t *)(e + 0x1f) = 7;                  /* 0x01f3cc: off screen, give up */
                    return;
                }
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x14) >> 1;
                if (*(int32_t *)(e + 0xc7) == 1) {
                    int32_t slot;                                /* [EBP-0x74] */
                    do {                                         /* 0x01f3b2-0x01f408: fn_01bea8's split
                         * draw -- half the time the absolute slots 2..9 (with the 2 -> +prng%2 and
                         * 3 -> 4+prng%2 sub-draws), half the time the player-relative slots 0/1 */
                        int32_t v = (int32_t)prng_next();
                        if (v % 100 < 0x32) {
                            v = (int32_t)prng_next();
                            slot = v % 8 + 2;
                            if (slot == 2) {
                                slot += (int32_t)prng_next() % 2;
                            } else if (slot == 3) {
                                slot = 4 + (int32_t)prng_next() % 2;
                            }
                        } else {
                            slot = (int32_t)prng_next() % 2;
                        }
                    } while (slot == *(int32_t *)(e + 0xff));
                    *(int32_t *)(e + 0x6f) = (slot == 0 || slot == 1) ? 1u : 0u;
                    *(int32_t *)(e + 0xff) = (uint32_t)slot;
                    int32_t keep_x = *(int32_t *)(e + 0xbf);     /* [EBP-0x70] */
                    int32_t keep_y = *(int32_t *)(e + 0xc3);     /* [EBP-0x6c] */
                    /* 0x01f413-0x01f57f: FOUR arms. Slots 0/1 hide beside the player at the type's
                     * script-row byte 0 offset (as in 0x01cf64/0x01df8b) -- and unlike fn_01bea8 there
                     * is no separate player-Y write before the draw, so keep_y is the entity's own Y.
                     * Slots 2..5 (reachable through the sub-draws here) use the player-relative jitter
                     * rectangles, slots 6..9 the absolute ones. */
                    uint32_t off = ai_script_byte(ty, *(int32_t *)(e + 0x113), 0);
                    if (slot == 0) {
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x - (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else if (slot == 1) {
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x + (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else if (slot < 6) {
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        int32_t rr     = (int32_t)prng_next() % span;
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + p_x + abs32(rr));
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        int32_t rr_y   = (int32_t)prng_next() % span_y;
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) + p_y + abs32(rr_y));
                    } else {
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        int32_t rr     = (int32_t)prng_next() % span;
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + abs32(rr));
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        int32_t rr_y   = (int32_t)prng_next() % span_y;
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) + abs32(rr_y));
                    }
                    /* 0x01f580: as in 0x01cf64 the screen clamp runs BEFORE the distance test, and the
                     * restore jumps past it (LAB 0x01f6bb) -- so a rejected spot keeps its raw value,
                     * unlike fn_01bea8/0x01df8b where the clamp re-catches it. */
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
body = body[:a] + new + body[b:]
doc = '''
/* =====================================================================
 * 0x01f064 -- THE BEHAVIOUR-0x07 ENTITY AI HANDLER (docs/18-ai-dispatch-map.md)
 *
 * Eighth of the 18; scratch/aicases.py reduces it to case 0x00 alone, so this body is fn_01bea8's
 * with the chooser re-derived from analysis/disasm/FUN_0001f064.txt (generator:
 * scratch/gen_01f064.py). The slot draw is fn_01bea8's split one (v%100 < 0x32 -> slots 2..9 via
 * v%8+2 with the 2 -> +prng%2 / 3 -> 4+prng%2 sub-draws, else slots 0/1), but the spot is built by
 * FOUR arms: slots 0/1 hide beside the player at the type's script-row byte 0 offset (the 0x01cf64 /
 * 0x01df8b source of offsets) with no separate player-Y write first, slots 2..5 use the
 * player-relative jitter rectangles (reachable here, unlike in 0x01df8b), slots 6..9 the absolute
 * ones. Ordering: the screen clamp runs BEFORE the too-close test and the restore jumps past it, so
 * a rejected spot keeps its raw value -- test E asserts the restored 0x5555 survives, which is the
 * opposite of fn_01bea8/0x01df8b.
 */
'''
open('/tmp/fn_01f064_body.c','w').write(doc + body)
print(len((doc+body).splitlines()), "lines")
