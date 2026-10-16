// func #178  FUN_0003f6bc  0003f6bc  size=337


undefined4 FUN_0003f6bc(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  int iVar2;
  undefined4 local_1b4;
  undefined1 local_1b0 [4];
  short local_1ac;
  short local_1aa;
  int local_28;
  uint local_24;
  undefined8 local_20;
  undefined4 local_18;
  
  local_20 = FUN_000418ee();
  local_28 = 0;
  local_18 = param_3;
  FUN_000367c9((int)local_20);
  FUN_0004192e(local_1b0,0x80,1,DAT_000639b8);
  if (local_1ac == -0x50ef) {
    local_24 = 0;
    DAT_00063e3c = 0;
    DAT_00063e40 = 0;
    do {
      uVar1 = local_24 + 1;
      if ((int)local_1aa <= (int)(local_24 & 0xffff)) {
        return 0;
      }
      if (((DAT_00063ec1 != '\0') || (DAT_00063ea4 != '\0')) || (DAT_00063e89 != '\0')) {
        return 1;
      }
      do {
      } while (DAT_00063e40 <= (int)(uVar1 & 0xffff));
      local_24 = uVar1;
      if ((uVar1 & 0xffff) == *(uint *)(&DAT_00053200 + local_28 * 4)) {
        FUN_0004185e(*(undefined4 *)(&DAT_00053d0c + (uint)(byte)(&DAT_000532f4)[local_28] * 4));
        local_28 = local_28 + 1;
      }
      iVar2 = FUN_0003f80d(DAT_000639b8);
    } while (iVar2 != 0);
    local_1b4 = 1;
  }
  else {
    local_1b4 = 0;
  }
  return local_1b4;
}


