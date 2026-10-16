// func #446  FUN_00035941  00035941  size=127

undefined4 FUN_00035941(void)

{
  int iVar1;
  undefined4 local_28;
  
  DAT_000530ac = FUN_000418ee();
  iVar1 = FUN_000357f2(0xff);
  if ((iVar1 != 0) && (iVar1 = FUN_000357f2(0xff), iVar1 != 0)) {
    return 0xffffffff;
  }
  iVar1 = FUN_000357f2(0x3f);
  if (iVar1 == 0) {
    FUN_0004238c(DAT_000530ac + 1);
    local_28 = 0;
  }
  else {
    local_28 = 0xffffffff;
  }
  return local_28;
}
