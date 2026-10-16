// func #516  FUN_00049c76  00049c76  size=142

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00049c76(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  char *pcVar1;
  int local_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  _DAT_00053784 = 0;
  uStack_c = param_2;
  uStack_8 = param_4;
  uStack_4 = param_3;
  pcVar1 = (char *)FUN_00049a6e(param_1,&DAT_00053788,&DAT_0005377c);
  if (*pcVar1 == '\0') {
    DAT_000537a7 = 0;
  }
  else {
    _DAT_00053784 = 1;
    local_10 = 0x3840;
    pcVar1 = (char *)FUN_00049a6e(pcVar1,&DAT_000537a7,&local_10);
    _DAT_00053780 = 18000 - local_10;
    if (*pcVar1 == ',') {
      pcVar1 = (char *)FUN_00049b84(pcVar1 + 1,&DAT_00053734);
    }
    if (*pcVar1 == ',') {
      FUN_00049b84(pcVar1 + 1,&DAT_00053758);
    }
  }
  return;
}
