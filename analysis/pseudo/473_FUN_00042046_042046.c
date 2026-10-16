// func #473  FUN_00042046  00042046  size=116

/* WARNING: Instruction at (ram,0x00042124) overlaps instruction at (ram,0x00042122)
    */
/* WARNING: Removing unreachable block (ram,0x00042062) */
/* WARNING: Removing unreachable block (ram,0x0004206e) */
/* WARNING: Removing unreachable block (ram,0x00042070) */
/* WARNING: Removing unreachable block (ram,0x000420e7) */
/* WARNING: Removing unreachable block (ram,0x00042082) */
/* WARNING: Removing unreachable block (ram,0x00042084) */
/* WARNING: Removing unreachable block (ram,0x00042087) */
/* WARNING: Removing unreachable block (ram,0x00042090) */
/* WARNING: Removing unreachable block (ram,0x00042111) */
/* WARNING: Removing unreachable block (ram,0x000420ac) */
/* WARNING: Removing unreachable block (ram,0x00042124) */
/* WARNING: Removing unreachable block (ram,0x000420d9) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Restarted to delay deadcode elimination for space: stack */

undefined4 FUN_00042046(int param_1,uint *param_2,int param_3,int param_4)

{
  ushort *puVar1;
  code *pcVar2;
  short sVar3;
  undefined2 uVar4;
  int *piVar5;
  undefined4 uVar6;
  short extraout_CX;
  uint uVar7;
  int iVar8;
  short extraout_DX;
  char *pcVar9;
  char cVar10;
  short unaff_BP;
  uint unaff_ESI;
  int iVar11;
  uint *puVar12;
  uint *puVar13;
  int unaff_EDI;
  char *pcVar14;
  char *pcVar15;
  undefined4 *puVar16;
  undefined2 in_ES;
  undefined2 in_DS;
  short in_GS;
  bool bVar17;
  undefined8 uVar18;
  int *piStack_c;
  undefined1 *local_8;
  
  local_8 = &stack0xfffffffc;
  *(byte *)(param_3 + 0x30) = *(byte *)(param_3 + 0x30) & (byte)param_1;
  uVar6 = *(undefined4 *)(param_3 + 2);
  *(byte *)((int)param_2 + 0x75) = *(byte *)((int)param_2 + 0x75) & (byte)param_2;
  out(*(undefined1 *)(unaff_ESI ^ *param_2),(short)param_2);
  *(byte *)(param_3 + 0x7b) = *(byte *)(param_3 + 0x7b) & (byte)((uint)param_2 >> 8);
  piStack_c = (int *)(param_1 + -0x241aae55);
  DAT_00053384 = &piStack_c;
  DAT_00053398 = &piStack_c;
  piVar5 = (int *)((int)piStack_c + *piStack_c);
  *piVar5 = *piVar5 + (int)piVar5;
  puVar1 = (ushort *)(unaff_EDI + 0x6d);
  sVar3 = ((unaff_BP + -1) - (short)uVar6 & 3U) - (*puVar1 & 3);
  *puVar1 = *puVar1 + (ushort)(0 < sVar3) * sVar3;
  DAT_00053390._0_2_ = 0x24;
  cVar10 = 'R';
  pcVar2 = (code *)swi(0x21);
  uVar18 = (*pcVar2)(0x3000,param_2,0x50484152,param_4 + 1);
  iVar8 = (int)((ulonglong)uVar18 >> 0x20);
  DAT_000533bb = (undefined1)uVar18;
  DAT_000533bc = (undefined1)((ulonglong)uVar18 >> 8);
  iVar11 = 0;
  pcVar14 = &DAT_00000081;
  sVar3 = (short)((ulonglong)uVar18 >> 0x10);
  if (sVar3 == 0x4458) {
    piStack_c = (int *)(uint)(byte)(cVar10 - 0x30);
    DAT_00053384 = (int **)(_DAT_0000005c + 0xfffU & 0xfffff000);
    pcVar14 = &DAT_00000081;
    iVar11 = 0;
    pcVar2 = (code *)swi(0x21);
    (*pcVar2)(CONCAT11(0x4a,(char)in_DS),iVar8,_DAT_0000005c + 0xfffU >> 0xc);
    uVar4 = SUB42(local_8,0);
    DAT_000533b9 = 0x2c;
  }
  else {
    if (sVar3 != 0x4243) {
      pcVar2 = (code *)swi(0x21);
      uVar18 = (*pcVar2)(0xff00,0x78);
      if ((char)uVar18 == '\0') {
        DAT_000533b9 = 0x2c;
        uVar4 = 0;
        register0x00000010 = (BADSPACEBASE *)&stack0xfffffffc;
      }
      else {
        if (in_GS != 0) {
          DAT_00053368 = in_GS;
        }
        pcVar2 = (code *)swi(0x31);
        (*pcVar2)(6,(int)((ulonglong)uVar18 >> 0x20),in_DS);
        uVar4 = 1;
        DAT_00053390._0_2_ = in_ES;
        DAT_000533b9 = _DAT_0000002c;
        if (extraout_DX != 0 || extraout_CX != 0) {
          uVar4 = 0x101;
        }
      }
      goto LAB_000421d1;
    }
    pcVar2 = (code *)swi(0x21);
    DAT_00053364 = iVar8;
    (*pcVar2)(CONCAT11(0x4a,(char)((ulonglong)uVar18 >> 0x10)),*(int *)(iVar8 + 0x1c),
              (int)&local_8 - *(int *)(iVar8 + 0x1c));
    pcVar14 = (char *)(*(int *)(iVar8 + 0x10) + 0x81);
    iVar11 = (uint)*(ushort *)(*(int *)(iVar8 + 0x10) + 0x2c) << 4;
    uVar4 = 9;
    DAT_00053390._0_2_ = in_DS;
    DAT_000533b9 = in_DS;
  }
  register0x00000010 = (BADSPACEBASE *)&stack0xfffffffc;
LAB_000421d1:
  DAT_000533b2 = (char)uVar4;
  DAT_000533b3 = (undefined1)((ushort)uVar4 >> 8);
  DAT_00042319 = in_DS;
  _DAT_000533b5 = iVar11;
  *(int *)((int)register0x00000010 + -4) = iVar11;
  pcVar9 = &DAT_000640c0;
  bVar17 = true;
  uVar7 = (uint)(byte)pcVar14[-1];
  do {
    pcVar15 = pcVar14;
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    pcVar15 = pcVar14 + 1;
    bVar17 = *pcVar14 == ' ';
    pcVar14 = pcVar15;
  } while (bVar17);
  if (!bVar17) {
    pcVar14 = pcVar15 + -1;
    for (iVar8 = uVar7 + 1; iVar8 != 0; iVar8 = iVar8 + -1) {
      *pcVar9 = *pcVar14;
      pcVar14 = pcVar14 + 1;
      pcVar9 = pcVar9 + 1;
    }
  }
  *pcVar9 = '\0';
  pcVar9[1] = '\0';
  puVar13 = *(uint **)((int)register0x00000010 + -4);
  *(char **)((int)register0x00000010 + -4) = pcVar9 + 1;
  *(uint *)((int)register0x00000010 + -8) = CONCAT22(6,in_DS);
  _DAT_000533b0 = 0;
  do {
    if (((*puVar13 | 0x20202020) == 0x37386f6e) && ((char)puVar13[1] == '=')) {
      _DAT_000533b0 = _DAT_000533b0 + 1;
    }
    do {
      puVar12 = puVar13;
      puVar13 = (uint *)((int)puVar12 + 1);
    } while ((char)*puVar12 != '\0');
  } while (*(char *)puVar13 != '\0');
  puVar13 = puVar12 + 1;
  pcVar14 = pcVar9 + 1;
  do {
    uVar7 = *puVar13;
    DAT_00053394 = pcVar14 + 1;
    *pcVar14 = (char)*puVar13;
    puVar13 = (uint *)((int)puVar13 + 1);
    pcVar14 = DAT_00053394;
  } while ((char)uVar7 != '\0');
  DAT_0005338c = *(undefined4 *)((int)register0x00000010 + -4);
  uVar7 = 0x108a4;
  if (DAT_000533b2 == '\x01') {
    uVar7 = 0x1000;
  }
  puVar16 = &DAT_00053818;
  _DAT_00053380 = (undefined1 *)register0x00000010;
  for (uVar7 = uVar7 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *puVar16 = 0;
    puVar16 = puVar16 + 1;
  }
  for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
    *(undefined1 *)puVar16 = 0;
    puVar16 = (undefined4 *)((int)puVar16 + 1);
  }
  DAT_00053388 = &DAT_000640c0;
  *(undefined4 *)((int)register0x00000010 + -4) = 0x422cc;
  FUN_00044618(0xff);
  *(undefined4 *)((int)register0x00000010 + -4) = 0x422d3;
  uVar6 = FUN_000445c9();
  *(undefined4 *)((int)register0x00000010 + -4) = uVar6;
  *(undefined4 *)((int)register0x00000010 + -8) = 0x42308;
  FUN_00044663(0,0xff);
  pcVar2 = (code *)swi(0x21);
  uVar6 = (*pcVar2)(CONCAT22((short)((uint)*(undefined4 *)((int)register0x00000010 + -4) >> 0x10),
                             CONCAT11(0x4c,(char)*(undefined4 *)((int)register0x00000010 + -4))));
  return uVar6;
}
