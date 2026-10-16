// func #171  FUN_0003ee0a  0003ee0a  size=124


void FUN_0003ee0a(void)

{
  int iVar1;
  
  FUN_000418ee();
  DAT_00063e20 = DAT_00063e18;
  iVar1 = DAT_00063e20;
  while (DAT_00063e20 = iVar1, DAT_00063e20 != 0) {
    iVar1 = *(int *)(DAT_00063e20 + 0x51);
    if (DAT_00063e20 != 0) {
      FUN_0003e14c(DAT_00063e20,0);
      FUN_00041c28();
    }
  }
  DAT_00062a8c = 0;
  return;
}


