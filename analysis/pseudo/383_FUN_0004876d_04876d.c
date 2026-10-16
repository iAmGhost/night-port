// func #383  FUN_0004876d  0004876d  size=67


void FUN_0004876d(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 *extraout_EDX;
  undefined8 uVar1;
  
  FUN_000487b0();
  FUN_00047d24(param_1,extraout_EDX,extraout_EDX);
  uVar1 = FUN_00047d4e(0x5c17f0bd,0xb8aa3b29,*(undefined4 *)extraout_EDX,
                       *(undefined4 *)((int)extraout_EDX + 4),param_3,param_4,param_2);
  *extraout_EDX = uVar1;
  *(undefined2 *)(extraout_EDX + 1) = 0x3fff;
  return;
}


