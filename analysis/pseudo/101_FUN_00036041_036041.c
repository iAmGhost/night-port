// func #101  FUN_00036041  00036041  size=335


void FUN_00036041(void)

{
  undefined4 uVar1;
  int iVar2;
  char extraout_DL;
  undefined1 local_30;
  uint local_2c;
  uint local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = 0;
  FUN_000367c9(uVar1);
  FUN_00036853(uVar1,0xfffffd00,2);
  FUN_0004192e(&DAT_000636b8,3,0x100,DAT_000639b8);
  for (local_2c = 0; local_2c < 0x300; local_2c = local_2c + 1) {
    (&DAT_000636b8)[local_2c] = (byte)(&DAT_000636b8)[local_2c] >> 2;
  }
  FUN_00036853(uVar1,0x80,0);
  while (local_24 < 64000) {
    local_2c = FUN_00041c8f(DAT_000639b8);
    if ((local_2c & 0xc0) == 0xc0) {
      local_2c = local_2c & 0x3f;
      iVar2 = FUN_00041c8f(DAT_000639b8);
      while (local_2c = local_2c - 1, local_2c != 0xffffffff) {
        if (iVar2 != 0xff) {
          local_30 = (undefined1)iVar2;
          (&DAT_000a0000)[local_24] = local_30;
        }
        local_24 = local_24 + 1;
      }
    }
    else if ((local_2c & 0xff) == 0xff) {
      local_24 = local_24 + 1;
    }
    else {
      local_30 = (undefined1)local_2c;
      (&DAT_000a0000)[local_24] = local_30;
      local_24 = local_24 + 1;
    }
  }
  if (extraout_DL == '\x01') {
    FUN_00040042();
  }
  return;
}


