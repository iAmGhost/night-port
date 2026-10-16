// func #511  FUN_00048f2c  00048f2c  size=31

void FUN_00048f2c(undefined8 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined2 uVar1;
  undefined8 uVar2;
  
  uVar1 = *(undefined2 *)(param_1 + 1);
  uVar2 = FUN_00048f4c(*(undefined4 *)param_1,*(undefined4 *)((int)param_1 + 4),param_3,param_2,
                       param_1,param_4);
  *param_1 = uVar2;
  *(undefined2 *)(param_1 + 1) = uVar1;
  return;
}
