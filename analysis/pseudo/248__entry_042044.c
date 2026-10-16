// func #248  _entry  00042044  size=2  [thunk]


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 _entry(void)

{
  code *pcVar1;
  short sVar2;
  undefined2 uVar3;
  undefined4 uVar4;
  short extraout_CX;
  uint uVar5;
  int iVar6;
  undefined *puVar7;
  short extraout_DX;
  char *pcVar8;
  undefined1 *puVar9;
  int iVar10;
  uint *puVar11;
  uint *puVar12;
  char *pcVar13;
  char *pcVar14;
  undefined4 *puVar15;
  undefined2 in_ES;
  undefined2 in_DS;
  short in_GS;
  bool bVar16;
  undefined8 uVar17;
  undefined4 in_stack_00000004;
  
  DAT_00053390._0_2_ = 0x24;
  uVar3 = 0x5048;
  pcVar1 = (code *)swi(0x21);
  DAT_00053384 = (undefined1 *)register0x00000010;
  DAT_00053398 = (undefined1 *)register0x00000010;
  uVar17 = (*pcVar1)(0x3000);
  iVar6 = (int)((ulonglong)uVar17 >> 0x20);
  DAT_000533bb = (undefined1)uVar17;
  DAT_000533bc = (undefined1)((ulonglong)uVar17 >> 8);
  iVar10 = 0;
  pcVar13 = &DAT_00000081;
  sVar2 = (short)((ulonglong)uVar17 >> 0x10);
  if (sVar2 == 0x4458) {
    DAT_00053384 = (undefined1 *)(_DAT_0000005c + 0xfffU & 0xfffff000);
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)(CONCAT11(0x4a,(char)in_DS),iVar6,_DAT_0000005c + 0xfffU >> 0xc);
    uVar3 = (undefined2)in_stack_00000004;
    DAT_000533b9 = 0x2c;
  }
  else {
    if (sVar2 != 0x4243) {
      pcVar1 = (code *)swi(0x21);
      uVar17 = (*pcVar1)(0xff00,0x78);
      if ((char)uVar17 == '\0') {
        DAT_000533b9 = 0x2c;
        uVar3 = 0;
        puVar9 = &stack0x00000008;
      }
      else {
        if (in_GS != 0) {
          DAT_00053368 = in_GS;
        }
        pcVar1 = (code *)swi(0x31);
        (*pcVar1)(6,(int)((ulonglong)uVar17 >> 0x20),CONCAT22(uVar3,in_DS));
        uVar3 = 1;
        puVar9 = &stack0x0000000c;
        DAT_00053390._0_2_ = in_ES;
        DAT_000533b9 = _DAT_0000002c;
        if (extraout_DX != 0 || extraout_CX != 0) {
          uVar3 = 0x101;
          puVar9 = &stack0x0000000c;
        }
      }
      goto LAB_000421d1;
    }
    pcVar1 = (code *)swi(0x21);
    DAT_00053364 = iVar6;
    (*pcVar1)(CONCAT11(0x4a,(char)((ulonglong)uVar17 >> 0x10)),*(int *)(iVar6 + 0x1c),
              (int)&stack0x00000004 - *(int *)(iVar6 + 0x1c));
    pcVar13 = (char *)(*(int *)(iVar6 + 0x10) + 0x81);
    iVar10 = (uint)*(ushort *)(*(int *)(iVar6 + 0x10) + 0x2c) << 4;
    uVar3 = 9;
    DAT_00053390._0_2_ = in_DS;
    DAT_000533b9 = in_DS;
  }
  puVar9 = &stack0x00000008;
LAB_000421d1:
  DAT_000533b2 = (char)uVar3;
  DAT_000533b3 = (undefined1)((ushort)uVar3 >> 8);
  *(int *)(puVar9 + -4) = iVar10;
  pcVar8 = &DAT_000640c0;
  bVar16 = true;
  uVar5 = (uint)(byte)pcVar13[-1];
  do {
    pcVar14 = pcVar13;
    if (uVar5 == 0) break;
    uVar5 = uVar5 - 1;
    pcVar14 = pcVar13 + 1;
    bVar16 = *pcVar13 == ' ';
    pcVar13 = pcVar14;
  } while (bVar16);
  DAT_00042319 = in_DS;
  _DAT_000533b5 = iVar10;
  if (!bVar16) {
    pcVar13 = pcVar14 + -1;
    for (iVar6 = uVar5 + 1; iVar6 != 0; iVar6 = iVar6 + -1) {
      *pcVar8 = *pcVar13;
      pcVar13 = pcVar13 + 1;
      pcVar8 = pcVar8 + 1;
    }
  }
  *pcVar8 = '\0';
  pcVar8[1] = '\0';
  puVar12 = *(uint **)(puVar9 + -4);
  *(char **)(puVar9 + -4) = pcVar8 + 1;
  *(uint *)(puVar9 + -8) = CONCAT22(6,in_DS);
  _DAT_000533b0 = 0;
  do {
    if (((*puVar12 | 0x20202020) == 0x37386f6e) && ((char)puVar12[1] == '=')) {
      _DAT_000533b0 = _DAT_000533b0 + 1;
    }
    do {
      puVar11 = puVar12;
      puVar12 = (uint *)((int)puVar11 + 1);
    } while ((char)*puVar11 != '\0');
  } while (*(char *)puVar12 != '\0');
  puVar12 = puVar11 + 1;
  pcVar13 = pcVar8 + 1;
  do {
    uVar5 = *puVar12;
    DAT_00053394 = pcVar13 + 1;
    *pcVar13 = (char)*puVar12;
    puVar12 = (uint *)((int)puVar12 + 1);
    pcVar13 = DAT_00053394;
  } while ((char)uVar5 != '\0');
  _DAT_0005338c = *(undefined4 *)(puVar9 + -4);
  puVar7 = &UNK_000108a4;
  if (DAT_000533b2 == '\x01') {
    puVar7 = (undefined *)0x1000;
  }
  puVar15 = &DAT_00053818;
  _DAT_00053380 = puVar9;
  for (uVar5 = (uint)puVar7 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *puVar15 = 0;
    puVar15 = puVar15 + 1;
  }
  for (iVar6 = 0; iVar6 != 0; iVar6 = iVar6 + -1) {
    *(undefined1 *)puVar15 = 0;
    puVar15 = (undefined4 *)((int)puVar15 + 1);
  }
  _DAT_00053388 = &DAT_000640c0;
  *(undefined4 *)(puVar9 + -4) = 0x422cc;
  FUN_00044618(0xff);
  *(undefined4 *)(puVar9 + -4) = 0x422d3;
  uVar4 = FUN_000445c9();
  *(undefined4 *)(puVar9 + -4) = uVar4;
  *(undefined4 *)(puVar9 + -8) = 0x42308;
  FUN_00044663(0,0xff);
  pcVar1 = (code *)swi(0x21);
  uVar4 = (*pcVar1)(CONCAT22((short)((uint)*(undefined4 *)(puVar9 + -4) >> 0x10),
                             CONCAT11(0x4c,(char)*(undefined4 *)(puVar9 + -4))));
  return uVar4;
}


