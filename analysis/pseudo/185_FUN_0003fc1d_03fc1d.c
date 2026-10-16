// func #185  FUN_0003fc1d  0003fc1d  size=160


void FUN_0003fc1d(void)

{
  undefined1 local_32c [768];
  undefined1 local_2c [2];
  char local_2a;
  byte local_29;
  uint local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  FUN_0004192e(local_2c,4,1,(int)local_20);
  if ((local_2a == '\0') && (local_29 == 0)) {
    local_24 = 0x100;
  }
  else {
    local_24 = (uint)local_29;
  }
  FUN_0004192e(local_32c,local_20._4_4_ + -4,1,(undefined4)local_20);
  FUN_00040042();
  return;
}


