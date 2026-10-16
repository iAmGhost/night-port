// func #344  FUN_0004574a  0004574a  size=340


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0004574a(int *param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  undefined *puVar3;
  int iVar4;
  
  iVar2 = (int)(CONCAT44(param_1[4] >> 0x1f,param_1[4]) % 0xc);
  puVar3 = &DAT_00051960;
  if (-0xb048160 < param_1[5]) {
    iVar4 = param_1[5] + param_1[4] / 0xc;
    for (; iVar2 < 0; iVar2 = iVar2 + 0xc) {
      iVar4 = iVar4 + -1;
    }
    if (-1 < iVar4) {
      iVar1 = FUN_000494cb(iVar4 + 0x76c,param_1[4] % 0xc);
      if (iVar1 != 0) {
        puVar3 = &DAT_0005197a;
      }
      iVar2 = param_1[3] + (int)*(short *)(puVar3 + iVar2 * 2) + iVar4 * 0x16d + (iVar4 + 3 >> 2);
      iVar1 = iVar2 + -1;
      if (iVar4 != 0) {
        iVar1 = iVar2 + -2;
      }
      for (iVar2 = *param_1 + (param_1[1] + param_1[2] * 0x3c) * 0x3c; iVar2 < 0;
          iVar2 = iVar2 + 0x15180) {
        iVar1 = iVar1 + -1;
      }
      FUN_000498f6(iVar1,iVar2,0,param_1);
      FUN_00049a35();
      iVar2 = iVar2 + DAT_0005377c;
      if (param_1[8] < 0) {
        FUN_00049613(param_1);
      }
      if (0 < param_1[8]) {
        iVar2 = iVar2 - _DAT_00053780;
      }
      for (; iVar2 < 0; iVar2 = iVar2 + 0x15180) {
        iVar1 = iVar1 + -1;
      }
      if (0x63dd < iVar1) {
        if (iVar1 != 0x63de) {
          iVar2 = iVar2 + (iVar1 + -0x63df) * 0x15180;
          goto LAB_00045897;
        }
        iVar2 = iVar2 + -0x15180;
        if ((0 < DAT_0005377c) && (-1 < iVar2)) goto LAB_00045897;
      }
    }
  }
  iVar2 = -1;
LAB_00045897:
  return CONCAT44(param_2,iVar2);
}


