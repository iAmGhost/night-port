#ifndef NIGHT_STAGE_H
#define NIGHT_STAGE_H

#include <stdint.h>
#include <stdio.h>
#include "res.h"

typedef struct Stage Stage;

/* key bitmasks (down/pressed) */
enum {
    K_LEFT  = 1 << 0,
    K_RIGHT = 1 << 1,
    K_JUMP  = 1 << 2,
    K_PUNCH = 1 << 3,
    K_KICK  = 1 << 4,
    K_UP    = 1 << 5,
    K_DOWN  = 1 << 6,
};

/* stage_frame return codes */
enum { STG_PLAY, STG_ADVANCE, STG_GAMEOVER, STG_VICTORY };

/* episode = 1..9. demo != 0: built-in input script for headless runs. */
Stage  *stage_open(const Res *res, int episode, int demo, FILE *logf);
void    stage_close(Stage *s);
int     stage_frame(Stage *s, uint32_t *vpx, uint8_t down, uint8_t pressed, uint8_t anykey);
void    stage_animtest(Stage *s, uint32_t *vpx);   /* --animtest: player animation sheet */
int     stage_episode(const Stage *s);
int     stage_music_track(const Stage *s);
void    draw_text(uint32_t *vpx, int x, int y, const char *str, uint32_t col, int scale);

#endif
