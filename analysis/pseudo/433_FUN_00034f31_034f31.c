// func #433  FUN_00034f31  00034f31  size=376

undefined4 FUN_00034f31(undefined4 param_1,undefined4 param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  undefined8 uVar5;
  undefined4 local_34;
  uint local_2c;
  uint local_28;
  int local_20;
  
  uVar5 = FUN_000418ee();
  iVar4 = (int)((ulonglong)uVar5 >> 0x20);
  local_20 = (int)uVar5;
  iVar1 = FUN_000350aa(local_20,&DAT_00051470);
  if (iVar1 < 0) {
    local_34 = 0xffffffff;
  }
  else {
    *param_3 = local_20;
    iVar1 = FUN_0003510b(local_20 + 4);
    local_2c = FUN_0003518a(local_20 + 10);
    if (0x10 < local_2c) {
      local_2c = 0x10;
    }
    param_3[7] = local_2c;
    iVar2 = FUN_0003518a(local_20 + 0xc);
    param_3[1] = iVar2;
    local_20 = local_20 + iVar1 + 8;
    for (local_28 = 0; local_28 < local_2c; local_28 = local_28 + 1) {
      iVar1 = FUN_000350aa(local_20,&DAT_00051475);
      if (iVar1 < 0) {
        return 0xfffffffe;
      }
      *(int *)(local_28 * 0xe + iVar4) = local_20 + 8;
      uVar3 = FUN_0003510b(local_20 + 4);
      iVar1 = local_28 * 0xe + iVar4;
      *(undefined4 *)(iVar1 + 6) = uVar3;
      local_20 = local_20 + *(int *)(iVar1 + 6) + 8;
      iVar1 = local_28 * 0xe + iVar4;
      *(undefined1 *)(iVar1 + 4) = 0x90;
      *(undefined1 *)(local_28 * 0xe + iVar4 + 5) = *(undefined1 *)(iVar1 + 4);
      uVar3 = FUN_000351dc(local_28 * 0xe + iVar4);
      *(undefined4 *)(local_28 * 0xe + iVar4 + 10) = uVar3;
    }
    param_3[3] = 0;
    param_3[2] = 500000;
    *(undefined1 *)(param_3 + 8) = 0;
    param_3[6] = 0x10000;
    FUN_00035329(iVar4,param_3);
    local_34 = 1;
  }
  return local_34;
}
