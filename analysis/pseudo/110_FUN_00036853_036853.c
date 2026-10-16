// func #110  FUN_00036853  00036853  size=184


void FUN_00036853(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  int local_24;
  
  uVar4 = FUN_000418ee();
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  local_24 = 0;
  while( true ) {
    if (DAT_000633a0 <= local_24) {
      return;
    }
    uVar1 = FUN_00041f5d((int)uVar4);
    iVar2 = FUN_000423e5(&DAT_0005d884 + local_24 * 0x15,(int)uVar4,uVar1);
    if (iVar2 == 0) break;
    local_24 = local_24 + 1;
  }
  if (param_3 == 0) {
    FUN_0004247a(DAT_000639b8,*(int *)(&DAT_0005d895 + local_24 * 0x15) + iVar3,0);
    return;
  }
  if (param_3 != 2) {
    return;
  }
  FUN_0004247a(DAT_000639b8,
               iVar3 + *(int *)(&DAT_0005d895 + local_24 * 0x15) +
                       *(int *)(&DAT_0005d891 + local_24 * 0x15),0);
  return;
}


