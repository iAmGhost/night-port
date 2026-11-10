#ifndef OFPALRAMP_H
#define OFPALRAMP_H
#include <stdint.h>

/* docs/436: the panel-side palette ramp's interpolation, as a pure function in a header, because the ramp
 * path is device-only (docs/407: a knob that exists to fix a device number stays off the measurement host)
 * and a device-only path that also hides its math is a path nothing can be trusted to get right.
 * scripts/pocket/test_pal_ramp.c is that nothing. */

/* One channel of the ramp. The signedness is the whole bug risk and the test found it here: subtracting two
 * uint32_t channel values yields a UNSIGNED value, and `unsigned * (int32_t)k` converts the multiplier to
 * unsigned, so a fade toward black multiplied 0xFFFFFFEE by 25 and divided that, producing 0xbffefb7c where
 * 0x00001a was wanted. Converting to int32_t BEFORE the arithmetic is the fix; the first cut of this header
 * had it after, and scripts/pocket/test_pal_ramp.c is the only reason it did not ship. */
static inline uint32_t pal_ramp_ch(uint32_t a, uint32_t b, uint32_t sh, uint32_t k, uint32_t n)
{
    int32_t av = (int32_t)((a >> sh) & 0xffu);
    int32_t bv = (int32_t)((b >> sh) & 0xffu);
    return (uint32_t)(av + (int32_t)((bv - av) * (int32_t)k / (int32_t)n));
}

/* Ramp step k of n between `from` and `target` into `out`. The contract is the endpoints: k == 0 reproduces
 * `from`, k >= n reproduces `target` EXACTLY (a ramp that never lands is simply the wrong palette on the
 * panel), and no intermediate may leave the straight line by more than truncation. */
static inline void pal_ramp_step(const uint32_t *from, const uint32_t *target, uint32_t k, uint32_t n,
                                 uint32_t *out)
{
    if (n == 0u) n = 1u;
    if (k >= n) { for (int i = 0; i < 256; i++) out[i] = target[i]; return; }
    for (int i = 0; i < 256; i++) {
        uint32_t a = from[i], b = target[i];
        out[i] = (pal_ramp_ch(a, b, 16u, k, n) << 16) |
                 (pal_ramp_ch(a, b, 8u, k, n) << 8) |
                  pal_ramp_ch(a, b, 0u, k, n);
    }
}

#endif
