// func #147  FUN_0003c72b  0003c72b  size=172


int FUN_0003c72b(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if ((*(int *)(iVar1 + 0x34c) != 0) && (*(int *)(iVar1 + 0x34c) != 0)) {
    FUN_0003c72b(*(int *)(iVar1 + 0x34c),0);
    FUN_00041c28();
  }
  if (*(int *)(iVar1 + 0x348) != 0) {
    FUN_00041c31(*(undefined4 *)(iVar1 + 0x348));
    *(undefined4 *)(iVar1 + 0x348) = 0;
  }
  if (*(int *)(iVar1 + 0x44) != 0) {
    FUN_00041c31(*(undefined4 *)(iVar1 + 0x44));
    *(undefined4 *)(iVar1 + 0x44) = 0;
  }
  return iVar1;
}


