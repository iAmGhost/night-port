// func #50  FUN_0001776f  0001776f  size=440


undefined4 FUN_0001776f(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  int *in_stack_00000000;
  int *in_stack_00000004;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  iVar1 = (int)uVar4;
  *(undefined4 *)(iVar1 + 0x17) = 2;
  if (*(int *)(iVar1 + 0xc3) < *(int *)(iVar1 + 0xcf)) {
    uVar4 = FUN_000151d5(iVar1,*(int *)(iVar1 + 0xc3) + *(int *)(iVar1 + 0xcf) >> 1);
    if ((int)((ulonglong)uVar4 >> 0x20) < (int)uVar4) {
      if (*(int *)(iVar1 + 0xcb) < 0xa0) {
        *in_stack_00000000 = iVar3;
      }
      else {
        *in_stack_00000000 = iVar3;
        *in_stack_00000000 = -*in_stack_00000000;
      }
    }
    else if (*(int *)(iVar1 + 0xcb) < 0xa0) {
      *in_stack_00000000 = iVar3;
      *in_stack_00000000 = -*in_stack_00000000;
    }
    else {
      *in_stack_00000000 = iVar3;
    }
    iVar2 = FUN_000151d5(iVar1);
    if (iVar2 - iVar3 <= *(int *)(iVar1 + 0xc3)) {
      *in_stack_00000004 = 0;
      *in_stack_00000000 = 0;
      return 1;
    }
    *in_stack_00000004 = iVar3;
    *in_stack_00000004 = -*in_stack_00000004;
  }
  else {
    uVar4 = FUN_000151d5(iVar1,*(int *)(iVar1 + 0xc3) + *(int *)(iVar1 + 0xcf) >> 1);
    if ((int)((ulonglong)uVar4 >> 0x20) < (int)uVar4) {
      if (*(int *)(iVar1 + 0xcb) < 0xa0) {
        *in_stack_00000000 = iVar3;
      }
      else {
        *in_stack_00000000 = iVar3;
        *in_stack_00000000 = -*in_stack_00000000;
      }
    }
    else if (*(int *)(iVar1 + 0xcb) < 0xa0) {
      *in_stack_00000000 = iVar3;
      *in_stack_00000000 = -*in_stack_00000000;
    }
    else {
      *in_stack_00000000 = iVar3;
    }
    iVar2 = FUN_000151d5(iVar1);
    if (*(int *)(iVar1 + 0xc3) <= iVar3 + iVar2) {
      *in_stack_00000004 = 0;
      *in_stack_00000000 = 0;
      return 1;
    }
    *in_stack_00000004 = iVar3;
  }
  return 0;
}


