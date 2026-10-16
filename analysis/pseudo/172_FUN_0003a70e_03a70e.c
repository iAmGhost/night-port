// func #132  FUN_0003a70e  0003a70e  size=283


void FUN_0003a70e(void)

{
  undefined1 local_3c [20];
  int local_28;
  undefined4 local_24;
  undefined4 local_20;
  
  FUN_000418ee();
  local_28 = 0;
  local_20 = DAT_00063e24;
  local_24 = DAT_00063e28;
  DAT_00052ff8 = 0;
  do {
    FUN_000428f8();
    FUN_0003a594(0xaa8c0,DAT_000633b0 + 0xa8c0,0x5140);
    FUN_00036041(local_3c,0);
    do {
      if ((DAT_00063ec1 != '\0') || (DAT_00063ea4 != '\0')) break;
    } while (DAT_00063e89 == '\0');
    if (DAT_00063e89 != '\0') break;
    if (local_28 != *(int *)(&DAT_00052f08 + DAT_00063370 * 4)) {
      FUN_0004185e(DAT_00053dd8);
    }
    do {
      do {
      } while (DAT_00063ec1 != '\0');
    } while (DAT_00063ea4 != '\0');
    local_28 = local_28 + 1;
  } while (local_28 < *(int *)(&DAT_00052f08 + DAT_00063370 * 4));
  FUN_0003a5f7(DAT_000633b0);
  DAT_00063e24 = local_20;
  DAT_00063e28 = local_24;
  DAT_00052ff8 = 1;
  return;
}


