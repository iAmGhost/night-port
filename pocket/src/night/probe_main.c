/* main.c -- openfpgaOS bring-up probe for the NIGHT Pocket port (task-2b).
 *
 * Purpose: prove the harness end to end -- the vendored SDK, the host
 * riscv64-elf toolchain, the APF core tree assembly, and the desktop (app_pc)
 * build of the same sources.  It draws what the NIGHT host will need to draw:
 * a 320x200 indexed picture centered in the 320x240 framebuffer, a 256-entry
 * palette pushed in one bulk call, and a pad-driven loop with edge detection.
 * The real host replaces this file; nothing here is game code.
 */
#include "of.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SCREEN_W 320
#define SCREEN_H 200

int main(void) {
    const struct of_capabilities *caps = of_get_caps();
    of_video_init();

    printf("openfpgaOS night probe: cpu=%u Hz sdram=%u KB heap=%u KB fb=%ux%u stride=%u mixer=%u voices\n",
           caps->cpu_freq_hz, caps->sdram_size / 1024, caps->heap_size / 1024,
           caps->fb_width, caps->fb_height, caps->fb_stride, caps->mixer_voices);

    /* A 64-step grey ramp plus a colour wedge: enough to see whether the
     * palette push and the indexed surface agree on both targets. */
    uint32_t pal[256];
    for (int i = 0; i < 256; i++) {
        uint32_t r, g, b;
        if (i < 64) { r = g = b = (uint32_t)(i * 4); }
        else        { r = (uint32_t)(255 - (i - 64) * 4); g = 64; b = (uint32_t)((i - 64) * 4); }
        pal[i] = (r << 16) | (g << 8) | b;
    }
    of_video_palette_bulk(pal, 256);
    of_video_set_color_mode(OF_VIDEO_MODE_8BIT);

    unsigned frame = 0;
    while (1) {
        of_input_poll();
        if (of_btn_pressed(OF_BTN_START)) break;

        uint8_t *fb = of_video_surface();
        memset(fb, 0, OF_FB_SIZE_8BIT);
        int top = (240 - SCREEN_H) / 2;                 /* 20 rows of bar top and bottom */
        for (int y = 0; y < SCREEN_H; y++)
            for (int x = 0; x < SCREEN_W; x++)
                fb[(top + y) * 320 + x] = (uint8_t)(((x + y + frame) >> 2) & 63);
        of_video_flip();
        frame++;
        usleep(16000);
    }
    printf("night probe: %u frames, exit on Start\n", frame);
    return 0;
}
