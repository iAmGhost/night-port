// func #288  FUN_00042c8d  00042c8d  size=63


void FUN_00042c8d(int param_1)

{
  int iVar1;
  int iVar2;
  
  if (param_1 == DAT_00053354) {
    DAT_00053354 = *(int *)(DAT_00053354 + 8);
  }
  if (param_1 == DAT_00053350) {
    DAT_00053350 = *(int *)(DAT_00053350 + 8);
  }
  iVar1 = *(int *)(param_1 + 4);
  iVar2 = *(int *)(param_1 + 8);
  if (iVar1 != 0) {
    *(int *)(iVar1 + 8) = iVar2;
  }
  if (iVar2 != 0) {
    *(int *)(iVar2 + 4) = iVar1;
  }
  return;
}


