// func #51  FUN_00017927  00017927  size=895


undefined4 FUN_00017927(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined8 uVar5;
  int *in_stack_00000000;
  int *in_stack_00000004;
  
  uVar5 = FUN_000418ee();
  iVar4 = (int)((ulonglong)uVar5 >> 0x20);
  iVar1 = (int)uVar5;
  *(undefined4 *)(iVar1 + 0x17) = 2;
  iVar2 = *(int *)(DAT_00063354 + 0x12) +
          (*(int *)(DAT_00063354 + 0x16) - *(int *)(DAT_00063354 + 0x12) >> 1);
  *(int *)(iVar1 + 0x67) = *(int *)(iVar1 + 0xcf) - *(int *)(iVar1 + 0xc3);
  if (*(int *)(iVar1 + 0xbf) < *(int *)(iVar1 + 0xcb)) {
    uVar5 = FUN_0001519f(iVar1,(*(int *)(iVar1 + 0xbf) + *(int *)(iVar1 + 0xcb) >> 1) +
                               *(int *)(iVar1 + 0x67));
    if ((int)((ulonglong)uVar5 >> 0x20) < (int)uVar5) {
      iVar3 = FUN_0001519f(iVar1);
      if ((*(int *)(iVar1 + 0xcb) - iVar3) % 3 == 0) {
        if (*(int *)(iVar1 + 0xcf) < iVar2) {
          *in_stack_00000004 = iVar4;
        }
        else {
          *in_stack_00000004 = iVar4;
          *in_stack_00000004 = -*in_stack_00000004;
        }
      }
    }
    else {
      iVar3 = FUN_0001519f(iVar1);
      if ((*(int *)(iVar1 + 0xcb) - iVar3) % 3 == 0) {
        if (*(int *)(iVar1 + 0xcf) < iVar2) {
          *in_stack_00000004 = iVar4;
          *in_stack_00000004 = -*in_stack_00000004;
        }
        else {
          *in_stack_00000004 = iVar4;
        }
      }
    }
    iVar3 = FUN_0001519f(iVar1);
    if (iVar3 - iVar4 <= *(int *)(iVar1 + 0xbf) + 0x1e) {
      if (*(int *)(iVar1 + 0xcf) < iVar2) {
        iVar2 = FUN_000151d5(iVar1);
        if (iVar2 - iVar4 <= *(int *)(iVar1 + 0xc3)) {
          *in_stack_00000000 = 0;
          *in_stack_00000004 = 0;
          return 1;
        }
      }
      else {
        iVar2 = FUN_000151d5(iVar1);
        if (*(int *)(iVar1 + 0xc3) <= iVar4 + iVar2) {
          *in_stack_00000000 = 0;
          *in_stack_00000004 = 0;
          return 1;
        }
      }
    }
    *in_stack_00000000 = iVar4;
    *in_stack_00000000 = -*in_stack_00000000;
  }
  else {
    uVar5 = FUN_0001519f(iVar1,(*(int *)(iVar1 + 0xbf) + *(int *)(iVar1 + 0xcb) >> 1) +
                               *(int *)(iVar1 + 0x67));
    if ((int)((ulonglong)uVar5 >> 0x20) < (int)uVar5) {
      iVar3 = FUN_0001519f(iVar1);
      if ((iVar3 - *(int *)(iVar1 + 0xcb)) % 3 == 0) {
        if (iVar2 < *(int *)(iVar1 + 0xcf)) {
          *in_stack_00000004 = iVar4;
        }
        else {
          *in_stack_00000004 = iVar4;
          *in_stack_00000004 = -*in_stack_00000004;
        }
      }
    }
    else {
      iVar3 = FUN_0001519f(iVar1);
      if ((iVar3 - *(int *)(iVar1 + 0xcb)) % 3 == 0) {
        if (iVar2 < *(int *)(iVar1 + 0xcf)) {
          *in_stack_00000004 = iVar4;
          *in_stack_00000004 = -*in_stack_00000004;
        }
        else {
          *in_stack_00000004 = iVar4;
        }
      }
    }
    iVar3 = FUN_0001519f(iVar1);
    if (*(int *)(iVar1 + 0xbf) + -0x1e <= iVar4 + iVar3) {
      if (*(int *)(iVar1 + 0xcf) < iVar2) {
        iVar2 = FUN_000151d5(iVar1);
        if (iVar2 - iVar4 <= *(int *)(iVar1 + 0xc3)) {
          *in_stack_00000000 = 0;
          *in_stack_00000004 = 0;
          return 1;
        }
      }
      else {
        iVar2 = FUN_000151d5(iVar1);
        if (*(int *)(iVar1 + 0xc3) <= iVar4 + iVar2) {
          *in_stack_00000000 = 0;
          *in_stack_00000004 = 0;
          return 1;
        }
      }
    }
    *in_stack_00000000 = iVar4;
  }
  if (*(int *)(iVar1 + 0xbf) < *(int *)(iVar1 + 0xcb)) {
    *in_stack_00000000 = iVar4;
    *in_stack_00000000 = -*in_stack_00000000;
  }
  else {
    *in_stack_00000000 = iVar4;
  }
  return 0;
}


