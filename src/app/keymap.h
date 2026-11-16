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

/* ---- the controller layout, in the shape an Xbox pad reports ----
 *
 * X punch, A kick, B jump, Y *and* RB duck, LB F1, START confirm, SELECT the pause menu (docs/93), the cross
 * and the left stick move -- and read those letters as POSITIONS, not as labels, because they are the Xbox
 * One diamond: punch WEST, kick SOUTH, jump EAST, duck NORTH. pocket/src/night/main_of.c's g_pad_keys, wasm/runtime/shell.js's GP_MAP and
 * src/app/main.c's g_pad_btns all run this table (docs/333 is the measurement behind it), and it lives here
 * -- port-neutral, with no SDL_GamepadButton and no OF_BTN in sight -- because the fixture suite is the only
 * thing this port has ever that a controller can be checked against: no machine this port is built on has a
 * pad plugged into it. A position is worth asserting for the SLOT it arms, not for the letter on it, since
 * the slot is what the game reads.
 *
 * That last sentence is load-bearing for a host that names its buttons by the letter printed on a Nintendo
 * diamond -- X top, A right, B bottom, Y left, which is what the Analogue Pocket face says and what
 * openfpgaOS's OF_BTN_* follows.  Its letters sit one pair-swap away from Xbox's: the top button is duck
 * there, not punch, and the right one is jump, not kick.  Such a host must INVERT the A/B and X/Y pairs when
 * it turns a name into a position, or it ships a layout mirrored from the one every other host runs; pocket/src/night/main_of.c's g_pad_keys is the one that does.
 *
 * Duck has two homes on purpose: it is held longer than anything else in the game, and RB is the only other
 * button in comfortable reach of the thumb that is already doing everything else. */
enum {
    NIGHT_PAD_X, NIGHT_PAD_A, NIGHT_PAD_B, NIGHT_PAD_Y, NIGHT_PAD_RB, NIGHT_PAD_LB,
    NIGHT_PAD_START, NIGHT_PAD_SELECT, NIGHT_PAD_UP, NIGHT_PAD_DOWN, NIGHT_PAD_LEFT,
    NIGHT_PAD_RIGHT, NIGHT_PAD_N
};
extern const uint8_t night_pad_sc[NIGHT_PAD_N];

void night_key_apply(uint8_t sc, int make);

#endif
