// func #47  FUN_000174a2  000174a2  size=142


undefined4 FUN_000174a2(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  int *in_stack_00000000;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  iVar2 = (int)uVar4;
  *(undefined4 *)(iVar2 + 0x17) = 2;
  iVar1 = FUN_0001519f(iVar2);
  if ((-1 < iVar1) && (iVar1 = FUN_0001519f(iVar2), iVar1 < 0x140)) {
    return 1;
  }
  iVar1 = FUN_0001519f(iVar2);
  if (iVar1 < 0) {
    *in_stack_00000000 = *in_stack_00000000 + iVar3;
  }
  iVar2 = FUN_0001519f(iVar2);
  if (0x13f < iVar2) {
    *in_stack_00000000 = *in_stack_00000000 - iVar3;
  }
  return 0;
}


