/* docs/91: the host-side scancode -> shipped DS key-byte map, shared by the SDL host and the test binary so
 * the mapping is asserted, not assumed. The per-key assignment below is MEASURED on the original
 * (scripts/playtest/probe_orig_keybytes.txt): holding a key in a live room sets exactly one byte in the
 * DS 0x63e80 window -- Up 0x48 -> DAT_00063ed0, Down 0x50 -> DAT_00063ed8, Left 0x4b -> DAT_00063ed3,
 * Right 0x4d -> DAT_00063ed5, Space 0x39 -> DAT_00063ec1, Enter 0x1c -> DAT_00063ea4,
 * F1 0x3b -> DAT_00063ec3.
 *
 * An earlier host guess crossed the pairs (up/left on ed0/ed3, down/right on ed5/ed8); the consumers
 * (fn_03272f in the room, fn_038f3d on the level-up screen, fn_039c03 in the setup menu, fn_038445 on the
 * game-over card) are ported at the shipped addresses, so the host assignment -- not the consumers -- is what
 * was wrong. Consequences of the corrected map, in the shipped code's own terms: the setup menu walks its
 * entry index with Up/Down (0x00039d2a reads DAT_63ed0, 0x00039d61 reads DAT_63ed8) and changes the selected
 * value with Left/Right (0x00039de2 / 0x00039ebb read DAT_63ed3 / DAT_63ed5); the level-up help scroll uses
 * Left/Right (0x000392af / 0x00039334).
 *
 * The "any key" bytes (DAT_00063eb2 / DAT_00063ebe) stay as the host has always driven them -- the shipped
 * writer sits in the unanalyzed keyboard-handler block at 0x0003fdxx-0x0003fexx (the only WRITE xrefs Ghidra
 * has for 0x63ea5 / 0x63eb2 / 0x63ec0, at 0x0003fe06) and a held-key sample never showed them set, so their
 * exact trigger is unresolved; the port sets them on every non-arrow make and that is what the shipped
 * key-waits and the setup menu need (docs/60). F1 is deliberately excluded: measured, it sets only
 * DAT_00063ec3, and setting the any-key bytes on F1 would leave the level-up screen instead of opening the
 * help pages its footer advertises.
 */
#include "keymap.h"

#include "../f/f.h"

void night_key_apply(uint8_t sc, int make) {
    switch (sc) {                                       /* the four held-arrow bytes, measured map */
    case NIGHT_SC_UP:    g_dat_63ed0 = (uint8_t)make; break;
    case NIGHT_SC_DOWN:  g_dat_63ed8 = (uint8_t)make; break;
    case NIGHT_SC_LEFT:  g_dat_63ed3 = (uint8_t)make; break;
    case NIGHT_SC_RIGHT: g_dat_63ed5 = (uint8_t)make; break;
    default: break;
    }

    uint8_t *byte =
        (sc == NIGHT_SC_ENTER) ? &g_dat_63ea4 :         /* confirm */
        (sc == NIGHT_SC_ESC)   ? &g_dat_63e89 :         /* abort */
        (sc == NIGHT_SC_F1)    ? &g_dat_63ec3 :         /* the level-up help pages, measured */
        (sc == NIGHT_SC_UP || sc == NIGHT_SC_DOWN ||
         sc == NIGHT_SC_LEFT || sc == NIGHT_SC_RIGHT) ? (uint8_t *)0
                                                      : &g_dat_63ec1;  /* a plain key press */
    if (byte != (uint8_t *)0) *byte = (uint8_t)make;

    if (sc != NIGHT_SC_UP && sc != NIGHT_SC_DOWN && sc != NIGHT_SC_LEFT &&
        sc != NIGHT_SC_RIGHT && sc != NIGHT_SC_F1) {
        /* If these latched, the setup menu (fn_039c03) would leave on its very first frame -- its entry
         * check spins until DAT_00063e89 / 0x63eb2 / 0x63ebe are clear and then exits with m = 0x64
         * (0x00039ce6-d25) -- and every "wait for a key" loop would fall through (docs/60). */
        g_dat_63eb2 = (uint8_t)make;
        g_dat_63ebe = (uint8_t)make;
    }

    g_dat_62c58[sc & 0x7f] = make ? 1 : 0;              /* scan-code slot, as fn_0343d1 arms it */
}
