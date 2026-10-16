// func #428  FUN_000326c1  000326c1  size=109

int * FUN_000326c1(void)

{
  int *piVar1;
  int local_24;
  
  piVar1 = (int *)FUN_000418ee();
  for (local_24 = 0; local_24 < *piVar1; local_24 = local_24 + 1) {
    FUN_00041c31(piVar1[local_24 + 0x1a]);
    FUN_00041c31(piVar1[local_24 + 0x7e]);
  }
  return piVar1;
}
