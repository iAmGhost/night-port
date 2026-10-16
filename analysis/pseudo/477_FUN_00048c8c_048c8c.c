// func #389  FUN_00048c8c  00048c8c  size=468


void FUN_00048c8c(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  undefined2 uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  ushort uVar8;
  short sVar9;
  uint *puVar10;
  undefined4 *puVar11;
  undefined4 *puVar12;
  undefined4 uVar13;
  undefined4 *puVar14;
  bool bVar15;
  bool bVar16;
  undefined8 uVar17;
  undefined4 uStack_24;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  undefined4 uStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  
  puVar12 = &uStack_18;
  puVar11 = &uStack_18;
  uVar13 = 0;
  *(byte *)((int)param_1 + 9) = *(byte *)((int)param_1 + 9) & 0x7f;
  bVar16 = false;
  if (((short)param_1[2] == 0) && (bVar16 = false, param_1[1] == 0)) {
    bVar16 = *param_1 == 0;
  }
  uStack_18 = param_3;
  uStack_14 = param_4;
  uStack_10 = param_2;
  if (bVar16) {
    iVar6 = 0;
  }
  else {
    if ((short)param_1[2] < 0) {
      uVar13 = 0xffffffff;
      uStack_1c = 0xbfff;
      uStack_20 = 0x80000000;
      puVar11 = &uStack_24;
      uStack_24 = 0;
      puVar10 = param_1;
      FUN_0004791a(param_1);
      *(byte *)((int)puVar10 + 9) = *(byte *)((int)puVar10 + 9) ^ 0x80;
    }
    iVar6 = 0;
    uVar7 = param_1[1];
    uVar5 = *param_1;
    for (uVar8 = (short)param_1[2] + 4; 0x3ffe < uVar8; uVar8 = uVar8 - 1) {
      bVar16 = CARRY4(uVar5,uVar5);
      uVar5 = uVar5 * 2;
      bVar15 = CARRY4(uVar7,uVar7);
      uVar3 = uVar7 * 2;
      uVar7 = uVar3 + bVar16;
      iVar6 = iVar6 * 2 + (uint)(bVar15 || CARRY4(uVar3,(uint)bVar16));
    }
    if (uVar7 == 0 && uVar5 == 0) {
      sVar9 = 0;
    }
    else {
      for (; -1 < (int)uVar7; uVar7 = uVar7 * 2 + (uint)bVar16) {
        bVar16 = CARRY4(uVar5,uVar5);
        uVar5 = uVar5 * 2;
        uVar8 = uVar8 - 1;
      }
      sVar9 = uVar8 - 4;
    }
    *param_1 = uVar5;
    param_1[1] = uVar7;
    *(short *)(param_1 + 2) = sVar9;
    puVar12 = puVar11;
  }
  *(undefined4 *)((int)puVar12 + -4) = uVar13;
  *(int *)((int)puVar12 + -8) = iVar6;
  uVar7 = *param_1;
  uVar5 = param_1[1];
  uVar3 = param_1[2];
  uVar4 = param_1[2];
  *(undefined4 *)((int)puVar12 + -0xc) = 0x48d33;
  uVar17 = FUN_00047d4e(uVar7,uVar5,uVar7,uVar5);
  uVar13 = (undefined4)((ulonglong)uVar17 >> 0x20);
  *(int *)((int)puVar12 + -0xc) = (int)uVar17;
  *(undefined4 *)((int)puVar12 + -0x10) = uVar13;
  *(uint *)((int)puVar12 + -0x14) = CONCAT22((short)uVar3,(ushort)uVar4);
  *(undefined4 *)((int)puVar12 + -0x18) = 0x48d51;
  uVar17 = FUN_0004794a((int)uVar17,uVar13,0x1bf21f8c,0xa6829a79);
  LOCK();
  uVar13 = *(undefined4 *)((int)puVar12 + -0xc);
  *(undefined4 *)((int)puVar12 + -0xc) = (int)uVar17;
  UNLOCK();
  LOCK();
  uVar1 = *(undefined4 *)((int)puVar12 + -0x10);
  *(undefined4 *)((int)puVar12 + -0x10) = (int)((ulonglong)uVar17 >> 0x20);
  UNLOCK();
  LOCK();
  uVar7 = *(uint *)((int)puVar12 + -0x14);
  *(uint *)((int)puVar12 + -0x14) = (ushort)uVar4 | 0x40030000;
  UNLOCK();
  *(undefined4 *)((int)puVar12 + -0x18) = 0x48d77;
  uVar17 = FUN_00047d4e(uVar13,uVar1,0x9d7bfdb,0xec96f0d6);
  *(undefined4 *)((int)puVar12 + -0x18) = 0x48d92;
  uVar17 = FUN_0004794a((int)uVar17,(int)((ulonglong)uVar17 >> 0x20),0xe536e187,0xe6d5051a);
  uVar5 = *param_1;
  uVar3 = param_1[1];
  uVar4 = param_1[2];
  *(undefined4 *)((int)puVar12 + -0x18) = 0x48da6;
  uVar17 = FUN_00047d4e((int)uVar17,(int)((ulonglong)uVar17 >> 0x20),uVar5,uVar3);
  uVar13 = (undefined4)((ulonglong)uVar17 >> 0x20);
  *(int *)((int)puVar12 + -0x18) = (int)uVar17;
  *(undefined4 *)((int)puVar12 + -0x1c) = uVar13;
  *(uint *)((int)puVar12 + -0x20) = (uint)(ushort)uVar4 << 0x10 | uVar7 & 0xffff;
  uVar5 = CONCAT22((short)uVar7,*(undefined2 *)((int)puVar12 + -0x14));
  *(undefined4 *)((int)puVar12 + -0x24) = 0x48dc4;
  uVar17 = FUN_0004794a((int)uVar17,uVar13,*(undefined4 *)((int)puVar12 + -0xc),
                        *(undefined4 *)((int)puVar12 + -0x10));
  LOCK();
  uVar13 = *(undefined4 *)((int)puVar12 + -0x18);
  *(undefined4 *)((int)puVar12 + -0x18) = (int)uVar17;
  UNLOCK();
  LOCK();
  uVar1 = *(undefined4 *)((int)puVar12 + -0x1c);
  *(undefined4 *)((int)puVar12 + -0x1c) = (int)((ulonglong)uVar17 >> 0x20);
  UNLOCK();
  LOCK();
  uVar7 = *(uint *)((int)puVar12 + -0x20);
  *(uint *)((int)puVar12 + -0x20) = uVar5 << 0x10 | (uVar5 ^ 0x80000000) >> 0x10;
  UNLOCK();
  *(undefined4 *)((int)puVar12 + -0x24) = 0x48de2;
  uVar17 = FUN_0004794a(uVar13,uVar1,*(undefined4 *)((int)puVar12 + -0xc),
                        *(undefined4 *)((int)puVar12 + -0x10));
  uVar13 = *(undefined4 *)((int)puVar12 + -0x18);
  *(undefined4 *)((int)puVar12 + -0x18) = 0x48df4;
  uVar17 = FUN_00047b79((int)uVar17,(int)((ulonglong)uVar17 >> 0x20),uVar13,
                        *(undefined4 *)((int)puVar12 + -0x1c));
  uVar13 = (undefined4)((ulonglong)uVar17 >> 0x20);
  *(undefined8 *)param_1 = uVar17;
  *(short *)(param_1 + 2) = (short)uVar7;
  uVar7 = *(uint *)((int)puVar12 + -8);
  puVar14 = &DAT_00048c64;
  while (uVar7 != 0) {
    uVar5 = uVar7 & 1;
    uVar7 = uVar7 >> 1;
    if (uVar5 != 0) {
      *(uint *)((int)puVar12 + -8) = uVar7;
      *(undefined4 **)((int)puVar12 + -0xc) = puVar14;
      uVar7 = param_1[1];
      uVar5 = *param_1;
      uVar2 = *(undefined2 *)(puVar14 + 2);
      uVar13 = puVar14[1];
      uVar1 = *puVar14;
      *(undefined4 *)((int)puVar12 + -0x10) = 0x48e2f;
      uVar17 = FUN_00047d4e(uVar1,uVar13,uVar5,uVar7);
      uVar13 = (undefined4)((ulonglong)uVar17 >> 0x20);
      *(undefined8 *)param_1 = uVar17;
      *(undefined2 *)(param_1 + 2) = uVar2;
      puVar14 = *(undefined4 **)((int)puVar12 + -0xc);
      uVar7 = *(uint *)((int)puVar12 + -8);
    }
    puVar14 = (undefined4 *)((int)puVar14 + 10);
  }
  *(short *)(param_1 + 2) = (short)param_1[2] + (short)*(undefined4 *)((int)puVar12 + -4);
  *(undefined4 *)((int)puVar12 + -4) = 0xbfff;
  *(undefined4 *)((int)puVar12 + -8) = 0x80000000;
  *(undefined4 *)((int)puVar12 + -0xc) = 0;
  *(undefined4 *)((int)puVar12 + -0x10) = 0x48e59;
  FUN_0004791a(param_1,uVar13,param_1);
  return;
}


