// func #176  FUN_0003f462  0003f462  size=304


void FUN_0003f462(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int extraout_ECX;
  undefined8 uVar3;
  int in_stack_00000000;
  int local_34;
  int local_30;
  int local_2c;
  
  uVar3 = FUN_000418ee();
  local_30 = (int)uVar3 + (int)((ulonglong)uVar3 >> 0x20) * DAT_00063e64;
  local_2c = local_30 / DAT_000531f4;
  FUN_0003f3be(local_2c,local_30 % DAT_000531f4);
  local_30 = local_30 % DAT_000531f4;
  for (local_34 = 0; local_34 < extraout_ECX; local_34 = local_34 + 1) {
    if (DAT_000531f4 < local_30 + param_3) {
      iVar1 = (local_30 + param_3) - DAT_000531f4;
      iVar2 = param_3 - iVar1;
      FUN_00041e6c(&DAT_000a0000 + local_30,in_stack_00000000,iVar2);
      local_2c = local_2c + 1;
      FUN_0003f3be(local_2c);
      FUN_00041e6c(&DAT_000a0000,in_stack_00000000 + iVar2,iVar1);
      local_30 = DAT_00063e64 - iVar2;
    }
    else {
      FUN_00041e6c(&DAT_000a0000 + local_30,in_stack_00000000,param_3);
      local_30 = local_30 + DAT_00063e64;
      if (DAT_000531f4 <= local_30) {
        local_30 = local_30 - DAT_000531f4;
        local_2c = local_2c + 1;
        FUN_0003f3be(local_2c);
      }
    }
    in_stack_00000000 = in_stack_00000000 + param_3;
  }
  return;
}


