/* Probe 2: is the port's REGION CHAIN intact after the real data load?
 * heap_free_host / heap_malloc walk it with B_NEXT(region) (region+0x08); a corrupted link makes
 * them pick the wrong region, which would explain the teardown faulting inside heap_free.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../port/src/f/f.h"
#include "../port/src/sys/sys.h"

static void chain(const char *tag)
{
    uint8_t *hb = (uint8_t *)g_heap_base;
    printf("%s: head=%x cursor=%x highwater=%x used_arena_max=%x\n", tag, g_heap_head, g_heap_cursor,
           g_heap_highwater, 0);
    uint32_t r = g_heap_head, n = 0;
    uint32_t prev = 0;
    while (r) {
        uint32_t *h = (uint32_t *)(hb + r);
        uint32_t total = h[0], nx = h[2], pv = h[1];
        printf("   region %02u @%x: total=%x next=%x prev=%x root=%x largest=%x hwm=%x alloc=%u free=%u sent=%x(s=%x p=%x n=%x)",
               n, r, total, nx, pv, h[3], h[4], h[5], h[6], h[7], r + 0x20, h[8], h[9], h[10]);
        if (pv != prev) printf("   <-- PREV MISMATCH (walked prev=%x)", prev);
        if (n && r <= prev) printf("   <-- NOT ASCENDING");
        if (nx && nx <= r) printf("   <-- NEXT NOT ASCENDING");
        if (nx && nx > 0x4000000u) printf("   <-- NEXT OUT OF ARENA");
        printf("\n");
        prev = r; r = nx;
        if (++n > 200) { printf("   (chain too long, stop)\n"); break; }
    }
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    static uint8_t pool[0x53800]; memset(pool, 0, sizeof pool);
    g_ndpool = pool; g_node_list_active = 0; g_node_list_done = 0;
    static uint32_t dev[8]; g_dat_53730 = dev; g_dat_536dc = 8; g_dat_53619 = 0; g_dat_536d8 = 0;
    g_dat_633b0 = SCREEN_BASE; sys_ioctl_dx = 0; sys_ioctl_cf = 0; sys_file_image_set(0, 0);
    sys_file_real_dir("NIGHT");
    heap_arena_init();
    chain("arena init");
    heap_arena_cap_set(48u << 20);
    (void)heap_malloc(0x4000);
    chain("after seed malloc 0x4000");

    g_639b8 = fn_04270d((const uint8_t *)"NIGHT.RES", "rb");
    fn_0366a4((uint8_t *)g_639b8);
    chain("after index");
    fn_03690b();
    chain("after fn_03690b master load");

    static uint8_t recbuf[0x14 * 0x200];
    g_dat_5d0b4 = recbuf; g_dat_63370 = 1;
    g_dat_63e14 = 0; g_dat_63e18 = 0; g_dat_63e1c = 0; g_dat_63e20 = 0;
    for (uint32_t i = 1; i < 0x104u; i++) { g_dat_639ec[i] = 0; g_dat_53818[i] = 0; }
    g_dat_63dfc = 0; g_dat_6334c = 0; g_dat_63354 = 0; g_dat_63358 = 0; g_dat_6335c = 0; g_dat_62a90 = 0;
    g_dat_53c7c = (uint8_t *)calloc(1, 0x40);
    fn_0374ad();
    chain("after fn_0374ad stage load");
    fn_0376c8();
    chain("after fn_0376c8 spawn");

    sys_file_real_dir(0);
    return 0;
}
