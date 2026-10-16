// func #400  FUN_000492d0  000492d0  size=85


longlong FUN_000492d0(uint param_1,uint param_2)

{
  int iVar1;
  undefined8 uVar2;
  
  if (DAT_000536dc <= param_1) {
    return (ulonglong)param_2 << 0x20;
  }
  if ((int)param_1 < 6) {
    iVar1 = param_1 * 4;
    if ((PTR_DAT_00053730[iVar1 + 1] & 0x40) == 0) {
      PTR_DAT_00053730[iVar1 + 1] = PTR_DAT_00053730[iVar1 + 1] | 0x40;
      uVar2 = FUN_000492b2(param_1);
      param_1 = (uint)((ulonglong)uVar2 >> 0x20);
      if ((int)uVar2 != 0) {
        PTR_DAT_00053730[iVar1 + 1] = PTR_DAT_00053730[iVar1 + 1] | 0x20;
      }
    }
  }
  return CONCAT44(param_2,*(undefined4 *)(PTR_DAT_00053730 + param_1 * 4));
}


