// func #35  FUN_00015ed1  00015ed1  size=365


undefined4 FUN_00015ed1(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined8 uVar5;
  undefined4 local_2c;
  
  iVar1 = FUN_000418ee();
  iVar2 = FUN_0001519f(DAT_00053c7c);
  iVar3 = FUN_000151d5(DAT_00053c7c);
  if (*(int *)(iVar1 + 0x17) == 0x1f) {
    DAT_00053c6c = 1;
    local_2c = 0;
  }
  else if ((*(uint *)(iVar1 + 0x17) < 0xb) || (0x14 < *(uint *)(iVar1 + 0x17))) {
    *(undefined4 *)(iVar1 + 0x117) = 0;
    if (*(int *)(iVar1 + 0x1f) == 4) {
      if (*(int *)(iVar1 + 0x83) == 0) {
        *(undefined4 *)(iVar1 + 0x83) = 1;
        iVar4 = FUN_00041e9f();
        *(int *)(iVar1 + 0x113) = iVar4 % 10;
      }
    }
    else {
      *(undefined4 *)(iVar1 + 0x83) = 0;
    }
    if (*(int *)(iVar1 + 0x1f) == 4) {
      iVar4 = FUN_0001519f(iVar1,(&DAT_00053eb8)
                                 [*(int *)(iVar1 + 0x113) * 10 + *(int *)(iVar1 + 0xd) * 0x88]);
      uVar5 = FUN_00041f76(iVar4 - iVar2);
      if ((int)uVar5 < (int)((ulonglong)uVar5 >> 0x20)) {
        iVar1 = FUN_000151d5(iVar1);
        iVar1 = FUN_00041f76(iVar1 - iVar3);
        if (iVar1 < 10) {
          return 1;
        }
      }
    }
    local_2c = 0;
  }
  else {
    *(undefined4 *)(iVar1 + 0x117) = 1;
    DAT_00053c6c = 1;
    iVar2 = FUN_00041e9f();
    *(int *)(iVar1 + 0x113) = iVar2 % 10;
    local_2c = 2;
  }
  return local_2c;
}


