// func #76  FUN_0003203d  0003203d  size=95


void FUN_0003203d(void)

{
  undefined4 uVar1;
  undefined4 local_28;
  undefined4 local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = DAT_00053c70;
  for (local_28 = 0; local_28 < DAT_00053c68; local_28 = local_28 + 1) {
    FUN_00012efa(local_24,uVar1);
    local_24 = *(int *)(local_24 + 0x13f);
  }
  return;
}


