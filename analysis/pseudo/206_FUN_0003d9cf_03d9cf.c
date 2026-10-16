// func #163  FUN_0003d9cf  0003d9cf  size=750


undefined4 FUN_0003d9cf(void)

{
  int local_34;
  int local_30;
  int local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  FUN_000367c9((int)((ulonglong)local_20 >> 0x20));
  FUN_0004192e((int *)local_20,1,4,DAT_000639b8);
  local_30 = 0;
  for (local_34 = 0; local_34 < *(int *)local_20; local_34 = local_34 + 1) {
    FUN_0004247a(DAT_000639b8,0x30,1);
    FUN_0004192e(&local_24,1,4,DAT_000639b8);
    FUN_0004247a(DAT_000639b8,local_24,1);
    local_30 = local_30 + local_24;
  }
  FUN_0003de85((int *)local_20,*(int *)local_20,local_30);
  FUN_00036853(local_20._4_4_,4,0);
  local_30 = 0;
  for (local_34 = 0; local_34 < *(int *)local_20; local_34 = local_34 + 1) {
    *(int *)(local_34 * 4 + ((int *)local_20)[0xd]) = local_30;
    FUN_0004192e(((int *)local_20)[1] + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 8) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(local_34 * 4 + *(int *)((int)(int *)local_20 + 0xc),1,4,DAT_000639b8);
    FUN_0004192e(local_34 * 4 + *(int *)((int)(int *)local_20 + 0x10),1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x14) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x24) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x18) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x28) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x1c) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(local_34 * 4 + *(int *)((int)(int *)local_20 + 0x2c),1,4,DAT_000639b8);
    FUN_0004192e(local_34 * 4 + *(int *)((int)(int *)local_20 + 0x20),1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x30) + local_34 * 4,1,4,DAT_000639b8);
    FUN_0004192e(&local_24,1,4,DAT_000639b8);
    FUN_0004192e(*(int *)((int)(int *)local_20 + 0x38) + local_30,local_24,1,DAT_000639b8);
    local_30 = local_30 + local_24;
  }
  return 1;
}


