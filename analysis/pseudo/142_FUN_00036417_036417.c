// func #104  FUN_00036417  00036417  size=288


void FUN_00036417(undefined4 param_1,undefined4 param_2,char param_3)

{
  undefined1 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined8 uVar4;
  undefined1 local_2c;
  uint local_28;
  int local_24;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  uVar2 = (undefined4)uVar4;
  local_24 = 0;
  FUN_000367c9(uVar2);
  FUN_00036853(uVar2,0xfffffd00,2);
  FUN_0004192e(&DAT_000636b8,3,0x100,DAT_000639b8);
  for (local_28 = 0; (int)local_28 < 0x300; local_28 = local_28 + 1) {
    (&DAT_000636b8)[local_28] = (byte)(&DAT_000636b8)[local_28] >> 2;
  }
  FUN_00036853(uVar2,0x80,0);
  while (local_24 < 0x4b000) {
    local_28 = FUN_00041c8f(DAT_000639b8);
    if ((local_28 & 0xc0) == 0xc0) {
      local_28 = local_28 & 0x3f;
      uVar1 = FUN_00041c8f(DAT_000639b8);
      while (local_28 = local_28 - 1, local_28 != 0xffffffff) {
        *(undefined1 *)(local_24 + iVar3) = uVar1;
        local_24 = local_24 + 1;
      }
    }
    else {
      local_2c = (undefined1)local_28;
      *(undefined1 *)(local_24 + iVar3) = local_2c;
      local_24 = local_24 + 1;
    }
  }
  if (param_3 == '\x01') {
    FUN_00040042();
  }
  return;
}


