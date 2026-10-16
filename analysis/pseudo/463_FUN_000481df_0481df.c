// func #376  FUN_000481df  000481df  size=307


void FUN_000481df(uint *param_1,ushort param_2)

{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  ushort uVar5;
  uint uVar6;
  bool bVar7;
  
  if ((param_2 == 0x400 || param_2 == 0x800) && ((short)(ushort)param_1[2] < 0)) {
    param_2 = param_2 ^ 0xc00;
  }
  if (param_2 == 0xc00) {
    param_2 = 0x400;
  }
  uVar5 = (ushort)param_1[2] & 0x7fff;
  if (0x403e < uVar5) {
    return;
  }
  uVar4 = *param_1;
  if (-0x21 < (short)(uVar5 + 0xbfc1)) {
    uVar3 = *(uint *)(&DAT_0004815f + (uint)(ushort)(uVar5 + 0xbfe1) * 4);
    uVar4 = uVar4 & uVar3;
    *param_1 = *param_1 ^ uVar4;
    uVar3 = uVar3 + 1;
    if (param_2 != 0x400) {
      if (param_2 == 0) {
        uVar4 = uVar4 * 2;
        if (uVar4 < uVar3) {
          return;
        }
        if (uVar3 == uVar4) {
          uVar6 = (uint)((int)uVar3 < 0);
          if ((int)uVar3 < 0) {
            uVar2 = param_1[1];
          }
          else {
            uVar2 = *param_1;
          }
          if ((uVar2 & (uVar3 * 2 | uVar6)) == 0) {
            return;
          }
          uVar3 = uVar3 & 0x7fffffff | (uint)(uVar6 != 0) << 0x1f;
        }
      }
      if (uVar4 != 0) {
        uVar4 = *param_1;
        *param_1 = *param_1 + uVar3;
        uVar6 = param_1[1];
        param_1[1] = uVar6 + CARRY4(uVar4,uVar3);
        if (CARRY4(uVar6,(uint)CARRY4(uVar4,uVar3))) {
          param_1[1] = (uint)(CONCAT14(1,param_1[1]) >> 1);
          *(short *)(param_1 + 2) = (short)param_1[2] + 1;
        }
      }
    }
    return;
  }
  *param_1 = 0;
  if ((short)(uVar5 + 0xbfe1) < -0x20) {
    if ((param_2 != 0x400) &&
       ((param_2 != 0 || ((uVar5 == 0x3ffe && ((param_1[1] != 0x80000000 || (uVar4 != 0)))))))) {
      param_1[1] = 0x80000000;
      *(ushort *)(param_1 + 2) = (ushort)param_1[2] | 0x3fff;
      return;
    }
    param_1[1] = 0;
    *(undefined2 *)(param_1 + 2) = 0;
    return;
  }
  uVar3 = *(uint *)(&DAT_0004815f + (uint)(ushort)(uVar5 + 0xc001) * 4);
  uVar6 = param_1[1] & uVar3;
  param_1[1] = param_1[1] ^ uVar6;
  uVar3 = uVar3 + 1;
  if (param_2 == 0x400) {
    return;
  }
  if (param_2 == 0) {
    bVar7 = (int)uVar4 < 0;
    uVar4 = uVar4 << 1 | (uint)bVar7;
    uVar6 = uVar6 * 2 + (uint)bVar7;
    if (uVar6 < uVar3) {
      return;
    }
    if (uVar3 == uVar6) {
      if (uVar4 != 0) goto LAB_000482c2;
      uVar2 = uVar3 * 2;
      if (!CARRY4(uVar3,uVar3)) {
        uVar2 = param_1[1] & uVar2;
      }
      if (uVar2 == 0) {
        return;
      }
      uVar3 = uVar3 & 0x7fffffff;
    }
  }
  if (uVar4 == 0 && uVar6 == 0) {
    return;
  }
LAB_000482c2:
  puVar1 = param_1 + 1;
  uVar4 = *puVar1;
  *puVar1 = *puVar1 + uVar3;
  if (CARRY4(uVar4,uVar3)) {
    param_1[1] = (uint)(CONCAT14(1,param_1[1]) >> 1);
    *(short *)(param_1 + 2) = (short)param_1[2] + 1;
  }
  return;
}


