// func #295  FUN_000431b5  000431b5  size=119


longlong FUN_000431b5(uint *param_1,uint param_2)

{
  uint uVar1;
  undefined8 uVar2;
  
  uVar1 = *param_1 + 3 & 0xfffffffc;
  if (uVar1 != 0) {
    if (((DAT_000533b2 == '\x01') && (DAT_000533b3 == '\0')) || (DAT_000533b2 == '\t')) {
      uVar1 = uVar1 + 8;
    }
    else {
      uVar2 = FUN_00042ea1();
      uVar1 = (int)((ulonglong)uVar2 >> 0x20) - (int)uVar2;
    }
    *param_1 = uVar1;
    uVar1 = uVar1 + 0x3c;
    if (*param_1 <= uVar1) {
      if (uVar1 < DAT_00053640) {
        uVar1 = DAT_00053640 & 0xfffffffe;
      }
      *param_1 = uVar1;
      uVar1 = uVar1 + 0xfff;
      if (*param_1 <= uVar1) {
        *param_1 = (uint)((uint3)(uVar1 >> 8) & 0xfffff0) << 8;
        return CONCAT44(param_2,(uint)((uVar1 & 0xfffff000) != 0));
      }
    }
  }
  return (ulonglong)param_2 << 0x20;
}


