/* test_pad_share.c -- the one-key-many-buttons rule that lets R1 and L1 both be F1.
 *
 * Run by make test.  Every case is a gesture, written as the button transitions a player actually
 * makes, and the expectation is the code the GUEST should see: 1 = make, 0 = break, -1 = send nothing.
 * The whole point is that F1 is typed ONCE per gesture however many shoulders take part in it.
 */
#include "of_pad_share.h"
#include <stdint.h>
#include <stdio.h>

static int fails, checks;

static void ck(const char *what, int got, int want) {
    checks++;
    if (got == want) return;
    fails++;
    printf("  [FAIL] %s: got %d want %d (-1=send nothing, 0=break, 1=make)\n", what, got, want);
}

/* One shared code, two buttons: 0 = L1, 1 = R1.  `down[]` is the per-button state the host keeps;
 * it is updated even when nothing is sent -- otherwise the second release cannot see the first. */
static uint8_t down[2];

static int ev(unsigned i, int now_held) {
    int other = (int)down[1 - i];
    int r = pad_share_event((int)down[i], now_held, other);
    down[i] = (uint8_t)now_held;                       /* the host updates state either way */
    return r;
}

int main(void) {
    /* A: press L1, add R1, lift R1, lift L1.  One make, one break. */
    down[0] = down[1] = 0;
    ck("A press L1",              ev(0, 1),  1);
    ck("A add R1 (key already down)", ev(1, 1), -1);
    ck("A lift R1 (key still held)",  ev(1, 0), -1);
    ck("A lift L1",              ev(0, 0),  0);

    /* B: the same gesture started on the other shoulder. */
    down[0] = down[1] = 0;
    ck("B press R1",              ev(1, 1),  1);
    ck("B add L1",                ev(0, 1), -1);
    ck("B lift L1",               ev(0, 0), -1);
    ck("B lift R1",               ev(1, 0),  0);

    /* C: the release order the sloppy thumb picks -- the first lift must NOT bring the key up,
     * and the second must bring it up exactly once. */
    down[0] = down[1] = 0;
    ck("C press L1",              ev(0, 1),  1);
    ck("C press R1",              ev(1, 1), -1);
    ck("C lift L1 FIRST (held via R1)", ev(0, 0), -1);
    ck("C lift R1",              ev(1, 0),  0);

    /* D: two separate taps on two different shoulders = two taps of F1, not one held key. */
    down[0] = down[1] = 0;
    ck("D tap L1 make",           ev(0, 1),  1);
    ck("D tap L1 break",          ev(0, 0),  0);
    ck("D tap R1 make",           ev(1, 1),  1);
    ck("D tap R1 break",          ev(1, 0),  0);

    /* E: nothing happening must not type anything. */
    down[0] = down[1] = 0;
    ck("E held stays held",       ev(0, 0), -1);
    down[0] = 1;
    ck("E released stays released", ev(0, 1), -1);
    ck("E both same",             pad_share_event(1, 1, 0), -1);

    printf("test_pad_share: %d checks, %d failure(s)\n", checks, fails);
    return fails ? 1 : 0;
}
