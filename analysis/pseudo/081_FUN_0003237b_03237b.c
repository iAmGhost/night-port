// func #81  FUN_0003237b  0003237b  size=125


void FUN_0003237b(void)

{
  int iVar1;
  int local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  iVar1 = local_20;
  while (local_20 = iVar1, local_20 != 0) {
    iVar1 = *(int *)(local_20 + 0x13f);
    if ((*(int *)(local_20 + 0xd) != 0) && (local_20 != 0)) {
      FUN_000112fd(local_20,0);
      FUN_00041c28();
    }
  }
  DAT_00053c88 = 0;
  return;
}


