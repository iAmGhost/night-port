// func #21  FUN_00012efa  00012efa  size=1429


void FUN_00012efa(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined8 uVar7;
  byte *pbVar8;
  undefined4 uVar9;
  int *piVar10;
  uint *puVar11;
  uint *puVar12;
  int iStack_38;
  int iStack_34;
  uint uStack_30;
  undefined4 uStack_2c;
  uint local_28;
  int *local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  uVar9 = (undefined4)((ulonglong)local_20 >> 0x20);
  pbVar8 = (byte *)local_20;
  puVar12 = &local_28;
  puVar11 = &local_28;
  if (((pbVar8[0x30] != 1) || (5 < DAT_00063360 % 0x17)) || (1 < *pbVar8)) {
    local_24 = *(int **)(*(int *)(*(int *)(pbVar8 + 0x123) + 4) + *(int *)(pbVar8 + 0x17) * 4);
    local_28 = (uint)*(ushort *)
                      ((*(int *)(pbVar8 + 0x11) % *local_24) * 2 + *(int *)((int)local_24 + 0x6d));
    if ((pbVar8[0x30] == 2) && (DAT_00063360 % 0x14 == 0)) {
      if (*(int *)(pbVar8 + 0x1b) == 0) {
        uStack_2c = 0x9c;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x133));
        iStack_38 = *(int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        FUN_0003bebb(uVar9,(*(int *)(pbVar8 + 1) - *(int *)(*(int *)(pbVar8 + 0x137) + local_28 * 4)
                           ) - DAT_000639dc,
                     ((*(int *)(pbVar8 + 5) + *(int *)(pbVar8 + 9)) -
                     *(int *)(*(int *)(pbVar8 + 0x13b) + local_28 * 4)) - DAT_000639e0,
                     *(undefined4 *)(*(int *)(pbVar8 + 299) + local_28 * 4));
      }
      else {
        uStack_2c = 0x9c;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(*(int *)(pbVar8 + 0x133) + local_28 * 4);
        iStack_38 = *(int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        FUN_0003bebb(uVar9,(*(int *)(pbVar8 + 1) -
                           (*(int *)(local_28 * 4 + *(int *)(pbVar8 + 299)) -
                           *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x137)))) - DAT_000639dc,
                     ((*(int *)(pbVar8 + 5) + *(int *)(pbVar8 + 9)) -
                     *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x13b))) - DAT_000639e0,
                     *(undefined4 *)(local_28 * 4 + *(int *)(pbVar8 + 299)));
      }
    }
    else if (*(int *)(pbVar8 + 0x1b) == 0) {
      uVar7 = local_20;
      if ((char)local_24[0x1b] == '\x01') {
        uStack_2c = 0xd0;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x133));
        piVar10 = (int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        iStack_38 = *piVar10 - (*piVar10 >> 0x1f) >> 1;
        puVar11 = (uint *)&iStack_38;
        piVar10 = (int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        FUN_0003c231(uVar9,(*(int *)(pbVar8 + 1) - *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x137))
                           ) - DAT_000639dc,
                     (((*(int *)(pbVar8 + 9) + (*piVar10 - (*piVar10 >> 0x1f) >> 1)) -
                      *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x13b))) - DAT_000639e0) +
                     (*(int *)(pbVar8 + 5) - (*(int *)(pbVar8 + 5) >> 0x1f) >> 1),
                     *(undefined4 *)(*(int *)(pbVar8 + 299) + local_28 * 4));
        uVar7 = local_20;
      }
      local_20._4_4_ = (undefined4)((ulonglong)uVar7 >> 0x20);
      local_20._0_4_ = (int)uVar7;
      *(uint *)((int)puVar11 + -4) = (uint)*(byte *)((int)local_20 + 0x1b);
      *(int *)((int)puVar11 + -8) =
           *(int *)((int)local_20 + 0x127) +
           *(int *)(*(int *)((int)local_20 + 0x133) + local_28 * 4);
      *(undefined4 *)((int)puVar11 + -0xc) =
           *(undefined4 *)(*(int *)((int)local_20 + 0x12f) + local_28 * 4);
      uVar9 = *(undefined4 *)(*(int *)((int)local_20 + 299) + local_28 * 4);
      iVar1 = *(int *)((int)local_20 + 5);
      iVar2 = *(int *)((int)local_20 + 9);
      iVar3 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x13b));
      iVar4 = *(int *)((int)local_20 + 1);
      iVar5 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x137));
      *(undefined4 *)((int)puVar11 + -0x10) = 0x1315e;
      FUN_0003bb42(local_20._4_4_,(iVar4 - iVar5) - DAT_000639dc,
                   ((iVar1 + iVar2) - iVar3) - DAT_000639e0,uVar9);
    }
    else {
      uVar7 = local_20;
      if ((char)local_24[0x1b] == '\x01') {
        uStack_2c = 0xd0;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x133));
        piVar10 = (int *)(local_28 * 4 + *(int *)(pbVar8 + 0x12f));
        iStack_38 = *piVar10 - (*piVar10 >> 0x1f) >> 1;
        puVar12 = (uint *)&iStack_38;
        piVar10 = (int *)(local_28 * 4 + *(int *)(pbVar8 + 0x12f));
        FUN_0003c231(uVar9,(*(int *)(pbVar8 + 1) -
                           (*(int *)(local_28 * 4 + *(int *)(pbVar8 + 299)) -
                           *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x137)))) - DAT_000639dc,
                     (((*(int *)(pbVar8 + 9) + (*piVar10 - (*piVar10 >> 0x1f) >> 1)) -
                      *(int *)(*(int *)(pbVar8 + 0x13b) + local_28 * 4)) - DAT_000639e0) +
                     (*(int *)(pbVar8 + 5) - (*(int *)(pbVar8 + 5) >> 0x1f) >> 1),
                     *(undefined4 *)(*(int *)(pbVar8 + 299) + local_28 * 4));
        uVar7 = local_20;
      }
      local_20._4_4_ = (undefined4)((ulonglong)uVar7 >> 0x20);
      local_20._0_4_ = (int)uVar7;
      *(uint *)((int)puVar12 + -4) = (uint)*(byte *)((int)local_20 + 0x1b);
      *(int *)((int)puVar12 + -8) =
           *(int *)((int)local_20 + 0x127) +
           *(int *)(*(int *)((int)local_20 + 0x133) + local_28 * 4);
      *(undefined4 *)((int)puVar12 + -0xc) =
           *(undefined4 *)(local_28 * 4 + *(int *)((int)local_20 + 0x12f));
      uVar9 = *(undefined4 *)(*(int *)((int)local_20 + 299) + local_28 * 4);
      iVar1 = *(int *)((int)local_20 + 5);
      iVar2 = *(int *)((int)local_20 + 9);
      iVar3 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x13b));
      iVar4 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 299));
      iVar5 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x137));
      iVar6 = *(int *)((int)local_20 + 1);
      *(undefined4 *)((int)puVar12 + -0x10) = 0x13316;
      FUN_0003bb42(local_20._4_4_,(iVar6 - (iVar4 - iVar5)) - DAT_000639dc,
                   ((iVar1 + iVar2) - iVar3) - DAT_000639e0,uVar9);
    }
  }
  return;
}


