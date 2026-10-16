// func #102  FUN_00036190  00036190  size=254


void FUN_00036190(void)

{
  undefined1 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined8 uVar4;
  uint local_2c;
  undefined1 local_28;
  uint local_24;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  uVar2 = (undefined4)uVar4;
  local_2c = 0;
  FUN_000367c9(uVar2);
  FUN_00036853(uVar2,0xfffffd00,2);
  FUN_0004192e(&DAT_000633b8,3,0x100,DAT_000639b8);
  for (local_24 = 0; local_24 < 0x300; local_24 = local_24 + 1) {
    (&DAT_000633b8)[local_24] = (byte)(&DAT_000633b8)[local_24] >> 2;
  }
  FUN_00036853(uVar2,0x80,0);
  while (local_2c < 64000) {
    local_24 = FUN_00041c8f(DAT_000639b8);
    if ((local_24 & 0xc0) == 0xc0) {
      local_24 = local_24 & 0x3f;
      uVar1 = FUN_00041c8f(DAT_000639b8);
      while (local_24 = local_24 - 1, local_24 != 0xffffffff) {
        *(undefined1 *)(local_2c + iVar3) = uVar1;
        local_2c = local_2c + 1;
      }
    }
    else {
      local_28 = (undefined1)local_24;
      *(undefined1 *)(local_2c + iVar3) = local_28;
      local_2c = local_2c + 1;
    }
  }
  return;
}


