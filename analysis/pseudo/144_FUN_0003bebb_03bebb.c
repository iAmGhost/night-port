// func #144  FUN_0003bebb  0003bebb  size=886


void FUN_0003bebb(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4,byte *param_5
                 ,char param_6,undefined1 param_7)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  int extraout_ECX;
  longlong lVar5;
  int in_stack_00000000;
  int local_34;
  int local_30;
  int local_28;
  int local_24;
  int local_20;
  int local_18;
  int local_14;
  
  lVar5 = FUN_000418ee();
  iVar3 = (int)((ulonglong)lVar5 >> 0x20);
  local_24 = 0;
  if ((((lVar5 < 0x14100000000) && (-1 < iVar3 + extraout_ECX)) && (param_3 < 0xc9)) &&
     (-1 < param_3 + in_stack_00000000)) {
    if (param_3 < 0) {
      for (local_28 = 0; -local_28 != param_3 && local_28 <= -param_3; local_28 = local_28 + 1) {
        bVar1 = *param_5;
        param_5 = param_5 + 2;
        for (local_30 = 0; local_30 < (int)(uint)bVar1; local_30 = local_30 + 1) {
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
      local_20 = (int)lVar5 + local_18 * 0x140 + iVar3;
      if (lVar5 < 0) {
        local_24 = -iVar3;
      }
      else if (0x140 < iVar3 + extraout_ECX) {
        local_14 = 0x140 - iVar3;
      }
      for (local_28 = 0; local_28 < in_stack_00000000; local_28 = local_28 + 1) {
        bVar1 = *param_5;
        local_34 = 0;
        param_5 = param_5 + 1;
        for (local_30 = 0; local_30 < (int)(uint)bVar1; local_30 = local_30 + 1) {
          local_34 = local_34 + (uint)*param_5;
          uVar2 = (uint)param_5[1];
          if (local_34 < local_24) {
            uVar4 = local_24 - local_34;
            if (uVar2 != uVar4 && -1 < (int)(uVar2 - uVar4)) {
              FUN_00041e20(local_20 + local_24,param_7,uVar2 - uVar4);
            }
          }
          else if (local_14 < (int)(local_34 + uVar2)) {
            iVar3 = (local_34 + uVar2) - local_14;
            if (iVar3 < (int)uVar2) {
              FUN_00041e20(local_20 + local_34,param_7,uVar2 - iVar3);
            }
          }
          else {
            FUN_00041e20(local_20 + local_34,param_7,uVar2);
          }
          param_5 = param_5 + uVar2 + 2;
          local_34 = local_34 + uVar2;
        }
        local_20 = local_20 + 0x140;
      }
    }
    else {
      local_20 = (int)lVar5 + local_18 * 0x140 + iVar3 + extraout_ECX + -1;
      if (iVar3 + extraout_ECX < 0x141) {
        if (lVar5 < 0) {
          local_14 = extraout_ECX + iVar3;
        }
      }
      else {
        local_24 = iVar3 + extraout_ECX + -0x140;
      }
      for (local_28 = 0; local_28 < in_stack_00000000; local_28 = local_28 + 1) {
        bVar1 = *param_5;
        local_34 = 0;
        param_5 = param_5 + 1;
        for (local_30 = 0; local_30 < (int)(uint)bVar1; local_30 = local_30 + 1) {
          local_34 = local_34 + (uint)*param_5;
          uVar2 = (uint)param_5[1];
          if (local_34 < local_24) {
            uVar4 = local_24 - local_34;
            if (uVar2 != uVar4 && -1 < (int)(uVar2 - uVar4)) {
              FUN_00041e20(((local_20 - uVar2) - local_34) + 1,param_7,uVar2 - uVar4);
            }
          }
          else if (local_14 < (int)(local_34 + uVar2)) {
            iVar3 = (local_34 + uVar2) - local_14;
            if (iVar3 < (int)uVar2) {
              FUN_00041e20((local_20 - local_14) + 1,param_7,uVar2 - iVar3);
            }
          }
          else {
            FUN_00041e20(((local_20 - local_34) - uVar2) + 1,param_7,uVar2);
          }
          param_5 = param_5 + uVar2 + 2;
          local_34 = local_34 + uVar2;
        }
        local_20 = local_20 + 0x140;
      }
    }
  }
  return;
}


