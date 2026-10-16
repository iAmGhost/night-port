// func #40  FUN_00016750  00016750  size=521


undefined4 FUN_00016750(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 local_30;
  
  iVar2 = FUN_000418ee();
  iVar3 = FUN_0001519f(DAT_00053c7c);
  iVar4 = FUN_000151d5(DAT_00053c7c);
  if (*(int *)(iVar2 + 0x17) == 0x1f) {
    DAT_00053c6c = 1;
    local_30 = 0;
  }
  else if ((*(uint *)(iVar2 + 0x17) < 0xb) || (0x14 < *(uint *)(iVar2 + 0x17))) {
    *(undefined4 *)(iVar2 + 0x117) = 0;
    if (*(int *)(iVar2 + 0x1f) == 4) {
      if (*(int *)(iVar2 + 0x83) == 0) {
        *(undefined4 *)(iVar2 + 0x83) = 1;
        iVar5 = FUN_00041e9f();
        *(int *)(iVar2 + 0x113) = iVar5 % 10;
      }
    }
    else {
      *(undefined4 *)(iVar2 + 0x83) = 0;
    }
    bVar1 = (&DAT_00053eb8)[*(int *)(iVar2 + 0xd) * 0x88 + *(int *)(iVar2 + 0x113) * 10];
    if (*(int *)(iVar2 + 0x1f) == 4) {
      if ((((&DAT_00053eb9)[*(int *)(iVar2 + 0x113) * 10 + *(int *)(iVar2 + 0xd) * 0x88] == '1') ||
          ((&DAT_00053eb9)[*(int *)(iVar2 + 0x113) * 10 + *(int *)(iVar2 + 0xd) * 0x88] == '4')) ||
         ((&DAT_00053eb9)[*(int *)(iVar2 + 0x113) * 10 + *(int *)(iVar2 + 0xd) * 0x88] == '.')) {
        iVar2 = FUN_0001519f(iVar2);
        iVar2 = FUN_00041f76(iVar2 - iVar3);
        if (iVar2 < (int)(uint)bVar1) {
          return 1;
        }
      }
      else {
        iVar5 = FUN_0001519f(iVar2);
        iVar3 = FUN_00041f76(iVar5 - iVar3);
        if (iVar3 < (int)(uint)bVar1) {
          iVar2 = FUN_000151d5(iVar2);
          iVar2 = FUN_00041f76(iVar2 - iVar4);
          if (iVar2 < 10) {
            return 1;
          }
        }
      }
    }
    local_30 = 0;
  }
  else {
    *(undefined4 *)(iVar2 + 0x117) = 1;
    DAT_00053c6c = 1;
    iVar3 = FUN_00041e9f();
    *(int *)(iVar2 + 0x113) = iVar3 % 10;
    local_30 = 2;
  }
  return local_30;
}


