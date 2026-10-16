// func #378  FUN_00048422  00048422  size=355


void FUN_00048422(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  uint uVar1;
  undefined4 uVar2;
  char cVar3;
  char extraout_CL;
  char extraout_CL_00;
  undefined2 extraout_CX;
  byte bVar5;
  byte extraout_CH;
  uint uVar4;
  undefined4 extraout_EDX;
  uint *puVar7;
  undefined4 *puVar8;
  bool bVar9;
  bool bVar10;
  undefined8 uVar11;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  uint uStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  undefined4 uStack_c;
  byte bVar6;
  
  bVar5 = *(byte *)((int)param_1 + 9);
  uVar4 = CONCAT31((int3)((uint)param_4 >> 8),bVar5) & 0xffff00ff;
  *(byte *)((int)param_1 + 9) = *(byte *)((int)param_1 + 9) & 0x7f;
  if ((((short)param_1[2] == 0x3fff) && (param_1[1] == 0x80000000)) && (*param_1 == 0)) {
    *param_1 = 0x2168c235;
    param_1[1] = 0xc90fdaa2;
    *(ushort *)(param_1 + 2) = CONCAT11(bVar5 & 0x80 | 0x3f,0xfe);
    return;
  }
  uStack_14 = param_3;
  uStack_10 = param_2;
  uStack_c = param_4;
  if (0x3ffe < (short)param_1[2]) {
    uStack_1c = 0x4848e;
    uStack_18 = uVar4;
    uVar11 = FUN_00047b79(0,0x80000000,*param_1,param_1[1]);
    *(undefined8 *)param_1 = uVar11;
    *(undefined2 *)(param_1 + 2) = 0x3fff;
    uVar4 = CONCAT22((short)(uStack_18 >> 0x10),CONCAT11(2,(char)uStack_18));
  }
  bVar9 = (ushort)param_1[2] < 0x3ffd;
  bVar10 = false;
  if ((ushort)param_1[2] == 0x3ffd) {
    bVar9 = param_1[1] < 0x8930a2f4;
    bVar10 = false;
    if (param_1[1] == 0x8930a2f4) {
      bVar9 = *param_1 < 0xf66ab09b;
      bVar10 = *param_1 == 0xf66ab09b;
    }
  }
  if (!bVar9 && !bVar10) {
    uVar1 = param_1[2];
    uStack_1c = 0x484d5;
    uStack_18 = uVar4;
    uVar11 = FUN_0004794a(*param_1,param_1[1],0xc265539e,0xddb3d742);
    uStack_20 = (undefined4)((ulonglong)uVar11 >> 0x20);
    uStack_1c = (undefined4)uVar11;
    uVar4 = param_1[2];
    uVar11 = FUN_00047d4e(*param_1,param_1[1],0xc265539e,0xddb3d742,CONCAT22(0x3fff,(short)uVar1));
    uVar11 = FUN_0004794a((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),0,0x80000000);
    uVar2 = uStack_1c;
    uStack_1c = 0x4851b;
    uVar11 = FUN_00047b79((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),uVar2,uStack_20);
    *(undefined8 *)param_1 = uVar11;
    *(short *)(param_1 + 2) = (short)uVar4;
    uVar4 = uStack_18 | 0x100;
  }
  puVar8 = &uStack_14;
  uStack_18 = 0x4853a;
  FUN_00048f9e(param_1,&LAB_00048312,8,uVar4);
  bVar6 = (byte)((ushort)extraout_CX >> 8);
  cVar3 = (char)extraout_CX;
  bVar5 = bVar6 >> 1;
  if ((bool)(bVar6 & 1)) {
    uStack_18 = 0x3ffe;
    uStack_1c = 0x860a91c1;
    puVar8 = &uStack_20;
    uStack_20 = 0x6b9b2c23;
    FUN_0004791a(param_1,extraout_EDX,param_1);
    cVar3 = extraout_CL;
    bVar5 = extraout_CH;
  }
  if ((bool)(bVar5 & 1)) {
    *(undefined4 *)((int)puVar8 + -4) = 0xbfff;
    *(undefined4 *)((int)puVar8 + -8) = 0xc90fdaa2;
    *(undefined4 *)((int)puVar8 + -0xc) = 0x2168c235;
    *(undefined4 *)((int)puVar8 + -0x10) = 0x48572;
    puVar7 = param_1;
    FUN_0004791a(param_1);
    *(byte *)((int)puVar7 + 9) = *(byte *)((int)puVar7 + 9) ^ 0x80;
    cVar3 = extraout_CL_00;
  }
  if (cVar3 < '\0') {
    *(byte *)((int)param_1 + 9) = *(byte *)((int)param_1 + 9) ^ 0x80;
  }
  return;
}


