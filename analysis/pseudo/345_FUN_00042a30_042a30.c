// func #283  FUN_00042a30  00042a30  size=12


void FUN_00042a30(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  
  uVar1 = FUN_00045a70(param_1,param_1,param_3,param_4,param_2);
  *(undefined4 *)uVar1 = (int)((ulonglong)uVar1 >> 0x20);
  return;
}


