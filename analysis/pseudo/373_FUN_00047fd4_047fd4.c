// func #373  FUN_00047fd4  00047fd4  size=127


undefined8 FUN_00047fd4(int *param_1,undefined4 param_2)

{
  uint uVar1;
  ushort uVar2;
  ushort uVar3;
  uint uVar4;
  bool bVar5;
  
  uVar4 = 0xffffff00;
  uVar1 = param_1[1];
  uVar2 = *(ushort *)(param_1 + 2);
  if ((int)(uVar1 << 0x18) < 0) {
    if (((uVar1 & 0x7f) == 0) && (*param_1 == 0)) {
      uVar4 = 0xfffffe00;
    }
    bVar5 = 0xfffffeff < uVar1;
    uVar1 = uVar1 + 0x100;
    if (bVar5) {
      uVar1 = 0x80000000;
      uVar2 = uVar2 + 1;
    }
  }
  uVar1 = uVar1 & uVar4;
  if ((uVar2 & 0x7fff) != 0) {
    if ((uVar2 & 0x7fff) == 0x7fff) {
      uVar1 = (uint)((CONCAT14(CARRY2(uVar2,uVar2),(uVar1 & 0x7fffffff) >> 7) | 0xff000000) >> 1);
    }
    else {
      uVar3 = (uVar2 & 0x7fff) + 0xc080;
      if ((short)uVar3 < 0) {
        uVar1 = 0;
      }
      else if ((short)uVar3 < 0xff) {
        uVar1 = (uint)(CONCAT14(CARRY2(uVar2,uVar2),(uVar1 & 0x7fffffff) >> 7 | (uint)uVar3 << 0x18)
                      >> 1);
      }
      else {
        uVar1 = (uint)(((ulonglong)CARRY2(uVar2,uVar2) << 0x20 | 0xff000000) >> 1);
      }
    }
  }
  return CONCAT44(param_2,uVar1);
}


