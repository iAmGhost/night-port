// func #256  FUN_00042361  00042361  size=43


void FUN_00042361(undefined1 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  
  if ((1 < DAT_000533b2) && (DAT_000533b2 < 9)) {
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)(4,param_3,DAT_000533b2,param_1);
    return;
  }
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)(param_1,param_3,DAT_000533b2,param_4,param_2);
  return;
}


