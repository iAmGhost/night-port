// func #245  FUN_00041fa1  00041fa1  size=1


void FUN_00041fa1(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  char cVar2;
  
  if (DAT_0005340c != '\x01') {
    return;
  }
  if (DAT_000533b4 == '\0') {
    if (DAT_000533b2 == '\t') {
      (**(code **)(DAT_00053364 + 0x30))(0xe02,param_2,0);
    }
    else if (DAT_000533b2 == '\0') {
      pcVar1 = (code *)swi(0x21);
      (*pcVar1)(CONCAT11(0xf3,(char)DAT_0005340a));
    }
    else if (DAT_000533b2 == '\x01') {
      FUN_000490b0(0,DAT_00053368,param_3,param_4,param_2,param_3,param_4);
    }
    else {
      cVar2 = FUN_000490b0(DAT_000533b2,0,param_3,param_4,param_2,param_3,param_4);
      if (cVar2 != '\x01') {
        FUN_0004391d();
      }
    }
  }
  else {
    FUN_00043958();
  }
  DAT_0005340c = 0;
  return;
}


