// func #113  FUN_00037347  00037347  size=358


void FUN_00037347(void)

{
  int local_28;
  
  FUN_000418ee();
  FUN_0003237b();
  FUN_0003ee0a();
  DAT_00053c88 = 0;
  for (local_28 = 1; local_28 < 0x104; local_28 = local_28 + 1) {
    if (((local_28 != 0xc9) && (local_28 != 0xfa)) && (*(int *)(&DAT_000639ec + local_28 * 4) != 0))
    {
      if (*(int *)(&DAT_000639ec + local_28 * 4) != 0) {
        FUN_0003d999(*(int *)(&DAT_000639ec + local_28 * 4),0);
        FUN_00041c28();
      }
      if ((&DAT_00053818)[local_28] != 0) {
        FUN_000100d9((&DAT_00053818)[local_28],0);
        FUN_00041c28();
      }
      *(undefined4 *)(&DAT_000639ec + local_28 * 4) = 0;
      (&DAT_00053818)[local_28] = 0;
    }
  }
  if (DAT_00063dfc != 0) {
    FUN_0003d999(DAT_00063dfc,0);
    FUN_00041c28();
  }
  DAT_00063dfc = 0;
  if (DAT_0006334c != 0) {
    FUN_0003c72b(DAT_0006334c,0);
    FUN_00041c28();
  }
  DAT_0006334c = 0;
  return;
}


