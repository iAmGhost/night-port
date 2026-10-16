// func #432  FUN_00034eb0  00034eb0  size=128

void FUN_00034eb0(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined2 uVar1;
  
  uVar1 = (undefined2)((uint)param_3 >> 0x10);
  FUN_000418ee();
  if (DAT_00063324 != 0) {
    FUN_0004231b();
    FUN_0003575d(0);
    FUN_00042361(8,8,SUB_00063334,CONCAT22(uVar1,DAT_00063338));
    FUN_00035ac0();
    FUN_0004232b();
    DAT_00063324 = 0;
    if (DAT_0006333c != 0) {
      FUN_00041c31(DAT_0006333c);
      DAT_0006333c = 0;
    }
  }
  return;
}
