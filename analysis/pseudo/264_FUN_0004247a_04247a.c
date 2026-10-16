// func #264  FUN_0004247a  0004247a  size=252


/* WARNING: Removing unreachable block (ram,0x000424f4) */

undefined4 FUN_0004247a(undefined4 *param_1,int param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  undefined4 *extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int extraout_ECX_02;
  
  if ((*(byte *)(param_1 + 3) & 6) == 0) {
    if (param_3 == 0) {
      iVar1 = FUN_00044848(param_1[4]);
      iVar1 = FUN_00042438(param_2 - (iVar1 - *(int *)(extraout_ECX_01 + 4)),extraout_ECX_01);
      if ((iVar1 != 0) &&
         (iVar1 = FUN_00044807(*(undefined4 *)(extraout_ECX_02 + 0x10),param_2), iVar1 == -1)) {
        return 0xffffffff;
      }
    }
    else if (param_3 < 2) {
      iVar1 = param_1[1];
      iVar2 = FUN_00042438(param_2,param_1);
      if ((iVar2 != 0) &&
         (iVar1 = FUN_00044807(*(undefined4 *)(extraout_ECX_00 + 0x10),param_2 - iVar1), iVar1 == -1
         )) {
        return 0xffffffff;
      }
    }
    else {
      if (param_3 != 2) goto LAB_000424a0;
      *param_1 = param_1[2];
      *(byte *)(param_1 + 3) = *(byte *)(param_1 + 3) & 0xef;
      param_1[1] = 0;
      iVar1 = FUN_00044807(param_1[4],param_2);
      if (iVar1 == -1) {
        return 0xffffffff;
      }
    }
  }
  else {
    if ((*(byte *)((int)param_1 + 0xd) & 0x10) == 0) {
      if (param_3 == 1) {
        param_2 = param_2 - param_1[1];
      }
      param_1[1] = 0;
      *param_1 = param_1[2];
    }
    else {
      iVar1 = FUN_00044748(param_1);
      param_1 = extraout_ECX;
      if (iVar1 != 0) {
        if (param_3 != 0) {
          return 0xffffffff;
        }
        if (-1 < param_2) {
          return 0xffffffff;
        }
LAB_000424a0:
        FUN_00042a0d(9);
        return 0xffffffff;
      }
    }
    *(byte *)(param_1 + 3) = *(byte *)(param_1 + 3) & 0xeb;
    iVar1 = FUN_00044807(param_1[4],param_2);
    if (iVar1 == -1) {
      return 0xffffffff;
    }
  }
  return 0;
}


