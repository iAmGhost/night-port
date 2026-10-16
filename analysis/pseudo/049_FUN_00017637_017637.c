// func #49  FUN_00017637  00017637  size=312


undefined4 FUN_00017637(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  int *in_stack_00000004;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  iVar1 = (int)uVar4;
  *(undefined4 *)(iVar1 + 0x17) = 2;
  iVar2 = FUN_000151d5(iVar1);
  if (*(int *)(iVar1 + 0xc3) < iVar2) {
    iVar2 = FUN_000151d5(iVar1);
    if ((*(int *)(iVar1 + 0xc3) < iVar2 - iVar3) &&
       (iVar2 = FUN_000151d5(iVar1), *(int *)(DAT_00063354 + 0x12) < iVar2 - iVar3)) {
      *in_stack_00000004 = iVar3;
      *in_stack_00000004 = -*in_stack_00000004;
LAB_0001773f:
      if (*(int *)(iVar1 + 0xab) < 0) {
        return 1;
      }
      *(int *)(iVar1 + 0xab) = *(int *)(iVar1 + 0xab) + -1;
      return 0;
    }
    *in_stack_00000004 = 0;
  }
  else {
    iVar2 = FUN_000151d5(iVar1);
    if (*(int *)(iVar1 + 0xc3) <= iVar2) {
      *in_stack_00000004 = 0;
      return 1;
    }
    iVar2 = FUN_000151d5(iVar1);
    if ((iVar3 + iVar2 < *(int *)(iVar1 + 0xc3)) &&
       (iVar2 = FUN_000151d5(iVar1), iVar3 + iVar2 < *(int *)(DAT_00063354 + 0x16))) {
      *in_stack_00000004 = iVar3;
      goto LAB_0001773f;
    }
    *in_stack_00000004 = 0;
  }
  return 1;
}


