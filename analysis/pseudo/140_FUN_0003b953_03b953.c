// func #140  FUN_0003b953  0003b953  size=100


void FUN_0003b953(void)

{
  undefined4 uVar1;
  undefined4 local_24;
  
  uVar1 = FUN_000418ee();
  DAT_000639d4 = DAT_000639cc;
  for (local_24 = 0; local_24 < DAT_000639c8; local_24 = local_24 + 1) {
    FUN_0003b718(DAT_000639d4,uVar1);
    DAT_000639d4 = *(int *)(DAT_000639d4 + 0x11);
  }
  return;
}


