// func #41  FUN_00016959  00016959  size=263


bool FUN_00016959(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  undefined8 uVar5;
  int *in_stack_00000000;
  
  uVar5 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar5 >> 0x20);
  iVar1 = (int)uVar5;
  *(undefined4 *)(iVar1 + 0x17) = 2;
  iVar2 = FUN_0001519f(iVar1);
  if (*(int *)(iVar1 + 0xbf) < iVar2) {
    iVar2 = FUN_0001519f(iVar1);
    if (iVar2 - iVar3 <= *(int *)(iVar1 + 0xbf)) {
      *in_stack_00000000 = 0;
      return true;
    }
    *in_stack_00000000 = iVar3;
    *in_stack_00000000 = -*in_stack_00000000;
  }
  else {
    iVar2 = FUN_0001519f(iVar1);
    if (*(int *)(iVar1 + 0xbf) <= iVar2) {
      *in_stack_00000000 = 0;
      return true;
    }
    iVar2 = FUN_0001519f(iVar1);
    if (*(int *)(iVar1 + 0xbf) <= iVar3 + iVar2) {
      *in_stack_00000000 = 0;
      return true;
    }
    *in_stack_00000000 = iVar3;
  }
  bVar4 = -1 < *(int *)(iVar1 + 0xab);
  if (bVar4) {
    *(int *)(iVar1 + 0xab) = *(int *)(iVar1 + 0xab) + -1;
  }
  return !bVar4;
}


