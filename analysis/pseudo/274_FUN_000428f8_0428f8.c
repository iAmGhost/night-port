// func #274  FUN_000428f8  000428f8  size=48


void FUN_000428f8(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 int param_5,undefined4 param_6)

{
  int iVar1;
  undefined1 *local_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  local_10 = &stack0x0000000c;
  uStack_c = param_2;
  uStack_8 = param_4;
  uStack_4 = param_3;
  iVar1 = FUN_00044c23(param_5,param_6,&local_10,&LAB_000428e5);
  *(undefined1 *)(param_5 + iVar1) = 0;
  return;
}


