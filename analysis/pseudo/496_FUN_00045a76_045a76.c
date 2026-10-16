// func #496  FUN_00045a76  00045a76  size=11

ulonglong FUN_00045a76(uint param_1,int param_2)

{
  uint uVar1;
  byte bVar2;
  uint extraout_EDX;
  byte bVar3;
  
  if (param_2 == 0) {
    return 0;
  }
  if (param_2 == 0) {
    return (ulonglong)param_1;
  }
  bVar3 = (byte)param_1;
  FUN_00042a30(param_1 & 0xff);
  if (extraout_EDX < 0x100) {
    if (2 < DAT_000533bb) {
      bVar2 = (byte)extraout_EDX;
      if (bVar2 == 0x50) {
        bVar3 = 0xe;
      }
      else if (bVar2 < 0x22) {
        if (0x1f < bVar2) {
          bVar3 = 5;
        }
      }
      else {
        bVar3 = 0x13;
      }
    }
    if (0x13 < bVar3) {
      bVar3 = 0x13;
    }
    uVar1 = *(int *)(&UNK_00053625 + bVar3) >> 0x18;
  }
  else {
    uVar1 = extraout_EDX >> 8 & 0xff;
  }
  FUN_00042a0d(uVar1);
  return CONCAT44(param_2,0xffffffff);
}
