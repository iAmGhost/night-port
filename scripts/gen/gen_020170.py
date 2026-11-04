src = open('src/f/f.c').read()
start = src.index('void fn_01bea8(uint8_t *e)\n{')
end = src.find("\n/* =====================================================================", start)
if end < 0: end = len(src)
body = src[start:end].rstrip() + "\n"
body = body.replace('void fn_01bea8(uint8_t *e)\n{', 'void fn_020170(uint8_t *e)\n{', 1)
a = body.index('    case 0x00:\n')
mark = "                /* LAB 0x019392: clamp the chosen spot into the current room band */"
b = body.index(mark)
new = '''    case 0x00:
        if (*(int32_t *)(e + 0xaf) != 4) {
            if (*(int32_t *)(e + 0xaf) == 0x0a) {
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) >> 1;
            } else {
                int32_t j = (int32_t)prng_next() % 0x32;         /* 0x00020390-0x000203a3 */
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) + j;
            }
            *(int32_t *)(e + 0x1f)  = 4;
            /* 0x000203d8-0x000203e2: no 1-in-6 forced action 4 here (unlike fn_01bea8 / 01df8b /
             * 01f064); the draw straight after the state write is the script-row cursor. */
            *(int32_t *)(e + 0x113) = (int32_t)prng_next() % 10;
        }
        if (*(int32_t *)(e + 0xaf) == 4) {
            if (*(int32_t *)(e + 0x8f) == 1) {
                *(int32_t *)(e + 0x97) = ai_rec_i32(ty, 0x14) >> 1;
                *(int32_t *)(e + 0x1f) = 3;
            } else {
                if ((int32_t)fn_01519f(e) < 0 || (int32_t)fn_01519f(e) > 0x13f) {
                    *(int32_t *)(e + 0x1f) = 7;                  /* 0x00020449: off screen, give up */
                    return;
                }
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x14) >> 1;
                if (*(int32_t *)(e + 0xc7) == 1) {
                    int32_t slot;                                /* [EBP-0x60] */
                    do {                                         /* 0x00020483-0x000204e6: slots 0..5 only
                         * -- prng()%4, with 2 -> +prng()%2 and 3 -> 4+prng()%2 */
                        slot = (int32_t)prng_next() % 4;
                        if (slot == 2) {
                            slot += (int32_t)prng_next() % 2;
                        } else if (slot == 3) {
                            slot = 4 + (int32_t)prng_next() % 2;
                        }
                    } while (slot == *(int32_t *)(e + 0xff));
                    *(int32_t *)(e + 0x6f) = (slot == 0 || slot == 1) ? 1u : 0u;
                    *(int32_t *)(e + 0xff) = (uint32_t)slot;
                    int32_t keep_x = *(int32_t *)(e + 0xbf);     /* [EBP-0x5c] */
                    int32_t keep_y = *(int32_t *)(e + 0xc3);     /* [EBP-0x58] */
                    uint32_t off = ai_script_byte(ty, *(int32_t *)(e + 0x113), 0);
                    if (slot == 0) {                             /* 0x00020534-0x0002056c */
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x - (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else if (slot == 1) {                      /* 0x00020577-0x000205af */
                        *(uint32_t *)(e + 0xbf) = (uint32_t)(p_x + (int32_t)off);
                        *(int32_t *)(e + 0xc3)  = (uint32_t)p_y;
                    } else {                                     /* 0x000205b4-0x00020642: the jitter
                         * rectangles OFFSET BY THE PLAYER; the absolute arm of fn_01bea8 is absent, so
                         * the rectangles are only ever read through slots 2..5 here. */
                        int32_t span   = J4_XHI(slot) - J4_XLO(slot);
                        int32_t rr     = (int32_t)prng_next() % span;
                        *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + p_x + abs32(rr));
                        int32_t span_y = J4_YHI(slot) - J4_YLO(slot);
                        int32_t rr_y   = (int32_t)prng_next() % span_y;
                        *(int32_t *)(e + 0xc3) = (uint32_t)(J4_YLO(slot) + p_y + abs32(rr_y));
                    }
                    /* 0x00020648: as in 0x01cf64/0x01f064 the screen clamp runs BEFORE the distance
                     * test and the restore jumps past it (LAB 0x000206e6). */
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
 * 0x020170 -- THE BEHAVIOUR-0x08 ENTITY AI HANDLER (docs/18-ai-dispatch-map.md)
 *
 * Ninth of the 18. scratch/aicases.py lists ONLY case 0x00 as differing from fn_01bea8 (every other
 * switch case matches), and the whole tail of case 0x00 is instruction-for-instruction identical:
 * the mnemonic streams from the room-band clamp on for 257 instructions match FUN_0001bea8 at
 * 0x1c4bb exactly. So this body is fn_01bea8's with the chooser re-derived from
 * analysis/disasm/FUN_00020170.txt (generator: scratch/gen_020170.py). Its draw is the simplest in
 * the family -- prng()%4, with 2 -> +prng()%2 and 3 -> 4+prng()%2, so ONLY slots 0..5 are reachable
 * (no split draw, no absolute slots) -- and the spot is either the type's script-row byte 0 offset
 * beside the player (slots 0/1) or the player-OFFSET jitter rectangles (slots 2..5); there is no
 * absolute-jitter arm at all. Like 0x01cf64 it has NO 1-in-6 forced action 4 (pinned by test E), and
 * like 0x01cf64/0x01f064 the screen clamp runs before the distance test with the restore jumping
 * past it (test F).
 */
'''
open('/tmp/fn_020170_body.c','w').write(doc + body)
print(len((doc+body).splitlines()), "lines")
