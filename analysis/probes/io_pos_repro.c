#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "f/f.h"
#include "sys/sys.h"
static void dump(const char *tag, res_reader *r) {
    printf("%-22s pos=%u remaining=%u end=%u id=%u flags=%02x/%02x  sysTell=%d\n", tag,
           r->pos, r->remaining, r->end, r->id, r->flags, r->flags2,
           sys_file_lseek(r->id, 0, 1));
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    static uint8_t pool[0x53800];
    g_ndpool = pool; g_node_list_active = 0; g_node_list_done = 0;
    static uint32_t dev_tab[8];
    g_dat_53730 = dev_tab; g_dat_536dc = 8; g_dat_53619 = 0; g_dat_536d8 = 0;
    const uint32_t rawLen = 0x14u, of0 = 4 + rawLen, cursor = of0 + 0x80u + 4u;
    uint8_t *img = calloc(1, cursor);
    img[0] = 1;
    uint8_t t2[0x14]; memset(t2, 0, sizeof t2); memcpy(t2, "ev1.dat", 7);
    t2[16] = (uint8_t)of0; t2[17] = (uint8_t)(of0 >> 8);
    uint8_t prev = 0x20;
    for (int k = 0; k < 0x14; k++) { uint8_t raw = (uint8_t)(t2[k] ^ prev); img[4 + k] = raw; prev = raw; }
    *(uint16_t *)(img + of0 + 4) = 0xaf11; *(uint16_t *)(img + of0 + 6) = 3;
    sys_file_image_set(img, cursor);
    heap_arena_init(); (void)heap_malloc(0x4000);
    res_reader *r = g_639b8 = fn_04270d((const uint8_t *)"night.res", "rb");
    dump("after open", r);
    fn_0366a4((uint8_t *)r);
    dump("after index", r);
    printf("find=%d\n", fn_0367c9("ev1.dat"));
    dump("after seek1", r);
    uint8_t b[0x80]; memset(b, 0xee, sizeof b);
    fn_04192e(b, 0x80, 1, (uint8_t *)r);
    printf("read1: %02x %02x %02x %02x (want 00 00 11 af)\n", b[0], b[1], b[2], b[3]);
    { uint8_t *hb2 = (uint8_t *)g_heap_base; uint32_t base = *(uint32_t *)((uint8_t *)r + 0x08);
      uint8_t *bb = hb2 + base;
      printf("buf[base..base+8]      = %02x %02x %02x %02x %02x %02x %02x %02x\n", bb[0],bb[1],bb[2],bb[3],bb[4],bb[5],bb[6],bb[7]);
      printf("buf[base+24..+32]      = %02x %02x %02x %02x %02x %02x %02x %02x\n", bb[24],bb[25],bb[26],bb[27],bb[28],bb[29],bb[30],bb[31]);
      printf("img[0..8]=%02x %02x %02x %02x %02x  img[24..32]=%02x %02x %02x %02x %02x\n",
             img[0],img[1],img[2],img[3],img[4], img[24],img[25],img[26],img[27],img[28]);
      printf("node bufsize(+0x14)=%u  buf+0x10(handle)=%u\n", *(uint32_t *)((uint8_t *)r + 0x14), *(uint32_t *)((uint8_t *)r + 0x10)); }
    dump("after read1", r);
    printf("find=%d\n", fn_0367c9("ev1.dat"));
    dump("after seek2", r);
    *(uint16_t *)(img + of0 + 4) = 0xbeef;   /* rewrite the record AFTER it was buffered */
    printf("-- rewriting img[28..29] to be ef (record tag) while the block is still buffered --\n");
    memset(b, 0xee, sizeof b);
    fn_04192e(b, 4, 1, (uint8_t *)r);
    printf("read2(4 B): %02x %02x %02x %02x\n", b[0], b[1], b[2], b[3]);
    dump("after read2 (4B)", r);
    memset(b, 0xee, sizeof b);
    fn_04192e(b, 0x80, 1, (uint8_t *)r);   /* runs past the buffered block -> refill */
    printf("read3(0x80 B) tag bytes at +4: %02x %02x (0x11af = stale block copy, 0xbeef = re-read)\n", b[4], b[5]);
    dump("after read3", r);
    dump("after read2", r);
    return 0;
}
