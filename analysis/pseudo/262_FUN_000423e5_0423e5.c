// func #262  FUN_000423e5  000423e5  size=83


int FUN_000423e5(byte *param_1,byte *param_2,int param_3)

{
  byte bVar1;
  byte local_8;
  
  while( true ) {
    if (param_3 == 0) {
      return 0;
    }
    local_8 = *param_2;
    bVar1 = *param_1;
    if ((0x40 < bVar1) && (bVar1 < 0x5b)) {
      bVar1 = bVar1 + 0x20;
    }
    if ((0x40 < local_8) && (local_8 < 0x5b)) {
      local_8 = local_8 + 0x20;
    }
    if (bVar1 != local_8) break;
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
    param_3 = param_3 + -1;
  }
  return (uint)bVar1 - (uint)local_8;
}


