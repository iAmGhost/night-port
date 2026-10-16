// func #348  FUN_00045a82  00045a82  size=102


undefined8 FUN_00045a82(byte param_1,undefined4 param_2)

{
  uint uVar1;
  byte bVar2;
  uint extraout_EDX;
  
  FUN_00042a30(param_1);
  if (extraout_EDX < 0x100) {
    if (2 < DAT_000533bb) {
      bVar2 = (byte)extraout_EDX;
      if (bVar2 == 0x50) {
        param_1 = 0xe;
      }
      else if (bVar2 < 0x22) {
        if (0x1f < bVar2) {
          param_1 = 5;
        }
      }
      else {
        param_1 = 0x13;
      }
    }
    if (0x13 < param_1) {
      param_1 = 0x13;
    }
    uVar1 = *(int *)(&UNK_00053625 + param_1) >> 0x18;
  }
  else {
    uVar1 = extraout_EDX >> 8 & 0xff;
  }
  FUN_00042a0d(uVar1);
  return CONCAT44(param_2,0xffffffff);
}


