// func #151  FUN_0003cb38  0003cb38  size=161


void FUN_0003cb38(void)

{
  int iVar1;
  int iVar2;
  
  iVar2 = FUN_000418ee();
  iVar1 = DAT_000639dc;
  if (DAT_000639e4 == '\0') {
    if (*(int *)(DAT_00053c7c + 0x17) == 3) {
      if (DAT_000639dc < *(int *)(iVar2 + 8) + -0x142) {
        DAT_000639dc = DAT_000639dc + 3;
      }
      else {
        DAT_000639dc = *(int *)(iVar2 + 8) + -0x140;
      }
    }
    else if (DAT_000639dc < *(int *)(iVar2 + 8) + -0x140) {
      DAT_000639dc = DAT_000639dc + 1;
    }
  }
  if (iVar1 != DAT_000639dc) {
    FUN_0003d75c(DAT_000639dc - iVar1);
  }
  return;
}


