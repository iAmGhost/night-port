// func #1  FUN_000100d9  000100d9  size=168


int * FUN_000100d9(void)

{
  int iVar1;
  int *piVar2;
  int local_24;
  
  piVar2 = (int *)FUN_000418ee();
  if (piVar2[1] != 0) {
    for (local_24 = 0; local_24 < *piVar2; local_24 = local_24 + 1) {
      if ((*(int *)(piVar2[1] + local_24 * 4) != 0) &&
         (iVar1 = *(int *)(piVar2[1] + local_24 * 4), iVar1 != 0)) {
        FUN_00010376(iVar1,0);
        FUN_00041c28();
      }
    }
    FUN_00041c31(piVar2[1]);
  }
  return piVar2;
}


