// func #293  FUN_00042fc2  00042fc2  size=175


undefined8 FUN_00042fc2(uint param_1)

{
  code *pcVar1;
  undefined4 uVar2;
  int iVar3;
  uint *puVar4;
  uint uVar5;
  uint *puVar6;
  uint *puVar7;
  uint *puVar8;
  byte bVar9;
  undefined8 uVar10;
  uint local_18;
  
  puVar8 = &local_18;
  local_18 = param_1;
  puVar6 = &local_18;
  if ((DAT_0005363c != 0) && (puVar6 = &local_18, DAT_00053384 != -2)) {
    iVar3 = FUN_000431b5(&local_18);
    uVar2 = 0;
    if (iVar3 == 0) goto LAB_00042fb9;
    bVar9 = DAT_000533b2 == '\0';
    if (DAT_000533b2 == '\x01') {
      puVar4 = (uint *)FUN_00042eef(local_18);
      puVar7 = puVar4;
      puVar8 = &local_18;
      if (puVar4 != (uint *)0x0) {
        local_18 = *puVar4;
        puVar8 = &local_18;
      }
    }
    else {
      pcVar1 = (code *)swi(0x21);
      uVar5 = (*pcVar1)(0x80004800);
      puVar8 = (uint *)&stack0xffffffec;
      puVar7 = (uint *)~-(uint)bVar9;
      puVar4 = (uint *)(uVar5 & (uint)puVar7);
    }
    puVar6 = puVar8;
    if (((puVar4 != (uint *)0x0) && (uVar5 = *puVar8 - 4, uVar5 <= *puVar8)) &&
       (*puVar8 = uVar5, 0x37 < uVar5)) {
      *puVar4 = uVar5;
      puVar8[-1] = 0x43046;
      uVar10 = FUN_00042e2d(puVar4,puVar4,puVar7);
      iVar3 = (int)((ulonglong)uVar10 >> 0x20);
      puVar6 = (uint *)uVar10;
      uVar5 = *puVar6;
      *puVar8 = uVar5;
      *puVar6 = uVar5 | 1;
      *(undefined4 *)(iVar3 + 0x14) = 0xffffffff;
      *(int *)(iVar3 + 0x18) = *(int *)(iVar3 + 0x18) + 1;
      puVar8[-1] = 0x43067;
      FUN_00041c31(puVar6 + 1);
      uVar2 = 1;
      goto LAB_00042fb9;
    }
  }
  puVar8 = puVar6;
  uVar2 = 0;
LAB_00042fb9:
  return CONCAT44(*(undefined4 *)((int)puVar8 + 0xc),uVar2);
}


