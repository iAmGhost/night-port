// func #107  FUN_00036626  00036626  size=126


undefined4 FUN_00036626(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 local_28;
  int local_24;
  
  uVar1 = FUN_000418ee();
  local_28 = 0;
  for (local_24 = 0; local_24 < DAT_000633a0; local_24 = local_24 + 1) {
    uVar2 = FUN_00041f5d(uVar1);
    iVar3 = FUN_000423e5(&DAT_0005d884 + local_24 * 0x15,uVar1,uVar2);
    if (iVar3 == 0) {
      local_28 = *(undefined4 *)(&DAT_0005d891 + local_24 * 0x15);
    }
  }
  return local_28;
}


