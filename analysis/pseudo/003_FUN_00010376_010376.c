// func #3  FUN_00010376  00010376  size=106


int FUN_00010376(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (*(int *)(iVar1 + 0x6d) != 0) {
    FUN_00041c31(*(undefined4 *)(iVar1 + 0x6d));
  }
  if (*(int *)(iVar1 + 0x71) != 0) {
    FUN_00041c31(*(undefined4 *)(iVar1 + 0x71));
  }
  if (*(int *)(iVar1 + 0x75) != 0) {
    FUN_00041c31(*(undefined4 *)(iVar1 + 0x75));
  }
  return iVar1;
}


