// func #459  FUN_0003c65b  0003c65b  size=143

int FUN_0003c65b(void)

{
  int iVar1;
  int local_2c;
  int local_28;
  undefined4 *local_24;
  int local_20;
  
  local_20 = FUN_000418ee();
  iVar1 = local_20;
  for (local_2c = 0; local_2c < 200; local_2c = local_2c + 1) {
    local_24 = *(undefined4 **)(&DAT_00062d68 + local_2c * 4);
    local_28 = 0;
    while( true ) {
      FUN_00041e6c(local_20 + local_28,*local_24,(int)*(short *)(local_24 + 2));
      local_28 = local_28 + *(short *)(local_24 + 2);
      if (0x13f < local_28) break;
      local_24 = (undefined4 *)local_24[1];
    }
    local_20 = local_20 + 0x140;
    iVar1 = local_2c;
  }
  return iVar1;
}
