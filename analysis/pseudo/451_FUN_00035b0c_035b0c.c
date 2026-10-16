// func #451  FUN_00035b0c  00035b0c  size=132

void FUN_00035b0c(undefined4 param_1,undefined4 param_2,byte param_3)

{
  byte bVar1;
  char extraout_DL;
  
  bVar1 = FUN_000418ee();
  if (((bVar1 & 0xf0) == 0xb0) && (extraout_DL == '\a')) {
    *(uint *)(&DAT_000530bc + (uint)(bVar1 & 0xf) * 4) = (uint)param_3;
    FUN_00035b91(bVar1,7,param_3);
  }
  else {
    FUN_00035a19(bVar1,extraout_DL,param_3);
  }
  return;
}
