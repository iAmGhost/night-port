// func #303  FUN_0004391d  0004391d  size=59


void FUN_0004391d(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  code *pcVar1;
  
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)(0x2504,DAT_00053404,param_3,7);
  if ('\x02' < DAT_000533b2) {
    FUN_00043958();
  }
  return;
}


