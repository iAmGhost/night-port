// func #154  FUN_0003ce0b  0003ce0b  size=693


int FUN_0003ce0b(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_000639e4 == '\0') {
    FUN_00040491();
  }
  else if (DAT_000639e0 < *(int *)(iVar1 + 4)) {
    FUN_00040491();
    FUN_00040491();
  }
  else if (*(int *)(iVar1 + 0xc) < DAT_000639e0 + 200) {
    FUN_00040491();
    FUN_00040491();
  }
  else {
    FUN_00040491();
  }
  *(undefined4 *)(iVar1 + 0x22) = 0;
  *(undefined4 *)(iVar1 + 0x1e) = 0;
  iVar1 = DAT_00063360 / 10;
  if (DAT_00063360 % 10 == 0) {
    iVar1 = FUN_00040042();
  }
  return iVar1;
}


