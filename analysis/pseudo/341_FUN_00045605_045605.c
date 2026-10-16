// func #341  FUN_00045605  00045605  size=24


void FUN_00045605(char *param_1)

{
  undefined1 uVar1;
  undefined1 *extraout_EDX;
  
  while (*param_1 != '\0') {
    uVar1 = FUN_000494bd(*param_1);
    *extraout_EDX = uVar1;
    param_1 = extraout_EDX + 1;
  }
  return;
}


