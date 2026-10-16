// func #494  FUN_0004589e  0004589e  size=188

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0004589e(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  uint extraout_ECX;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  byte bVar8;
  int local_20;
  int local_1c;
  
  bVar8 = 0;
  iVar3 = FUN_0004595b(DAT_00053388,0,&local_20);
  iVar3 = iVar3 + 1;
  local_1c = iVar3 * 4;
  puVar4 = (undefined4 *)FUN_00041af8(local_1c + (local_20 - (int)DAT_00053388) + 5);
  iVar2 = local_1c;
  if (puVar4 == (undefined4 *)0x0) {
    _DAT_00053378 = 0;
    DAT_0005337c = (undefined4 *)0x0;
  }
  else {
    DAT_0005337c = (undefined4 *)((int)puVar4 + extraout_ECX);
    puVar6 = DAT_00053388;
    puVar7 = puVar4;
    for (uVar5 = extraout_ECX >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + (uint)bVar8 * -2 + 1;
      puVar7 = puVar7 + (uint)bVar8 * -2 + 1;
    }
    for (uVar5 = extraout_ECX & 3; puVar1 = DAT_0005337c, uVar5 != 0; uVar5 = uVar5 - 1) {
      *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + (uint)bVar8 * -2 + 1);
      puVar7 = (undefined4 *)((int)puVar7 + (uint)bVar8 * -2 + 1);
    }
    *DAT_0005337c = DAT_0005338c;
    FUN_0004595b(puVar4,puVar1 + 1,&local_20);
    _DAT_00053378 = iVar3;
    *(undefined4 *)((int)DAT_0005337c + iVar2) = 0;
  }
  DAT_00064090 = iVar3;
  DAT_00064094 = DAT_0005337c;
  return;
}
