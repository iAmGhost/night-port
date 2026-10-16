// func #239  FUN_00041ec3  00041ec3  size=16


void FUN_00041ec3(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  
  uVar1 = FUN_00041e99(param_1,param_1,param_3,param_4,param_2);
  if ((undefined4 *)uVar1 != (undefined4 *)0x0) {
    *(undefined4 *)uVar1 = (int)((ulonglong)uVar1 >> 0x20);
  }
  return;
}


