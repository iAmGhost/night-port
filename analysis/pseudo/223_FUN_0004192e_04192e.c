// func #223  FUN_0004192e  0004192e  size=458


uint FUN_0004192e(char *param_1,uint param_2,int param_3,int *param_4)

{
  char cVar1;
  int iVar2;
  int extraout_ECX;
  uint uVar3;
  uint uVar4;
  char *pcVar5;
  char *pcVar6;
  byte bVar7;
  undefined8 uVar8;
  char *local_18;
  uint local_14;
  uint local_10;
  
  bVar7 = 0;
  if ((*(byte *)(param_4 + 3) & 1) == 0) {
    FUN_00042a0d(4);
    *(byte *)(extraout_ECX + 0xc) = *(byte *)(extraout_ECX + 0xc) | 0x20;
    return 0;
  }
  local_14 = param_3 * param_2;
  if (local_14 == 0) {
    return 0;
  }
  if (param_4[2] == 0) {
    FUN_00042a43(param_4);
  }
  local_10 = 0;
  local_18 = param_1;
  if ((*(byte *)(param_4 + 3) & 0x40) == 0) {
    pcVar5 = param_1 + local_14;
    do {
      if (param_4[1] == 0) {
        uVar8 = FUN_00041d47(param_4);
        param_1 = (char *)((ulonglong)uVar8 >> 0x20);
        if ((int)uVar8 == 0) break;
      }
      pcVar6 = (char *)*param_4;
      param_4[1] = param_4[1] + -1;
      *param_4 = (int)(pcVar6 + 1);
      cVar1 = *pcVar6;
      if (cVar1 == '\r') {
        if (param_4[1] == 0) {
          uVar8 = FUN_00041d47(param_4);
          param_1 = (char *)((ulonglong)uVar8 >> 0x20);
          if ((int)uVar8 == 0) break;
        }
        pcVar6 = (char *)*param_4;
        param_4[1] = param_4[1] + -1;
        *param_4 = (int)(pcVar6 + 1);
        cVar1 = *pcVar6;
      }
      if (cVar1 == '\x1a') goto LAB_00041a32;
      *param_1 = cVar1;
      local_10 = local_10 + 1;
      param_1 = param_1 + 1;
    } while (param_1 != pcVar5);
  }
  else {
    do {
      while( true ) {
        uVar4 = param_4[1];
        if (uVar4 != 0) {
          if (local_14 < uVar4) {
            uVar4 = local_14;
          }
          pcVar5 = (char *)*param_4;
          pcVar6 = local_18;
          for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
            *(undefined4 *)pcVar6 = *(undefined4 *)pcVar5;
            pcVar5 = pcVar5 + (uint)bVar7 * -8 + 4;
            pcVar6 = pcVar6 + (uint)bVar7 * -8 + 4;
          }
          for (uVar3 = uVar4 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
            *pcVar6 = *pcVar5;
            pcVar5 = pcVar5 + (uint)bVar7 * -2 + 1;
            pcVar6 = pcVar6 + (uint)bVar7 * -2 + 1;
          }
          local_18 = local_18 + uVar4;
          local_10 = local_10 + uVar4;
          local_14 = local_14 - uVar4;
          *param_4 = *param_4 + uVar4;
          param_4[1] = param_4[1] - uVar4;
        }
        if (local_14 == 0) goto LAB_00041ae6;
        if ((local_14 < (uint)param_4[5]) && ((*(byte *)((int)param_4 + 0xd) & 4) == 0)) break;
        uVar4 = local_14;
        if (((*(byte *)((int)param_4 + 0xd) & 4) == 0) && (0x200 < local_14)) {
          uVar4 = (uint)((uint3)(local_14 >> 8) & 0xfffffe) << 8;
        }
        iVar2 = FUN_00042aba(param_4[4],local_18,uVar4);
        if (iVar2 == -1) {
          *(byte *)(param_4 + 3) = *(byte *)(param_4 + 3) | 0x20;
          goto LAB_00041ae6;
        }
        if (iVar2 == 0) goto LAB_00041a32;
        local_18 = local_18 + iVar2;
        local_14 = local_14 - iVar2;
        local_10 = local_10 + iVar2;
      }
      iVar2 = FUN_00041d47(param_4);
    } while (iVar2 != 0);
  }
LAB_00041ae6:
  return local_10 / param_2;
LAB_00041a32:
  *(byte *)(param_4 + 3) = *(byte *)(param_4 + 3) | 0x10;
  goto LAB_00041ae6;
}


