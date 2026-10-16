// func #77  FUN_0003209c  0003209c  size=89


void FUN_0003209c(void)

{
  undefined4 local_24;
  undefined4 local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  for (local_24 = 0; local_24 < DAT_00053c68; local_24 = local_24 + 1) {
    FUN_00012616(local_20);
    local_20 = *(int *)(local_20 + 0x13f);
  }
  return;
}


