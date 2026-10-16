// func #313  FUN_000440a9  000440a9  size=734


int FUN_000440a9(undefined4 *param_1,int *param_2)

{
  byte bVar1;
  int iVar2;
  undefined6 *puVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  undefined4 *extraout_ECX_01;
  undefined4 *extraout_ECX_02;
  undefined4 *extraout_ECX_03;
  undefined4 *extraout_ECX_04;
  undefined4 *extraout_ECX_05;
  undefined4 *extraout_ECX_06;
  undefined4 *extraout_ECX_07;
  undefined4 *puVar7;
  undefined4 *extraout_ECX_08;
  int *piVar8;
  char *pcVar9;
  char *pcVar10;
  int iVar11;
  char local_88;
  char local_87 [79];
  int local_38;
  int local_34;
  int *local_30;
  int local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  int local_1c;
  
  pcVar10 = &local_88;
  iVar11 = 0;
  local_1c = 0;
  local_30 = param_2;
  while( true ) {
    iVar5 = (*(code *)*param_1)(param_1);
    iVar6 = local_1c;
    if (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 2) == 0) break;
    local_1c = local_1c + 1;
    param_1 = extraout_ECX;
  }
  puVar7 = extraout_ECX;
  if ((*(byte *)(extraout_ECX + 4) & 2) != 0) goto LAB_000442c9;
  iVar2 = extraout_ECX[3];
  extraout_ECX[3] = iVar2 + -1;
  pcVar10 = &local_88;
  if (iVar2 != 0) {
    if ((iVar5 == 0x2b) || (pcVar10 = &local_88, iVar5 == 0x2d)) {
      local_88 = (char)iVar5;
      iVar5 = FUN_000445a6(extraout_ECX);
      pcVar10 = local_87;
      local_1c = iVar6 + 1;
      puVar7 = extraout_ECX_00;
      if (iVar5 == -1) goto LAB_000442c9;
    }
    if ((((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) != 0) || (iVar5 == 0x2e)) {
      local_20._0_2_ = 0;
      local_20._2_2_ = 0;
      local_2c = 0;
      if (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) != 0) {
        local_2c = 1;
        do {
          *pcVar10 = (char)iVar5;
          pcVar10 = pcVar10 + 1;
          if ((*(byte *)(puVar7 + 4) & 0x10) != 0) {
            local_20._2_2_ = (short)iVar5 + local_20._2_2_ * 10 + -0x30;
          }
          iVar11 = iVar11 + 1;
          iVar5 = FUN_000445a6(puVar7);
          puVar7 = extraout_ECX_01;
          if (iVar5 == -1) goto LAB_000442c9;
        } while (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) != 0);
      }
      pcVar9 = pcVar10;
      iVar6 = iVar11;
      if (iVar5 == 0x2e) {
        *pcVar10 = '.';
        iVar5 = FUN_000445a6(puVar7);
        pcVar10 = pcVar10 + 1;
        puVar7 = extraout_ECX_02;
        if (iVar5 == -1) goto LAB_000442c9;
        if ((local_2c == 0) && (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) == 0))
        goto LAB_000442c2;
        iVar11 = iVar11 + 1;
        do {
          if (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) == 0) break;
          iVar11 = iVar11 + 1;
          *pcVar10 = (char)iVar5;
          iVar5 = FUN_000445a6(puVar7);
          pcVar10 = pcVar10 + 1;
          puVar7 = extraout_ECX_03;
        } while (iVar5 != -1);
        if ((*(byte *)(puVar7 + 4) & 0x10) != 0) {
          local_24 = 0;
          pcVar9 = pcVar10;
          while( true ) {
            pcVar9 = pcVar9 + -1;
            local_20._0_2_ = (undefined2)local_24;
            if (*pcVar9 == '.') break;
            local_28 = 10;
            local_24 = (uint)CONCAT12(*pcVar9 + -0x30,(undefined2)local_24);
            local_24 = local_24 / 10;
          }
        }
        pcVar9 = pcVar10;
        iVar6 = iVar11;
        if (iVar5 == -1) goto LAB_000442c9;
      }
      pcVar10 = pcVar9;
      iVar11 = iVar6;
      if (((*(byte *)(puVar7 + 4) & 0x10) == 0) && ((iVar5 == 0x65 || (iVar5 == 0x45)))) {
        iVar11 = iVar6 + 1;
        *pcVar9 = (char)iVar5;
        iVar5 = FUN_000445a6(puVar7);
        pcVar10 = pcVar9 + 1;
        puVar7 = extraout_ECX_04;
        if (iVar5 == -1) goto LAB_000442c9;
        if ((iVar5 == 0x2b) || (iVar5 == 0x2d)) {
          iVar11 = iVar6 + 2;
          *pcVar10 = (char)iVar5;
          iVar5 = FUN_000445a6(extraout_ECX_04);
          pcVar10 = pcVar9 + 2;
          puVar7 = extraout_ECX_05;
          if (iVar5 == -1) goto LAB_000442c9;
        }
        if (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) == 0) {
          iVar11 = 0;
        }
        else {
          do {
            iVar11 = iVar11 + 1;
            *pcVar10 = (char)iVar5;
            iVar5 = FUN_000445a6(puVar7);
            pcVar10 = pcVar10 + 1;
            puVar7 = extraout_ECX_06;
            if (iVar5 == -1) goto LAB_000442c9;
          } while (((&DAT_0005185c)[(byte)((char)iVar5 + 1)] & 0x20) != 0);
        }
      }
    }
  }
LAB_000442c2:
  (*(code *)puVar7[1])(iVar5,puVar7);
  puVar7 = extraout_ECX_07;
LAB_000442c9:
  if ((0 < iVar11) && (iVar11 = iVar11 + local_1c, (*(byte *)(puVar7 + 4) & 1) != 0)) {
    *pcVar10 = '\0';
    iVar6 = CONCAT22(local_20._2_2_,(undefined2)local_20);
    if ((*(byte *)(puVar7 + 4) & 0x10) == 0) {
      (*(code *)PTR_FUN_00053648)(&local_88,&local_38);
      iVar6 = CONCAT22(local_20._2_2_,(undefined2)local_20);
      puVar7 = extraout_ECX_08;
    }
    else if (local_88 == '-') {
      iVar6 = -CONCAT22(local_20._2_2_,(undefined2)local_20);
    }
    if ((*(byte *)(puVar7 + 4) & 4) == 0) {
      if ((*(byte *)(puVar7 + 4) & 8) == 0) {
        puVar4 = (undefined4 *)*local_30;
        *local_30 = (int)(puVar4 + 1);
        piVar8 = (int *)*puVar4;
      }
      else {
        puVar4 = (undefined4 *)*local_30;
        *local_30 = (int)(puVar4 + 1);
        piVar8 = (int *)*puVar4;
      }
    }
    else {
      puVar3 = (undefined6 *)*local_30;
      *local_30 = (int)(puVar3 + 1);
      piVar8 = (int *)*puVar3;
    }
    bVar1 = *(byte *)(puVar7 + 4);
    if ((bVar1 & 0x10) == 0) {
      if (((bVar1 & 0x20) != 0) || ((bVar1 & 0x40) != 0)) {
        *piVar8 = local_38;
        piVar8[1] = local_34;
        return iVar11;
      }
      local_20 = iVar6;
      iVar6 = FUN_00049132(local_38,local_34);
    }
    *piVar8 = iVar6;
  }
  return iVar11;
}


