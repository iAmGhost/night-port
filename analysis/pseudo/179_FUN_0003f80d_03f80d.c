// func #179  FUN_0003f80d  0003f80d  size=120


undefined4 FUN_0003f80d(void)

{
  undefined4 local_38;
  undefined1 local_34 [4];
  short local_30;
  short local_2e;
  int local_24;
  undefined4 local_20;
  
  local_20 = FUN_000418ee();
  local_24 = 0;
  FUN_0004192e(local_34,0x10,1,local_20);
  if (local_30 == -0xe06) {
    while( true ) {
      if (local_2e <= local_24) break;
      local_24 = local_24 + 1;
      FUN_0003f885(local_20);
    }
    local_38 = 1;
  }
  else {
    local_38 = 0;
  }
  return local_38;
}


