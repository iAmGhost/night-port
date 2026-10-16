// func #213  FUN_000412e9  000412e9  size=87


undefined8 FUN_000412e9(void)

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
      goto LAB_00041334;
    }
    uVar1 = FUN_0004238c(DAT_00053334 + 0xe);
  } while ((uVar1 & 0x80) == 0);
  local_24 = 1;
LAB_00041334:
  return CONCAT44(extraout_EDX,local_24);
}


