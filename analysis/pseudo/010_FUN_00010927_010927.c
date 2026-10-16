// func #10  FUN_00010927  00010927  size=303


void FUN_00010927(void)

{
  int iVar1;
  undefined4 *puVar2;
  byte *pbVar3;
  byte bVar4;
  byte abStack_c16 [3040];
  ushort local_36;
  undefined2 local_34;
  uint local_28;
  byte local_24 [4];
  int local_20;
  
  bVar4 = 0;
  FUN_000418ee();
  local_28 = DAT_00051acc;
  *(undefined2 *)(local_24 + (uint)bVar4 * -8) = (&DAT_00051ad0)[(uint)bVar4 * -4];
  local_24[(uint)bVar4 * -4 + (uint)bVar4 * -8 + 2] =
       (&DAT_00051ad2)[(uint)bVar4 * -4 + (uint)bVar4 * -8];
  puVar2 = &DAT_00051ad4;
  pbVar3 = abStack_c16 + 0xbd2;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(undefined4 *)pbVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    pbVar3 = pbVar3 + ((uint)bVar4 * -2 + 1) * 4;
  }
  *(undefined2 *)pbVar3 = *(undefined2 *)puVar2;
  local_36 = (ushort)DAT_00053c5c & 0xf;
  local_34 = (undefined2)(DAT_00053c5c >> 4);
  FUN_00041e6c(DAT_00053c5c,&local_28,7);
  FUN_00010570(abStack_c16 + 0xbd2,7);
  FUN_00041e6c(&local_28,DAT_00053c5c,7);
  DAT_00053c60 = ((local_28 >> 0x10 & 0xff) - (local_28 >> 8 & 0xff)) + 1;
  if (DAT_00053c64 != 0) {
    FUN_00041c28(DAT_00053c64);
  }
  DAT_00053c64 = thunk_FUN_00041bdc((DAT_00053c60 + 1) * 5);
  for (local_20 = 0; local_20 < DAT_00053c60; local_20 = local_20 + 1) {
    FUN_000107b9((local_28 >> 8 & 0xff) + local_20,local_20 * 5 + DAT_00053c64);
  }
  *(uint *)(DAT_00053c60 * 5 + DAT_00053c64 + 1) =
       (local_28 >> 0x18) + (uint)local_24[0] * 0x4b + (uint)local_24[1] * 0x1194 + -0x96;
  return;
}


