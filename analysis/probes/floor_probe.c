#include <stdio.h>
#include <string.h>
#include "../../src/f/f.h"
#include "../../src/sys/sys.h"
int main(void){
  static uint8_t pool[0x4000]; memset(pool,0,sizeof pool);
  g_ndpool=pool; g_node_list_active=0; g_node_list_done=0; g_dat_633b0=SCREEN_BASE;
  uintptr_t saved = g_heap_base; heap_arena_init();
  heap_arena_cap_set(1u<<20); heap_region_floor_set(0x1000);
  uint32_t d2 = heap_malloc(0x10); uint32_t r2 = d2 - 0x30;
  printf("d2=%x region=%x total=%x root=%x largest=%x hwm=%x alloc=%u free=%u\n",
         d2, r2, g_heap_base==0?0:((uint32_t*)((uint8_t*)g_heap_base+r2))[0],
         ((uint32_t*)((uint8_t*)g_heap_base+r2))[3], ((uint32_t*)((uint8_t*)g_heap_base+r2))[4],
         ((uint32_t*)((uint8_t*)g_heap_base+r2))[5], ((uint32_t*)((uint8_t*)g_heap_base+r2))[6],
         ((uint32_t*)((uint8_t*)g_heap_base+r2))[7]);
  printf("head=%x cursor=%x highwater=%x\n", g_heap_head, g_heap_cursor, g_heap_highwater);
  uint32_t d3 = heap_malloc(0x10);
  printf("d3=%x (same region? %s) head=%x cursor=%x\n", d3, (d3-0x30==r2)?"yes":"NO", g_heap_head, g_heap_cursor);
  uint32_t r3 = d3 - 0x30;
  if (r3 != r2) { uint32_t *h=(uint32_t*)((uint8_t*)g_heap_base+r3); printf("new region %x total=%x\n", r3, h[0]); }
  g_heap_base = saved; return 0;
}
