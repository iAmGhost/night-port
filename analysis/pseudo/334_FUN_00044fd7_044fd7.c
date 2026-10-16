// func #334  FUN_00044fd7  00044fd7  size=91


void FUN_00044fd7(char *param_1,int param_2)

{
  char cVar1;
  byte bVar2;
  
  *(undefined1 *)(param_2 + 0x14) = 0;
  do {
    cVar1 = *param_1;
    if (cVar1 == '-') {
      *(byte *)(param_2 + 0x14) = *(byte *)(param_2 + 0x14) | 8;
    }
    else if (cVar1 == '#') {
      *(byte *)(param_2 + 0x14) = *(byte *)(param_2 + 0x14) | 1;
    }
    else if (cVar1 == '+') {
      bVar2 = *(byte *)(param_2 + 0x14);
      *(byte *)(param_2 + 0x14) = bVar2 | 4;
      *(byte *)(param_2 + 0x14) = bVar2 & 0xfd | 4;
    }
    else if (cVar1 == ' ') {
      if ((*(byte *)(param_2 + 0x14) & 4) == 0) {
        *(byte *)(param_2 + 0x14) = *(byte *)(param_2 + 0x14) | 2;
      }
    }
    else {
      if (cVar1 != '0') {
        return;
      }
      *(undefined1 *)(param_2 + 0x16) = 0x30;
    }
    param_1 = param_1 + 1;
  } while( true );
}


