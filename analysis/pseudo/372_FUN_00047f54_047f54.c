// func #372  FUN_00047f54  00047f54  size=128


void FUN_00047f54(uint param_1,int param_2,uint *param_3)

{
  uint uVar1;
  uint uVar2;
  ushort uVar3;
  undefined2 uVar5;
  uint extraout_ECX;
  uint uVar4;
  uint uVar6;
  uint extraout_EDX;
  bool bVar7;
  
  uVar6 = param_2 << 0xb | param_1 >> 0x15;
  uVar1 = param_1 << 0xb;
  uVar3 = (ushort)(param_2 >> 0x14) & 0x7ff;
  uVar4 = param_2 >> 0x14 & 0xffff07ff;
  uVar5 = (undefined2)(uVar4 >> 0x10);
  uVar2 = uVar1;
  if (uVar3 == 0) {
    if ((uVar6 == 0) && (uVar1 == 0)) {
      uVar4 = 0;
    }
    else {
      uVar4 = CONCAT22(uVar5,0x3c01);
      if (uVar6 == 0) {
        uVar4 = CONCAT22(uVar5,0x3be1);
        uVar2 = 0;
        uVar6 = uVar1;
      }
      for (; -1 < (int)uVar6; uVar6 = uVar6 * 2 + (uint)bVar7) {
        bVar7 = CARRY4(uVar2,uVar2);
        uVar2 = uVar2 * 2;
        uVar4 = CONCAT22((short)(uVar4 >> 0x10),(short)uVar4 + -1);
      }
    }
  }
  else {
    if (uVar3 == 0x7ff) {
      uVar4 = CONCAT22(uVar5,0x7fff);
      if (((param_2 << 0xb & 0x7fffffffU) != 0 || param_1 >> 0x15 != 0) || uVar1 != 0) {
        FUN_000463dc(CONCAT22((short)(uVar1 >> 0x10),0x8101));
        uVar6 = extraout_EDX | 0x40000000;
        uVar4 = extraout_ECX;
      }
    }
    else {
      uVar4 = CONCAT22(uVar5,(short)uVar4 + 0x3c00);
    }
    uVar6 = uVar6 | 0x80000000;
  }
  *param_3 = uVar2;
  param_3[1] = uVar6;
  *(short *)(param_3 + 2) = (short)(((uint)CARRY4(uVar4,uVar4) << 0x10 | uVar4 * 2 & 0xffff) >> 1);
  return;
}


