// func #131  FUN_0003a5f7  0003a5f7  size=279


void FUN_0003a5f7(void)

{
  int iVar1;
  int local_28;
  int local_24;
  
  iVar1 = FUN_000418ee();
  local_24 = 0;
  DAT_00063e24 = 0;
  do {
    if (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104)) {
      for (local_28 = 0; local_28 < 200; local_28 = local_28 + 1) {
        if (local_28 % 2 == 0) {
          FUN_00041e6c(((local_28 + 1) * 0x140 - local_24) + 0x9ffff,local_28 * 0x140 + iVar1,
                       local_24 + 1);
        }
        else {
          FUN_00041e6c(&DAT_000a0000 + local_28 * 0x140,
                       (((local_28 + 1) * 0x140 + iVar1) - local_24) + -1,local_24 + 1);
        }
      }
    }
    do {
    } while (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104));
    DAT_00063e24 = DAT_00063e24 - (int)(0x1ea / (longlong)DAT_00053104);
    local_24 = local_24 + 4;
  } while (local_24 < 0x140);
  return;
}


