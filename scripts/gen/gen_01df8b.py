src = open('src/f/f.c').read()
start = src.index('void fn_01bea8(uint8_t *e)\n{')
end = src.find("\n/* =====================================================================", start)
if end < 0: end = len(src)
body = src[start:end].rstrip() + "\n"
body = body.replace('void fn_01bea8(uint8_t *e)\n{', 'void fn_01df8b(uint8_t *e)\n{', 1)

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
                    *(int32_t *)(e + 0x1f) = 7;                  /* 0x01e273: off screen, give up */
                    return;
                }
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x14) >> 1;
                if (*(int32_t *)(e + 0xc7) == 1) {
                    int32_t slot;                                /* [EBP-0x74] */
                    /* 0x01e2a0-0x01e2f2: a PHASE LATCH on entity[0x5f], like fn_01aec8's, but this one
                     * alternates between the two script-byte sides next to the player and the four
                     * ABSOLUTE jitter slots -- and it is re-written on both branches (the 0/1 branch
                     * writes it twice, 0x01e2a8 and 0x01e2f0, which is what the original does). */
                    do {
                        if (*(int32_t *)(e + 0x5f) == 0) {
                            *(int32_t *)(e + 0x5f) = 1;
                            slot = (int32_t)prng_next() % 2;               /* slots 0/1 */
                            *(int32_t *)(e + 0x5f) = 1;
                        } else {
                            slot = (int32_t)prng_next() % 4 + 6;          /* absolute slots 6..9 */
                            *(int32_t *)(e + 0x5f) = 0;
                        }
                    } while (slot == *(int32_t *)(e + 0xff));
                    *(int32_t *)(e + 0x6f) = (slot == 0 || slot == 1) ? 1u : 0u;
                    *(int32_t *)(e + 0xff) = (uint32_t)slot;
                    int32_t keep_x = *(int32_t *)(e + 0xbf);     /* [EBP-0x70] */
                    int32_t keep_y = *(int32_t *)(e + 0xc3);     /* [EBP-0x6c] */
                    uint32_t off = ai_script_byte(ty, *(int32_t *)(e + 0x113), 0);
                    if (slot == 0) {
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x - (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else if (slot == 1) {
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x + (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else if (slot < 6) {
                        /* Player-relative jitter. This handler's draw only yields 0/1 or 6..9, so the
                         * arm is unreachable in play -- kept because the original has it (0x01e400). */
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        int32_t rr     = (int32_t)prng_next() % span;
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + p_x + abs32(rr));
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        int32_t rr_y   = (int32_t)prng_next() % span_y;
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) + p_y + abs32(rr_y));
                    } else {                                     /* absolute jitter (0x01e482-0x01e503) */
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        int32_t rr     = (int32_t)prng_next() % span;
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + abs32(rr));
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        int32_t rr_y   = (int32_t)prng_next() % span_y;
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) + abs32(rr_y));
                    }
                    if (abs32(*(int32_t *)(e + 0xbf) - (int32_t)fn_01519f(e)) < 10 &&
                        abs32(*(int32_t *)(e + 0xc3) - (int32_t)fn_0151d5(e)) < 5) {
                        *(int32_t *)(e + 0xbf) = (uint32_t)keep_x;   /* too close: keep the old spot */
                        *(int32_t *)(e + 0xc3) = (uint32_t)keep_y;
                    } else {
                        *(int32_t *)(e + 0xc7) = 0;
                    }
                }
                /* LAB 0x01e575: unlike 0x01cf64 the screen clamp sits OUTSIDE the entity[0xc7] gate, so
                 * it also corrects an out-of-range entity when no spot was drawn at all; and it runs
                 * AFTER the distance test, so it re-clamps a restored spot (cf. fn_01bea8). */
                if (*(int32_t *)(e + 0xbf) < 0) {
                    *(int32_t *)(e + 0xbf) = 0x14;
                } else if (*(int32_t *)(e + 0xbf) > 0x13f) {
                    *(int32_t *)(e + 0xbf) = 0x12c;
                }
'''
body = body[:a] + new + body[b:]
doc = '''
/* =====================================================================
 * 0x01df8b -- THE BEHAVIOUR-0x06 ENTITY AI HANDLER (docs/18-ai-dispatch-map.md)
 *
 * Seventh of the 18. scripts/analysis/aicases.py shows it is the fn_01bea8 body with case 0x00 rewritten:
 * every other switch case matches a sibling byte-for-byte after name/cast canonicalization, so this
 * body was generated by scripts/gen/gen_01df8b.py and the case-0 block re-derived from
 * analysis/disasm/FUN_0001df8b.txt. The chooser is a phase latch on entity[0x5f]: while the latch is
 * 0 it draws slot = prng()%2 and hides the entity beside the player at the type's script-row byte 0
 * offset (the same source of offsets as 0x01cf64); when the latch is 1 it draws slot = prng()%4 + 6
 * and uses the ABSOLUTE jitter rectangles of slots 6..9. The slot may not repeat entity[0xff]. Three
 * orderings/layout facts, each pinned by a test: (1) the player-relative jitter arm (slots 2..5) is
 * present but unreachable, since the draw can never produce those slots; (2) the too-close test runs
 * BEFORE the screen clamp (like fn_019e48/01bea8), so a restored spot is re-clamped -- test F; and
 * (3) the clamp sits outside the entity[0xc7] gate, so an out-of-range entity[0xbf] is corrected even
 * when no spot is drawn at all -- test E.
 */
'''
open('/tmp/fn_01df8b_body.c','w').write(doc + body)
print(len((doc+body).splitlines()), "lines")
