// func #342  FUN_0004564f  0004564f  size=56


void FUN_0004564f(int param_1)

{
  code *pcVar1;
  uint uVar2;
  uint uVar3;
  longlong lVar4;
  
  if (param_1 != 0) {
    uVar2 = param_1 * DAT_00064000 + 500;
    uVar3 = uVar2 / 1000;
    if (uVar3 == 0) {
      uVar3 = 1;
    }
    lVar4 = (ulonglong)(uVar2 % 1000) << 0x20;
    do {
      pcVar1 = (code *)swi(0x21);
      lVar4 = (*pcVar1)(CONCAT11(0x2c,(char)lVar4),(int)((ulonglong)lVar4 >> 0x20));
      uVar3 = uVar3 - 1;
    } while (uVar3 != 0);
  }
  return;
}


