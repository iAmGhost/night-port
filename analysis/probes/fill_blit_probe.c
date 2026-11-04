#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../../src/f/f.h"
int main(void){
    static const uint8_t A[] = { 2, 2,4,0x11,0x22,0x33,0x44, 1,3,0xaa,0xbb,0xcc };
    static const uint8_t Bk[]= { 1, 0,1,0x99 };
    static const uint8_t C[] = { 2, 2,5,0x11,0x22,0x33,0x44,0x55, 1,3,0xaa,0xbb,0xcc };
    static const uint8_t D[] = { 0 };
    static uint8_t stream[64]; size_t o=0;
    memcpy(stream+o,A,sizeof A); o+=sizeof A; memcpy(stream+o,Bk,sizeof Bk); o+=sizeof Bk;
    memcpy(stream+o,C,sizeof C); o+=sizeof C; memcpy(stream+o,D,sizeof D); o+=sizeof D;
    uint8_t *scr = g_video_ram; memset(scr,0,0x10000);
    fn_03c231(0xa0000u, 0, 0, 0x20, 2, stream, 0, 0xcd);
    printf("row0:"); for(int c=0;c<0x20;c++) printf(" %02x", scr[c]); printf("\n");
    printf("row1:"); for(int c=0;c<0x20;c++) printf(" %02x", scr[0x140+c]); printf("\n");
    printf("row2:"); for(int c=0;c<0x20;c++) printf(" %02x", scr[0x280+c]); printf("\n");
    return 0;
}
