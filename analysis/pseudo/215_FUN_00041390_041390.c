// func #215  FUN_00041390  00041390  size=87


undefined8 FUN_00041390(void)

{
  uint uVar1;
  undefined4 extraout_EDX;
  undefined4 local_24;
  int local_20;
  
  FUN_000418ee();
  local_20 = 0x7fff;
  do {
    local_20 = local_20 + -1;
    if (local_20 == -1) {
      local_24 = 0;
      goto LAB_000413db;
    }
    uVar1 = FUN_0004238c(DAT_00053334 + 0xc);
  } while ((uVar1 & 0x80) != 0);
  local_24 = 1;
LAB_000413db:
  return CONCAT44(extraout_EDX,local_24);
}


