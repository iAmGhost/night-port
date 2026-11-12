/* docs/91 + docs/92: the host-side key event writer, reproducing the shipped IRQ 9 handler found at
 * 0x0003fd40 (installed on IRQ 9 by the block at 0x0003feb8). Linear disassembly in
 * analysis/disasm/kbd/kbd_handler_dis.txt; the handler per raw scan code read from port 0x60:
 *
 *   idx   = sc & 0x7f, ALIASED by the binary search at 0x0003fd9d-0x0003fdd9:
 *             Z 0x2c -> 0x1d   V 0x2f -> 0x1d        (punch: CTRL or Z or V)
 *             X 0x2d -> 0x38   B 0x30 -> 0x38        (kick:  ALT  or X or B)
 *             C 0x2e -> 0x2a   F 0x21 -> 0x2a  G 0x22 -> 0x2a   (jump: SHIFT or C)
 *   break (sc & 0x80): DAT_00063e88[idx] = 0                        (0x0003fe69-6e)
 *   make  : if (DAT_00063e88[idx] == 0) {                           (0x0003fdf2-ff)
 *             DAT_00063e88[idx] = 1;                                (0x0003fe06)
 *             if (DAT_00062c58[idx] != 0) DAT_00062cd8[idx]++;      (0x0003fe26 auto-repeat counter)
 *             else                      DAT_00062c58[idx] = 1;      (0x0003fe33 first make arms slot)
 *           }
 *           DAT_0006339c = 0;                                       (0x0003fe3a)
 *           if (player[0x17] == 0x28) { player[0x17] = 1; player[0x11] = 0 }  (0x0003fe44-60)
 *
 * Every "key byte" the port reads is one slot of the DAT_00063e88 table: 0x63e89=+0x01 Esc,
 * 0x63ea4=+0x1c Enter, 0x63ea5=+0x1d CTRL, 0x63eb2=+0x2a LShift, 0x63ebe=+0x36 RShift,
 * 0x63ec0=+0x38 LAlt, 0x63ec1=+0x39 Space, 0x63ec3=+0x3b F1, arrows +0x48/+0x4b/+0x4d/+0x50.
 * All assignments are MEASURED on the original (probe_orig_combat_keys.txt / probe_orig_modlatches.txt:
 * holding shift/ctrl/alt/space sets exactly +0x2a/+0x1d/+0x38/+0x39; Z sets +0x1d, X sets +0x38; the slot
 * array arms at the ALIASED index and its repeat counter at DAT_00062cd8+idx ticks while held).
 * The docs/60 "any key bytes 0x63eb2/0x63ebe" guess is retired: those are the two shift held-bytes.
 */
#include "keymap.h"

#include "../f/f.h"
#include <stdio.h>
#include <stdlib.h>

/* The controller layout, one table for every port (keymap.h spells the reason it lives here). The codes are
 * the modifier SLOTS, not the letters: night_key_alias() above is what turns Z/V into 0x1d in the first
 * place, and a controller has no letters printed on it, so there is no reason to go round the alias to get
 * to the same byte (docs/91 §the alias table, docs/333). */
const uint8_t night_pad_sc[NIGHT_PAD_N] = {
    [NIGHT_PAD_X]      = NIGHT_SC_CTRL,     /* punch -> the CTRL slot  */
    [NIGHT_PAD_A]      = NIGHT_SC_ALT,      /* kick  -> the ALT  slot  */
    [NIGHT_PAD_B]      = NIGHT_SC_LSHIFT,   /* jump  -> the SHIFT slot */
    [NIGHT_PAD_Y]      = NIGHT_SC_SPACE,    /* duck                    */
    [NIGHT_PAD_RB]     = NIGHT_SC_SPACE,    /* duck, again             */
    [NIGHT_PAD_LB]     = NIGHT_SC_F1,
    [NIGHT_PAD_START]  = NIGHT_SC_ENTER,
    [NIGHT_PAD_SELECT] = NIGHT_SC_ESC,
    [NIGHT_PAD_UP]     = NIGHT_SC_UP,
    [NIGHT_PAD_DOWN]   = NIGHT_SC_DOWN,
    [NIGHT_PAD_LEFT]   = NIGHT_SC_LEFT,
    [NIGHT_PAD_RIGHT]  = NIGHT_SC_RIGHT,
};

/* the handler's alias table, 0x0003fd9d-0x0003fdd9 */
static uint8_t night_key_alias(uint8_t idx) {
    switch (idx) {
    case 0x2c: case 0x2f: return 0x1d;                  /* Z, V -> CTRL slot */
    case 0x2d: case 0x30: return 0x38;                  /* X, B -> ALT slot */
    case 0x21: case 0x22: case 0x2e: return 0x2a;       /* F, G, C -> SHIFT slot */
    default: return idx;
    }
}

/* the DAT_00063e88[idx] held-byte, modelled as the individual globals the shipped readers use */
static uint8_t *night_held_byte(uint8_t idx) {
    switch (idx) {
    case 0x01: return &g_dat_63e89;   case 0x1c: return &g_dat_63ea4;
    case 0x1d: return &g_dat_63ea5;   case 0x2a: return &g_dat_63eb2;
    case 0x36: return &g_dat_63ebe;   case 0x38: return &g_dat_63ec0;
    case 0x39: return &g_dat_63ec1;   case 0x3b: return &g_dat_63ec3;
    case 0x48: return &g_dat_63ed0;   case 0x4b: return &g_dat_63ed3;
    case 0x4d: return &g_dat_63ed5;   case 0x50: return &g_dat_63ed8;
    default:   return &g_dat_63e88[idx];                /* the rest of the 0x80-byte table */
    }
}

void night_key_apply(uint8_t sc, int make) {
    uint8_t idx = night_key_alias(sc & 0x7f);
    uint8_t *held = night_held_byte(idx);

    if (getenv("NIGHT_LATCHDBG"))     /* writer census: every change to a DAT_63e88 latch byte (docs/323 §4) */
        fprintf(stderr, "latchwrite sc=%02x make=%d idx=%02x before=%u\n",
                sc, make, idx, (unsigned)*held);

    if (sc & 0x80) { make = 0; }                        /* explicit break code wins */
    if (!make) {
        *held = 0;                                      /* 0x0003fe69-6e: only the held byte clears */
        return;
    }

    if (*held == 0) {                                   /* 0x0003fdf2-ff */
        *held = 1;                                      /* 0x0003fe06 */
        if (g_dat_62c58[idx] != 0) {                    /* 0x0003fe12-26: a re-make while the slot
                                                         * state machine still holds it -> auto-repeat */
            g_dat_62cd8[idx]++;
        } else {
            g_dat_62c58[idx] = 1;                       /* 0x0003fe33: first make arms the slot */
        }
    }

    g_dat_6339c = 0;                                    /* 0x0003fe3a */
    if (g_player_ent != (uint8_t *)0 && g_player_ent[0x17] == 0x28) {
        g_player_ent[0x17] = 1;                         /* 0x0003fe44-60: a key leaves state 0x28 */
        g_player_ent[0x11] = 0;
    }
}
