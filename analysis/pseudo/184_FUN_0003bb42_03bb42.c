// func #143  FUN_0003bb42  0003bb42  size=889


void FUN_0003bb42(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4,byte *param_5
                 ,char param_6)

{
  byte *pbVar1;
  byte bVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  int extraout_ECX;
  longlong lVar6;
  int in_stack_00000000;
  int local_34;
  int local_30;
  int local_28;
  int local_24;
  int local_20;
  int local_18;
  int local_14;
  
  lVar6 = FUN_000418ee();
  iVar4 = (int)((ulonglong)lVar6 >> 0x20);
  local_24 = 0;
  if ((((lVar6 < 0x14100000000) && (-1 < iVar4 + extraout_ECX)) && (param_3 < 0xc9)) &&
     (-1 < param_3 + in_stack_00000000)) {
    if (param_3 < 0) {
      for (local_28 = 0; -local_28 != param_3 && local_28 <= -param_3; local_28 = local_28 + 1) {
        bVar2 = *param_5;
        param_5 = param_5 + 2;
        for (local_30 = 0; local_30 < (int)(uint)bVar2; local_30 = local_30 + 1) {
          param_5 = param_5 + *param_5 + 2;
        }
        param_5 = param_5 + -1;
      }
      in_stack_00000000 = in_stack_00000000 + param_3;
      local_18 = 0;
    }
    else {
      local_18 = param_3;
      if (200 < param_3 + in_stack_00000000) {
        in_stack_00000000 = 200 - param_3;
      }
    }
    local_14 = extraout_ECX;
    if (param_6 == '\0') {
      local_20 = (int)lVar6 + local_18 * 0x140 + iVar4;
      if (lVar6 < 0) {
        local_24 = -iVar4;
      }
      else if (0x140 < iVar4 + extraout_ECX) {
        local_14 = 0x140 - iVar4;
      }
      for (local_28 = 0; local_28 < in_stack_00000000; local_28 = local_28 + 1) {
        bVar2 = *param_5;
        local_34 = 0;
        param_5 = param_5 + 1;
        for (local_30 = 0; local_30 < (int)(uint)bVar2; local_30 = local_30 + 1) {
          local_34 = local_34 + (uint)*param_5;
          pbVar1 = param_5 + 2;
          uVar3 = (uint)param_5[1];
          if (local_34 < local_24) {
            uVar5 = local_24 - local_34;
            if (uVar3 != uVar5 && -1 < (int)(uVar3 - uVar5)) {
              FUN_00041e6c(local_20 + local_24,pbVar1 + uVar5,uVar3 - uVar5);
            }
          }
          else if (local_14 < (int)(local_34 + uVar3)) {
            iVar4 = (local_34 + uVar3) - local_14;
            if (iVar4 < (int)uVar3) {
              FUN_00041e6c(local_20 + local_34,pbVar1,uVar3 - iVar4);
            }
          }
          else {
            FUN_00041e6c(local_20 + local_34,pbVar1,uVar3);
          }
          param_5 = pbVar1 + uVar3;
          local_34 = local_34 + uVar3;
        }
        local_20 = local_20 + 0x140;
      }
    }
    else {
      if (iVar4 + extraout_ECX < 0x141) {
        if (lVar6 < 0) {
          local_14 = extraout_ECX + iVar4;
        }
      }
      else {
        local_24 = iVar4 + extraout_ECX + -0x140;
      }
      for (local_28 = 0; local_28 < in_stack_00000000; local_28 = local_28 + 1) {
        bVar2 = *param_5;
        local_34 = 0;
        param_5 = param_5 + 1;
        for (local_30 = 0; local_30 < (int)(uint)bVar2; local_30 = local_30 + 1) {
          local_34 = local_34 + (uint)*param_5;
          uVar3 = (uint)param_5[1];
          if (local_34 < local_24) {
            if (uVar3 != local_24 - local_34 && -1 < (int)(uVar3 - (local_24 - local_34))) {
              FUN_00040475();
            }
          }
          else if (local_14 < (int)(local_34 + uVar3)) {
            if ((int)((local_34 + uVar3) - local_14) < (int)uVar3) {
              FUN_00040475();
            }
          }
          else {
            FUN_00040475();
          }
          param_5 = param_5 + uVar3 + 2;
          local_34 = local_34 + uVar3;
        }
      }
    }
  }
  return;
}


