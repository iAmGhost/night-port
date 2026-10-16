// func #321  FUN_00044748  00044748  size=191


undefined8 FUN_00044748(undefined4 *param_1,undefined4 param_2)

{
  ushort uVar1;
  int iVar2;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  undefined4 *extraout_ECX_01;
  undefined4 uVar3;
  
  uVar3 = 0;
  if ((*(byte *)((int)param_1 + 0xd) & 0x10) == 0) {
    if ((param_1[2] != 0) &&
       (uVar1 = *(ushort *)(param_1 + 3),
       *(byte *)(param_1 + 3) = (byte)*(ushort *)(param_1 + 3) & 0xef, (uVar1 & 0x2000) == 0)) {
      iVar2 = 0;
      if (param_1[1] != 0) {
        iVar2 = FUN_00044807(param_1[4],-param_1[1],1);
        param_1 = extraout_ECX_01;
      }
      if (iVar2 == -1) {
        *(byte *)(param_1 + 3) = *(byte *)(param_1 + 3) | 0x20;
        uVar3 = 0xffffffff;
      }
    }
  }
  else {
    *(byte *)((int)param_1 + 0xd) = *(byte *)((int)param_1 + 0xd) & 0xef;
    if (((*(byte *)(param_1 + 3) & 2) != 0) && (param_1[2] != 0)) {
      iVar2 = FUN_00049236(param_1[4],param_1[2],param_1[1]);
      param_1 = extraout_ECX;
      if (iVar2 == -1) {
        *(byte *)(extraout_ECX + 3) = *(byte *)(extraout_ECX + 3) | 0x20;
        uVar3 = 0xffffffff;
      }
      else if (iVar2 != extraout_ECX[1]) {
        FUN_00042a0d(0xc);
        uVar3 = 0xffffffff;
        *(byte *)(extraout_ECX_00 + 3) = *(byte *)(extraout_ECX_00 + 3) | 0x20;
        param_1 = extraout_ECX_00;
      }
    }
  }
  param_1[1] = 0;
  *param_1 = param_1[2];
  return CONCAT44(param_2,uVar3);
}


