// func #479  FUN_00042a08  00042a08  size=4

void FUN_00042a08(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  
  uVar1 = FUN_00045a6a(0xd,0xd,param_3,param_4,param_2);
  *(undefined4 *)uVar1 = (int)((ulonglong)uVar1 >> 0x20);
  return;
}
