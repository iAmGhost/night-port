// func #139  FUN_0003b718  0003b718  size=571


void FUN_0003b718(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined8 uVar5;
  byte *pbVar6;
  byte local_34 [12];
  int local_28;
  int local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  FUN_00041f42(*(undefined4 *)((int)local_20 + 1),local_34,10);
  local_28 = FUN_00041f5d(local_34);
  if (*(char *)local_20 == '\0') {
    pbVar6 = local_34;
    uVar5 = local_20;
    for (local_24 = 0; local_20._4_4_ = (undefined4)((ulonglong)uVar5 >> 0x20),
        local_20._0_4_ = (char *)uVar5, local_24 < local_28; local_24 = local_24 + 1) {
      *(undefined4 *)(pbVar6 + -4) = 0;
      *(int *)(pbVar6 + -8) =
           *(int *)(DAT_000639c0 + 0x38) +
           *(int *)(*(int *)(DAT_000639c0 + 0x34) + (uint)local_34[local_24] * 4 + -0xc0);
      *(undefined4 *)(pbVar6 + -0xc) = 7;
      iVar1 = *(int *)((int)(char *)local_20 + 0xd);
      iVar2 = *(int *)((int)(char *)local_20 + 5);
      iVar3 = *(int *)((int)(char *)local_20 + 9);
      *(undefined4 *)(pbVar6 + -0x10) = 0x3b7e0;
      FUN_0003bb42(local_20._4_4_,(local_24 * 5 + iVar3) - DAT_000639dc,
                   ((iVar1 - iVar2) - DAT_000639e0) + -0xf,4);
      local_20 = uVar5;
      pbVar6 = pbVar6 + -0xc;
      uVar5 = local_20;
    }
    *(undefined4 *)(pbVar6 + -4) = 0;
    *(int *)(pbVar6 + -8) =
         *(int *)(DAT_000639c0 + 0x38) + *(int *)(*(int *)(DAT_000639c0 + 0x34) + 0x28);
    *(undefined4 *)(pbVar6 + -0xc) = *(undefined4 *)(*(int *)(DAT_000639c0 + 8) + 0x28);
    uVar4 = *(undefined4 *)(*(int *)(DAT_000639c0 + 4) + 0x28);
    iVar1 = *(int *)((int)(char *)local_20 + 0xd);
    iVar2 = *(int *)((int)(char *)local_20 + 5);
    iVar3 = *(int *)((int)(char *)local_20 + 9);
    *(undefined4 *)(pbVar6 + -0x10) = 0x3b83b;
    FUN_0003bb42(local_20._4_4_,(iVar3 - DAT_000639dc) + -0xf,
                 ((iVar1 - iVar2) - DAT_000639e0) + -0xe,uVar4);
    local_20 = uVar5;
  }
  else {
    pbVar6 = local_34;
    uVar5 = local_20;
    for (local_24 = 0; local_20._4_4_ = (undefined4)((ulonglong)uVar5 >> 0x20),
        local_20._0_4_ = (char *)uVar5, local_24 < local_28; local_24 = local_24 + 1) {
      *(undefined4 *)(pbVar6 + -4) = 0;
      *(int *)(pbVar6 + -8) =
           *(int *)(DAT_000639c4 + 0x38) +
           *(int *)(*(int *)(DAT_000639c4 + 0x34) + (uint)local_34[local_24] * 4 + -0xc0);
      *(undefined4 *)(pbVar6 + -0xc) = 0xc;
      iVar1 = *(int *)((int)(char *)local_20 + 0xd);
      iVar2 = *(int *)((int)(char *)local_20 + 5);
      iVar3 = *(int *)((int)(char *)local_20 + 9);
      *(undefined4 *)(pbVar6 + -0x10) = 0x3b8b9;
      FUN_0003bb42(local_20._4_4_,(local_24 * 0xb + iVar3) - DAT_000639dc,
                   ((iVar1 - iVar2) - DAT_000639e0) + -0xf,10);
      local_20 = uVar5;
      pbVar6 = pbVar6 + -0xc;
      uVar5 = local_20;
    }
    *(undefined4 *)(pbVar6 + -4) = 0;
    *(int *)(pbVar6 + -8) =
         *(int *)(DAT_000639c4 + 0x38) + *(int *)(*(int *)(DAT_000639c4 + 0x34) + 0x28);
    *(undefined4 *)(pbVar6 + -0xc) = *(undefined4 *)(*(int *)(DAT_000639c4 + 8) + 0x28);
    uVar4 = *(undefined4 *)(*(int *)(DAT_000639c4 + 4) + 0x28);
    iVar1 = *(int *)((int)(char *)local_20 + 0xd);
    iVar2 = *(int *)((int)(char *)local_20 + 5);
    iVar3 = *(int *)((int)(char *)local_20 + 9);
    *(undefined4 *)(pbVar6 + -0x10) = 0x3b914;
    FUN_0003bb42(local_20._4_4_,(iVar3 - DAT_000639dc) + -0x18,
                 ((iVar1 - iVar2) - DAT_000639e0) + -0xd,uVar4);
    local_20 = uVar5;
  }
  iVar1 = (int)(char *)local_20;
  if (DAT_00063360 % 3 == 0) {
    *(int *)((int)(char *)local_20 + 5) = *(int *)((int)(char *)local_20 + 5) + 1;
  }
  if (0x28 < *(int *)((int)(char *)local_20 + 5)) {
    *(undefined4 *)(pbVar6 + -0x10) = 0x3b946;
    FUN_0003b67e(iVar1,0);
    *(undefined4 *)(pbVar6 + -0x10) = 0x3b94b;
    FUN_00041c28();
  }
  return;
}


