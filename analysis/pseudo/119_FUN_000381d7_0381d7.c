// func #119  FUN_000381d7  000381d7  size=461


void FUN_000381d7(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  int *piVar2;
  undefined4 uVar3;
  int extraout_ECX;
  int iVar4;
  undefined8 uVar5;
  int in_stack_00000000;
  int local_38;
  
  uVar5 = FUN_000418ee();
  iVar4 = (int)((ulonglong)uVar5 >> 0x20);
  uVar3 = (undefined4)uVar5;
  local_38 = 0;
  uVar1 = *(undefined4 *)(DAT_00053c7c + 0x17);
  *(undefined4 *)(DAT_00053c7c + 0x17) = 2;
  *(int *)(DAT_00053c7c + 0x1b) = in_stack_00000000;
  piVar2 = *(int **)(*(int *)(*(int *)(DAT_00053c7c + 0x123) + 4) + 8);
  for (; local_38 < extraout_ECX; local_38 = local_38 + 1) {
    if (in_stack_00000000 == 0) {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + iVar4;
    }
    else {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) - iVar4;
    }
    *(int *)(DAT_00053c7c + 9) = *(int *)(DAT_00053c7c + 9) + param_3;
    *(int *)(DAT_00053c7c + 0x11) = *(int *)(DAT_00053c7c + 0x11) + 1;
    if (*piVar2 <= *(int *)(DAT_00053c7c + 0x11)) {
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
    }
    FUN_00013aad(DAT_00053c70);
    if (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104)) {
      FUN_0003ce0b(DAT_00063354,uVar3);
      FUN_0003203d(uVar3);
      FUN_0003df96(uVar3);
      if (DAT_00053c84 != 0) {
        FUN_0001348f(DAT_00053c84,uVar3,0);
        FUN_000137c1(DAT_00053c84,uVar3);
      }
      FUN_000404c6();
    }
    do {
    } while (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104));
    DAT_00063e24 = DAT_00063e24 - (int)(0x1ea / (longlong)DAT_00053104);
  }
  *(undefined4 *)(DAT_00053c7c + 0x17) = uVar1;
  return;
}


