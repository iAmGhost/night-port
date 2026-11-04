#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../port/src/f/f.h"
#include "../port/src/sys/sys.h"
int main(void){
  setvbuf(stdout, 0, _IONBF, 0);
  static uint8_t pool[0x53800]; memset(pool,0,sizeof pool);
  g_ndpool=pool; g_node_list_active=0; g_node_list_done=0;
  static uint32_t dev[8]; g_dat_53730=dev; g_dat_536dc=8; g_dat_53619=0; g_dat_536d8=0;
  g_dat_633b0=SCREEN_BASE; sys_ioctl_dx=0; sys_ioctl_cf=0; sys_file_image_set(0,0);
  sys_file_real_dir("NIGHT");
  heap_arena_init(); heap_arena_cap_set(48u<<20); (void)heap_malloc(0x4000);
  g_639b8 = fn_04270d((const uint8_t*)"NIGHT.RES","rb");
  fn_0366a4((uint8_t*)g_639b8);
  fn_03690b();
  static uint8_t recbuf[0x14*0x200];
  g_dat_5d0b4=recbuf; g_dat_63370=1;
  g_dat_63e14=0; g_dat_63e18=0; g_dat_63e1c=0; g_dat_63e20=0;
  g_dat_53c7c = (uint8_t*)calloc(1,0x40);
  fn_0374ad();
  uint8_t *hb=(uint8_t*)g_heap_base;
  for (int t=1;t<0x65;t++){
    if(!g_dat_639ec[t] && !g_dat_53818[t]) continue;
    uint32_t ao=g_dat_53818[t];
    uint32_t *c=(uint32_t*)(hb+ao);
    printf("type %02x: spr=%s act_off=%x count=%u array_off=%x\n", t, g_dat_639ec[t]?"y":"n", ao, c[0], c[1]);
    if(c[1]){ uint32_t *a=(uint32_t*)(hb+c[1]);
      printf("    array[0..5]:"); for(int i=0;i<6 && i<(int)c[0];i++) printf(" %x", a[i]);
      printf("\n"); }
  }
  static uint8_t cfg[0x40]; g_dat_53c7c = cfg;
  fn_0376c8();
  printf("spawn -> entity %p\n", (void*)g_dat_53c7c);
  /* scan every action record for bad sub-object offsets (0 = allocation failed at load) */
  int badzero=0, badbig=0, ok=0;
  for (int t=1;t<0x65;t++){
    uint32_t ao=g_dat_53818[t]; if(!ao) continue;
    uint32_t *c=(uint32_t*)(hb+ao); uint32_t *a=(uint32_t*)(hb+c[1]);
    for(uint32_t i=0;i<c[0];i++){ if(!a[i]) continue; uint8_t *r=hb+a[i];
      uint32_t cnt=*(uint32_t*)(r+0);
      uint32_t f1=*(uint32_t*)(r+0x6d), f2=*(uint32_t*)(r+0x71), f3=*(uint32_t*)(r+0x75);
      if(!f1||!f2||!f3){ if(badzero<4) printf("  ZERO type %02x rec %u cnt=%u fields %x %x %x\n",t,i,cnt,f1,f2,f3); badzero++; }
      else if(f1>0x4000000||f2>0x4000000||f3>0x4000000){ if(badbig<4) printf("  HUGE type %02x rec %u cnt=%u fields %x %x %x\n",t,i,cnt,f1,f2,f3); badbig++; }
      else ok++;
    }
  }
  printf("records: ok=%d zerofield=%d hugefield=%d\n", ok, badzero, badbig);
  /* sprite-struct sub-object validity (fn_03d9cf allocated 14 per struct) */
  for (uint32_t i = 1; i < 0x104u; i++) {
    if (!g_dat_639ec[i]) continue;
    uint32_t *sp = (uint32_t *)g_dat_639ec[i];
    printf("spr slot %02x: count=%u slots:", i, sp[0]);
    int bad = 0;
    for (int k = 1; k <= 14; k++) { if (sp[k] > 0x4000000u) bad = 1; printf(" %x", sp[k]); }
    printf("%s\n", bad ? "  <-- BAD" : "");
  }
  /* dry run of fn_037347's loop: what would be freed, and is it sane? */
  for (uint32_t i = 1; i < 0x104u; i++) {
    if (i == 0xc9 || i == 0xfa) continue;
    if (!g_dat_639ec[i]) continue;
    uint32_t ao = g_dat_53818[i];
    printf("free slot %02x: spr=%p act=%x", i, (void*)g_dat_639ec[i], ao);
    if (ao) { uint32_t *c=(uint32_t*)(hb+ao); uint32_t cnt=c[0], arr=c[1];
      int bad = (arr == 0 || arr > 0x4000000u);
      printf(" cnt=%u arr=%x%s", cnt, arr, bad ? "  <-- BAD ARRAY" : "");
      if (!bad) { uint32_t *a=(uint32_t*)(hb+arr);
        for (uint32_t k=0;k<cnt;k++) if (a[k]==0 || a[k]>0x4000000u) { printf("  rec[%u]=%x BAD", k, a[k]); break; } }
    }
    printf("\n");
  }
  fn_037347();
  printf("teardown done\n");
  sys_file_real_dir(0);
  return 0;
}
