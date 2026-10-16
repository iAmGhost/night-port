// func #398  FUN_00049236  00049236  size=124


uint FUN_00049236(undefined4 param_1,undefined4 param_2,uint param_3)

{
  code *pcVar1;
  bool bVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint extraout_EDX;
  undefined4 *puVar6;
  undefined4 *puVar7;
  byte bVar8;
  undefined4 local_14;
  
  puVar7 = &local_14;
  local_14 = param_2;
  uVar3 = FUN_000492d0();
  if ((uVar3 & 0x80) != 0) {
    bVar8 = 0;
    pcVar1 = (code *)swi(0x21);
    uVar3 = (*pcVar1)(0x4202,0,param_1,0);
    puVar7 = (undefined4 *)&stack0xfffffff0;
    puVar6 = (undefined4 *)&stack0xfffffff0;
    if ((bVar8 & 1) != 0) goto LAB_00049273;
  }
  bVar8 = 0;
  pcVar1 = (code *)swi(0x21);
  iVar4 = (*pcVar1)(CONCAT11(0x40,(char)uVar3),*puVar7,param_1,param_3);
  puVar6 = puVar7 + 1;
  bVar2 = (bVar8 & 1) != 0;
  uVar3 = (iVar4 << 1 | (uint)bVar8) >> 1;
  uVar5 = uVar3 | (uint)bVar2 << 0x1f;
  if (!bVar2) {
    if (uVar5 == param_3) {
      return uVar5;
    }
    *puVar7 = 0x492a8;
    FUN_00042a0d(0xc);
    return extraout_EDX;
  }
LAB_00049273:
  *(undefined4 *)((int)puVar6 + -4) = 0x49278;
  uVar3 = FUN_00045a82(uVar3 & 0xffff);
  return uVar3;
}


