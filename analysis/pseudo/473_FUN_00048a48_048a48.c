// func #385  FUN_00048a48  00048a48  size=503


/* WARNING: Removing unreachable block (ram,0x00048ac9) */

void FUN_00048a48(longlong *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  ushort uVar1;
  undefined4 uVar2;
  longlong *plVar3;
  char cVar4;
  uint uVar5;
  short sVar6;
  uint *puVar7;
  undefined1 *puVar8;
  ushort uVar9;
  uint unaff_ESI;
  uint uVar10;
  longlong lVar11;
  undefined8 uVar12;
  longlong lVar13;
  undefined4 uStack_30;
  longlong *local_2c;
  uint uVar14;
  uint uStack_24;
  undefined8 local_20;
  ushort local_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  undefined4 uStack_c;
  
  uStack_24 = (uint)(ushort)((*(ushort *)(param_1 + 1) & 0x7fff) + 0xc002);
  lVar13 = param_1[1];
  uVar10 = CONCAT22((short)((uint)param_1 >> 0x10),0x3ffe);
  uStack_30 = 0x48a89;
  local_2c = param_1;
  uStack_14 = param_3;
  uStack_10 = param_4;
  uStack_c = param_2;
  lVar11 = FUN_000485eb((int)*param_1,*(undefined4 *)((int)param_1 + 4),0x2168c235,0xc90fdaa2);
  plVar3 = local_2c;
  *local_2c = lVar11;
  *(short *)(local_2c + 1) = (short)lVar13;
  puVar7 = &uStack_24;
  uVar5 = uVar10;
  uVar14 = unaff_ESI;
  if ((*(byte *)((int)local_2c + 9) & 0x80) != 0) {
    local_2c = (longlong *)0x3ffe;
    uStack_30 = 0xc90fdaa2;
    FUN_0004791a(plVar3,(int)((ulonglong)lVar11 >> 0x20),plVar3);
    puVar7 = &uStack_30;
    uVar5 = 0x2168c234;
    uVar14 = uVar10;
  }
  *(uint *)((int)puVar7 + -4) =
       CONCAT31((int3)(CONCAT22((short)(uVar5 >> 0x10),CONCAT11((byte)unaff_ESI,(char)uVar5)) >> 8),
                (char)uVar5 + ((byte)unaff_ESI & 2)) & 0xffffff07;
  sVar6 = (short)plVar3[1] + -0x3ffe;
  cVar4 = (char)uVar14;
  if (sVar6 < -0x20) {
    local_20 = -0x8000000000000000;
    local_18 = 0x3fff;
    puVar8 = (undefined1 *)((int)puVar7 + -4);
  }
  else {
    if (-1 < sVar6) {
      sVar6 = 0;
    }
    uVar5 = (uint)(ushort)-sVar6;
    if (8 < -sVar6) {
      uVar5 = 8;
    }
    *(uint *)((int)puVar7 + -8) = (uint)(ushort)(&DAT_00048a36)[uVar5];
    uVar5 = CONCAT31((int3)(uVar14 >> 8),cVar4 + 1U) & 0xffffff02;
    if (((cVar4 + 1U & 2) != 0) || ((char)(uVar5 >> 8) == '\x01')) {
      *(uint *)((int)puVar7 + -0xc) = uVar5;
      lVar13 = *plVar3;
      uVar2 = *(undefined4 *)((int)plVar3 + 4);
      uVar9 = *(ushort *)(plVar3 + 1);
      *(undefined4 *)((int)puVar7 + -0x10) = 0x48b59;
      uVar12 = FUN_00047d4e((int)lVar13,uVar2,(int)lVar13,uVar2);
      *(undefined4 *)((int)puVar7 + -0x10) = 0x48b72;
      local_20 = FUN_00048f4c((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),8 - (int)local_2c,
                              &DAT_000489dc + (int)local_2c * 10);
      uVar5 = *(uint *)((int)puVar7 + -0xc);
      local_18 = uVar9;
    }
    puVar8 = (undefined1 *)((int)puVar7 + -8);
    if (((char)uVar5 == '\0') || ((char)(uVar5 >> 8) == '\x01')) {
      *(undefined4 *)((int)puVar7 + -0xc) = 0x48ba6;
      FUN_00048f9e(plVar3,&DAT_00048982 + (int)local_2c * 10,8 - (int)local_2c);
      puVar8 = (undefined1 *)((int)puVar7 + -8);
    }
  }
  uVar9 = local_18;
  lVar13 = local_20;
  if ((cVar4 + 1U & 2) == 0) {
    uVar9 = *(ushort *)(plVar3 + 1);
    lVar13 = *plVar3;
  }
  if ((uVar14 & 4) != 0) {
    uVar9 = uVar9 ^ 0x8000;
  }
  if ((char)(uVar14 >> 8) == '\x01') {
    uVar1 = local_18;
    lVar13 = local_20;
    if ((cVar4 + 3U & 2) == 0) {
      uVar1 = *(ushort *)(plVar3 + 1);
      lVar13 = *plVar3;
    }
    if ((lVar13 == 0) && ((uVar1 & 0x7fff) == 0)) {
      lVar13 = -0x8000000000000000;
      uVar9 = uVar9 | 0x7fff;
    }
    else {
      *(undefined4 *)(puVar8 + -4) = 0x48c2e;
      lVar13 = FUN_00047b79();
    }
  }
  *plVar3 = lVar13;
  *(ushort *)(plVar3 + 1) = uVar9;
  return;
}


