// func #294  FUN_00043071  00043071  size=324


undefined8 FUN_00043071(uint param_1,undefined4 param_2)

{
  int *piVar1;
  undefined3 uVar2;
  undefined4 uVar3;
  int iVar4;
  uint *puVar5;
  uint *puVar6;
  uint uVar7;
  uint *puVar8;
  ushort in_DS;
  undefined8 uVar9;
  uint local_18;
  
  local_18 = param_1;
  if (((DAT_000533b2 == 1) && (DAT_000533b3 == '\0')) || (DAT_000533b2 == 9)) {
    uVar3 = FUN_00042fc2(param_1);
    goto LAB_000431ac;
  }
  if ((DAT_0005363c == 0) || (DAT_00053384 == 0xfffffffe)) goto LAB_000430a8;
  iVar4 = FUN_000431b5(&local_18);
  uVar3 = 0;
  if (iVar4 == 0) goto LAB_000431ac;
  if (((1 < DAT_000533b2) && (DAT_000533b2 < 9)) && (DAT_000533b4 == '\0')) {
    uVar2 = SegmentLimit((uint)in_DS);
    DAT_00053384 = (ushort)uVar2 + 1;
  }
  uVar7 = local_18 + DAT_00053384;
  if (uVar7 < DAT_00053384) {
    uVar7 = 0xfffffffe;
  }
  uVar9 = FUN_00045bb7(uVar7);
  puVar8 = (uint *)((ulonglong)uVar9 >> 0x20);
  puVar5 = (uint *)uVar9;
  if (((puVar5 != (uint *)0xffffffff) && (puVar5 < (uint *)0xfffffff9)) && (puVar5 < puVar8)) {
    uVar7 = (int)puVar8 - (int)puVar5;
    local_18 = uVar7 - 4;
    puVar8 = DAT_00053350;
    if (local_18 <= uVar7) {
      for (; ((puVar8 != (uint *)0x0 && ((uint *)puVar8[2] != (uint *)0x0)) &&
             ((puVar5 < puVar8 || ((uint *)puVar8[2] <= puVar5)))); puVar8 = (uint *)puVar8[2]) {
      }
      if (puVar8 == (uint *)0x0) {
LAB_0004316f:
        if (local_18 < 0x38) goto LAB_000430a8;
        *puVar5 = local_18;
        uVar9 = FUN_00042e2d(puVar5,puVar5);
        local_18 = *(uint *)uVar9;
      }
      else {
        puVar6 = (uint *)(*puVar8 + (int)puVar8);
        uVar9 = CONCAT44(puVar8,puVar6);
        if (puVar5 + -1 != puVar6) goto LAB_0004316f;
        *puVar8 = *puVar8 + uVar7;
        *(undefined4 *)((int)puVar6 + uVar7) = 0xffffffff;
        local_18 = uVar7;
      }
      iVar4 = (int)((ulonglong)uVar9 >> 0x20);
      *(uint *)uVar9 = local_18 | 1;
      piVar1 = (int *)(iVar4 + 0x18);
      *piVar1 = *piVar1 + 1;
      *(undefined4 *)(iVar4 + 0x14) = 0xffffffff;
      FUN_00041c31((uint *)uVar9 + 1);
      uVar3 = 1;
      goto LAB_000431ac;
    }
  }
LAB_000430a8:
  uVar3 = 0;
LAB_000431ac:
  return CONCAT44(param_2,uVar3);
}


