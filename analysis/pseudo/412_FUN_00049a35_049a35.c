// func #412  FUN_00049a35  00049a35  size=162


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00049a35(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  char *pcVar1;
  undefined8 uVar2;
  int local_10;
  undefined4 uStack_c;
  undefined4 uStack_4;
  
  uStack_4 = 0x49a3f;
  uVar2 = FUN_0004a24d(&DAT_00051840);
  uStack_c = (undefined4)((ulonglong)uVar2 >> 0x20);
  if ((int)uVar2 == 0) {
    return;
  }
  _DAT_00053784 = 0;
  uStack_4 = param_3;
  pcVar1 = (char *)FUN_00049a6e((int)uVar2,&DAT_00053788,&DAT_0005377c);
  if (*pcVar1 == '\0') {
    DAT_000537a7 = 0;
  }
  else {
    _DAT_00053784 = 1;
    local_10 = DAT_0005377c + -0xe10;
    pcVar1 = (char *)FUN_00049a6e(pcVar1,&DAT_000537a7,&local_10);
    _DAT_00053780 = DAT_0005377c - local_10;
    if (*pcVar1 == ',') {
      pcVar1 = (char *)FUN_00049b84(pcVar1 + 1,&DAT_00053734);
    }
    if (*pcVar1 == ',') {
      FUN_00049b84(pcVar1 + 1,&DAT_00053758);
    }
  }
  return;
}


