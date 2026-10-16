// func #440  FUN_000353d0  000353d0  size=538

void FUN_000353d0(void)

{
  undefined1 uVar1;
  byte *pbVar2;
  undefined1 *puVar3;
  byte bVar4;
  byte bVar5;
  int iVar6;
  int iVar7;
  int *extraout_EDX;
  int local_40;
  
  bVar4 = FUN_000418ee();
  if (bVar4 < 0xf0) {
    *(byte *)(extraout_EDX + 1) = bVar4;
  }
  if (bVar4 == 0xff) {
    pbVar2 = (byte *)*extraout_EDX;
    *extraout_EDX = *extraout_EDX + 1;
    bVar4 = *pbVar2;
    iVar6 = FUN_000351dc(extraout_EDX);
    if (0x2e < bVar4) {
      if (bVar4 < 0x30) {
        *(undefined1 *)((int)extraout_EDX + 5) = 0x2f;
        *(undefined1 *)(extraout_EDX + 1) = *(undefined1 *)((int)extraout_EDX + 5);
      }
      else if (bVar4 == 0x51) {
        iVar7 = FUN_0003510b(*extraout_EDX);
        DAT_00063308 = iVar7 >> 8;
        FUN_0003565a(DAT_00063308);
      }
    }
    *extraout_EDX = *extraout_EDX + iVar6;
    return;
  }
  bVar5 = bVar4 & 0xf0;
  if (bVar5 < 0xb0) {
    if (bVar5 < 0x90) {
      if (bVar5 != 0x80) {
        return;
      }
    }
    else if ((0x90 < bVar5) && (bVar5 != 0xa0)) {
      return;
    }
  }
  else if (0xb0 < bVar5) {
    if (bVar5 < 0xd0) {
      if (bVar5 != 0xc0) {
        return;
      }
    }
    else if (0xd0 < bVar5) {
      if (bVar5 < 0xe0) {
        return;
      }
      if (0xe0 < bVar5) {
        if (bVar5 != 0xf0) {
          return;
        }
        iVar6 = FUN_000351dc(extraout_EDX);
        FUN_00035c2d(bVar4);
        for (local_40 = 0; local_40 < iVar6; local_40 = local_40 + 1) {
          puVar3 = (undefined1 *)*extraout_EDX;
          *extraout_EDX = *extraout_EDX + 1;
          FUN_00035c2d(*puVar3);
        }
        return;
      }
      goto LAB_000354a4;
    }
    puVar3 = (undefined1 *)*extraout_EDX;
    *extraout_EDX = *extraout_EDX + 1;
    FUN_00035bf4(bVar4,*puVar3);
    return;
  }
LAB_000354a4:
  puVar3 = (undefined1 *)*extraout_EDX;
  *extraout_EDX = *extraout_EDX + 1;
  uVar1 = *puVar3;
  puVar3 = (undefined1 *)*extraout_EDX;
  *extraout_EDX = *extraout_EDX + 1;
  FUN_00035b0c(bVar4,uVar1,*puVar3);
  return;
}
