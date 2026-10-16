// func #84  FUN_00034295  00034295  size=106


void FUN_00034295(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_00063ed3 == '\0') {
    if ((DAT_00063ed5 != '\0') && (*(int *)(DAT_00053c7c + 0x1b) == 1)) {
      FUN_0001545e(DAT_00053c7c,iVar1);
    }
  }
  else if (*(int *)(DAT_00053c7c + 0x1b) == 0) {
    FUN_0001545e(DAT_00053c7c,-iVar1);
  }
  return;
}


