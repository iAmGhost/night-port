// func #464  FUN_0003f592  0003f592  size=178

void FUN_0003f592(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined1 extraout_CL;
  undefined8 uVar1;
  int local_2c;
  int local_28;
  int local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = (int)uVar1 + (int)((ulonglong)uVar1 >> 0x20) * DAT_00063e64;
  local_28 = local_24 / DAT_000531f4;
  FUN_0003f3be(local_28,local_24 % DAT_000531f4);
  local_24 = local_24 % DAT_000531f4;
  for (local_2c = 0; local_2c < param_3; local_2c = local_2c + 1) {
    (&DAT_000a0000)[local_24] = extraout_CL;
    local_24 = local_24 + DAT_00063e64;
    if (DAT_000531f4 <= local_24) {
      local_24 = local_24 - DAT_000531f4;
      local_28 = local_28 + 1;
      FUN_0003f3be(local_28);
    }
  }
  return;
}
