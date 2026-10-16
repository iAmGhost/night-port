// func #114  FUN_000374ad  000374ad  size=539


void FUN_000374ad(void)

{
  int local_28;
  
  FUN_000418ee();
  *(undefined1 *)(DAT_00053c7c + 0x30) = 0;
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
  FUN_00037b03(0);
  DAT_00063354 = DAT_0006334c;
  FUN_0003cad3(DAT_0006334c);
  DAT_00063398 = (uint)*(byte *)(DAT_00063354 + 0x11);
  if (DAT_00063398 == 0) {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + -0x1e;
  }
  else {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + 0x15e;
  }
  *(undefined4 *)(DAT_00053c7c + 5) = 0;
  *(int *)(DAT_00053c7c + 9) =
       DAT_000639e0 +
       *(int *)(&DAT_00052c3c + (DAT_00063370 + -1) * 0x28 + *(int *)(DAT_00063354 + 0x40) * 4);
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  return;
}


