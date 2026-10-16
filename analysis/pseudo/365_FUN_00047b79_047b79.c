// func #365  FUN_00047b79  00047b79  size=427


longlong FUN_00047b79(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  longlong lVar4;
  int iVar5;
  uint uVar6;
  ushort uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint unaff_ESI;
  undefined2 uVar13;
  uint uVar11;
  uint uVar12;
  byte bVar14;
  ushort uVar15;
  ushort uVar16;
  bool bVar17;
  bool bVar18;
  bool bVar19;
  undefined4 local_20;
  undefined4 local_1c;
  
  uVar13 = (undefined2)(unaff_ESI >> 0x10);
  uVar7 = (ushort)unaff_ESI;
  if (((param_4 == 0) && (param_3 == 0)) && ((unaff_ESI & 0x7fff0000) == 0)) {
    if (((param_1 == 0) && (param_2 == 0)) && ((unaff_ESI & 0x7fff) == 0)) {
      FUN_000463dc(CONCAT22(uVar13,0x8101));
      uVar10 = 0xc0000000;
    }
    else {
      FUN_000463dc(0x8304);
      uVar10 = 0x80000000;
    }
    return (ulonglong)uVar10 << 0x20;
  }
  if ((param_1 == 0) && (param_2 == 0)) {
    if ((ushort)(uVar7 * 2) == 0) {
      return 0;
    }
    unaff_ESI = CONCAT22(uVar13,(short)(CONCAT12(CARRY2(uVar7,uVar7),uVar7 * 2) >> 1));
  }
  uVar7 = (ushort)(unaff_ESI >> 0x10) & 0x7fff;
  uVar10 = (int)(unaff_ESI << 0x10 | unaff_ESI >> 0x10) >> 0x10 & 0x7fff;
  uVar15 = (ushort)uVar10;
  if (uVar15 == 0) {
    do {
      bVar17 = CARRY4(param_1,param_1);
      param_1 = param_1 * 2;
      param_2 = param_2 * 2 + (uint)bVar17;
      uVar15 = (short)uVar10 - 1;
      uVar10 = (uint)uVar15;
    } while (-1 < (int)param_2);
  }
  if ((unaff_ESI & 0x7fff0000) == 0) {
    do {
      bVar17 = CARRY4(param_3,param_3);
      param_3 = param_3 * 2;
      param_4 = param_4 * 2 + (uint)bVar17;
      uVar7 = uVar7 - 1;
    } while (-1 < (int)param_4);
  }
  uVar16 = (uVar15 - uVar7) + 0x3fff;
  if (((short)uVar16 < 0) || (uVar16 < 0x7fff)) {
    if ((short)uVar16 < -0x40) {
      local_20 = 0;
      uVar10 = 0;
    }
    else {
      uVar10 = param_2;
      if (param_4 <= param_2) {
        uVar10 = param_2 - param_4;
      }
      bVar17 = param_4 <= param_2;
      uVar2 = CONCAT44(uVar10,param_1) / (ulonglong)param_4;
      local_1c = (uint)uVar2;
      uVar3 = (ulonglong)param_3 * (uVar2 & 0xffffffff);
      iVar5 = (int)uVar3;
      lVar4 = (ulonglong)param_4 * (uVar2 & 0xffffffff) + (uVar3 >> 0x20);
      if (bVar17) {
        lVar4 = lVar4 + CONCAT44(param_4,param_3);
      }
      uVar8 = -iVar5;
      uVar10 = (uint)(iVar5 != 0);
      uVar1 = param_1 - (uint)lVar4;
      uVar11 = uVar1 - uVar10;
      for (iVar5 = (param_2 - (int)((ulonglong)lVar4 >> 0x20)) -
                   (uint)(param_1 < (uint)lVar4 || uVar1 < uVar10); iVar5 != 0;
          iVar5 = iVar5 + (uint)(bVar19 || CARRY4(uVar10,(uint)bVar18))) {
        bVar18 = local_1c == 0;
        local_1c = local_1c - 1;
        bVar17 = (bool)(bVar17 ^ bVar18);
        bVar18 = CARRY4(uVar8,param_3);
        uVar8 = uVar8 + param_3;
        bVar19 = CARRY4(uVar11,param_4);
        uVar10 = uVar11 + param_4;
        uVar11 = uVar10 + bVar18;
      }
      if (param_4 <= uVar11) {
        uVar11 = uVar11 - param_4;
        bVar18 = 0xfffffffe < local_1c;
        local_1c = local_1c + 1;
        bVar17 = (bool)(bVar17 ^ bVar18);
      }
      uVar2 = CONCAT44(uVar11,uVar8) / (ulonglong)param_4;
      local_20 = (uint)uVar2;
      if (local_20 != 0) {
        uVar3 = (ulonglong)param_3 * (uVar2 & 0xffffffff);
        iVar5 = (int)uVar3;
        lVar4 = (ulonglong)param_4 * (uVar2 & 0xffffffff) + (uVar3 >> 0x20);
        uVar6 = (uint)lVar4;
        uVar9 = -iVar5;
        uVar10 = (uint)(iVar5 != 0);
        uVar1 = uVar8 - uVar6;
        uVar12 = uVar1 - uVar10;
        for (iVar5 = (uVar11 - (int)((ulonglong)lVar4 >> 0x20)) -
                     (uint)(uVar8 < uVar6 || uVar1 < uVar10); iVar5 != 0;
            iVar5 = iVar5 + (uint)(bVar19 || CARRY4(uVar10,(uint)bVar18))) {
          bVar19 = local_20 == 0;
          local_20 = local_20 - 1;
          bVar18 = local_1c < bVar19;
          local_1c = local_1c - bVar19;
          bVar17 = (bool)(bVar17 ^ bVar18);
          bVar18 = CARRY4(uVar9,param_3);
          uVar9 = uVar9 + param_3;
          bVar19 = CARRY4(uVar12,param_4);
          uVar10 = uVar12 + param_4;
          uVar12 = uVar10 + bVar18;
        }
      }
      uVar10 = local_1c;
      uVar7 = (uVar15 - uVar7) + 0x3ffe;
      if (bVar17) {
        uVar10 = (uint)(CONCAT14(1,local_1c) >> 1);
        local_20 = (uint)(CONCAT14((local_1c & 1) != 0,local_20) >> 1);
        uVar7 = uVar16;
      }
      if ((short)uVar7 < 1) {
        if (uVar7 == 0) {
          bVar14 = 1;
        }
        else {
          bVar14 = -(char)uVar7;
        }
        local_20 = local_20 >> (bVar14 & 0x1f) | uVar10 << 0x20 - (bVar14 & 0x1f);
        uVar10 = uVar10 >> (bVar14 & 0x1f) | 0 << 0x20 - (bVar14 & 0x1f);
      }
    }
  }
  else {
    uVar10 = 0x80000000;
    local_20 = 0;
  }
  return CONCAT44(uVar10,local_20);
}


