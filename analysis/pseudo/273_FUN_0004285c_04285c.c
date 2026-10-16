// func #273  FUN_0004285c  0004285c  size=137


uint FUN_0004285c(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int extraout_ECX_02;
  int extraout_ECX_03;
  int iVar3;
  uint uVar4;
  undefined1 auStack_20 [16];
  
  if (*(int *)(param_1 + 0xc) == 0) {
    uVar4 = 0xffffffff;
  }
  else {
    uVar4 = 0;
    if ((*(byte *)(param_1 + 0xd) & 0x10) != 0) {
      uVar4 = FUN_00044748(param_1);
      param_1 = extraout_ECX;
    }
    iVar1 = FUN_00042928(param_1);
    iVar3 = extraout_ECX_00;
    if (iVar1 != -1) {
      FUN_00044807(*(undefined4 *)(extraout_ECX_00 + 0x10),iVar1,0);
      iVar3 = extraout_ECX_01;
    }
    if (param_2 != 0) {
      uVar2 = FUN_00044bec(*(undefined4 *)(iVar3 + 0x10));
      uVar4 = uVar4 | uVar2;
      iVar3 = extraout_ECX_02;
    }
    if ((*(byte *)(iVar3 + 0xc) & 8) != 0) {
      FUN_00041c31(*(undefined4 *)(iVar3 + 8));
      *(undefined4 *)(extraout_ECX_03 + 8) = 0;
      iVar3 = extraout_ECX_03;
    }
    if ((*(byte *)(iVar3 + 0xd) & 8) != 0) {
      FUN_000427f3(auStack_20,*(undefined1 *)(iVar3 + 0x19));
      thunk_FUN_00049415(auStack_20);
    }
  }
  return uVar4;
}


