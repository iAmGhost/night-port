// func #230  FUN_00041d47  00041d47  size=171


undefined8 FUN_00041d47(undefined4 *param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  undefined4 *extraout_ECX_01;
  undefined4 *extraout_ECX_02;
  undefined4 *puVar3;
  undefined4 uVar4;
  
  if (param_1[2] == 0) {
    FUN_00042a43();
    param_1 = extraout_ECX;
  }
  if (((*(byte *)((int)param_1 + 0xd) & 0x20) != 0) && ((*(byte *)((int)param_1 + 0xd) & 6) != 0)) {
    FUN_00043234(0x2000);
    param_1 = extraout_ECX_00;
  }
  uVar1 = param_1[3];
  *param_1 = param_1[2];
  *(byte *)(param_1 + 3) = *(byte *)(param_1 + 3) & 0xfb;
  if (((uVar1 & 0x2400) == 0x2400) && (param_1[4] == 0)) {
    param_1[1] = 0;
    iVar2 = FUN_00043263();
    puVar3 = extraout_ECX_01;
    if (iVar2 != -1) {
      *(char *)*extraout_ECX_01 = (char)iVar2;
      extraout_ECX_01[1] = 1;
    }
  }
  else {
    if ((*(byte *)((int)param_1 + 0xd) & 4) == 0) {
      uVar4 = param_1[5];
    }
    else {
      uVar4 = 1;
    }
    uVar4 = FUN_00042aba(param_1[4],*param_1,uVar4);
    extraout_ECX_02[1] = uVar4;
    puVar3 = extraout_ECX_02;
  }
  if ((int)puVar3[1] < 1) {
    if (puVar3[1] == 0) {
      *(byte *)(puVar3 + 3) = *(byte *)(puVar3 + 3) | 0x10;
    }
    else {
      puVar3[1] = 0;
      *(byte *)(puVar3 + 3) = *(byte *)(puVar3 + 3) | 0x20;
    }
  }
  return CONCAT44(param_2,puVar3[1]);
}


