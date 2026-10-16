// func #364  FUN_00047b4f  00047b4f  size=42


void FUN_00047b4f(undefined4 *param_1,undefined4 *param_2,undefined8 *param_3,undefined4 param_4)

{
  undefined2 uVar1;
  undefined8 uVar2;
  
  uVar1 = *(undefined2 *)(param_1 + 2);
  uVar2 = FUN_00047b79(*param_1,param_1[1],*param_2,param_2[1],param_3,param_4);
  *param_3 = uVar2;
  *(undefined2 *)(param_3 + 1) = uVar1;
  return;
}


