// func #173  FUN_0003eff2  0003eff2  size=133


void FUN_0003eff2(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined2 uVar1;
  
  uVar1 = (undefined2)((uint)param_3 >> 0x10);
  FUN_000418ee();
  FUN_0004231b();
  if (DAT_00063e4a == '\x01') {
    FUN_00042361(0x1c,0x1c,DAT_00063e44,CONCAT22(uVar1,DAT_00063e48));
    FUN_0004231d(0x43,0x36);
    FUN_0004231d(0x40,0xff);
    FUN_0004231d(0x40,0xff);
    DAT_00063e4a = '\0';
  }
  FUN_0004232b();
  return;
}


