// func #222  FUN_00041901  00041901  size=45


uint FUN_00041901(undefined1 *param_1,undefined4 param_2,int param_3)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  int *extraout_ECX;
  int extraout_ECX_00;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  char *pcVar7;
  ushort in_SS;
  byte bVar8;
  undefined8 uVar9;
  char *pcStack_18;
  uint uStack_14;
  uint uStack_10;
  
  bVar8 = 0;
  if ((param_1 < &stack0x00000000) &&
     (uVar2 = -((int)param_1 - (int)&stack0x00000000),
     DAT_00053394 <= uVar2 && -DAT_00053394 != (int)param_1 - (int)&stack0x00000000)) {
    return uVar2;
  }
  if (in_SS != DAT_0005333c) {
    return (uint)in_SS;
  }
  uVar9 = FUN_000429ea("Stack Overflow!\r\n",1);
  uVar2 = (uint)((ulonglong)uVar9 >> 0x20);
  pcStack_18 = (char *)uVar9;
  if ((*(byte *)(extraout_ECX + 3) & 1) == 0) {
    FUN_00042a0d(4);
    *(byte *)(extraout_ECX_00 + 0xc) = *(byte *)(extraout_ECX_00 + 0xc) | 0x20;
    return 0;
  }
  uStack_14 = param_3 * uVar2;
  if (uStack_14 == 0) {
    return 0;
  }
  if (extraout_ECX[2] == 0) {
    FUN_00042a43(extraout_ECX);
  }
  uStack_10 = 0;
  if ((*(byte *)(extraout_ECX + 3) & 0x40) == 0) {
    pcVar6 = pcStack_18 + uStack_14;
    do {
      if (extraout_ECX[1] == 0) {
        uVar9 = FUN_00041d47(extraout_ECX);
        pcStack_18 = (char *)((ulonglong)uVar9 >> 0x20);
        if ((int)uVar9 == 0) break;
      }
      pcVar7 = (char *)*extraout_ECX;
      extraout_ECX[1] = extraout_ECX[1] + -1;
      *extraout_ECX = (int)(pcVar7 + 1);
      cVar1 = *pcVar7;
      if (cVar1 == '\r') {
        if (extraout_ECX[1] == 0) {
          uVar9 = FUN_00041d47(extraout_ECX);
          pcStack_18 = (char *)((ulonglong)uVar9 >> 0x20);
          if ((int)uVar9 == 0) break;
        }
        pcVar7 = (char *)*extraout_ECX;
        extraout_ECX[1] = extraout_ECX[1] + -1;
        *extraout_ECX = (int)(pcVar7 + 1);
        cVar1 = *pcVar7;
      }
      if (cVar1 == '\x1a') goto LAB_00041a32;
      *pcStack_18 = cVar1;
      uStack_10 = uStack_10 + 1;
      pcStack_18 = pcStack_18 + 1;
    } while (pcStack_18 != pcVar6);
  }
  else {
    do {
      while( true ) {
        uVar5 = extraout_ECX[1];
        if (uVar5 != 0) {
          if (uStack_14 < uVar5) {
            uVar5 = uStack_14;
          }
          pcVar6 = (char *)*extraout_ECX;
          pcVar7 = pcStack_18;
          for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
            *(undefined4 *)pcVar7 = *(undefined4 *)pcVar6;
            pcVar6 = pcVar6 + (uint)bVar8 * -8 + 4;
            pcVar7 = pcVar7 + (uint)bVar8 * -8 + 4;
          }
          for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *pcVar7 = *pcVar6;
            pcVar6 = pcVar6 + (uint)bVar8 * -2 + 1;
            pcVar7 = pcVar7 + (uint)bVar8 * -2 + 1;
          }
          pcStack_18 = pcStack_18 + uVar5;
          uStack_10 = uStack_10 + uVar5;
          uStack_14 = uStack_14 - uVar5;
          *extraout_ECX = *extraout_ECX + uVar5;
          extraout_ECX[1] = extraout_ECX[1] - uVar5;
        }
        if (uStack_14 == 0) goto LAB_00041ae6;
        if ((uStack_14 < (uint)extraout_ECX[5]) && ((*(byte *)((int)extraout_ECX + 0xd) & 4) == 0))
        break;
        uVar5 = uStack_14;
        if (((*(byte *)((int)extraout_ECX + 0xd) & 4) == 0) && (0x200 < uStack_14)) {
          uVar5 = (uint)((uint3)(uStack_14 >> 8) & 0xfffffe) << 8;
        }
        iVar3 = FUN_00042aba(extraout_ECX[4],pcStack_18,uVar5);
        if (iVar3 == -1) {
          *(byte *)(extraout_ECX + 3) = *(byte *)(extraout_ECX + 3) | 0x20;
          goto LAB_00041ae6;
        }
        if (iVar3 == 0) goto LAB_00041a32;
        pcStack_18 = pcStack_18 + iVar3;
        uStack_14 = uStack_14 - iVar3;
        uStack_10 = uStack_10 + iVar3;
      }
      iVar3 = FUN_00041d47(extraout_ECX);
    } while (iVar3 != 0);
  }
LAB_00041ae6:
  return uStack_10 / uVar2;
LAB_00041a32:
  *(byte *)(extraout_ECX + 3) = *(byte *)(extraout_ECX + 3) | 0x10;
  goto LAB_00041ae6;
}


