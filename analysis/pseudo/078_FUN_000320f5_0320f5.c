// func #78  FUN_000320f5  000320f5  size=99


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000320f5(void)

{
  undefined4 local_24;
  undefined4 local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  _DAT_00063388 = 0;
  for (local_24 = 0; local_24 < DAT_00053c68; local_24 = local_24 + 1) {
    FUN_0001441e(local_20);
    local_20 = *(int *)(local_20 + 0x13f);
  }
  return;
}


