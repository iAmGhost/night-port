// func #390  FUN_00048e60  00048e60  size=204


void FUN_00048e60(uint *param_1)

{
  uint uVar1;
  ushort uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  bool bVar8;
  
  uVar2 = (ushort)param_1[2];
  uVar5 = *param_1;
  if (uVar5 == 0) {
    if (param_1[1] != 0) {
      if (uVar2 == 0x7fff) {
        return;
      }
      goto LAB_00048ea6;
    }
    if ((uVar2 & 0x7fff) == 0) {
      return;
    }
  }
  else {
LAB_00048ea6:
    if (((uVar2 & 0x7fff) == 0x7fff) || (CARRY2(uVar2,uVar2))) goto LAB_00048e92;
    if (((uVar2 & 0x7fff) == 0) || (CARRY4(param_1[1],param_1[1]))) {
      uVar3 = param_1[1];
      uVar7 = 0;
      uVar2 = (uVar2 & 0x7fff) + 0xc001;
      uVar4 = uVar5;
      if ((uVar2 & 1) == 0) {
        uVar7 = uVar3 & 1;
        uVar3 = uVar3 >> 1;
        uVar4 = (uint)(CONCAT14(uVar7 != 0,uVar5) >> 1);
        uVar7 = (uint)((uVar5 & 1) != 0) << 0x1f;
      }
      *(short *)(param_1 + 2) = ((short)uVar2 >> 1) + 0x3fff;
      uVar5 = (uint)(((ulonglong)uVar3 | 0x100000000) >> 1);
      if (uVar3 == 0xffffffff) {
        bVar8 = uVar4 != 0xffffffff;
        iVar6 = -1;
        if (uVar4 == 0xffffffff) goto LAB_00048f1e;
        uVar3 = (uint)(CONCAT44(uVar4,uVar7) / 0xffffffff);
        uVar5 = 0xffffffff;
      }
      else {
        while( true ) {
          uVar1 = (uint)(CONCAT44(uVar3,uVar4) / (ulonglong)uVar5);
          if (uVar5 - 1 <= uVar1) break;
          uVar5 = (uint)(CONCAT14(CARRY4(uVar5,uVar1),uVar5 + uVar1) >> 1);
        }
        uVar3 = (uint)((CONCAT44(uVar3,uVar4) % (ulonglong)uVar5 << 0x20 | (ulonglong)uVar7) /
                      (ulonglong)uVar5);
        uVar5 = uVar5 + uVar1;
      }
      iVar6 = (int)(((ulonglong)uVar5 | 0x100000000) >> 1);
      uVar4 = (uint)(CONCAT14((uVar5 & 1) != 0,uVar3) >> 1);
      uVar5 = (uint)((uVar3 & 1) != 0);
      bVar8 = CARRY4(uVar4,uVar5);
      uVar4 = uVar4 + uVar5;
LAB_00048f1e:
      *param_1 = uVar4;
      param_1[1] = iVar6 + (uint)bVar8;
      return;
    }
  }
  *(undefined2 *)(param_1 + 2) = 0xffff;
  param_1[1] = 0;
  *param_1 = 0;
LAB_00048e92:
  *(byte *)((int)param_1 + 7) = *(byte *)((int)param_1 + 7) | 0xc0;
  return;
}


