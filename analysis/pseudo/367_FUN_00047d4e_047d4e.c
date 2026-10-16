// func #367  FUN_00047d4e  00047d4e  size=249


ulonglong FUN_00047d4e(uint param_1,uint param_2,uint param_3,uint param_4)

{
  longlong lVar1;
  ulonglong uVar2;
  longlong lVar3;
  ulonglong uVar4;
  ulonglong uVar5;
  uint uVar6;
  byte bVar7;
  ushort uVar8;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint unaff_ESI;
  uint uVar14;
  bool bVar15;
  ushort uVar9;
  
  if ((param_1 == 0) && (param_2 == 0)) {
    uVar9 = (ushort)unaff_ESI;
    if ((ushort)(uVar9 * 2) == 0) {
      return 0;
    }
    unaff_ESI = CONCAT22((short)(unaff_ESI >> 0x10),
                         (short)(CONCAT12(CARRY2(uVar9,uVar9),uVar9 * 2) >> 1));
  }
  if (((param_4 == 0) && (param_3 == 0)) && ((unaff_ESI & 0x7fff0000) == 0)) {
    return 0;
  }
  uVar9 = (ushort)(((int)unaff_ESI >> 0x10 & 0x80007fffU) +
                  ((int)(unaff_ESI << 0x10 | unaff_ESI >> 0x10) >> 0x10 & 0x80007fffU));
  uVar8 = uVar9 + 0xc002;
  if ((uVar9 < 0x3ffe) || (uVar8 < 0x7fff)) {
    if ((short)uVar8 < -0x40) {
      return 0;
    }
    uVar12 = (uint)((ulonglong)param_1 * (ulonglong)param_3 >> 0x20);
    lVar1 = (ulonglong)param_4 * (ulonglong)param_1;
    lVar3 = lVar1 + (ulonglong)uVar12;
    uVar10 = (uint)lVar3;
    uVar14 = (uint)((ulonglong)lVar3 >> 0x20);
    uVar13 = (uint)((ulonglong)param_3 * (ulonglong)param_2 >> 0x20);
    uVar6 = (uint)((ulonglong)param_3 * (ulonglong)param_2);
    uVar11 = uVar10 + uVar6;
    uVar6 = (uint)CARRY4(uVar10,uVar6);
    uVar10 = uVar14 + uVar13;
    uVar2 = (ulonglong)param_2 * (ulonglong)param_4 +
            CONCAT44((uint)CARRY4((uint)((ulonglong)lVar1 >> 0x20),(uint)CARRY4(uVar12,(uint)lVar1))
                     + (uint)(CARRY4(uVar14,uVar13) || CARRY4(uVar10,uVar6)),uVar10 + uVar6);
    uVar6 = (uint)uVar2;
    if (-1 < (longlong)uVar2) {
      bVar15 = CARRY4(uVar11,uVar11);
      uVar11 = uVar11 * 2;
      uVar2 = CONCAT44((int)(uVar2 >> 0x20) * 2 +
                       (uint)(CARRY4(uVar6,uVar6) || CARRY4(uVar6 * 2,(uint)bVar15)),
                       uVar6 * 2 + (uint)bVar15);
      uVar8 = uVar9 + 0xc001;
    }
    uVar5 = uVar2 >> 0x20;
    uVar6 = (uint)uVar2;
    if (CARRY4(uVar11,uVar11)) {
      bVar15 = true;
      if (((uVar11 & 0x7fffffff) == 0) &&
         (bVar15 = (int)((ulonglong)param_1 * (ulonglong)param_3) != 0, !bVar15)) {
        bVar15 = (uVar2 & 1) != 0;
      }
      uVar10 = (uint)bVar15;
      uVar4 = uVar2 + uVar10;
      uVar2 = uVar2 + uVar10;
      if (CARRY4((uint)uVar5,(uint)CARRY4(uVar6,uVar10))) {
        uVar2 = CONCAT44((int)(CONCAT14(1,(int)(uVar4 >> 0x20)) >> 1),
                         (int)(CONCAT14((uVar4 & 0x100000000) != 0,(int)uVar4) >> 1));
        uVar8 = uVar8 + 1;
        if (uVar8 == 0x7fff) goto LAB_00047e3d;
      }
    }
    uVar6 = (uint)(uVar2 >> 0x20);
    if ((short)uVar8 < 1) {
      if (uVar8 == 0) {
        bVar7 = 1;
      }
      else {
        bVar7 = -(char)uVar8;
      }
      uVar2 = CONCAT44(uVar6 >> (bVar7 & 0x1f) | 0 << 0x20 - (bVar7 & 0x1f),
                       (uint)uVar2 >> (bVar7 & 0x1f) | uVar6 << 0x20 - (bVar7 & 0x1f));
    }
  }
  else {
LAB_00047e3d:
    uVar2 = 0x8000000000000000;
  }
  return uVar2;
}


