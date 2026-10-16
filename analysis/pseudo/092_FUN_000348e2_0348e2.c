// func #92  FUN_000348e2  000348e2  size=92


void FUN_000348e2(void)

{
  FUN_000418ee();
  (&DAT_00063f14)[DAT_00063f50] = 0;
  *(undefined4 *)(&DAT_00063f28 + (uint)DAT_00063f50 * 4) = DAT_00063f08;
  DAT_00063f50 = DAT_00063f50 + 1;
  if (9 < DAT_00063f50) {
    DAT_00063f50 = 0;
  }
  return;
}


