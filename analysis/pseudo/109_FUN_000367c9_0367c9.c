// func #109  FUN_000367c9  000367c9  size=138


undefined4 FUN_000367c9(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  int local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = 0;
  while( true ) {
    if (DAT_000633a0 <= local_24) {
      return 0;
    }
    uVar2 = FUN_00041f5d(uVar1);
    iVar3 = FUN_000423e5(&DAT_0005d884 + local_24 * 0x15,uVar1,uVar2);
    if (iVar3 == 0) break;
    local_24 = local_24 + 1;
  }
  FUN_0004247a(DAT_000639b8,*(undefined4 *)(&DAT_0005d895 + local_24 * 0x15),0);
  return 1;
}


