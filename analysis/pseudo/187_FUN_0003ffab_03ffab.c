// func #187  FUN_0003ffab  0003ffab  size=98


void FUN_0003ffab(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined2 uVar1;
  
  uVar1 = (undefined2)((uint)param_3 >> 0x10);
  FUN_000418ee();
  if (DAT_00063e86 == '\x01') {
    FUN_00042361(9,9,DAT_00063e80,CONCAT22(uVar1,DAT_00063e84));
    DAT_00063e86 = '\0';
    DAT_00000417 = DAT_00063f51;
  }
  return;
}


