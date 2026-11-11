/* of_pad_share.h -- one guest key code, several pad buttons (docs: R1 and L1 are both F1).
 *
 * The keyboard seam is state-based: a press sends the make code, a release sends the break.  When two
 * pad buttons map to the same guest code, the naive loop sends the make twice (once per button) and
 * the break twice -- the game sees F1 pressed and released twice, so anything F1 opens gets toggled
 * twice by one gesture.  Worse, releasing either button would send a break while the other is still
 * held, so the key would come up under a thumb that has not lifted.
 *
 * So the decision per button transition is:
 *   press   -- send the make, unless the key is already down through the other button
 *   release -- send the break, unless the key is still held through the other button
 *
 * `other_down` must be read from the state BEFORE this entry is updated.  The helper skips self by
 * construction (the caller asks about the *others*), so updating first or asking first agree.
 *
 * This is a header rather than a few lines inside pad_pump() because it is the whole risk of the
 * feature, and the rest of pad_pump() is not reachable from a unit test.  See
 * scripts/pocket/test_pad_share.c, which runs in make test.
 */
#ifndef OF_PAD_SHARE_H
#define OF_PAD_SHARE_H

/* -1 = send nothing, 0 = send the break code, 1 = send the make code. */
static inline int pad_share_event(int was_down, int now_held, int other_down) {
    if (was_down == now_held) return -1;              /* nothing happened on this button */
    if (now_held) return other_down ? -1 : 1;         /* make, unless already down elsewhere */
    return other_down ? -1 : 0;                       /* break, unless still held elsewhere */
}

#endif /* OF_PAD_SHARE_H */
