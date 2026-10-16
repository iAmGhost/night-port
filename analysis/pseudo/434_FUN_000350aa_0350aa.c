// func #434  FUN_000350aa  000350aa  size=96

undefined4 FUN_000350aa(void)

{
  undefined8 uVar1;
  int local_24;
  char *local_20;
  char *local_1c;
  
  uVar1 = FUN_000418ee();
  local_24 = 0;
  while( true ) {
    local_1c = (char *)((ulonglong)uVar1 >> 0x20);
    local_20 = (char *)uVar1;
    if (3 < local_24) {
      return 1;
    }
    uVar1 = CONCAT44(local_1c + 1,local_20 + 1);
    if (*local_20 != *local_1c) break;
    local_24 = local_24 + 1;
  }
  return 0xffffffff;
}
