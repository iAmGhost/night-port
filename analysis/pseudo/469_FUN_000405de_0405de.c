// func #469  FUN_000405de  000405de  size=82

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000405de(undefined4 param_1,undefined4 param_2,uint param_3)

{
  uint extraout_ECX;
  ulonglong uVar1;
  
  uVar1 = FUN_000418ee();
  _DAT_00040014 = uVar1 & 0xffff0000ffff;
  _DAT_0004001c = param_3 & 0xffff;
  _DAT_00040020 = extraout_ECX & 0xffff;
  return;
}
