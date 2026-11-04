/* Probe 3: validate every block header the stage teardown is about to free.
 * Output is deliberately bounded (only anomalies + a summary).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../port/src/f/f.h"
#include "../port/src/sys/sys.h"

static uint8_t *hb;
static int anomalies = 0, checked = 0, regions = 0;

static uint32_t find_region(uint32_t blk)          /* same walk as heap_free_host */
{
    uint32_t r = g_heap_head;
    for (;;) {
        uint32_t nx = ((uint32_t *)(hb + r))[2];
        if (nx == 0) break;
        if (r > blk) { r = nx; continue; }
        if (blk < nx) break;
        r = nx;
    }
    return r;
}

static void chk(uint32_t off, const char *what)
{
    if (off == 0) return;
    checked++;
    uint32_t blk = off - 4;
    uint32_t r = find_region(blk);
    uint32_t *rh = (uint32_t *)(hb + r);
    uint32_t total = rh[0], nx = rh[2];
    uint32_t sz = ((uint32_t *)(hb + blk))[0];
    int bad = 0;
    if (blk < r + 0x2c || blk >= r + total) bad = 1;               /* outside this region's data area */
    else if ((sz & 1) == 0) bad = 2;                              /* already free */
    else if (sz < 0xc || sz > total) bad = 3;                     /* implausible size */
    else if (blk + (sz & ~1u) > r + total) bad = 4;               /* overruns the region */
    if (bad && anomalies < 25)
        printf("BAD(%d) %-12s off=%-8x region=%-8x total=%-6x next=%-8x size=%08x\n",
               bad, what, off, r, total, nx, sz);
    if (bad) anomalies++;
}

int main(void)
{
    setvbuf(stdout, 0, _IONBF, 0);
    static uint8_t pool[0x53800]; memset(pool, 0, sizeof pool);
    g_ndpool = pool; g_node_list_active = 0; g_node_list_done = 0;
    static uint32_t dev[8]; g_dat_53730 = dev; g_dat_536dc = 8; g_dat_53619 = 0; g_dat_536d8 = 0;
    g_dat_633b0 = SCREEN_BASE; sys_ioctl_dx = 0; sys_ioctl_cf = 0; sys_file_image_set(0, 0);
    sys_file_real_dir("NIGHT");
    heap_arena_init(); heap_arena_cap_set(48u << 20); heap_region_floor_set(0x10000); (void)heap_malloc(0x4000);

    g_639b8 = fn_04270d((const uint8_t *)"NIGHT.RES", "rb");
    fn_0366a4((uint8_t *)g_639b8);
    fn_03690b();
    static uint8_t recbuf[0x14 * 0x200];
    g_dat_5d0b4 = recbuf; g_dat_63370 = 1;
    g_dat_63e14 = 0; g_dat_63e18 = 0; g_dat_63e1c = 0; g_dat_63e20 = 0;
    for (uint32_t i = 1; i < 0x104u; i++) { g_dat_639ec[i] = 0; g_dat_53818[i] = 0; }
    g_dat_63dfc = 0; g_dat_6334c = 0; g_dat_63354 = 0; g_dat_63358 = 0; g_dat_6335c = 0; g_dat_62a90 = 0;
    g_dat_53c7c = (uint8_t *)calloc(1, 0x40);
    fn_0374ad();
    fn_0376c8();
    hb = (uint8_t *)g_heap_base;

    for (uint32_t r = g_heap_head, n = 0; r; r = ((uint32_t *)(hb + r))[2]) { if (++n > 100000) break; regions = n; }
    printf("regions in chain: %d\n", regions);

    /* validate everything fn_037347 is about to free, WITHOUT freeing it */
    for (uint32_t i = 1; i < 0x104u; i++) {
        if (i == 0xc9u || i == 0xfau) continue;
        if (!g_dat_639ec[i]) continue;
        uint32_t *sp = (uint32_t *)g_dat_639ec[i];
        chk((uint32_t)((uint8_t *)sp - hb), "spr struct");
        for (int k = 1; k <= 14; k++) chk(sp[k], "spr sub");
        uint32_t ao = g_dat_53818[i];
        if (ao) {
            chk(ao, "act cont");
            uint32_t *c = (uint32_t *)(hb + ao);
            uint32_t cnt = c[0], arr = c[1];
            chk(arr, "act array");
            if (arr && arr < 0x4000000u) {
                uint32_t *a = (uint32_t *)(hb + arr);
                for (uint32_t k = 0; k < cnt && k < 4096; k++) {
                    chk(a[k], "act rec");
                    if (a[k] && a[k] < 0x4000000u) {
                        uint8_t *r = hb + a[k];
                        for (int f = 0; f < 3; f++) chk(*(uint32_t *)(r + 0x6d + f * 4), "act sub");
                    }
                }
            }
        }
    }
    if (g_dat_63dfc) {
        uint32_t *sp = (uint32_t *)g_dat_63dfc;
        chk((uint32_t)((uint8_t *)sp - hb), "stage spr");
        for (int k = 1; k <= 14; k++) chk(sp[k], "stage sub");
    }
    if (g_dat_6334c) chk((uint32_t)(g_dat_6334c - hb), "room chain");

    printf("checked=%d anomalies=%d\n", checked, anomalies);

    /* now actually run the teardown, so we see whether it faults after a clean validation */
    fn_037347();
    printf("fn_037347 survived\n");
    sys_file_real_dir(0);
    return 0;
}
