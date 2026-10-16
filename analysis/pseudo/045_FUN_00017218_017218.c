// func #45  FUN_00017218  00017218  size=224


bool FUN_00017218(void)

{
  int iVar1;
  int iVar2;
  bool bVar3;
  undefined8 uVar4;
  int *in_stack_00000000;
  int *in_stack_00000004;
  
  uVar4 = FUN_000418ee();
  iVar2 = (int)((ulonglong)uVar4 >> 0x20);
  iVar1 = (int)uVar4;
  *(undefined4 *)(iVar1 + 0x17) = 2;
  switch(*(undefined4 *)(iVar1 + 0xbb)) {
  case 0:
    *in_stack_00000000 = iVar2;
    *in_stack_00000004 = iVar2;
    break;
  case 1:
    *in_stack_00000000 = iVar2;
    *in_stack_00000000 = -*in_stack_00000000;
    *in_stack_00000004 = iVar2;
    break;
  case 2:
    *in_stack_00000000 = iVar2;
    *in_stack_00000004 = iVar2;
    *in_stack_00000004 = -*in_stack_00000004;
    break;
  case 3:
    *in_stack_00000000 = iVar2;
    *in_stack_00000000 = -*in_stack_00000000;
    *in_stack_00000004 = iVar2;
    *in_stack_00000004 = -*in_stack_00000004;
  }
  bVar3 = -1 < *(int *)(iVar1 + 0xab);
  if (bVar3) {
    *(int *)(iVar1 + 0xab) = *(int *)(iVar1 + 0xab) + -1;
  }
  return !bVar3;
}


