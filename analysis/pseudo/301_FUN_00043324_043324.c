// func #301  FUN_00043324  00043324  size=56


void FUN_00043324(undefined4 param_1,undefined4 *param_2,undefined2 *param_3)

{
  undefined4 extraout_ECX;
  undefined2 *puVar1;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  undefined2 in_ES;
  undefined2 in_DS;
  byte in_CF;
  undefined8 uVar2;
  
  puVar1 = param_3;
  uVar2 = FUN_0004335c();
  *param_2 = (int)uVar2;
  param_2[1] = puVar1;
  param_2[2] = extraout_ECX;
  param_2[3] = (int)((ulonglong)uVar2 >> 0x20);
  param_2[4] = unaff_ESI;
  param_2[5] = unaff_EDI;
  param_2[6] = -(uint)in_CF;
  param_3[3] = in_DS;
  *param_3 = in_ES;
  return;
}


