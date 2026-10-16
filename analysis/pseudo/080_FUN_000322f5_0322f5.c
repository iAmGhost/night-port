// func #80  FUN_000322f5  000322f5  size=134


void FUN_000322f5(void)

{
  int local_20;
  
  FUN_000418ee();
  FUN_00032158();
  DAT_00053c78 = DAT_00053c70;
  for (local_20 = 0; local_20 < DAT_00053c68; local_20 = local_20 + 1) {
    if ((DAT_00053c78 != DAT_00053c7c) && (*DAT_00053c78 != '\x02')) {
      FUN_00031a8e(DAT_00053c78);
    }
    DAT_00053c78 = *(char **)(DAT_00053c78 + 0x13f);
  }
  return;
}


