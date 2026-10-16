// func #120  FUN_000383a4  000383a4  size=161


void FUN_000383a4(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined8 uVar1;
  undefined4 extraout_ECX;
  int *piVar2;
  undefined4 in_stack_00000000;
  int local_34;
  int local_30;
  byte local_2c [12];
  undefined8 local_20;
  int local_18;
  undefined4 local_14;
  
  local_20 = FUN_000418ee();
  local_18 = param_3;
  local_14 = extraout_ECX;
  FUN_00041f42(in_stack_00000000,local_2c,10);
  local_30 = FUN_00041f5d(local_2c);
  piVar2 = &local_34;
  uVar1 = local_20;
  for (local_34 = 0; local_20._4_4_ = (int)((ulonglong)uVar1 >> 0x20),
      local_20._0_4_ = (undefined4)uVar1, local_34 < local_30; local_34 = local_34 + 1) {
    *(undefined4 *)((int)piVar2 + -4) = 0;
    *(int *)((int)piVar2 + -8) =
         *(int *)(local_20._4_4_ + 0x38) +
         *(int *)((uint)local_2c[local_34] * 4 + *(int *)(local_20._4_4_ + 0x34) + -0xc0);
    *(undefined4 *)((int)piVar2 + -0xc) = 7;
    *(undefined4 *)((int)piVar2 + -0x10) = 0x3843b;
    FUN_0003bb42((undefined4)local_20,local_34 * 8 + local_18,local_14,7);
    local_20 = uVar1;
    piVar2 = (int *)((int)piVar2 + -0xc);
    uVar1 = local_20;
  }
  return;
}


