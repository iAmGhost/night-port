/* docs/436: the palette ramp's math, tested on its own.
 *
 * Why a separate test exists for ~20 lines of interpolation: the ramp path lives behind `#ifndef OF_PC`
 * (docs/407: a knob that exists to fix a device number must not perturb the host that is the measurement
 * basis), so the desktop shim never runs it, and `frames.bin` is not reproducible (docs/410). That is the
 * third host-side feature in this project with no desktop validation -- and the row-hash lesson (docs/432)
 * was that a device-only path with untested math ships a wrong number, not a missing one. So the part of the
 * path that CAN be tested is separated into of_pal_ramp.h and tested here.
 *
 *   cc -I. test_pal_ramp.c -o ../../build/test_pal_ramp && ../../build/test_pal_ramp
 */
#include <stdio.h>
#include "of_pal_ramp.h"

static int fails;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static void fill(uint32_t *t, uint32_t v) { for (int i = 0; i < 256; i++) t[i] = v; }

int main(void)
{
    uint32_t from[256], target[256], out[256];

    /* The endpoints are the contract. A ramp that never lands is just a wrong palette on the panel, which is
     * the class of bug docs/422 (palette hash covering 37 of 256 entries) and docs/373 (index 255) both were:
     * an upload that stops short looks like a fade and IS a wrong colour. */
    fill(from, 0x000000u); fill(target, 0xffffffu);
    pal_ramp_step(from, target, 0, 50, out);
    for (int i = 0; i < 256; i++) CHECK(out[i] == from[i], "k=0 must reproduce `from` at %d (got %06x)\n", i, out[i]);
    pal_ramp_step(from, target, 50, 50, out);
    for (int i = 0; i < 256; i++) CHECK(out[i] == target[i], "k=n must land exactly on `target` at %d\n", i);
    pal_ramp_step(from, target, 999, 50, out);
    for (int i = 0; i < 256; i++) CHECK(out[i] == target[i], "k>n must also land exactly on `target` at %d\n", i);

    /* n == 0 would divide by zero; the shipped callers derive n from a write count that can be small but the
     * guard must not be a crash. */
    pal_ramp_step(from, target, 1, 0, out);
    CHECK(out[0] == target[0], "n=0 with k=1 must be treated as n=1 and land on target (got %06x)\n", out[0]);

    /* Monotonic and bounded, both directions. The original's fade-in is black -> room (docs/436: 26.5, 43.8,
     * 78.2, 95.4 mean luminance), and a step that overshoots or flickers backwards is the flicker symptom
     * docs/423/424 were chasing. */
    for (uint32_t n = 1; n <= 60; n++) {
        uint32_t prev_lum = 0;
        for (uint32_t k = 1; k <= n; k++) {
            pal_ramp_step(from, target, k, n, out);
            uint32_t lum = ((out[7] >> 16) & 0xff) + ((out[7] >> 8) & 0xff) + (out[7] & 0xff);
            CHECK(lum >= prev_lum, "n=%u k=%u went backwards (%u -> %u)\n", n, k, prev_lum, lum);
            prev_lum = lum;
            for (int c = 0; c < 3; c++) {
                uint32_t sh = c == 0 ? 16u : (c == 1 ? 8u : 0u);
                uint32_t v = (out[13] >> sh) & 0xffu;
                CHECK(v >= 0u && v <= 255u, "channel %d out of range (%u) at n=%u k=%u\n", c, v, n, k);
            }
        }
    }

    /* Fade-out mirror (fn_0406f5): room -> black, and black must be exact. */
    fill(from, 0x123456u); fill(target, 0x000000u);
    pal_ramp_step(from, target, 50, 50, out);
    CHECK(out[100] == 0u, "fade-out must end at exactly black (got %06x)\n", out[100]);
    pal_ramp_step(from, target, 25, 50, out);
    CHECK(out[100] == 0x091a2bu, "fade-out half-way should be half the colour (got %06x)\n", out[100]);

    /* Within one unit of the straight line, everywhere. */
    fill(from, 0x0a141eu); fill(target, 0xf0e8e0u);
    for (uint32_t k = 1; k < 40; k++) {
        pal_ramp_step(from, target, k, 40, out);
        for (int c = 0; c < 3; c++) {
            uint32_t sh = c == 0 ? 16u : (c == 1 ? 8u : 0u);
            int a = (int)((from[5] >> sh) & 0xff), b = (int)((target[5] >> sh) & 0xff);
            int want = a + (b - a) * (int)k / 40;
            int got = (int)((out[5] >> sh) & 0xff);
            CHECK(got >= want - 1 && got <= want + 1, "n=40 k=%u ch=%d off the line: want %d got %d\n", k, c, want, got);
        }
    }

    printf("%s: palette ramp math (%s)\n", fails ? "FAILED" : "PASS",
           fails ? "see FAIL lines above" : "endpoints exact, monotone, bounded, on the line");
    return fails ? 1 : 0;
}
