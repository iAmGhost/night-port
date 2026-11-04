src = open('port/src/f/f.c').read()
start = src.index('void fn_01bea8(uint8_t *e)\n{')
end = src.find("\n/* =====================================================================", start)
if end < 0: end = len(src)
body = src[start:end].rstrip() + "\n"
body = body.replace('void fn_01bea8(uint8_t *e)\n{', 'void fn_0221b8(uint8_t *e)\n{', 1)
a = body.index('    case 0x00:\n')
mark = "                /* LAB 0x019392: clamp the chosen spot into the current room band */"
b = body.index(mark)
new = '''    case 0x00:
        if (*(int32_t *)(e + 0xaf) != 4) {
            if (*(int32_t *)(e + 0xaf) == 0x0a) {
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) >> 1;
            } else {
                int32_t j = (int32_t)prng_next() % 0x32;         /* 0x0002243f-0x00022453 */
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x18) + j;
            }
            *(int32_t *)(e + 0x1f)  = 4;
            *(int32_t *)(e + 0x113) = (int32_t)prng_next() % 10;  /* no 1-in-6 forced action 4 */
        }
        if (*(int32_t *)(e + 0xaf) == 4) {
            if (*(int32_t *)(e + 0x8f) == 1) {
                *(int32_t *)(e + 0x97) = ai_rec_i32(ty, 0x14) >> 1;
                *(int32_t *)(e + 0x1f) = 3;
            } else {
                if ((int32_t)fn_01519f(e) < 0 || (int32_t)fn_01519f(e) > 0x13f) {
                    *(int32_t *)(e + 0x1f) = 7;                  /* 0x000224f3: off screen, give up */
                    return;
                }
                *(int32_t *)(e + 0xab) = ai_rec_i32(ty, 0x14) >> 1;
                if (*(int32_t *)(e + 0xc7) == 1) {
                    int32_t slot;                                /* [EBP-0x58] */
                    do {                                         /* 0x00022524-0x00022587: same draw as
                         * 0x020170 -- prng()%4 with 2 -> +prng()%2 and 3 -> 4+prng()%2, slots 0..5 */
                        slot = (int32_t)prng_next() % 4;
                        if (slot == 2) {
                            slot += (int32_t)prng_next() % 2;
                        } else if (slot == 3) {
                            slot = 4 + (int32_t)prng_next() % 2;
                        }
                    } while (slot == *(int32_t *)(e + 0xff));
                    if (slot == 0 || slot == 1) *(int32_t *)(e + 0xc3) = (uint32_t)p_y;
                    *(int32_t *)(e + 0x6f) = (slot == 0 || slot == 1) ? 1u : 0u;
                    *(int32_t *)(e + 0xff) = (uint32_t)slot;
                    int32_t keep_x = *(int32_t *)(e + 0xbf);     /* [EBP-0x70] */
                    int32_t keep_y = *(int32_t *)(e + 0xc3);     /* [EBP-0x6c] */
                    /* 0x000225f0-0x00022641: ONE arm for every slot -- the jitter rectangle of the
                     * slot offset by the player in X, and the player's own Y. No script-row offset and
                     * no absolute arm; the entity[0xc3] pre-write above is redundant against the
                     * unconditional Y below, but the original really does it twice. */
                    int32_t span = J4_XHI(slot) - J4_XLO(slot);
                    int32_t rr   = (int32_t)prng_next() % span;
                    *(int32_t *)(e + 0xbf) = (uint32_t)(J4_XLO(slot) + p_x + abs32(rr));
                    *(int32_t *)(e + 0xc3) = (uint32_t)p_y;
                    /* 0x00022647: clamp before the distance test; the restore jumps past it
                     * (LAB 0x000226e5), so a rejected spot keeps its raw value. */
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
 * 0x0221b8 -- THE BEHAVIOUR-0x15 ENTITY AI HANDLER (docs/18-ai-dispatch-map.md)
 *
 * Tenth of the 18; scratch/aicases.py again shows only case 0x00 differing from fn_01bea8, and the
 * 257-instruction mnemonic stream from the room-band clamp on (FUN_000221b8 @0x226f4 vs
 * FUN_0001bea8 @0x1c4bb) is identical, so the body is the template with the chooser re-derived from
 * analysis/disasm/FUN_000221b8.txt (generator: scratch/gen_0221b8.py). Its draw matches 0x020170
 * (prng()%4 with the 2/3 sub-draws, slots 0..5, no 1-in-6 forced action 4), but the spot has a
 * SINGLE arm for every slot: X from the slot's jitter rectangle offset by the player, Y always the
 * player's own Y (0x0002263b) -- no script-row offset arm and no absolute arm at all. The
 * entity[0xc3] pre-write for slots 0/1 (0x00022595) is redundant against that unconditional Y write;
 * it is kept because the original writes it. The clamp runs before the distance test with the
 * restore jumping past it (LAB 0x000226e5).
 */
'''
open('/tmp/fn_0221b8_body.c','w').write(doc + body)
print(len((doc+body).splitlines()), "lines")
