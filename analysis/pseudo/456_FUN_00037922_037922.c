// func #456  FUN_00037922  00037922  size=480

void FUN_00037922(void)

{
  int local_40;
  
  FUN_000418ee();
  for (local_40 = 0; local_40 < 100; local_40 = local_40 + 1) {
    FUN_00041c31(*(undefined4 *)(&DAT_00053d0c + local_40 * 4));
  }
  if (DAT_000639c0 != 0) {
    FUN_0003d999(DAT_000639c0,0);
    FUN_00041c28();
  }
  DAT_000639c0 = 0;
  if (DAT_00053c8c != 0) {
    FUN_0003d999(DAT_00053c8c,0);
    FUN_00041c28();
  }
  DAT_00053c8c = 0;
  if (DAT_00063e00 != 0) {
    FUN_0003d999(DAT_00063e00,0);
    FUN_00041c28();
  }
  DAT_00063e00 = 0;
  if (DAT_000639ec != 0) {
    FUN_0003d999(DAT_000639ec,0);
    FUN_00041c28();
  }
  DAT_000639ec = 0;
  if (DAT_00063d10 != 0) {
    FUN_0003d999(DAT_00063d10,0);
    FUN_00041c28();
  }
  DAT_00063d10 = 0;
  if (DAT_00053b3c != 0) {
    FUN_000100d9(DAT_00053b3c,0);
    FUN_00041c28();
  }
  DAT_00053b3c = 0;
  if (DAT_00063dd4 != 0) {
    FUN_0003d999(DAT_00063dd4,0);
    FUN_00041c28();
  }
  DAT_00063dd4 = 0;
  if (DAT_00053c00 != 0) {
    FUN_000100d9(DAT_00053c00,0);
    FUN_00041c28();
  }
  DAT_00053c00 = 0;
  return;
}
