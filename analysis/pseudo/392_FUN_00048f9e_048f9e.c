// func #392  FUN_00048f9e  00048f9e  size=72


void FUN_00048f9e(undefined8 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined2 uVar1;
  undefined8 uVar2;
  
  uVar1 = *(undefined2 *)(param_1 + 1);
  uVar2 = FUN_00047d4e(*(undefined4 *)param_1,*(undefined4 *)((int)param_1 + 4),
                       *(undefined4 *)param_1,*(undefined4 *)((int)param_1 + 4),param_2,param_3,
                       param_4);
  uVar2 = FUN_00048f4c((int)uVar2,(int)((ulonglong)uVar2 >> 0x20),param_3,param_2);
  uVar2 = FUN_00047d4e((int)uVar2,(int)((ulonglong)uVar2 >> 0x20),*(undefined4 *)param_1,
                       *(undefined4 *)((int)param_1 + 4));
  *param_1 = uVar2;
  *(undefined2 *)(param_1 + 1) = uVar1;
  return;
}


