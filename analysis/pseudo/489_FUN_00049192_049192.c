// func #397  FUN_00049192  00049192  size=164


uint FUN_00049192(int param_1,int *param_2)

{
  uint uVar1;
  int iVar2;
  uint extraout_ECX;
  int extraout_EDX;
  int *extraout_EDX_00;
  byte bVar3;
  undefined8 uVar4;
  
  if ((*(byte *)(param_2 + 3) & 2) == 0) {
    FUN_00042a0d(4);
    *(byte *)(extraout_EDX + 0xc) = *(byte *)(extraout_EDX + 0xc) | 0x20;
LAB_000491ad:
    uVar1 = 0xffffffff;
  }
  else {
    if (param_2[2] == 0) {
      FUN_00042a43(param_2);
      param_2 = extraout_EDX_00;
    }
    uVar1 = 0x400;
    if ((param_1 == 10) && (uVar1 = 0x600, (*(byte *)(param_2 + 3) & 0x40) == 0)) {
      *(byte *)((int)param_2 + 0xd) = *(byte *)((int)param_2 + 0xd) | 0x10;
      *(undefined1 *)*param_2 = 0xd;
      iVar2 = param_2[1];
      *param_2 = *param_2 + 1;
      param_2[1] = iVar2 + 1;
      if (iVar2 + 1 == param_2[5]) {
        uVar4 = FUN_00044748(param_2);
        param_2 = (int *)((ulonglong)uVar4 >> 0x20);
        uVar1 = extraout_ECX;
        if ((int)uVar4 != 0) goto LAB_000491ad;
      }
    }
    *(byte *)((int)param_2 + 0xd) = *(byte *)((int)param_2 + 0xd) | 0x10;
    bVar3 = (byte)param_1;
    *(byte *)*param_2 = bVar3;
    iVar2 = param_2[1];
    *param_2 = *param_2 + 1;
    param_2[1] = iVar2 + 1;
    if (((uVar1 & param_2[3]) != 0) || (iVar2 + 1 == param_2[5])) {
      iVar2 = FUN_00044748(param_2);
      if (iVar2 != 0) goto LAB_000491ad;
    }
    uVar1 = (uint)bVar3;
  }
  return uVar1;
}


