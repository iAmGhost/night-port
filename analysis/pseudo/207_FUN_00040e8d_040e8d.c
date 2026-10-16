// func #207  FUN_00040e8d  00040e8d  size=112


void FUN_00040e8d(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined2 uVar1;
  undefined4 local_20;
  
  uVar1 = (undefined2)((uint)param_3 >> 0x10);
  FUN_000418ee();
  if (DAT_00063f90 != 0) {
    if (DAT_00063f84 == 10) {
      local_20 = 0x72;
    }
    else {
      local_20 = DAT_00063f84 + 8;
    }
    FUN_000407f8(DAT_00063fb0);
    FUN_00042361(local_20,local_20,DAT_00063f54,CONCAT22(uVar1,DAT_00063f58));
    FUN_00041410();
  }
  return;
}


