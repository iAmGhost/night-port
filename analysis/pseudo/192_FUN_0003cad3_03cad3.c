// func #150  FUN_0003cad3  0003cad3  size=101


void FUN_0003cad3(void)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_000418ee();
  if (*(char *)((int)piVar1 + 0x11) == '\0') {
    DAT_000639dc = *piVar1;
  }
  else {
    DAT_000639dc = piVar1[2] + -0x140;
  }
  DAT_000639e0 = piVar1[1];
  DAT_000639e4 = (char)piVar1[4];
  return;
}


