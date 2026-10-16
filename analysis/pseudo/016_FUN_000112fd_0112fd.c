// func #16  FUN_000112fd  000112fd  size=234


int FUN_000112fd(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_00053c68 == 1) {
    DAT_00053c70 = 0;
    DAT_00053c74 = 0;
    DAT_00053c78 = 0;
  }
  else if (DAT_00053c70 == iVar1) {
    DAT_00053c70 = *(int *)(iVar1 + 0x13f);
    *(undefined4 *)(*(int *)(iVar1 + 0x13f) + 0x143) = 0;
  }
  else if (DAT_00053c74 == iVar1) {
    DAT_00053c74 = *(int *)(iVar1 + 0x143);
    *(undefined4 *)(*(int *)(iVar1 + 0x143) + 0x13f) = 0;
  }
  else {
    *(undefined4 *)(*(int *)(iVar1 + 0x143) + 0x13f) = *(undefined4 *)(iVar1 + 0x13f);
    *(undefined4 *)(*(int *)(iVar1 + 0x13f) + 0x143) = *(undefined4 *)(iVar1 + 0x143);
  }
  DAT_00053c68 = DAT_00053c68 + -1;
  return iVar1;
}


