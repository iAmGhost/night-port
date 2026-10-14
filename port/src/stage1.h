#ifndef NIGHT_STAGE1_H
#define NIGHT_STAGE1_H

#include <stdint.h>
#include <stdio.h>
#include "res.h"

typedef struct Stage1 Stage1;

/* key bitmasks (down/pressed) */
enum {
    K_LEFT  = 1 << 0,
    K_RIGHT = 1 << 1,
    K_JUMP  = 1 << 2,
    K_PUNCH = 1 << 3,
    K_KICK  = 1 << 4,
};

/* Load stage 1: background (stage1.pcx), stage script (stage1.stg),
 * walkmap (stage1.wal), player + spawned entities (.spr/.act tables),
 * punch SFX (1.voc). demo != 0: built-in input script (for headless runs). */
Stage1 *stage1_open(const Res *res, int demo, FILE *logf);
void    stage1_close(Stage1 *s);

/* Advance one frame; render into vpx (640x480 ARGB8888). */
void    stage1_frame(Stage1 *s, uint32_t *vpx, uint8_t down, uint8_t pressed);

#endif
