// func #172  FUN_0003ef29  0003ef29  size=201


void FUN_0003ef29(void)

{
  uint uVar1;
  undefined2 in_CS;
  undefined6 uVar2;
  
  uVar1 = FUN_000418ee();
  FUN_0004231b();
  if (DAT_00063e4a == '\0') {
    DAT_00063e2c = uVar1 & 0xffff;
    DAT_00063e30 = 0;
    uVar2 = FUN_0004232d(0x1c);
    DAT_00063e48 = (undefined2)((uint6)uVar2 >> 0x20);
    DAT_00063e44 = (undefined4)uVar2;
    FUN_00042361(0x1c,0x1c,&LAB_0003ee88,in_CS);
    FUN_0004231d(0x43,0x36);
    FUN_0004231d(0x40,(ushort)uVar1 & 0xff);
    FUN_0004231d(0x40,(int)(uVar1 & 0xff00) >> 8);
    DAT_00063e4a = '\x01';
  }
  FUN_0004232b();
  return;
}


