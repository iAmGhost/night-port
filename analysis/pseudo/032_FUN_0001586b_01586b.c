// func #32  FUN_0001586b  0001586b  size=436


void FUN_0001586b(void)

{
  byte *pbVar1;
  int iVar2;
  undefined8 uVar3;
  int local_28;
  int local_24;
  int local_1c;
  
  uVar3 = FUN_000418ee();
  local_1c = (int)((ulonglong)uVar3 >> 0x20);
  pbVar1 = (byte *)uVar3;
  if (local_1c != 0) {
    iVar2 = *(int *)(DAT_00063354 + 0x12) + DAT_000639e0;
    if (*(char *)(DAT_00063354 + 0x10) == '\0') {
      local_28 = *(int *)(DAT_00063354 + 0x16);
    }
    else {
      local_28 = *(int *)(*(int *)(DAT_00063354 + 0x34c) + 0x16);
    }
    local_28 = local_28 + DAT_000639e0;
    if (*pbVar1 < 2) {
      for (local_24 = 0; local_24 < DAT_00062a8c; local_24 = local_24 + 1) {
        if ((*(int *)(&DAT_0005c8bc + local_24 * 0x14) < *(int *)(pbVar1 + 1)) &&
           (*(int *)(pbVar1 + 1) < *(int *)(&DAT_0005c8c4 + local_24 * 0x14))) {
          if (local_1c < 1) {
            if ((*(int *)(&DAT_0005c8c0 + local_24 * 0x14) < *(int *)(pbVar1 + 9) + local_1c) &&
               (*(int *)(pbVar1 + 9) + local_1c < *(int *)(&DAT_0005c8c8 + local_24 * 0x14))) {
              local_1c = *(int *)(&DAT_0005c8c8 + local_24 * 0x14) - *(int *)(pbVar1 + 9);
            }
          }
          else if ((*(int *)(&DAT_0005c8c0 + local_24 * 0x14) < *(int *)(pbVar1 + 9) + local_1c) &&
                  (*(int *)(pbVar1 + 9) + local_1c < *(int *)(&DAT_0005c8c8 + local_24 * 0x14))) {
            local_1c = *(int *)(&DAT_0005c8c0 + local_24 * 0x14) - *(int *)(pbVar1 + 9);
          }
        }
      }
    }
    if ((*(int *)(pbVar1 + 9) < iVar2) || (local_28 < *(int *)(pbVar1 + 9))) {
      *(int *)(pbVar1 + 9) = *(int *)(pbVar1 + 9) + local_1c;
    }
    else {
      *(int *)(pbVar1 + 9) = *(int *)(pbVar1 + 9) + local_1c;
      if (*(int *)(pbVar1 + 9) < iVar2) {
        *(int *)(pbVar1 + 9) = iVar2;
      }
      else if (local_28 < *(int *)(pbVar1 + 9)) {
        *(int *)(pbVar1 + 9) = local_28;
      }
    }
  }
  return;
}


