// func #156  FUN_0003d4e4  0003d4e4  size=290


void FUN_0003d4e4(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if ((((*(char *)(iVar1 + 0x3b) == '\0') && (0xbe < *(int *)(DAT_00053c7c + 1) - DAT_000639dc)) &&
      ((*(int *)(iVar1 + 0x2e) == 0 || (DAT_000639dc + 0x140 <= *(int *)(iVar1 + 0x2e))))) &&
     (((*(int *)(iVar1 + 0x36) == 0 || (DAT_000639dc + 0x140 <= *(int *)(iVar1 + 0x36))) ||
      (DAT_00053c68 == 1)))) {
    FUN_0003cb38(DAT_00063354);
  }
  if ((((*(char *)(iVar1 + 0x3a) == '\0') && (*(int *)(DAT_00053c7c + 1) - DAT_000639dc < 0x82)) &&
      ((*(int *)(iVar1 + 0x2a) == 0 || (*(int *)(iVar1 + 0x2a) <= DAT_000639dc)))) &&
     (((*(int *)(iVar1 + 0x32) == 0 || (*(int *)(iVar1 + 0x32) <= DAT_000639dc)) ||
      (DAT_00053c68 == 1)))) {
    FUN_0003cbd9(DAT_00063354);
  }
  if (DAT_000639e0 + 0xc2 <= *(int *)(DAT_00053c7c + 9)) {
    FUN_0003cc6b(DAT_00063354);
  }
  return;
}


