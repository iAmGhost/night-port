// func #125  FUN_00038c23  00038c23  size=187


void FUN_00038c23(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined8 uVar1;
  byte bVar2;
  undefined4 extraout_ECX;
  byte *pbVar3;
  int in_stack_00000000;
  byte local_30 [12];
  int local_24;
  undefined8 local_20;
  int local_18;
  undefined4 local_14;
  
  local_20 = FUN_000418ee();
  local_18 = param_3;
  local_14 = extraout_ECX;
  FUN_00041f42(in_stack_00000000,local_30,10);
  bVar2 = local_30[0];
  if (in_stack_00000000 < 10) {
    local_30[1] = 0x30;
    local_30[2] = 0;
  }
  else {
    local_30[2] = local_30[0];
    local_30[0] = local_30[1];
    local_30[1] = bVar2;
  }
  pbVar3 = local_30;
  uVar1 = local_20;
  for (local_24 = 1; local_20._4_4_ = (int)((ulonglong)uVar1 >> 0x20),
      local_20._0_4_ = (undefined4)uVar1, -1 < local_24; local_24 = local_24 + -1) {
    *(undefined4 *)(pbVar3 + -4) = 0;
    *(int *)(pbVar3 + -8) =
         *(int *)(local_20._4_4_ + 0x38) +
         *(int *)((uint)local_30[local_24] * 4 + *(int *)(local_20._4_4_ + 0x34) + -0xc0);
    *(undefined4 *)(pbVar3 + -0xc) = 7;
    *(undefined4 *)(pbVar3 + -0x10) = 0x38cd4;
    FUN_0003bb42((undefined4)local_20,(1 - local_24) * 7 + local_18,local_14,6);
    local_20 = uVar1;
    pbVar3 = pbVar3 + -0xc;
    uVar1 = local_20;
  }
  return;
}


