// func #444  FUN_000357f2  000357f2  size=212

undefined8 FUN_000357f2(void)

{
  undefined1 uVar1;
  uint uVar2;
  int iVar3;
  undefined4 extraout_EDX;
  undefined4 local_28;
  undefined4 local_24;
  
  uVar1 = FUN_000418ee();
  for (local_24 = 0; local_24 < 0x2000; local_24 = local_24 + 1) {
    uVar2 = FUN_0004238c(DAT_000530ac + 1);
    if ((uVar2 & 0x40) == 0) break;
  }
  if (local_24 < 0x2000) {
    FUN_0004231d(DAT_000530ac + 1,uVar1);
    for (local_24 = 0; local_24 < 0x2000; local_24 = local_24 + 1) {
      uVar2 = FUN_0004238c(DAT_000530ac + 1);
      if ((uVar2 & 0x80) == 0) break;
    }
    if (local_24 < 30000) {
      iVar3 = FUN_0004238c(DAT_000530ac);
      if (iVar3 == 0xfe) {
        local_28 = 0;
      }
      else {
        local_28 = 0xfffffffe;
      }
    }
    else {
      local_28 = 0xffffffff;
    }
  }
  else {
    local_28 = 0xffffffff;
  }
  return CONCAT44(extraout_EDX,local_28);
}
