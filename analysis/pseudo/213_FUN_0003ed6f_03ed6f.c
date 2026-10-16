// func #170  FUN_0003ed6f  0003ed6f  size=155


void FUN_0003ed6f(void)

{
  char *pcVar1;
  
  FUN_000418ee();
  DAT_00063e20 = DAT_00063e18;
  pcVar1 = DAT_00063e20;
  while (DAT_00063e20 = pcVar1, DAT_00063e20 != (char *)0x0) {
    FUN_0003e20f(DAT_00063e20);
    if (*DAT_00063e20 == '\0') {
      pcVar1 = *(char **)(DAT_00063e20 + 0x51);
      if (DAT_00063e20 != (char *)0x0) {
        FUN_0003e14c(DAT_00063e20,0);
        FUN_00041c28();
      }
    }
    else {
      pcVar1 = *(char **)(DAT_00063e20 + 0x51);
    }
  }
  return;
}


