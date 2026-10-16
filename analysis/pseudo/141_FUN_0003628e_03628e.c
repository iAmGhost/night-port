// func #103  FUN_0003628e  0003628e  size=393


void FUN_0003628e(void)

{
  int iVar1;
  int iVar2;
  undefined1 uVar3;
  undefined4 uVar4;
  char extraout_DL;
  int extraout_EDX;
  int extraout_EDX_00;
  int iVar5;
  int extraout_EDX_01;
  int extraout_EDX_02;
  undefined1 local_38;
  uint local_34;
  int local_2c;
  int local_28;
  int local_24;
  
  uVar4 = FUN_000418ee();
  local_24 = 0;
  local_28 = 0;
  local_2c = 0;
  FUN_000367c9(uVar4);
  FUN_00036853(uVar4,0xfffffd00,2);
  FUN_0004192e(&DAT_000636b8,3,0x100,DAT_000639b8);
  for (local_34 = 0; (int)local_34 < 0x300; local_34 = local_34 + 1) {
    (&DAT_000636b8)[local_34] = (byte)(&DAT_000636b8)[local_34] >> 2;
  }
  if (extraout_DL == '\x01') {
    FUN_00040042();
  }
  FUN_00036853(uVar4,0x80,0);
  FUN_0003f3be(0);
  iVar5 = extraout_EDX;
  while (local_2c < 0x4b000) {
    local_34 = FUN_00041c8f(DAT_000639b8,iVar5);
    if ((local_34 & 0xc0) == 0xc0) {
      local_34 = local_34 & 0x3f;
      uVar3 = FUN_00041c8f(DAT_000639b8);
      iVar5 = extraout_EDX_00;
      while (local_34 = local_34 - 1, local_34 != 0xffffffff) {
        iVar1 = local_28 + 1;
        (&DAT_000a0000)[local_28] = uVar3;
        iVar2 = local_2c + 1;
        iVar5 = local_2c;
        local_2c = iVar2;
        local_28 = iVar1;
        if (0xffff < iVar1) {
          local_28 = 0;
          local_24 = local_24 + 1;
          FUN_0003f3be(local_24);
          iVar5 = extraout_EDX_01;
        }
      }
    }
    else {
      iVar1 = local_28 + 1;
      local_38 = (undefined1)local_34;
      (&DAT_000a0000)[local_28] = local_38;
      iVar2 = local_2c + 1;
      iVar5 = local_2c;
      local_2c = iVar2;
      local_28 = iVar1;
      if (0xffff < iVar1) {
        local_28 = 0;
        local_24 = local_24 + 1;
        FUN_0003f3be(local_24);
        iVar5 = extraout_EDX_02;
      }
    }
  }
  return;
}


