// func #86  FUN_00034353  00034353  size=126


void FUN_00034353(void)

{
  undefined1 uVar1;
  int extraout_EDX;
  
  uVar1 = FUN_000418ee();
  (&DAT_00063f14)[DAT_00063f50] = uVar1;
  *(int *)(&DAT_00063f28 + (uint)DAT_00063f50 * 4) = extraout_EDX - DAT_00063f0c;
  (&DAT_00063f1e)[DAT_00063f50] = 0;
  DAT_00063f0c = extraout_EDX;
  DAT_00063f50 = DAT_00063f50 + 1;
  if (9 < DAT_00063f50) {
    DAT_00063f50 = 0;
  }
  return;
}


