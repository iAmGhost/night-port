#ifndef NIGHT_KEYMAP_H
#define NIGHT_KEYMAP_H
/* docs/91: the host-side scancode -> shipped DS key-byte map, measured on the ORIGINAL.
 *
 * Measured in a live episode-1 room by scripts/playtest/probe_orig_keybytes.txt (new harness ops
 * `key_down` / `key_up` / `readbytes` in scripts/playtest_orig_drive.py): holding a key and dumping the
 * 0x68-byte window at DS 0x63e80 shows exactly ONE byte set per key --
 *
 *     Up 0x48 -> DAT_00063ed0      Left  0x4b -> DAT_00063ed3      Space 0x39 -> DAT_00063ec1
 *     Down 0x50 -> DAT_00063ed8    Right 0x4d -> DAT_00063ed5      Enter 0x1c -> DAT_00063ea4
 *     F1 0x3b -> DAT_00063ec3      (the level-up screen's "help pages are being shown" mode byte,
 *                                   read at 0x0003922a/0x0003923b and 0x0003b3b1 and written by the
 *                                   keyboard handler in the unanalyzed 0x0003fdxx-0x0003fexx block)
 *
 * The DOS key-name -> scancode anchor for that table is the game's own scan-code slot array
 * DAT_00062c58: holding Enter / Space / F1 arms slots 0x1c / 0x39 / 0x3b (measured by dumping the array),
 * so the emulator's KBD_* names address the standard scancodes.
 */
#include <stdint.h>

enum {
    NIGHT_SC_ESC   = 0x01,
    NIGHT_SC_F     = 0x21,
    NIGHT_SC_G     = 0x22,
    NIGHT_SC_ENTER = 0x1c,
    NIGHT_SC_CTRL  = 0x1d,     /* held byte DAT_00063ea5 (docs/92) */
    NIGHT_SC_LSHIFT = 0x2a,    /* held byte DAT_00063eb2 */
    NIGHT_SC_Z     = 0x2c,     /* -> 0x1d, the punch alias (handler 0x0003fdd1) */
    NIGHT_SC_X     = 0x2d,     /* -> 0x38, the kick alias */
    NIGHT_SC_C     = 0x2e,     /* -> 0x2a, the jump alias */
    NIGHT_SC_V     = 0x2f,     /* -> 0x1d */
    NIGHT_SC_B     = 0x30,     /* -> 0x38 */
    NIGHT_SC_RSHIFT = 0x36,    /* held byte DAT_00063ebe, its OWN slot */
    NIGHT_SC_ALT   = 0x38,     /* held byte DAT_00063ec0 */
    NIGHT_SC_SPACE = 0x39,     /* held byte DAT_00063ec1 */
    NIGHT_SC_F1    = 0x3b,
    NIGHT_SC_LEFT  = 0x4b,
    NIGHT_SC_UP    = 0x48,
    NIGHT_SC_DOWN  = 0x50,
    NIGHT_SC_RIGHT = 0x4d
};

void night_key_apply(uint8_t sc, int make);

#endif
