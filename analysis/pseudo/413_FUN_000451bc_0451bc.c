// func #340  FUN_000451bc  000451bc  size=1097


ulonglong FUN_000451bc(ushort *param_1,int *param_2,int param_3,ushort *param_4)

{
  byte bVar1;
  uint *puVar2;
  int *piVar3;
  short *psVar4;
  undefined4 *puVar5;
  byte *pbVar6;
  bool bVar7;
  uint uVar8;
  int iVar9;
  ushort *extraout_ECX;
  ushort *puVar10;
  ushort *extraout_ECX_00;
  ushort *extraout_ECX_01;
  ushort *extraout_ECX_02;
  ushort *extraout_ECX_03;
  ushort *extraout_ECX_04;
  ushort *extraout_ECX_05;
  ushort *extraout_ECX_06;
  ushort *extraout_ECX_07;
  undefined4 extraout_ECX_08;
  undefined4 uVar11;
  uint unaff_EBP;
  ushort uVar12;
  ushort in_DS;
  ushort uVar13;
  bool bVar14;
  undefined8 uVar15;
  
  bVar7 = true;
  bVar1 = *(byte *)(param_3 + 0x15);
  *(undefined4 *)(param_3 + 0xc) = 0;
  if (bVar1 < 0x69) {
    if (0x57 < bVar1) {
      if (bVar1 < 0x59) goto LAB_00045209;
      if (bVar1 == 100) goto LAB_0004523d;
    }
  }
  else {
    if (0x69 < bVar1) {
      if (bVar1 < 0x75) {
        bVar14 = bVar1 == 0x6f;
      }
      else {
        if (bVar1 < 0x76) goto LAB_00045209;
        bVar14 = bVar1 == 0x78;
      }
      if (!bVar14) goto LAB_00045247;
LAB_00045209:
      if ((*(byte *)(param_3 + 0x14) & 0x20) == 0) {
        if ((*(byte *)(param_3 + 0x14) & 0x10) == 0) {
          puVar2 = (uint *)*param_2;
          *param_2 = (int)(puVar2 + 1);
          unaff_EBP = *puVar2;
        }
        else {
          puVar10 = (ushort *)*param_2;
          *param_2 = (int)(puVar10 + 2);
          unaff_EBP = (uint)*puVar10;
        }
      }
      else {
        puVar2 = (uint *)*param_2;
        *param_2 = (int)(puVar2 + 1);
        unaff_EBP = *puVar2;
      }
      *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) & 0xf9;
    }
LAB_0004523d:
    if (*(int *)(param_3 + 8) != -1) {
      *(undefined1 *)(param_3 + 0x16) = 0x20;
    }
  }
