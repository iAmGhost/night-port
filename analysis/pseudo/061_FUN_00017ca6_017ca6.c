// func #52  FUN_00017ca6  00017ca6  size=621


undefined4 FUN_00017ca6(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined8 uVar5;
  int *in_stack_00000000;
  int *in_stack_00000004;
  
  uVar5 = FUN_000418ee();
  iVar4 = (int)((ulonglong)uVar5 >> 0x20);
  iVar1 = (int)uVar5;
  *(undefined4 *)(iVar1 + 0x17) = 2;
  uVar2 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf3));
  uVar5 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf7),uVar2);
  if ((int)uVar5 < (int)((ulonglong)uVar5 >> 0x20)) {
    if (*(int *)(iVar1 + 0xf3) < 0) {
      iVar3 = FUN_0001519f(iVar1);
      if (iVar3 < *(int *)(iVar1 + 0xbf)) {
        return 1;
      }
    }
    else {
      iVar3 = FUN_0001519f(iVar1);
      if (*(int *)(iVar1 + 0xbf) < iVar3) {
        return 1;
      }
    }
    if (*(int *)(iVar1 + 0xf3) < 0) {
      *in_stack_00000000 = iVar4;
      *in_stack_00000000 = -*in_stack_00000000;
    }
    else {
      *in_stack_00000000 = iVar4;
    }
    *in_stack_00000004 = 0;
    iVar3 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf7));
    *(int *)(iVar1 + 0xfb) = *(int *)(iVar1 + 0xfb) + iVar3;
    iVar3 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf3));
    if (iVar3 < *(int *)(iVar1 + 0xfb)) {
      iVar3 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf3));
      *(int *)(iVar1 + 0xfb) = *(int *)(iVar1 + 0xfb) - iVar3;
      if (*(int *)(iVar1 + 0xf7) < 0) {
        *in_stack_00000004 = iVar4;
        *in_stack_00000004 = -*in_stack_00000004;
      }
      else {
        *in_stack_00000004 = iVar4;
      }
    }
  }
  else {
    if (*(int *)(iVar1 + 0xf7) < 0) {
      iVar3 = FUN_000151d5(iVar1);
      if ((iVar3 < *(int *)(iVar1 + 0xc3)) ||
         (iVar3 = FUN_000151d5(iVar1), iVar3 - iVar4 <= *(int *)(DAT_00063354 + 0x12))) {
        return 1;
      }
    }
    else {
      iVar3 = FUN_000151d5(iVar1);
      if ((*(int *)(iVar1 + 0xc3) < iVar3) ||
         (*(int *)(DAT_00063354 + 0x16) <= *(int *)(iVar1 + 0xef) + iVar4)) {
        return 1;
      }
    }
    if (*(int *)(iVar1 + 0xf7) < 0) {
      *in_stack_00000004 = iVar4;
      *in_stack_00000004 = -*in_stack_00000004;
    }
    else {
      *in_stack_00000004 = iVar4;
    }
    iVar3 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf3));
    *(int *)(iVar1 + 0xfb) = *(int *)(iVar1 + 0xfb) + iVar3;
    *in_stack_00000000 = 0;
    iVar3 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf7));
    if (iVar3 < *(int *)(iVar1 + 0xfb)) {
      iVar3 = FUN_00041f76(*(undefined4 *)(iVar1 + 0xf7));
      *(int *)(iVar1 + 0xfb) = *(int *)(iVar1 + 0xfb) - iVar3;
      if (*(int *)(iVar1 + 0xf3) < 0) {
        *in_stack_00000000 = iVar4;
        *in_stack_00000000 = -*in_stack_00000000;
      }
      else {
        *in_stack_00000000 = iVar4;
      }
    }
  }
  return 0;
}


