// func #427  FUN_000323f8  000323f8  size=105

void FUN_000323f8(void)

{
  int iVar1;
  int local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  iVar1 = local_20;
  while (local_20 = iVar1, local_20 != 0) {
    iVar1 = *(int *)(local_20 + 0x13f);
    if (local_20 != 0) {
      FUN_000112fd(local_20,0);
      FUN_00041c28();
    }
  }
  return;
}
