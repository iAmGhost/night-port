// func #409  FUN_00049613  00049613  size=691


undefined8 FUN_00049613(int param_1,undefined4 param_2)

{
  bool bVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int extraout_ECX_02;
  int extraout_ECX_03;
  int extraout_ECX_04;
  int iVar6;
  undefined *puVar7;
  int iVar8;
  undefined *puVar9;
  uint uVar10;
  undefined8 uVar11;
  
  uVar10 = 0;
  if (*PTR_DAT_000537ca == '\0') goto LAB_000498b7;
  uVar2 = FUN_000495eb(&DAT_00053734,&DAT_00053758,*(undefined4 *)(param_1 + 0x14));
  if (uVar2 == 0) {
    puVar7 = &DAT_00053734;
    puVar9 = &DAT_00053758;
  }
  else {
    puVar7 = &DAT_00053758;
    puVar9 = &DAT_00053734;
  }
  iVar5 = *(int *)(extraout_ECX + 0x10);
  iVar3 = FUN_000494cb(*(int *)(extraout_ECX + 0x14) + 0x76c);
  iVar6 = iVar5 * 2;
  if (iVar3 == 0) {
    iVar3 = *(int *)(&DAT_00051960 + iVar6);
    iVar6 = *(int *)(&DAT_0005195e + iVar6);
  }
  else {
    iVar3 = *(int *)(&DAT_0005197a + iVar6);
    iVar6 = *(int *)(&DAT_00051978 + iVar6);
  }
  iVar6 = (iVar3 >> 0x10) - (iVar6 >> 0x10);
  bVar1 = false;
  param_1 = extraout_ECX_00;
  if (*(int *)(puVar7 + 0x20) == 0) {
    if (*(int *)(puVar7 + 0x10) < iVar5) {
      uVar10 = 1;
    }
    else if (iVar5 == *(int *)(puVar7 + 0x10)) {
      iVar3 = *(int *)(extraout_ECX_00 + 0xc) -
              ((*(int *)(extraout_ECX_00 + 0x18) + 7) - *(int *)(puVar7 + 0x18)) % 7;
      iVar8 = (*(int *)(extraout_ECX_00 + 0xc) + -1) -
              ((*(int *)(extraout_ECX_00 + 0x18) + 6) - *(int *)(puVar7 + 0x18)) % 7;
      if (*(int *)(puVar7 + 0xc) == 5) {
        iVar4 = iVar6 + -7;
        if ((iVar4 < iVar3) && (uVar10 = 1, iVar8 <= iVar4)) {
LAB_00049798:
          uVar10 = 1;
          bVar1 = true;
        }
      }
      else {
        iVar4 = (*(int *)(puVar7 + 0xc) + -1) * 7 + 1;
        if ((iVar4 <= iVar3) && (uVar10 = 1, iVar8 < iVar4)) goto LAB_00049798;
      }
    }
  }
  else {
    iVar3 = *(int *)(puVar7 + 0x1c);
    if (*(int *)(puVar7 + 0x20) == 1) {
      uVar11 = FUN_000494cb(*(int *)(extraout_ECX_00 + 0x14) + 0x76c);
      iVar3 = (int)((ulonglong)uVar11 >> 0x20);
      if (((int)uVar11 != 0) && (DAT_00051962 >> 0x10 < iVar3)) {
        iVar3 = iVar3 + 1;
      }
      iVar3 = iVar3 + -1;
      param_1 = extraout_ECX_01;
    }
    if ((iVar3 <= *(int *)(param_1 + 0x1c)) && (uVar10 = 1, iVar3 == *(int *)(param_1 + 0x1c)))
    goto LAB_00049798;
  }
  if (bVar1) {
    iVar3 = FUN_000498c6(param_1,puVar7);
    uVar10 = (uint)(iVar3 == 0);
    param_1 = extraout_ECX_02;
  }
  if (uVar10 == 0) {
    if (uVar2 != 0) {
      uVar10 = uVar2;
    }
    goto LAB_000498b7;
  }
  iVar3 = 0;
  if (*(int *)(puVar9 + 0x20) == 0) {
    if (*(int *)(puVar9 + 0x10) < iVar5) {
      uVar10 = 0;
    }
    else if (iVar5 == *(int *)(puVar9 + 0x10)) {
      iVar5 = *(int *)(param_1 + 0xc) -
              ((*(int *)(param_1 + 0x18) + 7) - *(int *)(puVar9 + 0x18)) % 7;
      iVar8 = (*(int *)(param_1 + 0xc) + -1) -
              ((*(int *)(param_1 + 0x18) + 6) - *(int *)(puVar9 + 0x18)) % 7;
      uVar10 = 0;
      if (*(int *)(puVar9 + 0xc) == 5) {
        iVar6 = iVar6 + -7;
        if (iVar6 < iVar5) {
          if (iVar8 <= iVar6) {
LAB_00049895:
            uVar10 = 0;
            iVar3 = 1;
          }
        }
        else {
LAB_0004983d:
          uVar10 = 1;
        }
      }
      else {
        iVar6 = (*(int *)(puVar9 + 0xc) + -1) * 7 + 1;
        if (iVar5 < iVar6) goto LAB_0004983d;
        if (iVar8 < iVar6) goto LAB_00049895;
      }
    }
  }
  else {
    iVar5 = *(int *)(puVar9 + 0x1c);
    if (*(int *)(puVar9 + 0x20) == 1) {
      uVar11 = FUN_000494cb(*(int *)(param_1 + 0x14) + 0x76c);
      iVar5 = (int)((ulonglong)uVar11 >> 0x20);
      if (((int)uVar11 != 0) && (DAT_00051962 >> 0x10 < iVar5)) {
        iVar5 = iVar5 + 1;
      }
      iVar5 = iVar5 + -1;
      param_1 = extraout_ECX_03;
    }
    if ((iVar5 <= *(int *)(param_1 + 0x1c)) && (uVar10 = 0, iVar5 == *(int *)(param_1 + 0x1c)))
    goto LAB_00049895;
  }
  if (iVar3 != 0) {
    uVar10 = FUN_000498c6(param_1,puVar9);
    param_1 = extraout_ECX_04;
  }
  if (uVar2 != 0) {
    uVar10 = uVar2 - uVar10;
  }
LAB_000498b7:
  *(uint *)(param_1 + 0x20) = uVar10;
  return CONCAT44(param_2,uVar10);
}


