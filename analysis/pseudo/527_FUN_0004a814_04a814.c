// func #527  FUN_0004a814  0004a814  size=57

int FUN_0004a814(void)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint *puVar5;
  int unaff_EDI;
  uint *puVar6;
  bool bVar7;
  undefined8 uVar8;
  
  uVar8 = FUN_0004a923();
  puVar5 = (uint *)((ulonglong)uVar8 >> 0x20);
  puVar6 = (uint *)(unaff_EDI + ((int)uVar8 + 1) * 8);
  uVar2 = *puVar5;
  uVar3 = puVar5[1];
  iVar4 = 0;
  while( true ) {
    bVar7 = uVar3 < *puVar6;
    if (uVar3 == *puVar6) {
      bVar7 = uVar2 < puVar6[1];
    }
    if (bVar7) break;
    puVar6 = puVar6 + 2;
    iVar4 = iVar4 + 1;
  }
  while( true ) {
    uVar1 = puVar6[-2];
    bVar7 = uVar3 < uVar1;
    if (uVar3 == uVar1) {
      bVar7 = uVar2 < puVar6[-1];
    }
    if (!bVar7) break;
    iVar4 = iVar4 + -1;
    puVar6 = puVar6 + -2;
  }
  return iVar4;
}
