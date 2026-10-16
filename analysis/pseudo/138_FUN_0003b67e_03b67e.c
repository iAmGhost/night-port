// func #138  FUN_0003b67e  0003b67e  size=154


int FUN_0003b67e(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_000639cc == iVar1) {
    DAT_000639cc = *(int *)(iVar1 + 0x11);
    *(undefined4 *)(*(int *)(iVar1 + 0x11) + 0x15) = 0;
  }
  else if (DAT_000639d0 == iVar1) {
    DAT_000639d0 = *(int *)(iVar1 + 0x15);
    *(undefined4 *)(*(int *)(iVar1 + 0x15) + 0x11) = 0;
  }
  else {
    *(undefined4 *)(*(int *)(iVar1 + 0x15) + 0x11) = *(undefined4 *)(iVar1 + 0x11);
    *(undefined4 *)(*(int *)(iVar1 + 0x11) + 0x15) = *(undefined4 *)(iVar1 + 0x15);
  }
  DAT_000639c8 = DAT_000639c8 + -1;
  return iVar1;
}


