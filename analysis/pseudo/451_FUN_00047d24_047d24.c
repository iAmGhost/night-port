// func #366  FUN_00047d24  00047d24  size=42


void FUN_00047d24(undefined4 *param_1,undefined4 *param_2,undefined8 *param_3,undefined4 param_4)

{
  undefined2 uVar1;
  undefined8 uVar2;
  
  uVar1 = *(undefined2 *)(param_1 + 2);
  uVar2 = FUN_00047d4e(*param_1,param_1[1],*param_2,param_2[1],param_3,param_4);
  *param_3 = uVar2;
  *(undefined2 *)(param_3 + 1) = uVar1;
  return;
}


