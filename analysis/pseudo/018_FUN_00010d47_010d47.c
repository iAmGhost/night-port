// func #13  FUN_00010d47  00010d47  size=151


void FUN_00010d47(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  FUN_000418ee();
  uVar2 = DAT_00063e28;
  uVar1 = DAT_00063e24;
  if (DAT_00053c48 < DAT_00063e38 / 0x1ea) {
    DAT_00063e38 = 0;
    DAT_00052ff8 = 0;
    FUN_00010b1d(*(undefined4 *)
                  (&DAT_00052da4 + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
    DAT_00052ff8 = 1;
  }
  DAT_00063e24 = uVar1;
  DAT_00063e28 = uVar2;
  return;
}


