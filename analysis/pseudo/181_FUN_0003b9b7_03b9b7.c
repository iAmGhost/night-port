// func #141  FUN_0003b9b7  0003b9b7  size=114


void FUN_0003b9b7(void)

{
  int iVar1;
  
  FUN_000418ee();
  DAT_000639d4 = DAT_000639cc;
  iVar1 = DAT_000639d4;
  while (DAT_000639d4 = iVar1, DAT_000639d4 != 0) {
    iVar1 = *(int *)(DAT_000639d4 + 0x11);
    if (DAT_000639d4 != 0) {
      FUN_0003b67e(DAT_000639d4,0);
      FUN_00041c28();
    }
  }
  return;
}


