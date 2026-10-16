// func #168  FUN_0003e14c  0003e14c  size=195


int FUN_0003e14c(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_00063e14 == 1) {
    DAT_00063e18 = 0;
    DAT_00063e1c = 0;
    DAT_00063e20 = 0;
  }
  else if (DAT_00063e18 == iVar1) {
    DAT_00063e18 = *(int *)(iVar1 + 0x51);
    *(undefined4 *)(*(int *)(iVar1 + 0x51) + 0x55) = 0;
  }
  else if (DAT_00063e1c == iVar1) {
    DAT_00063e1c = *(int *)(iVar1 + 0x55);
    *(undefined4 *)(*(int *)(iVar1 + 0x55) + 0x51) = 0;
  }
  else {
    *(undefined4 *)(*(int *)(iVar1 + 0x55) + 0x51) = *(undefined4 *)(iVar1 + 0x51);
    *(undefined4 *)(*(int *)(iVar1 + 0x51) + 0x55) = *(undefined4 *)(iVar1 + 0x55);
  }
  DAT_00063e14 = DAT_00063e14 + -1;
  return iVar1;
}


