// func #18  FUN_00011827  00011827  size=293


void FUN_00011827(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int extraout_ECX;
  int iVar3;
  undefined8 uVar4;
  undefined4 in_stack_00000000;
  undefined4 local_38;
  undefined4 local_28;
  undefined4 local_24;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  iVar1 = (int)uVar4;
  if (*(int *)(iVar1 + 0x1b) == 0) {
    iVar2 = FUN_00041bdc(0x147);
    if (iVar2 == 0) {
      local_28 = 0;
    }
    else {
      local_28 = FUN_00010efc(iVar2,*(int *)(iVar1 + 1) + iVar3,param_3,
                              *(int *)(iVar1 + 9) + extraout_ECX,0xfa,2);
    }
    local_24 = local_28;
    *(undefined4 *)(local_28 + 0x1b) = 0;
  }
  else {
    iVar2 = FUN_00041bdc(0x147);
    if (iVar2 == 0) {
      local_38 = 0;
    }
    else {
      local_38 = FUN_00010efc(iVar2,*(int *)(iVar1 + 1) - iVar3,param_3,
                              *(int *)(iVar1 + 9) + extraout_ECX,0xfa,2);
    }
    local_24 = local_38;
    *(undefined4 *)(local_38 + 0x1b) = 1;
  }
  *(undefined1 *)(local_24 + 0x30) = 1;
  *(undefined4 *)(local_24 + 0x17) = in_stack_00000000;
  if (*(int *)(iVar1 + 0x17) != 4) {
    DAT_00063364 = 0x13;
    DAT_00063368 = 0;
  }
  return;
}


