// func #443  FUN_0003575d  0003575d  size=148

void FUN_0003575d(void)

{
  undefined4 local_20;
  
  local_20 = FUN_000418ee();
  DAT_0006332c = 0;
  DAT_00063318 = local_20;
  if (local_20 == 0) {
    DAT_00063318 = 0x10000;
  }
  if (0x10000 < (int)DAT_00063318) {
    DAT_00063318 = 0x10000;
    local_20 = 0;
  }
  FUN_0004231d(0x43,0x34);
  FUN_0004231d(0x40,local_20 & 0xff);
  FUN_0004231d(0x40,(int)local_20 >> 8);
  return;
}
