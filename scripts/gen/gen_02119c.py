src = open('port/src/f/f.c').read()
start = src.index('void fn_01bea8(uint8_t *e)\n{')
end = src.find("\n/* =====================================================================", start)
if end < 0: end = len(src)
body = src[start:end].rstrip() + "\n"
body = body.replace('void fn_01bea8(uint8_t *e)\n{', 'void fn_02119c(uint8_t *e)\n{', 1)

# ---- case 0x00 ----
a = body.index('    case 0x00:\n')
mark = "                /* LAB 0x019392: clamp the chosen spot into the current room band */"
b = body.index(mark)
new0 = '''    case 0x00:
        if (*(int32_t *)(e + 0xaf) != 4) {
            if (*(int32_t *)(e + 0xaf) == 0x0a) {
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) >> 1;
            } else {
                int32_t j = (int32_t)prng_next() % 0x32;         /* 0x000213c8-0x000213e2 */
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) + j;
            }
            *(int32_t *)(e + 0x1f)  = 4;
            if ((int32_t)prng_next() % 6 == 1) *(int32_t *)(e + 0xaf) = 4;   /* 0x000213f2-0x0002140d */
            *(int32_t *)(e + 0x113) = (int32_t)prng_next() % 10;
        }
        if (*(int32_t *)(e + 0xaf) == 4) {
            if (*(int32_t *)(e + 0x8f) == 1) {
                *(int32_t *)(e + 0x97) = ai_rec_i32(ty, 0x14) >> 1;
                *(int32_t *)(e + 0x1f) = 3;
            } else {
                if ((int32_t)fn_01519f(e) < 0 || (int32_t)fn_01519f(e) > 0x13f) {
                    *(int32_t *)(e + 0x1f) = 7;                  /* 0x00021494: off screen, give up */
                    return;
                }
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x14) >> 1;
                if (*(int32_t *)(e + 0xc7) == 1) {
                    int32_t keep_x = *(int32_t *)(e + 0xbf);     /* [EBP-0x54], saved BEFORE the latch */
                    int32_t keep_y = *(int32_t *)(e + 0xc3);     /* [EBP-0x50] */
                    int32_t slot;                                /* [EBP-0x58] */
                    /* 0x000214e9-0x00021662: a phase latch on entity[0x5f] with NO re-roll loop.
                     * Latch 0 (0x0002159d): slot = prng()%2 and the spot is that slot's jitter
                     * rectangle OFFSET BY THE PLAYER (the |prng % span| form).
                     * Latch 1 (0x000214f3): the slot is chosen by which half of the screen the player
                     * is in -- p_x <= 0xa0 -> slot 0xb, otherwise slot 0xa -- and the spot is drawn
                     * from the ABSOLUTE rectangle of that slot with a plain signed remainder (no
                     * abs(), no player offset). */
                    if (*(int32_t *)(e + 0x5f) == 0) {
                        slot = (int32_t)prng_next() % 2;
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        int32_t rr     = (int32_t)prng_next() % span;
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + p_x + abs32(rr));
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        int32_t rr_y   = (int32_t)prng_next() % span_y;
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) + p_y + abs32(rr_y));
                        *(int32_t *)(e + 0x5f) = 1;
                    } else {
                        slot = (p_x <= 0xa0) ? 0x0b : 0x0a;
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) +
                                                            (int32_t)prng_next() % span);
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) +
                                                            (int32_t)prng_next() % span_y);
                        *(int32_t *)(e + 0x5f) = 0;
                    }
                    if (slot == 0 || slot == 1) *(int32_t *)(e + 0xc3) = (uint32_t)p_y;
                    *(int32_t *)(e + 0x6f) = (slot == 0 || slot == 1) ? 1u : 0u;
                    *(int32_t *)(e + 0xff) = (uint32_t)slot;
                    /* 0x00021695: clamp before the distance test, restore jumps past it
                     * (LAB 0x00021723). */
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
body = body[:a] + new0 + body[b:]

# ---- case 0x0b: no prng coin flip on the not-reached path ----
oldb = """            } else if ((int32_t)prng_next() % 0x14 == 2) {
                *(int32_t *)(e + 0xaf) = 4;  *(int32_t *)(e + 0x1f) = 0;
            } else {
                *(int32_t *)(e + 0xaf) = 0x0a; *(int32_t *)(e + 0x1f) = 0;
            }"""
newb = """            } else {
                /* 0x00022016: unlike fn_01bea8 (and 0x020170 / 0x0221b8) there is NO prng()%0x14 == 2
                 * chance of jumping to action 4 when the walk target is not reached -- falling short
                 * always gives up with action 0x0a. */
                *(int32_t *)(e + 0xaf) = 0x0a; *(int32_t *)(e + 0x1f) = 0;
            }"""
assert body.count(oldb) == 1
body = body.replace(oldb, newb, 1)

doc = '''
/* =====================================================================
 * 0x02119c -- THE BEHAVIOUR-0x0b ENTITY AI HANDLER (docs/18-ai-dispatch-map.md)
 *
 * Eleventh of the 18. scratch/aicases.py flags TWO differing cases: 0x00 (the chooser) and 0x0b (the
 * scripted walk), everything else -- including the whole 257-instruction tail after the room-band
 * clamp, which matches FUN_0001bea8 @0x1c4bb mnemonic for mnemonic -- comes from the template
 * (generator: scratch/gen_0221b8-style scratch/gen_02119c.py). Case 0x00 is a phase latch on
 * entity[0x5f] WITHOUT a re-roll loop: latch 0 draws slot = prng()%2 and hides the entity inside that
 * slot's jitter rectangle offset by the player; latch 1 picks the slot from the player's screen half
 * (p_x <= 0xa0 -> slot 0xb, else slot 0xa) and draws it from the ABSOLUTE rectangle with a plain
 * signed remainder -- the only handler in the family that leaves the remainder signed (no abs()). The
 * entity[0xc3] pre-write for slots 0/1 makes those two spots level with the player. Case 0x0b drops
 * the prng()%0x14 == 2 coin flip that the siblings use when the walk target is not reached: falling
 * short always ends with action 0x0a (pinned by a 120-seed sweep). Unlike 0x020170/0x0221b8 this one
 * keeps the 1-in-6 forced action 4 (0x000213f2-0x0002140d).
 */
'''
open('/tmp/fn_02119c_body.c','w').write(doc + body)
print(len((doc+body).splitlines()), "lines")
