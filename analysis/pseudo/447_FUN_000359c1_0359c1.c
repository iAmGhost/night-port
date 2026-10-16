// func #447  FUN_000359c1  000359c1  size=87

undefined8 FUN_000359c1(void)

{
  int iVar1;
  undefined4 extraout_EDX;
  undefined4 local_24;
  undefined4 local_20;
  
  FUN_000418ee();
  local_20 = 0x300;
  do {
    if (0x330 < local_20) {
      local_24 = 0xffffffff;
LAB_00035a0d:
      return CONCAT44(extraout_EDX,local_24);
    }
    iVar1 = FUN_00035941(local_20);
    if (iVar1 == 0) {
      local_24 = 0;
      goto LAB_00035a0d;
    }
    local_20 = local_20 + 0x10;
  } while( true );
}
