// func #306  FUN_00043ca0  00043ca0  size=217


byte * FUN_00043ca0(byte *param_1,int param_2)

{
  byte bVar1;
  int iVar2;
  uint local_10;
  
  bVar1 = *(byte *)(param_2 + 0x10);
  *(undefined4 *)(param_2 + 0xc) = 0xffffffff;
  *(byte *)(param_2 + 0x10) = bVar1 | 1;
  *(byte *)(param_2 + 0x10) = bVar1 & 3 | 1;
  if (*param_1 == 0x2a) {
    param_1 = param_1 + 1;
    *(byte *)(param_2 + 0x10) = *(byte *)(param_2 + 0x10) & 0xfe;
  }
  bVar1 = *param_1;
  if (((&DAT_0005185c)[(byte)(bVar1 + 1)] & 0x20) != 0) {
    iVar2 = 0;
    do {
      local_10 = (uint)bVar1;
      param_1 = param_1 + 1;
      iVar2 = iVar2 * 10 + (local_10 - 0x30);
      bVar1 = *param_1;
    } while (((&DAT_0005185c)[(byte)(bVar1 + 1)] & 0x20) != 0);
    *(int *)(param_2 + 0xc) = iVar2;
  }
  if (*param_1 == 0x4e) {
    *(byte *)(param_2 + 0x10) = *(byte *)(param_2 + 0x10) | 8;
    param_1 = param_1 + 1;
  }
  else if (*param_1 == 0x46) {
    *(byte *)(param_2 + 0x10) = *(byte *)(param_2 + 0x10) | 4;
    param_1 = param_1 + 1;
  }
  bVar1 = *param_1;
  if (bVar1 < 0x68) {
    if (bVar1 == 0x4c) {
      *(byte *)(param_2 + 0x10) = *(byte *)(param_2 + 0x10) | 0x40;
      param_1 = param_1 + 1;
    }
  }
  else if (bVar1 < 0x69) {
    *(byte *)(param_2 + 0x10) = *(byte *)(param_2 + 0x10) | 0x10;
    param_1 = param_1 + 1;
  }
  else if ((0x6b < bVar1) && ((bVar1 < 0x6d || (bVar1 == 0x77)))) {
    *(byte *)(param_2 + 0x10) = *(byte *)(param_2 + 0x10) | 0x20;
    param_1 = param_1 + 1;
  }
  return param_1;
}


