// func #458  FUN_0003ba86  0003ba86  size=187

void FUN_0003ba86(undefined4 param_1,undefined4 param_2,int param_3)

{
  byte *pbVar1;
  int iVar2;
  byte *extraout_ECX;
  int iVar3;
  uint *puVar4;
  undefined8 uVar5;
  byte in_stack_00000000;
  uint auStack_3c [6];
  int local_24;
  undefined8 local_20;
  int local_18;
  byte *local_14;
  
  uVar5 = FUN_000418ee();
  puVar4 = auStack_3c + 2;
  local_18 = param_3;
  local_14 = extraout_ECX;
  for (local_24 = 0; local_24 < local_18; local_24 = local_24 + 1) {
    auStack_3c[5] = (uint)*local_14;
    auStack_3c[4] = 0;
    local_14 = local_14 + 1;
    for (auStack_3c[3] = 0; local_20._4_4_ = (int)((ulonglong)uVar5 >> 0x20),
        local_20._0_4_ = (int)uVar5, (int)auStack_3c[3] < (int)auStack_3c[5];
        auStack_3c[3] = auStack_3c[3] + 1) {
      pbVar1 = local_14 + 1;
      auStack_3c[4] = auStack_3c[4] + *local_14;
      local_14 = local_14 + 2;
      auStack_3c[2] = (uint)*pbVar1;
      *(uint *)((int)puVar4 + -4) = (uint)in_stack_00000000;
      iVar3 = local_20._4_4_ + local_24;
      iVar2 = (int)local_20 + auStack_3c[4];
      *(undefined4 *)((int)puVar4 + -8) = 0x3bb27;
      local_20 = uVar5;
      FUN_00034aba(iVar2,iVar3,auStack_3c[2],local_14);
      local_14 = local_14 + auStack_3c[2];
      auStack_3c[4] = auStack_3c[4] + auStack_3c[2];
      puVar4 = (uint *)((int)puVar4 + -4);
      uVar5 = local_20;
    }
  }
  return;
}
