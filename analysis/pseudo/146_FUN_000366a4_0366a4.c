// func #108  FUN_000366a4  000366a4  size=293


void FUN_000366a4(void)

{
  int iVar1;
  undefined4 uVar2;
  int local_24;
  
  uVar2 = FUN_000418ee();
  FUN_0004247a(uVar2,0,0);
  FUN_0004192e(&DAT_000633a0,1,4,uVar2);
  FUN_0004192e(DAT_000633b0,0x14,DAT_000633a0,uVar2);
  iVar1 = DAT_000633a0 * 0x14;
  while (local_24 = iVar1 + -1, 0 < local_24) {
    DAT_000633b0[local_24] = DAT_000633b0[local_24] ^ DAT_000633b0[iVar1 + -2];
    iVar1 = local_24;
  }
  *DAT_000633b0 = *DAT_000633b0 ^ 0x20;
  for (local_24 = 0; local_24 < DAT_000633a0; local_24 = local_24 + 1) {
    FUN_00041e6c(&DAT_0005d884 + local_24 * 0x15,DAT_000633b0 + local_24 * 0x14,0xc);
    FUN_00041e6c(&DAT_0005d891 + local_24 * 0x15,DAT_000633b0 + local_24 * 0x14 + 0xc,4);
    FUN_00041e6c(&DAT_0005d895 + local_24 * 0x15,DAT_000633b0 + local_24 * 0x14 + 0x10,4);
  }
  return;
}


