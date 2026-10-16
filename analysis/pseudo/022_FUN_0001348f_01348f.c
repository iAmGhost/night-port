// func #22  FUN_0001348f  0001348f  size=818


void FUN_0001348f(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  undefined8 uVar7;
  undefined1 uVar8;
  undefined1 uVar9;
  
  uVar7 = FUN_000418ee();
  uVar4 = (undefined4)((ulonglong)uVar7 >> 0x20);
  iVar1 = (int)uVar7;
  if (iVar1 == DAT_00053c7c) {
    iVar2 = (int)*(short *)(iVar1 + 0x23) / 100;
    iVar5 = (int)*(short *)(iVar1 + 0x23) % 100;
    iVar3 = (int)*(short *)(iVar1 + 0x29) / 100;
    iVar6 = (int)*(short *)(iVar1 + 0x29) % 100;
    uVar9 = (&DAT_00051c91)[iVar2];
    uVar8 = (&DAT_00051c90)[iVar2];
  }
  else {
    iVar2 = (int)*(short *)(iVar1 + 0x23) / 200;
    iVar5 = ((int)*(short *)(iVar1 + 0x23) / 2) % 100;
    iVar3 = (int)*(short *)(iVar1 + 0x29) / 200;
    iVar6 = ((int)*(short *)(iVar1 + 0x29) / 2) % 100;
    if (iVar2 < 6) {
      uVar8 = (&DAT_00051c90)[iVar2];
    }
    else {
      uVar8 = 0x95;
    }
    if (iVar2 < 5) {
      uVar9 = (&DAT_00051c91)[iVar2];
    }
    else {
      uVar9 = 0x95;
    }
  }
  if ((*(int *)(iVar1 + 0xd) == 0) || (*(int *)(iVar1 + 0x56) == 0)) {
    FUN_0003bb42(uVar4,param_3 * 0xaa + 4,3,
                 *(undefined4 *)(*(int *)(DAT_00053c8c + 4) + *(int *)(iVar1 + 0xd) * 4),
                 *(undefined4 *)(*(int *)(DAT_00053c8c + 8) + *(int *)(iVar1 + 0xd) * 4),
                 *(int *)(DAT_00053c8c + 0x38) +
                 *(int *)(*(int *)(DAT_00053c8c + 0x34) + *(int *)(iVar1 + 0xd) * 4),0,iVar2,iVar5,
                 iVar3,iVar6,uVar8,uVar9);
    FUN_0003bb42(uVar4,param_3 * 0xaa + 0x13,0x1d,**(undefined4 **)(DAT_00063e00 + 4),
                 **(undefined4 **)(DAT_00063e00 + 8),*(undefined4 *)(DAT_00063e00 + 0x38),0);
    if (iVar2 != 0) {
      FUN_0004052c();
    }
    if (iVar5 != 0) {
      FUN_0004052c();
      FUN_00040506();
    }
  }
  else {
    if ((0 < *(int *)(iVar1 + 0xd)) && (*(int *)(iVar1 + 0xd) < 10)) {
      iVar2 = (int)(CONCAT44(*(int *)(iVar1 + 0x56) >> 0x1f,*(undefined4 *)(iVar1 + 0x56)) / 3);
      iVar5 = 0x9e;
      iVar6 = 1;
    }
    FUN_0003bb42(uVar4,0x20,0xb3,*(undefined4 *)(*(int *)(DAT_00063e00 + 4) + iVar6 * 4),
                 *(undefined4 *)(*(int *)(DAT_00063e00 + 8) + iVar6 * 4),
                 *(int *)(DAT_00063e00 + 0x38) + *(int *)(iVar6 * 4 + *(int *)(DAT_00063e00 + 0x34))
                 ,0,iVar2,iVar5,iVar3,iVar6,uVar8,uVar9);
    FUN_0004052c();
  }
  return;
}


