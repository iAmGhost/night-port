// func #46  FUN_0001730b  0001730b  size=407


bool FUN_0001730b(void)

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
  iVar2 = FUN_0001519f(iVar1);
  iVar3 = FUN_000151d5(iVar1);
  if (*(int *)(iVar1 + 0xaf) != 1) {
    if (iVar2 < *(int *)(iVar1 + 0xb3)) {
      if (iVar2 + iVar4 < *(int *)(iVar1 + 0xb3)) {
        *in_stack_00000000 = iVar4;
      }
      else {
        *(int *)(iVar1 + 0xaf) = *(int *)(iVar1 + 0xaf) + 1;
      }
    }
    else if (*(int *)(iVar1 + 0xb3) < iVar2) {
      if (*(int *)(iVar1 + 0xb3) < iVar2 - iVar4) {
        *in_stack_00000000 = iVar4;
        *in_stack_00000000 = -*in_stack_00000000;
      }
      else {
        *(int *)(iVar1 + 0xaf) = *(int *)(iVar1 + 0xaf) + 1;
      }
    }
    else {
      *(int *)(iVar1 + 0xaf) = *(int *)(iVar1 + 0xaf) + 1;
    }
  }
  if (*(int *)(iVar1 + 0xaf) != 2) {
    if (iVar3 < *(int *)(iVar1 + 0xb7)) {
      if ((iVar3 + iVar4 < *(int *)(iVar1 + 0xb7)) &&
         (iVar3 + iVar4 < *(int *)(DAT_00063354 + 0x16))) {
        *in_stack_00000004 = iVar4;
      }
      else {
        *(int *)(iVar1 + 0xaf) = *(int *)(iVar1 + 0xaf) + 2;
      }
    }
    else if (*(int *)(iVar1 + 0xb7) < iVar3) {
      if ((*(int *)(iVar1 + 0xb7) < iVar3 - iVar4) &&
         (*(int *)(DAT_00063354 + 0x12) < iVar3 - iVar4)) {
        *in_stack_00000004 = iVar4;
        *in_stack_00000004 = -*in_stack_00000004;
      }
      else {
        *(int *)(iVar1 + 0xaf) = *(int *)(iVar1 + 0xaf) + 2;
      }
    }
    else {
      *(int *)(iVar1 + 0xaf) = *(int *)(iVar1 + 0xaf) + 2;
    }
  }
  return 2 < *(int *)(iVar1 + 0xaf);
}


