// func #177  FUN_0003f645  0003f645  size=119


void FUN_0003f645(void)

{
  undefined1 uVar1;
  int local_28;
  int local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = 0;
  local_28 = 0;
  FUN_0003f3be(0);
  while (DAT_00063e64 * DAT_00063e68 - local_28 != 0 && local_28 <= DAT_00063e64 * DAT_00063e68) {
    FUN_00041e20(&DAT_000a0000,uVar1,DAT_000531f4);
    local_28 = local_28 + DAT_000531f4;
    local_24 = local_24 + 1;
    FUN_0003f3be(local_24);
  }
  return;
}


