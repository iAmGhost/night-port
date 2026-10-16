// func #374  FUN_00048053  00048053  size=199


uint FUN_00048053(uint *param_1)

{
  bool bVar1;
  uint uVar2;
  uint uVar3;
  byte bVar4;
  ushort uVar5;
  ushort uVar6;
  ushort uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  bool bVar11;
  
  uVar5 = (ushort)param_1[2];
  uVar8 = param_1[1];
  uVar2 = *param_1;
  uVar10 = 0xfffff800;
  if ((int)(uVar2 << 0x15) < 0) {
    if ((uVar2 & 0x3ff) == 0) {
      uVar10 = 0xfffff000;
    }
    bVar11 = 0xfffff7ff < uVar2;
    uVar2 = uVar2 + 0x800;
    bVar1 = CARRY4(uVar8,(uint)bVar11);
    uVar8 = uVar8 + bVar11;
    if (bVar1) {
      uVar8 = 0x80000000;
      uVar5 = uVar5 + 1;
    }
  }
  uVar2 = uVar2 & uVar10;
  uVar6 = uVar5 & 0x7fff;
  uVar7 = uVar6 + 0xc400;
  if (uVar7 < 0x7ff) {
    if (uVar6 == 0x3c00) {
      uVar2 = uVar2 >> 0xc | uVar8 << 0x14;
    }
    else {
      uVar2 = uVar2 >> 0xb | uVar8 << 0x15;
    }
  }
  else if (uVar7 < 0xc400) {
    uVar2 = uVar2 >> 0xb | uVar8 << 0x15;
    if (uVar7 != 0x43ff) {
      FUN_000463dc(CONCAT22((short)(uVar2 >> 0x10),0x8408),
                   (uint)(CONCAT14(CARRY2(uVar5,uVar5),(uVar8 & 0x7fffffff) >> 10) >> 1) |
                   0x7ff00000,uVar5 * 2);
    }
  }
  else if ((short)uVar7 < -0x34) {
    uVar2 = 0;
  }
  else {
    bVar4 = -((char)uVar6 + -0xc);
    uVar3 = uVar2;
    uVar9 = uVar8;
    if (0x1f < bVar4) {
      bVar4 = bVar4 - 0x20;
      uVar9 = 0;
      uVar3 = uVar8;
      uVar10 = uVar2;
    }
    uVar8 = uVar10 >> (bVar4 & 0x1f) | uVar3 << 0x20 - (bVar4 & 0x1f);
    uVar2 = (uVar3 >> (bVar4 & 0x1f) | uVar9 << 0x20 - (bVar4 & 0x1f)) + (uint)CARRY4(uVar8,uVar8);
  }
  return uVar2;
}