LAB_00045247:
  bVar1 = *(byte *)(param_3 + 0x15);
  uVar11 = 10;
  uVar12 = in_DS;
  uVar13 = in_DS;
  if (bVar1 < 0x65) {
    if (bVar1 < 0x50) {
      if (bVar1 < 0x46) {
        bVar14 = bVar1 == 0x45;
      }
      else {
        if (bVar1 < 0x47) goto LAB_000452f3;
        bVar14 = bVar1 == 0x47;
      }
      if (!bVar14) goto LAB_00045540;
      goto LAB_0004531d;
    }
    if (0x50 < bVar1) {
      if (bVar1 < 0x58) {
        if (bVar1 != 0x53) goto LAB_00045540;
LAB_000453ac:
        param_1 = param_4;
        if ((*(byte *)(param_3 + 0x14) & 0x80) == 0) {
          if ((*(byte *)(param_3 + 0x14) & 0x40) == 0) {
            puVar5 = (undefined4 *)*param_2;
            *param_2 = (int)(puVar5 + 1);
            puVar10 = (ushort *)*puVar5;
          }
          else {
            puVar5 = (undefined4 *)*param_2;
            *param_2 = (int)(puVar5 + 1);
            puVar10 = (ushort *)*puVar5;
          }
          if (puVar10 != (ushort *)0x0) {
            param_1 = puVar10;
          }
        }
        else {
          puVar5 = (undefined4 *)*param_2;
          *param_2 = (int)(puVar5 + 2);
          if (((ushort *)*puVar5 != (ushort *)0x0) || (*(ushort *)(puVar5 + 1) != 0)) {
            param_1 = (ushort *)*puVar5;
            uVar12 = *(ushort *)(puVar5 + 1);
          }
        }
        bVar1 = *(byte *)(param_3 + 0x14);
        bVar7 = false;
        *(byte *)(param_3 + 0x14) = bVar1 & 0xf9;
        if (*(char *)(param_3 + 0x15) == 'S') {
          if ((bVar1 & 0x20) == 0) {
            uVar8 = (uint)(byte)*param_1;
            param_1 = (ushort *)((int)param_1 + 1);
          }
          else {
            uVar8 = (uint)*param_1;
            param_1 = param_1 + 1;
          }
        }
        else {
          if ((bVar1 & 0x20) == 0) {
            uVar8 = (uint)uVar12;
            uVar11 = *(undefined4 *)(param_3 + 8);
            goto LAB_00045362;
          }
          uVar8 = FUN_00045056(param_1,uVar12,*(undefined4 *)(param_3 + 8));
          param_1 = extraout_ECX_03;
        }
      }
      else {
        if (bVar1 < 0x59) {
LAB_00045458:
          if (((*(byte *)(param_3 + 0x14) & 1) != 0) && (unaff_EBP != 0)) {
            *(undefined1 *)(param_3 + 0x17) = 0x30;
            *(undefined1 *)(param_3 + 0x19) = 0;
            *(undefined1 *)(param_3 + 0x18) = *(undefined1 *)(param_3 + 0x15);
          }
          uVar11 = 0x10;
LAB_00045475:
          FUN_00049459(unaff_EBP,param_1,uVar11);
          puVar10 = extraout_ECX_04;
          if (*(char *)(param_3 + 0x15) == 'X') {
            FUN_00045605(param_1);
            puVar10 = extraout_ECX_05;
          }
          goto LAB_0004534a;
        }
        if (bVar1 < 99) goto LAB_00045540;
        if (99 < bVar1) goto LAB_0004536c;
        pbVar6 = (byte *)*param_2;
        *param_2 = (int)(pbVar6 + 4);
        bVar1 = *pbVar6;
        *(byte *)((int)param_1 + 1) = 0;
        *(byte *)param_1 = bVar1;
        *(undefined4 *)(param_3 + 8) = 1;
        uVar8 = 1;
        bVar7 = false;
        *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) & 0xf9;
      }
      goto LAB_0004556a;
    }
LAB_00045494:
    if (*(int *)(param_3 + 4) == 0) {
      if ((*(byte *)(param_3 + 0x14) & 0x80) == 0) {
        *(undefined4 *)(param_3 + 4) = 8;
      }
      else {
        *(undefined4 *)(param_3 + 4) = 0xd;
      }
    }
    *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) & 0xf9;
    puVar5 = (undefined4 *)*param_2;
    *param_2 = (int)(puVar5 + 1);
    uVar11 = *puVar5;
    puVar10 = param_1;
    if ((*(byte *)(param_3 + 0x14) & 0x80) != 0) {
      *param_2 = (int)(puVar5 + 2);
      FUN_00045079(puVar5[1] & 0xffff,param_1,4);
      *(byte *)(param_1 + 2) = 0x3a;
      puVar10 = (ushort *)((int)param_1 + 5);
    }
    FUN_00045079(uVar11,puVar10,8);
    puVar10 = extraout_ECX_06;
    if (*(char *)(param_3 + 0x15) == 'P') {
      FUN_00045605(param_1);
      puVar10 = extraout_ECX_07;
    }
LAB_00045359:
    param_1 = puVar10;
    uVar11 = 0xffffffff;
    uVar8 = (uint)in_DS;
    uVar12 = in_DS;
    in_DS = uVar13;
