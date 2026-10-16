// func #492  FUN_00044b96  00044b96  size=29

void FUN_00044b96(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 extraout_ECX;
  undefined4 *extraout_EDX;
  
  while (DAT_00063ff8 != (undefined4 *)0x0) {
    FUN_00041c31(DAT_00063ff8,*DAT_00063ff8,param_3,param_4,param_2);
    DAT_00063ff8 = extraout_EDX;
    param_4 = extraout_ECX;
  }
  return;
}
