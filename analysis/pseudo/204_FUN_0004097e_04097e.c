// func #204  FUN_0004097e  0004097e  size=500


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0004097e(void)

{
  undefined1 uVar1;
  undefined1 *puVar2;
  undefined8 uVar3;
  
  uVar3 = FUN_000418ee();
  puVar2 = (undefined1 *)uVar3;
  uVar1 = puVar2[1];
  FUN_00040948(*puVar2);
  switch(uVar1) {
  case 0:
    DAT_00063f9c = 0x14;
    break;
  case 1:
    DAT_00063f9c = 0x75;
    break;
  case 2:
    DAT_00063f9c = 0x77;
    break;
  case 3:
    DAT_00063f9c = 0x17;
  }
  DAT_00063f94 = (int)((ulonglong)uVar3 >> 0x20) + -2;
  _DAT_00063f8c = 0xffff;
  DAT_00063fa8 = puVar2 + 2;
  if (DAT_00063f94 < 0x10000) {
    DAT_00063f98 = DAT_00063f94;
    FUN_00041e20(DAT_00063fb4,0x80,0xffff);
  }
  else {
    DAT_00063f98 = 0xffff;
  }
  FUN_00041e6c(DAT_00063fb4,DAT_00063fa8,DAT_00063f98);
  DAT_00063f94 = DAT_00063f94 - DAT_00063f98;
  DAT_00063fa8 = DAT_00063fa8 + DAT_00063f98;
  DAT_00063f98 = DAT_00063f98 - 1;
  FUN_0004231d(10,5);
  FUN_0004231d(0xc,0);
  FUN_0004231d(0xb,0x49);
  FUN_0004231d(2,DAT_00063fac & 0xff);
  FUN_0004231d(2,(int)DAT_00063fac >> 8 & 0xff);
  FUN_0004231d(0x83,(int)DAT_00063fac >> 0x10 & 0xff);
  FUN_0004231d(3,DAT_00063f98 & 0xff);
  FUN_0004231d(3,(int)DAT_00063f98 >> 8 & 0xff);
  FUN_0004231d(10,1);
  FUN_00041340(DAT_00063f9c);
  FUN_00041340(DAT_00063f98 & 0xff);
  FUN_00041340((int)DAT_00063f98 >> 8 & 0xff);
  return;
}