LAB_00045362:
    uVar8 = FUN_00045032(param_1,uVar8,uVar11);
    param_1 = extraout_ECX_00;
  }
  else {
    if (bVar1 < 0x66) {
LAB_0004531d:
      uVar15 = FUN_000451b5(param_1,param_2,param_3);
      uVar8 = (uint)((ulonglong)uVar15 >> 0x20);
      param_1 = (ushort *)uVar15;
      uVar11 = 0xffffffff;
      uVar12 = (ushort)((ulonglong)uVar15 >> 0x20);
      goto LAB_00045362;
    }
    if (bVar1 < 0x6f) {
      if (bVar1 < 0x67) {
LAB_000452f3:
        if ((*(byte *)(param_3 + 0x14) & 0x10) != 0) {
          puVar5 = (undefined4 *)*param_2;
          *param_2 = (int)(puVar5 + 1);
          FUN_000450da(param_1,*puVar5,param_3);
          uVar11 = 0xffffffff;
          uVar8 = (uint)uVar12;
          in_DS = uVar12;
          goto LAB_00045362;
        }
      }
      else if (0x67 < bVar1) {
        if (bVar1 == 0x69) {
LAB_0004536c:
          if ((*(byte *)(param_3 + 0x14) & 0x20) == 0) {
            if ((*(byte *)(param_3 + 0x14) & 0x10) == 0) {
              puVar5 = (undefined4 *)*param_2;
              *param_2 = (int)(puVar5 + 1);
              FUN_00041f42(*puVar5,param_1);
              puVar10 = extraout_ECX_02;
              goto LAB_0004534a;
            }
            psVar4 = (short *)*param_2;
            *param_2 = (int)(psVar4 + 2);
            iVar9 = (int)*psVar4;
          }
          else {
            piVar3 = (int *)*param_2;
            *param_2 = (int)(piVar3 + 1);
            iVar9 = *piVar3;
          }
          FUN_000494a2(iVar9,param_1);
          puVar10 = extraout_ECX_01;
          goto LAB_0004534a;
        }
        goto LAB_00045540;
      }
      goto LAB_0004531d;
    }
    if (bVar1 < 0x70) {
      if ((*(byte *)(param_3 + 0x14) & 1) != 0) {
        *(byte *)param_1 = 0x30;
        param_1 = (ushort *)((int)param_1 + 1);
      }
      FUN_000494a2(unaff_EBP,param_1,8);
      puVar10 = extraout_ECX;
LAB_0004534a:
      if ((*(int *)(param_3 + 8) == 0) && ((byte)*param_1 == 0x30)) {
        *(byte *)puVar10 = 0;
      }
      goto LAB_00045359;
    }
    if (bVar1 < 0x73) {
      if (bVar1 == 0x70) goto LAB_00045494;
    }
    else {
      if (bVar1 < 0x74) goto LAB_000453ac;
      if (0x74 < bVar1) {
        if (bVar1 < 0x76) goto LAB_00045475;
        if (bVar1 == 0x78) goto LAB_00045458;
      }
    }
LAB_00045540:
    *(undefined4 *)(param_3 + 4) = 0;
    bVar1 = *(byte *)(param_3 + 0x15);
    *(byte *)((int)param_1 + 1) = 0;
    *(byte *)param_1 = bVar1;
    *(undefined4 *)(param_3 + 8) = 1;
    bVar7 = false;
    *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) & 0xf9;
    uVar8 = 1;
  }
LAB_0004556a:
  if (!bVar7) goto LAB_000455b0;
  if ((byte)*param_1 == 0x2d) {
    *(undefined1 *)(param_3 + 0x18) = 0;
    uVar8 = uVar8 - 1;
    *(undefined1 *)(param_3 + 0x17) = 0x2d;
  }
  else {
    if ((*(byte *)(param_3 + 0x14) & 2) == 0) {
      if ((*(byte *)(param_3 + 0x14) & 4) == 0) goto LAB_0004559f;
      *(undefined1 *)(param_3 + 0x17) = 0x2b;
    }
    else {
      *(undefined1 *)(param_3 + 0x17) = 0x20;
    }
    *(undefined1 *)(param_3 + 0x18) = 0;
  }
LAB_0004559f:
  if (*(int *)(param_3 + 8) < (int)uVar8) {
    *(uint *)(param_3 + 8) = uVar8;
  }
  else {
    *(uint *)(param_3 + 0xc) = *(int *)(param_3 + 8) - uVar8;
  }
LAB_000455b0:
  if (*(char *)(param_3 + 0x16) == '*') {
    *(undefined1 *)(param_3 + 0x17) = 0;
    *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) & 0xf9;
  }
  if (((*(int *)(param_3 + 8) == -1) || ((int)uVar8 < *(int *)(param_3 + 8))) &&
     (*(char *)(param_3 + 0x15) != 'c')) {
    *(uint *)(param_3 + 8) = uVar8;
  }
  iVar9 = FUN_00045032(param_3 + 0x17,in_DS,0xffffffff);
  *(int *)(param_3 + 4) =
       *(int *)(param_3 + 4) - (iVar9 + *(int *)(param_3 + 8) + *(int *)(param_3 + 0xc));
  return (ulonglong)CONCAT24(uVar12,extraout_ECX_08);
}


