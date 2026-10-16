// func #361  FUN_0004791a  0004791a  size=48


void FUN_0004791a(undefined4 *param_1,undefined4 param_2,undefined8 *param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6)

{
  undefined2 uVar1;
  undefined8 uVar2;
  
  uVar1 = *(undefined2 *)(param_1 + 2);
  uVar2 = FUN_0004794a(*param_1,param_1[1],param_5,param_6,param_3,param_4);
  *param_3 = uVar2;
  *(undefined2 *)(param_3 + 1) = uVar1;
  return;
}


