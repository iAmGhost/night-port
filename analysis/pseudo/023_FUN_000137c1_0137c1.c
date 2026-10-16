// func #23  FUN_000137c1  000137c1  size=327


void FUN_000137c1(void)

{
  undefined8 uVar1;
  int *piVar2;
  undefined1 *puVar3;
  int local_38;
  int local_34;
  int local_30;
  byte local_2c [12];
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  FUN_00041f42(*(undefined4 *)((int)local_20 + 0x2b),local_2c,10);
  local_30 = FUN_00041f5d(local_2c);
  local_34 = 5 - local_30;
  piVar2 = &local_38;
  uVar1 = local_20;
  for (local_38 = 0; local_20._4_4_ = (undefined4)((ulonglong)uVar1 >> 0x20), local_38 < local_34;
      local_38 = local_38 + 1) {
    *(undefined4 *)((int)piVar2 + -4) = 0;
    *(int *)((int)piVar2 + -8) =
         *(int *)(DAT_00063e04 + 0x38) + *(int *)(*(int *)(DAT_00063e04 + 0x34) + 4);
    *(undefined4 *)((int)piVar2 + -0xc) = 7;
    *(undefined4 *)((int)piVar2 + -0x10) = 0x13852;
    FUN_0003bb42(local_20._4_4_,local_38 * 6 + 0x41,0x14,5);
    local_20 = uVar1;
    piVar2 = (int *)((int)piVar2 + -0xc);
    uVar1 = local_20;
  }
  local_38 = 0;
  puVar3 = (undefined1 *)piVar2;
  while( true ) {
    local_20._4_4_ = (undefined4)((ulonglong)uVar1 >> 0x20);
    local_20._0_4_ = (int)uVar1;
    if (local_30 <= local_38) break;
    *(undefined4 *)(puVar3 + -4) = 0;
    *(int *)(puVar3 + -8) =
         *(int *)(DAT_00063e04 + 0x38) +
         *(int *)((uint)local_2c[local_38] * 4 + *(int *)(DAT_00063e04 + 0x34) + -0xbc);
    *(undefined4 *)(puVar3 + -0xc) = 7;
    *(undefined4 *)(puVar3 + -0x10) = 0x138b8;
    FUN_0003bb42(local_20._4_4_,(local_38 + local_34) * 6 + 0x41,0x14,5);
    local_20 = uVar1;
    local_38 = local_38 + 1;
    puVar3 = puVar3 + -0xc;
    uVar1 = local_20;
  }
  if ((*(int *)((int)local_20 + 0x52) != 0) && (DAT_00063360 % 2 == 0)) {
    *(int *)((int)local_20 + 0x2b) = *(int *)((int)local_20 + 0x2b) + 1;
    *(int *)((int)local_20 + 0x52) = *(int *)((int)local_20 + 0x52) + -1;
  }
  if (*(int *)((int)local_20 + 0x56) != 0) {
    *(short *)((int)local_20 + 0x23) = *(short *)((int)local_20 + 0x23) + 1;
    *(int *)((int)local_20 + 0x56) = *(int *)((int)local_20 + 0x56) + -1;
  }
  return;
}


