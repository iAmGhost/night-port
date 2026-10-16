// func #362  FUN_0004794a  0004794a  size=364


undefined8 FUN_0004794a(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  byte bVar3;
  short sVar4;
  ushort uVar5;
  short sVar6;
  byte bVar9;
  uint uVar7;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  ushort uVar13;
  uint unaff_ESI;
  uint uVar14;
  bool bVar15;
  bool bVar16;
  int iVar8;
  
  if ((param_1 == 0) && (param_2 == 0)) {
    uVar5 = (ushort)unaff_ESI;
    if ((ushort)(uVar5 * 2) == 0) {
      return CONCAT44(param_4,param_3);
    }
    unaff_ESI = CONCAT22((short)(unaff_ESI >> 0x10),
                         (short)(CONCAT12(CARRY2(uVar5,uVar5),uVar5 * 2) >> 1));
  }
  if (((param_4 == 0) && (param_3 == 0)) && ((unaff_ESI & 0x7fff0000) == 0)) {
    return CONCAT44(param_2,param_1);
  }
  uVar7 = (int)unaff_ESI >> 0x10 & 0x80007fff;
  sVar4 = (short)(uVar7 >> 0x10) + ((short)unaff_ESI >> 0xf & 0x8000U);
  uVar14 = (int)(unaff_ESI << 0x10 | unaff_ESI >> 0x10) >> 0x10 & 0x7fff;
  uVar5 = (ushort)(unaff_ESI >> 0x10) & 0x7fff;
  uVar13 = (ushort)uVar14;
  sVar6 = uVar5 - uVar13;
  iVar8 = CONCAT22(sVar4,sVar6);
  uVar10 = param_1;
  uVar11 = param_2;
  if (sVar6 != 0) {
    if (uVar5 < uVar13) {
      iVar8 = CONCAT22(sVar4,-sVar6);
      uVar10 = param_3;
      uVar11 = param_4;
      param_3 = param_1;
      uVar7 = uVar14;
      param_4 = param_2;
    }
    if (0x40 < (ushort)iVar8) {
      return CONCAT44(param_4,param_3);
    }
  }
  uVar5 = (ushort)iVar8 & 0xff;
  if (iVar8 < 0) {
    uVar5 = CONCAT11(0xff,(char)iVar8);
    bVar15 = param_3 != 0;
    param_3 = -param_3;
    param_4 = -(uint)bVar15 - param_4;
  }
  uVar13 = (ushort)uVar7;
  uVar14 = 0;
  bVar3 = (byte)uVar5;
  if (bVar3 != 0) {
    if (0x1f < bVar3) {
      uVar14 = (uint)(uVar10 != 0);
      uVar10 = uVar11;
      if (bVar3 == 0x40) {
        uVar14 = uVar14 | uVar11;
        uVar10 = 0;
      }
      uVar11 = 0;
    }
    uVar14 = uVar14 | 0U >> (bVar3 & 0x1f) | uVar10 << 0x20 - (bVar3 & 0x1f);
    uVar10 = uVar10 >> (bVar3 & 0x1f) | uVar11 << 0x20 - (bVar3 & 0x1f);
    uVar11 = uVar11 >> (bVar3 & 0x1f) | 0 << 0x20 - (bVar3 & 0x1f);
  }
  uVar2 = uVar10 + param_3;
  uVar12 = uVar11 + param_4 + (uint)CARRY4(uVar10,param_3);
  bVar9 = (char)(uVar5 >> 8) +
          (CARRY4(uVar11,param_4) || CARRY4(uVar11 + param_4,(uint)CARRY4(uVar10,param_3)));
  if ((char)bVar9 < '\0') {
    if (bVar3 == 0x40) {
      uVar10 = (uint)((uVar14 & 0x7fffffff) != 0);
      bVar15 = CARRY4(uVar2,uVar10);
      uVar2 = uVar2 + uVar10;
      uVar12 = uVar12 + bVar15;
    }
    bVar15 = uVar2 != 0;
    uVar2 = -uVar2;
    uVar12 = -(uint)bVar15 - uVar12;
    bVar9 = 0;
  }
  if ((CONCAT31((int3)(uVar2 >> 8),(byte)uVar2 | bVar9) == 0 && uVar12 == 0) ||
     (uVar7 = uVar7 & 0xffff, uVar13 == 0)) goto LAB_00047a9f;
  uVar10 = uVar2;
  if (bVar9 == 0) {
    bVar15 = (int)uVar14 < 0;
    uVar14 = uVar14 & 0x7fffffff | (uint)bVar15 << 0x1f;
    do {
      uVar13 = (short)uVar7 - 1;
      uVar7 = (uint)uVar13;
      if (uVar13 == 0) goto LAB_00047a9f;
      uVar10 = uVar2 * 2 + (uint)bVar15;
      uVar11 = (uint)(CARRY4(uVar2,uVar2) || CARRY4(uVar2 * 2,(uint)bVar15));
      bVar16 = CARRY4(uVar12,uVar12);
      uVar1 = uVar12 * 2;
      bVar15 = bVar16 || CARRY4(uVar1,uVar11);
      uVar12 = uVar1 + uVar11;
      uVar2 = uVar10;
    } while (!bVar16 && !CARRY4(uVar1,uVar11));
  }
  if (uVar13 != 0x7ffe) {
    uVar11 = (uint)(((ulonglong)uVar12 | 0x100000000) >> 1);
    uVar2 = (uint)(CONCAT14((uVar12 & 1) != 0,uVar10) >> 1);
    uVar12 = uVar11;
    if ((uVar10 & 1) == 0) goto LAB_00047a9f;
    bVar15 = CARRY4(uVar14,uVar14);
    if ((uVar14 & 0x7fffffff) == 0) {
      bVar15 = (uVar10 & 2) != 0;
      uVar2 = uVar2 & 0xfffffffe | (uint)bVar15;
    }
    bVar16 = CARRY4(uVar2,(uint)bVar15);
    uVar2 = uVar2 + bVar15;
    uVar14 = (uint)bVar16;
    uVar10 = uVar11 + uVar14;
    uVar12 = uVar10;
    if (!CARRY4(uVar11,uVar14)) goto LAB_00047a9f;
    uVar12 = (uint)(CONCAT14(1,uVar10) >> 1);
    uVar2 = (uint)(CONCAT14((uVar10 & 1) != 0,uVar2) >> 1);
    if (uVar13 != 0x7ffd) goto LAB_00047a9f;
  }
  uVar2 = 0;
  uVar12 = 0x80000000;
LAB_00047a9f:
  return CONCAT44(uVar12,uVar2);
}


