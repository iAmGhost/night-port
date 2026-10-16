// func #180  FUN_0003f885  0003f885  size=438


void FUN_0003f885(void)

{
  ushort local_334;
  int local_330;
  byte local_32c [768];
  int local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  
  local_20 = FUN_000418ee();
  FUN_0004192e(&local_2c,6,1,local_20);
  local_24 = local_28;
  local_334 = (ushort)local_28;
  if (local_334 < 0xc) {
    if (3 < local_334) {
      if (local_334 < 5) {
        FUN_0004247a(local_20,4,1);
        FUN_0004192e(local_32c,3,0x100,local_20);
        for (local_330 = 0; local_330 < 0x300; local_330 = local_330 + 1) {
          local_32c[local_330] = local_32c[local_330] >> 2;
        }
        FUN_00040042();
        return;
      }
      if (local_334 == 0xb) {
        FUN_0003fc1d(local_20,local_2c + -6);
        return;
      }
    }
  }
  else {
    if (local_334 < 0xd) {
      FUN_0003fb73(local_20,local_2c + -6);
      return;
    }
    if (local_334 < 0xf) {
      if (local_334 == 0xd) {
        FUN_0003fade();
        return;
      }
    }
    else {
      if (local_334 < 0x10) {
        FUN_0003fa3b(local_20,local_2c + -6);
        return;
      }
      if (local_334 == 0x10) {
        FUN_0003fb0e(local_20,local_2c + -6);
        return;
      }
    }
  }
  FUN_0004247a(local_20,local_2c + -6,1);
  return;
}


