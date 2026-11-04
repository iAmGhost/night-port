/* Probe: why does fn_037347 (stage teardown) fault in heap_free after a REAL stage load?
 *
 * It replicates fn_037347's free order exactly, but tracks every heap offset that is freed and
 * dumps the per-region bookkeeping (alloc/free counts) after every per-type slot, so the first
 * double free / counter underflow is named instead of surfacing later as a wild write.
 *
 *   cc -std=c11 -g -o /tmp/rtp scratch/real_teardown_probe.c port/src/f/f.c port/src/f/fx_04794a.c port/src/sys/sys.c -lm
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../port/src/f/f.h"
#include "../port/src/sys/sys.h"

#define ARENA_MAX (64u << 20)
static uint8_t *freed_bits;             /* one bit per heap offset: "already freed here" */
static const char *freed_what[64];      /* last few freed kinds, for the duplicate report */
static int freed_what_n = 0;

static int freed_mark(uint32_t off, const char *what)
{
    if (off == 0 || off >= ARENA_MAX) return -1;
    size_t byte = off >> 3;
    uint8_t bit = (uint8_t)(1u << (off & 7));
    if (freed_bits[byte] & bit) {
        printf("    !! DOUBLE FREE off=%x (%s)\n", off, what);
        return 1;
    }
    freed_bits[byte] |= bit;
    if (freed_what_n < 64) freed_what[freed_what_n++] = what;
    return 0;
}

static void stats(const char *tag)
{
    static uint32_t prev_alloc[4], prev_free[4]; int first = (strcmp(tag, "pre") == 0);
    int n = 0;
    for (uint32_t r = g_heap_head; r; r = (*(uint32_t **)((uint8_t *)g_heap_base + r))[2]) {
        uint32_t *h = (uint32_t *)((uint8_t *)g_heap_base + r);
        if (n < 4) {
            if (!first && h[6] > prev_alloc[n] && prev_alloc[n] == 0)
                printf("  %s region %x: ALLOC COUNT UNDERFLOW (was %u, now %u) free=%u\n",
                       tag, r, prev_alloc[n], h[6], h[7]);
            if (!first && h[7] > 4096) printf("  %s region %x: free count %u\n", tag, r, h[7]);
            prev_alloc[n] = h[6]; prev_free[n] = h[7];
        }
        if (++n > 8000) break;
    }
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    freed_bits = (uint8_t *)calloc(1, ARENA_MAX / 8);

    static uint8_t pool[0x53800]; memset(pool, 0, sizeof pool);
    g_ndpool = pool; g_node_list_active = 0; g_node_list_done = 0;
    static uint32_t dev[8]; g_dat_53730 = dev; g_dat_536dc = 8; g_dat_53619 = 0; g_dat_536d8 = 0;
    g_dat_633b0 = SCREEN_BASE; sys_ioctl_dx = 0; sys_ioctl_cf = 0; sys_file_image_set(0, 0);
    sys_file_real_dir("NIGHT");
    heap_arena_init(); heap_arena_cap_set(48u << 20); (void)heap_malloc(0x4000);

    g_639b8 = fn_04270d((const uint8_t *)"NIGHT.RES", "rb");
    fn_0366a4((uint8_t *)g_639b8);
    fn_03690b();
    static uint8_t recbuf[0x14 * 0x200];
    g_dat_5d0b4 = recbuf; g_dat_63370 = 1;
    g_dat_63e14 = 0; g_dat_63e18 = 0; g_dat_63e1c = 0; g_dat_63e20 = 0;
    for (uint32_t i = 1; i < 0x104u; i++) { g_dat_639ec[i] = 0; g_dat_53818[i] = 0; }  /* slot 0 = gun.spr/.act from the master load: the player type is 0 */
    g_dat_63dfc = 0; g_dat_6334c = 0; g_dat_63354 = 0; g_dat_63358 = 0; g_dat_6335c = 0; g_dat_62a90 = 0;

    g_dat_53c7c = (uint8_t *)calloc(1, 0x40);
    fn_0374ad();                       /* the real stage-entry transition */
    printf("after fn_0374ad: player=%p\n", (void *)g_dat_53c7c);
    fn_0376c8();                       /* the hero spawn */
    printf("after fn_0376c8: player=%p\n", (void *)g_dat_53c7c);
    stats("pre");

    uint8_t *hb = (uint8_t *)g_heap_base;

    /* ---- fn_037347's body, phase by phase ---- */
    fn_03237b();
    fn_03ee0a();
    g_dat_53c88 = 0;

    for (uint32_t i = 1; i < 0x104u; i++) {
        if (i == 0xc9u || i == 0xfau) continue;
        if (g_dat_639ec[i] == NULL) continue;
        printf("slot %02x\n", i);

        uint32_t *sp = (uint32_t *)g_dat_639ec[i];
        for (int k = 1; k <= 14; k++) freed_mark(sp[k], "spr sub");   /* fn_03dcbd frees these */
        (void)fn_03d999(sp, 0);
        freed_mark((uint32_t)(g_dat_639ec[i] - hb), "spr struct");
        heap_free_guard((uint32_t)(g_dat_639ec[i] - hb));

        uint32_t ao = g_dat_53818[i];
        if (ao != 0) {
            uint32_t *c = (uint32_t *)(hb + ao);
            uint32_t cnt = c[0], arr = c[1];
            if (arr != 0) {
                uint32_t *a = (uint32_t *)(hb + arr);
                for (uint32_t k = 0; k < cnt; k++) {
                    uint32_t off = a[k];
                    if (off == 0) continue;
                    uint8_t *r = hb + off;
                    for (int f = 0; f < 3; f++) {
                        uint32_t fo = *(uint32_t *)(r + 0x6d + f * 4);
                        if (fo) { freed_mark(fo, "act sub"); heap_free_host(fo); }
                    }
                    freed_mark(off, "act rec");
                    heap_free_guard(off);
                }
                freed_mark(arr, "act array");
                heap_free_host(arr);
            }
            freed_mark(ao, "act container");
            heap_free_guard(ao);
        }
        g_dat_639ec[i] = NULL;
        g_dat_53818[i] = 0;
        stats("   ");
    }

    if (g_dat_63dfc != NULL) {
        uint32_t *sp = (uint32_t *)g_dat_63dfc;
        for (int k = 1; k <= 14; k++) freed_mark(sp[k], "stage spr sub");
        (void)fn_03d999(sp, 0);
        freed_mark((uint32_t)((uint8_t *)g_dat_63dfc - hb), "stage spr");
        heap_free_guard((uint32_t)((uint8_t *)g_dat_63dfc - hb));
    }
    g_dat_63dfc = NULL;
    if (g_dat_6334c != NULL) {
        uint32_t r = fn_03c72b((uint32_t)(g_dat_6334c - hb));
        freed_mark(r, "room chain");
        heap_free_guard(r);
    }
    g_dat_6334c = NULL;
    stats("post");
    printf("teardown replica finished\n");

    sys_file_real_dir(0);
    return 0;
}
