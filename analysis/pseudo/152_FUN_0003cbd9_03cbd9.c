// func #152  FUN_0003cbd9  0003cbd9  size=146


void FUN_0003cbd9(void)

{
  int iVar1;
  int *piVar2;
  
  piVar2 = (int *)FUN_000418ee();
  iVar1 = DAT_000639dc;
  if (DAT_000639e4 == '\0') {
    if (*(int *)(DAT_00053c7c + 0x17) == 3) {
      if (*piVar2 + 2 < DAT_000639dc) {
        DAT_000639dc = DAT_000639dc + -3;
      }
      else {
        DAT_000639dc = *piVar2;
      }
    }
    else if (*piVar2 < DAT_000639dc) {
      DAT_000639dc = DAT_000639dc + -1;
    }
  }
  if (iVar1 != DAT_000639dc) {
    FUN_0003d75c(DAT_000639dc - iVar1);
  }
  return;
}


