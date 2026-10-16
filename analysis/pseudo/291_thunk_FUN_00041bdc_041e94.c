// func #236  thunk_FUN_00041bdc  00041e94  size=5  [thunk]


undefined8 thunk_FUN_00041bdc(int param_1,undefined4 param_2)

{
  int extraout_EDX;
  int iVar1;
  undefined8 uVar2;
  
  if (param_1 == 0) {
    param_1 = 1;
  }
LAB_00041bea:
  uVar2 = FUN_00041af8(param_1);
  iVar1 = (int)uVar2;
  if (iVar1 == 0) {
    if (DAT_00063fdc != (code *)0x0) goto code_r0x00041c03;
    goto LAB_00041c0c;
  }
LAB_00041c1f:
  return CONCAT44(param_2,iVar1);
code_r0x00041c03:
  uVar2 = (*DAT_00063fdc)((int)((ulonglong)uVar2 >> 0x20));
  param_1 = (int)((ulonglong)uVar2 >> 0x20);
  if ((int)uVar2 == 0) {
LAB_00041c0c:
    if (DAT_00063fd8 == (code *)0x0) goto LAB_00041c1f;
    (*DAT_00063fd8)();
    param_1 = extraout_EDX;
  }
  goto LAB_00041bea;
}


