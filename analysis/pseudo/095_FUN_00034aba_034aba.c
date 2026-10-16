// func #95  FUN_00034aba  00034aba  size=388


void FUN_00034aba(undefined4 param_1,undefined4 param_2,int param_3)

{
  int *piVar1;
  short sVar2;
  int *piVar3;
  int iVar4;
  int *piVar5;
  int extraout_ECX;
  undefined8 uVar6;
  undefined1 in_stack_00000000;
  int local_2c;
  int *local_24;
  int local_20;
  int local_18;
  int local_14;
  
  uVar6 = FUN_000418ee();
  iVar4 = (int)((ulonglong)uVar6 >> 0x20);
  local_20 = (int)uVar6;
  local_24 = *(int **)(&DAT_00062d68 + iVar4 * 4);
  piVar3 = (int *)FUN_00034a87();
  local_2c = 0;
  local_18 = param_3;
  local_14 = extraout_ECX;
  if (local_20 < 1) {
    local_14 = extraout_ECX - local_20;
    local_18 = param_3 + local_20;
    local_20 = 0;
    *(int **)(&DAT_00062d68 + iVar4 * 4) = piVar3;
  }
  if (0x13f < local_20 + local_18) {
    local_18 = 0x140 - local_20;
  }
  iVar4 = local_20 + local_18;
  while (piVar1 = local_24, local_2c = local_2c + (short)local_24[2], local_2c < local_20) {
    local_24 = (int *)local_24[1];
  }
  *(short *)(piVar3 + 2) = (short)local_18;
  *piVar3 = local_14;
  *(undefined1 *)((int)piVar3 + 10) = in_stack_00000000;
  sVar2 = (short)local_24[2] - ((short)local_2c - (short)local_20);
  if (iVar4 < 0x140) {
    for (; local_2c <= iVar4; local_2c = local_2c + (short)local_24[2]) {
      local_24 = (int *)local_24[1];
    }
    piVar5 = (int *)FUN_00034a87();
    *(short *)(piVar5 + 2) = (short)local_2c - (short)iVar4;
    *piVar5 = (*local_24 + (int)(short)local_24[2]) - (int)(short)piVar5[2];
    piVar5[1] = local_24[1];
    *(undefined1 *)((int)piVar5 + 10) = *(undefined1 *)((int)local_24 + 10);
    *(short *)(piVar1 + 2) = sVar2;
    piVar1[1] = (int)piVar3;
    piVar3[1] = (int)piVar5;
  }
  else {
    *(short *)(local_24 + 2) = sVar2;
    local_24[1] = (int)piVar3;
  }
  return;
}


