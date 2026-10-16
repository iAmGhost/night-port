// func #118  FUN_00038098  00038098  size=319


void FUN_00038098(void)

{
  undefined4 uVar1;
  
  uVar1 = FUN_000418ee();
  FUN_00013aad(DAT_00053c70);
  if (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104)) {
    FUN_0003ce0b(DAT_00063354,uVar1);
    FUN_0003203d(uVar1);
    FUN_0003df96(uVar1);
    FUN_0003b953(uVar1);
    if (DAT_00053c84 != 0) {
      FUN_0001348f(DAT_00053c84,uVar1,0);
      FUN_000137c1(DAT_00053c84,uVar1);
    }
    if (DAT_00053c88 != 0) {
      FUN_0001348f(DAT_00053c88,uVar1,1);
    }
    FUN_000404c6();
  }
  do {
  } while (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104));
  DAT_00063e24 = DAT_00063e24 - (int)(0x1ea / (longlong)DAT_00053104);
  return;
}


